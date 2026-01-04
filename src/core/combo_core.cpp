#include "core/combo.h"
#include "utils/evrgb_logger.h"

#include <algorithm>
#include <utility>

#include "sync/trigger_buffer.h"

namespace evrgb {

std::tuple<std::vector<RgbCameraInfo>, std::vector<dvsense::CameraDescription>> enumerateAllCameras()
{
    std::vector<RgbCameraInfo> rgb_cameras = enumerateAllRgbCameras();
    std::vector<dvsense::CameraDescription> dvs_cameras = enumerateAllDvsCameras();
    return std::make_tuple(rgb_cameras, dvs_cameras);
}

Combo::Combo(std::string rgb_serial, std::string dvs_serial, Arrangement arrangement, size_t max_buffer_size)
    : rgb_serial_(std::move(rgb_serial))
    , dvs_serial_(std::move(dvs_serial))
    , arrangement_(arrangement)
    , max_rgb_buffer_size_(max_buffer_size)
    , trigger_buffer_(std::make_unique<TriggerBuffer>(100))
    , event_vector_pool_(std::max(max_buffer_size, Combo::kDefaultEventPoolPreallocation), Combo::kDefaultEventPoolCapacity)
{
    LOG_INFO("Creating Combo (rgb_serial='%s', dvs_serial='%s', max_buffer_size='%zu')", rgb_serial_.c_str(), dvs_serial_.c_str(), max_buffer_size);

    rgb_camera_ = std::make_shared<HikvisionRgbCamera>();

    if (rgb_camera_->initialize(rgb_serial_)) {
        rgb_initialized_ = true;
    } else {
        LOG_WARN("RGB camera initialization failed (serial='%s')", rgb_serial_.c_str());
    }

    if (!dvs_serial_.empty()) {
        if (dvs_camera_.initialize(dvs_serial_)) {
            dvs_initialized_ = true;
            dvs_camera_created_ = true;
        } else {
            LOG_WARN("DVS camera initialization failed (serial='%s')", dvs_serial_.c_str());
        }
    } else {
        LOG_WARN("DVS camera initialization failed (no serial provided)");
    }

    if (dvs_camera_created_ && dvs_camera_.isConnected()) {
        LOG_INFO("DVS camera height: %d", dvs_camera_.getDvsCamera()->getHeight());
    }
}

Combo::~Combo()
{
    LOG_INFO("Destroying Combo");
    stopRgbCaptureThread();

    if (rgb_initialized_ && rgb_camera_) {
        rgb_camera_->destroy();
        rgb_initialized_ = false;
    }

    if (dvs_camera_created_) {
        dvs_camera_.destroy();
        dvs_initialized_ = false;
    }
}

bool Combo::init()
{
    bool success = true;

    if (!rgb_initialized_ && !rgb_serial_.empty()) {
        if (!rgb_camera_) rgb_camera_ = std::make_shared<HikvisionRgbCamera>();
        if (rgb_camera_->initialize(rgb_serial_)) {
            rgb_initialized_ = true;
            StringProperty model_prop;
            if (rgb_camera_->getDeviceModelName(model_prop).success()) {
                rgb_model_ = model_prop.value;
            }
        } else {
            LOG_WARN("RGB camera initialization failed (serial='%s')", rgb_serial_.c_str());
            success = false;
        }
    }

    if (!dvs_initialized_ && !dvs_serial_.empty()) {
        if (dvs_camera_.initialize(dvs_serial_)) {
            dvs_initialized_ = true;
            dvs_camera_created_ = true;
            std::string dvs_model;
            if (dvs_camera_.getDeviceModelName(dvs_model)) {
                dvs_model_ = dvs_model;
            }
        } else {
            LOG_WARN("DVS camera initialization failed (serial='%s')", dvs_serial_.c_str());
            success = false;
        }
    }

    return success;
}

bool Combo::start()
{
    bool success = true;

    if (dvs_initialized_ && dvs_camera_.isConnected()) {
        auto recorder = getSyncedDataRecorder();

        dvs_camera_.addTriggerInCallback([this](const dvsense::EventTriggerIn& trigger_event) {
            TriggerSignal trigger(trigger_event);
            addTriggerSignal(trigger);
        });

        internal_event_callback_id_ = addDvsEventCallback([this](dvsense::Event2D* begin, dvsense::Event2D* end) {
            std::lock_guard<std::mutex> lock(event_buffer_mutex_);
            if (begin && end && end > begin) {
                event_buffer_.insert(event_buffer_.end(), begin, end);
            }
        });

        if (!dvs_camera_.start()) {
            LOG_WARN("DVS camera start failed");
            success = false;
        } else {
            LOG_INFO("DVS camera started successfully");
        }
    }

    if (rgb_initialized_ && rgb_camera_) {
        if (!rgb_camera_->start()) {
            LOG_WARN("RGB camera start failed");
            success = false;
        } else {
            LOG_INFO("RGB camera started successfully");
            startRgbCaptureThread();
            startCallbackThread();
            startSyncThread();
        }
    }

    return success;
}

bool Combo::stop()
{
    bool success = true;

    stopSyncThread();
    stopCallbackThread();
    stopRgbCaptureThread();

    auto recorder = getSyncedDataRecorder();

    if (rgb_initialized_ && rgb_camera_) {
        if (!rgb_camera_->stop()) {
            LOG_WARN("RGB camera stop failed");
            success = false;
        }
    }

    if (dvs_initialized_ && dvs_camera_.isConnected()) {
        if (internal_event_callback_id_ != 0) {
            removeDvsEventCallback(internal_event_callback_id_);
            internal_event_callback_id_ = 0;
        }

        {
            std::lock_guard<std::mutex> lock(event_buffer_mutex_);
            event_buffer_.clear();
            last_frame_end_ts_ = 0;
        }

        if (recorder && recorder->requiresDvsRawRecording()) {
            stopDvsRawRecording();
        }

        if (!dvs_camera_.stop()) {
            LOG_WARN("DVS camera stop failed");
            success = false;
        } else {
            LOG_INFO("DVS camera stopped successfully");
        }
    }

    if (recorder) {
        recorder->stop();
    }

    return success;
}

bool Combo::destroy()
{
    bool success = true;

    stopRgbCaptureThread();
    stopCallbackThread();

    if (rgb_initialized_ && rgb_camera_) {
        rgb_camera_->destroy();
        rgb_initialized_ = false;
    }

    if (dvs_initialized_ && dvs_camera_.isConnected()) {
        dvs_camera_.destroy();
        dvs_initialized_ = false;
    }

    clearRgbBuffer();

    {
        std::lock_guard<std::mutex> lock(event_buffer_mutex_);
        event_buffer_.clear();
        last_frame_end_ts_ = 0;
    }

    auto recorder = getSyncedDataRecorder();
    if (recorder) {
        recorder->stop();
    }

    return success;
}

bool Combo::updateRgbImage(const cv::Mat& new_image)
{
    if (new_image.empty()) {
        return false;
    }

    static uint32_t image_index_counter = 0;

    std::lock_guard<std::mutex> lock(rgb_buffer_mutex_);
    rgb_image_buffer_.push(ImageWithIndex(new_image, image_index_counter++));

    if (rgb_image_buffer_.size() > max_rgb_buffer_size_) {
        rgb_image_buffer_.pop();
    }

    return true;
}

size_t Combo::getRgbBufferSize() const
{
    std::lock_guard<std::mutex> lock(rgb_buffer_mutex_);
    return rgb_image_buffer_.size();
}

void Combo::clearRgbBuffer()
{
    std::lock_guard<std::mutex> lock(rgb_buffer_mutex_);
    std::queue<ImageWithIndex> empty_queue;
    rgb_image_buffer_.swap(empty_queue);
}

size_t Combo::getMaxRgbBufferSize() const
{
    return max_rgb_buffer_size_;
}

bool Combo::addTriggerSignal(const TriggerSignal& trigger)
{
    if (!trigger_buffer_) {
        LOG_WARN("Trigger buffer not initialized");
        return false;
    }

    return trigger_buffer_->addTrigger(trigger);
}

void Combo::clearTriggerBuffer()
{
    if (trigger_buffer_) {
        trigger_buffer_->clear();
    }
}

void Combo::setRgbImageCallback(RgbImageCallback callback)
{
    std::lock_guard<std::mutex> lock(sync_callback_mutex_);
    rgb_image_callback_ = std::move(callback);
}

void Combo::setSyncedCallback(SyncedCallback callback)
{
    std::lock_guard<std::mutex> lock(sync_callback_mutex_);
    synced_callback_ = std::move(callback);
}

void Combo::setSyncedDataRecorder(std::shared_ptr<SyncedDataRecorder> recorder)
{
    std::lock_guard<std::mutex> lock(recorder_mutex_);
    recorder_ = std::move(recorder);
}

std::shared_ptr<SyncedDataRecorder> Combo::getSyncedDataRecorder() const
{
    std::lock_guard<std::mutex> lock(recorder_mutex_);
    return recorder_;
}

bool Combo::startRecording(const SyncedRecorderConfig& config)
{
    auto recorder = getSyncedDataRecorder();
    if (!recorder) {
        LOG_WARN("No recorder attached; cannot start recording");
        return false;
    }

    if (!recorder->isActive()) {
        SyncedRecorderConfig cfg = config;
        cfg.arrangement = toString(arrangement_);
        cfg.rgb_serial = rgb_serial_;
        cfg.dvs_serial = dvs_serial_;
        cfg.rgb_model = rgb_model_;
        cfg.dvs_model = dvs_model_;

        if (!recorder->start(cfg)) {
            LOG_WARN("Recorder start failed (dir=%s)", config.output_dir.c_str());
            return false;
        }
    }

    return startDvsRawRecording();
}

bool Combo::stopRecording()
{
    auto recorder = getSyncedDataRecorder();

    stopDvsRawRecording();

    if (recorder) {
        recorder->stop();
    }

    return true;
}

bool Combo::startDvsRawRecording()
{
    auto recorder = getSyncedDataRecorder();
    if (!recorder || !recorder->requiresDvsRawRecording()) {
        LOG_WARN("No recorder configured for DVS raw recording");
        return false;
    }

    if (!dvs_initialized_ || !dvs_camera_.isConnected()) {
        LOG_ERROR("DVS camera not ready, cannot start raw recording");
        return false;
    }

    if (dvs_camera_.isRecording()) {
        return true;
    }

    if (!dvs_camera_.startRecording(recorder->dvsRawPath())) {
        LOG_WARN("DVS raw recording failed to start at %s", recorder->dvsRawPath().c_str());
        return false;
    }

    return dvs_camera_.isRecording();
}

bool Combo::stopDvsRawRecording()
{
    if (!dvs_initialized_ || !dvs_camera_.isConnected()) {
        return false;
    }

    if (!dvs_camera_.isRecording()) {
        return true;
    }

    if (!dvs_camera_.stopRecording()) {
        LOG_WARN("DVS raw recording stop reported failure");
        return false;
    }
    return true;
}

uint32_t Combo::addDvsEventCallback(dvsense::EventsStreamHandleCallback cb)
{
    if (!dvs_camera_.isConnected()) {
        LOG_ERROR("DVS camera not connected, cannot add event callback");
        return 0;
    }

    auto cam = dvs_camera_.getDvsCamera();
    if (cam) {
        return cam->addEventsStreamHandleCallback(std::move(cb));
    }

    return 0;
}

bool Combo::removeDvsEventCallback(uint32_t callback_id)
{
    if (!dvs_camera_.isConnected()) {
        LOG_ERROR("DVS camera not connected, cannot remove event callback");
        return false;
    }

    auto cam = dvs_camera_.getDvsCamera();
    if (cam) {
        return cam->removeEventsStreamHandleCallback(callback_id);
    }

    return false;
}

const std::string EVRGB_API toString(Combo::Arrangement arrangement)
{
    switch (arrangement) {
            case Combo::Arrangement::STEREO:
                return "STEREO";
            case Combo::Arrangement::BEAM_SPLITTER:
                return "BEAM_SPLITTER";
            default:
                return "UNKNOWN";
        }
}

}  // namespace evrgb
