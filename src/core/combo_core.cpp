#include "core/combo.h"
#include "core/combo_types.h"
#include "utils/evrgb_logger.h"
#include "utils/calib_info.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

#include "sync/trigger_buffer.h"
#include <nlohmann/json.hpp>

namespace evrgb {

// Explicit implementations of to_json/from_json functions to avoid linking issues
void to_json(nlohmann::json& j, const CameraIntrinsics& intrinsics) {
    j = {
        {"fx", intrinsics.fx},
        {"fy", intrinsics.fy},
        {"cx", intrinsics.cx},
        {"cy", intrinsics.cy},
        {"skew", intrinsics.skew},
        {"width", intrinsics.width},
        {"height", intrinsics.height},
        {"distortion", intrinsics.distortion}
    };
}

void from_json(const nlohmann::json& j, CameraIntrinsics& intrinsics) {
    intrinsics.fx = j.value("fx", 0.0);
    intrinsics.fy = j.value("fy", 0.0);
    intrinsics.cx = j.value("cx", 0.0);
    intrinsics.cy = j.value("cy", 0.0);
    intrinsics.skew = j.value("skew", 0.0);
    intrinsics.width = j.value("width", 0);
    intrinsics.height = j.value("height", 0);
    intrinsics.distortion = j.value("distortion", std::vector<double>{});
}

void to_json(nlohmann::json& j, const RigidTransform& rt) {
    j = {
        {"rotation", nlohmann::json::array({rt.R(0,0), rt.R(0,1), rt.R(0,2), rt.R(1,0), rt.R(1,1), rt.R(1,2), rt.R(2,0), rt.R(2,1), rt.R(2,2)})},
        {"translation", nlohmann::json::array({rt.t[0], rt.t[1], rt.t[2]})}
    };
}

void from_json(const nlohmann::json& j, RigidTransform& rt) {
    if (j.contains("rotation")) {
        const auto& r = j.at("rotation");
        if (r.is_array() && r.size() == 9) {
            rt.R = cv::Matx33d(r[0].get<double>(), r[1].get<double>(), r[2].get<double>(),
                             r[3].get<double>(), r[4].get<double>(), r[5].get<double>(),
                             r[6].get<double>(), r[7].get<double>(), r[8].get<double>());
        }
    }
    if (j.contains("translation")) {
        const auto& t = j.at("translation");
        if (t.is_array() && t.size() == 3) {
            rt.t = cv::Vec3d(t[0].get<double>(), t[1].get<double>(), t[2].get<double>());
        }
    }
}

void to_json(nlohmann::json& j, const AffineTransform& a) {
    j = nlohmann::json::array({a.A(0,0), a.A(0,1), a.A(0,2), a.A(1,0), a.A(1,1), a.A(1,2)});
}

void from_json(const nlohmann::json& j, AffineTransform& a) {
    if (j.is_array() && j.size() == 6) {
        a.A = cv::Matx23d(j[0].get<double>(), j[1].get<double>(), j[2].get<double>(),
                         j[3].get<double>(), j[4].get<double>(), j[5].get<double>());
    }
}

void to_json(nlohmann::json& j, const ComboCalibrationInfo& calib) {
    j = nlohmann::json::object();
    std::visit([&j](auto&& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, RigidTransform>) {
            j["stereo_extrinsics"] = value;
        } else if constexpr (std::is_same_v<T, AffineTransform>) {
            j["beam_splitter_affine"] = value;
        }
    }, calib);
}

void from_json(const nlohmann::json& j, ComboCalibrationInfo& calib) {
    calib = std::monostate{};
    if (j.contains("stereo_extrinsics")) {
        calib = j.at("stereo_extrinsics").get<RigidTransform>();
    } else if (j.contains("beam_splitter_affine")) {
        calib = j.at("beam_splitter_affine").get<AffineTransform>();
    }
}

namespace {

std::string toUpperCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

}  // namespace

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
    init();
}

Combo::~Combo()
{
    LOG_INFO("Destroying Combo");
    stopRgbCaptureThread();

    if (rgb_initialized_ && rgb_camera_) {
        rgb_camera_->destroy();
        rgb_initialized_ = false;
    }

    if (dvs_initialized_ && dvs_camera_) {
        dvs_camera_->destroy();
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
            CameraStatus status = rgb_camera_->getDeviceModelName(model_prop);
            if (status.success()) {
                rgb_model_ = model_prop.value;
            } else {
                LOG_WARN("Failed to get RGB camera model name, error code: %d", status.code);
            }

        } else {
            LOG_WARN("RGB camera initialization failed (serial='%s')", rgb_serial_.c_str());
            success = false;
        }
    }

    if (!dvs_initialized_ && !dvs_serial_.empty()) {
        if (!dvs_camera_) dvs_camera_ = std::make_shared<DvsCamera>();
        if (dvs_camera_->initialize(dvs_serial_)) {
            dvs_initialized_ = true;
            std::string dvs_model;
            if (dvs_camera_->getDeviceModelName(dvs_model)) {
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

    if (dvs_initialized_ && dvs_camera_ && dvs_camera_->isConnected()) {
        auto recorder = getSyncedDataRecorder();

        dvs_camera_->addTriggerInCallback([this](const dvsense::EventTriggerIn& trigger_event) {
            TriggerSignal trigger(trigger_event);
            addTriggerSignal(trigger);
        });

        internal_event_callback_id_ = addDvsEventCallback([this](dvsense::Event2D* begin, dvsense::Event2D* end) {
            std::lock_guard<std::mutex> lock(event_buffer_mutex_);
            if (begin && end && end > begin) {
                event_buffer_.insert(event_buffer_.end(), begin, end);
            }
        });

        if (!dvs_camera_->start()) {
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

    if (dvs_initialized_ && dvs_camera_ && dvs_camera_->isConnected()) {
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

        if (!dvs_camera_->stop()) {
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

    if (dvs_initialized_ && dvs_camera_ && dvs_camera_->isConnected()) {
        dvs_camera_->destroy();
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

ComboMetadata Combo::getMetadata() const
{
    ComboMetadata meta{};
    meta.arrangement = arrangement_;
    meta.calibration = calibration_info;

    if (rgb_camera_) {
        meta.rgb.width = rgb_camera_->getWidth();
        meta.rgb.height = rgb_camera_->getHeight();
        if (!rgb_serial_.empty()) {
            meta.rgb.serial = rgb_serial_;
        }
        // Prefer querying the camera for fresh model info to avoid stale cache.
        StringProperty model_prop;
        if (rgb_camera_->getDeviceModelName(model_prop).success()) {
            meta.rgb.model = model_prop.value;
        }
        StringProperty vendor_prop;
        if (rgb_camera_->getString("DeviceVendorName", vendor_prop).success()) {
            meta.rgb.manufacturer = vendor_prop.value;
        } else {
            meta.rgb.manufacturer = "Unknown";
        }
        if (auto intrinsics = rgb_camera_->getIntrinsics()) {
            meta.rgb.intrinsics = intrinsics;
        }
    }

    if (dvs_camera_) {
        if (!dvs_serial_.empty()) {
            meta.dvs.serial = dvs_serial_;
        }
        // DVS model fetched live when possible.
        std::string dvs_model;
        if (dvs_camera_->getDeviceModelName(dvs_model)) {
            meta.dvs.model = dvs_model;
        }
        if (meta.dvs.manufacturer.empty()) {
            //TODO: query actual manufacturer if possible
            meta.dvs.manufacturer = "Dvsense";
        }
        if (auto intrinsics = dvs_camera_->getIntrinsics()) {
            meta.dvs.intrinsics = intrinsics;
        }
    }

    return meta;
}

bool Combo::applyMetadata(const ComboMetadata& metadata, bool apply_intrinsics)
{
    arrangement_ = metadata.arrangement;
    calibration_info = metadata.calibration;

    if (apply_intrinsics) {
        if (metadata.rgb.intrinsics && rgb_camera_) {
            rgb_camera_->setIntrinsics(*metadata.rgb.intrinsics);
        }
        if (metadata.dvs.intrinsics && dvs_camera_) {
            dvs_camera_->setIntrinsics(*metadata.dvs.intrinsics);
        }
    }

    return true;
}

bool Combo::saveMetadata(const std::string& path, std::string* error_message) const
{
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        if (error_message) {
            *error_message = "Failed to open metadata file: " + path;
        }
        return false;
    }

    try {
        nlohmann::json payload = getMetadata();
        out << payload.dump(2);
        return true;
    } catch (const std::exception& ex) {
        if (error_message) {
            *error_message = ex.what();
        }
        return false;
    }
}

bool Combo::loadMetadata(const std::string& path, std::string* error_message)
{
    std::ifstream in(path);
    if (!in.is_open()) {
        if (error_message) {
            *error_message = "Failed to open metadata file: " + path;
        }
        return false;
    }

    try {
        nlohmann::json payload;
        in >> payload;
        ComboMetadata metadata = payload.get<ComboMetadata>();
        return applyMetadata(metadata, true);
    } catch (const std::exception& ex) {
        if (error_message) {
            *error_message = ex.what();
        }
        return false;
    }
}

bool Combo::startDvsRawRecording()
{
    auto recorder = getSyncedDataRecorder();
    if (!recorder || !recorder->requiresDvsRawRecording()) {
        LOG_WARN("No recorder configured for DVS raw recording");
        return false;
    }

    if (!dvs_initialized_ || !dvs_camera_ || !dvs_camera_->isConnected()) {
        LOG_ERROR("DVS camera not ready, cannot start raw recording");
        return false;
    }

    if (dvs_camera_->isRecording()) {
        return true;
    }

    if (!dvs_camera_->startRecording(recorder->dvsRawPath())) {
        LOG_WARN("DVS raw recording failed to start at %s", recorder->dvsRawPath().c_str());
        return false;
    }

    return dvs_camera_->isRecording();
}

bool Combo::stopDvsRawRecording()
{
    if (!dvs_initialized_ || !dvs_camera_ || !dvs_camera_->isConnected()) {
        return false;
    }

    if (!dvs_camera_->isRecording()) {
        return true;
    }

    if (!dvs_camera_->stopRecording()) {
        LOG_WARN("DVS raw recording stop reported failure");
        return false;
    }
    return true;
}

uint32_t Combo::addDvsEventCallback(dvsense::EventsStreamHandleCallback cb)
{
    if (!dvs_camera_ || !dvs_camera_->isConnected()) {
        LOG_ERROR("DVS camera not connected, cannot add event callback");
        return 0;
    }

    auto cam = dvs_camera_->getDvsCamera();
    if (cam) {
        return cam->addEventsStreamHandleCallback(std::move(cb));
    }

    return 0;
}

bool Combo::removeDvsEventCallback(uint32_t callback_id)
{
    if (!dvs_camera_ || !dvs_camera_->isConnected()) {
        LOG_ERROR("DVS camera not connected, cannot remove event callback");
        return false;
    }

    auto cam = dvs_camera_->getDvsCamera();
    if (cam) {
        return cam->removeEventsStreamHandleCallback(callback_id);
    }

    return false;
}

ComboArrangement EVRGB_API arrangementFromString(const std::string& value)
{
    const std::string upper = toUpperCopy(value);
    if (upper == "BEAM_SPLITTER" || upper == "BEAM-SPLITTER") {
        return ComboArrangement::BEAM_SPLITTER;
    }
    return ComboArrangement::STEREO;
}

const std::string EVRGB_API toString(ComboArrangement arrangement)
{
    switch (arrangement) {
        case ComboArrangement::STEREO:
            return "STEREO";
        case ComboArrangement::BEAM_SPLITTER:
            return "BEAM_SPLITTER";
        default:
            return "UNKNOWN";
    }
}

void toJson(nlohmann::json& j, const CameraMetadata& metadata)
{
    j = {
        {"manufacturer", metadata.manufacturer},
        {"model", metadata.model},
        {"serial", metadata.serial},
        {"width", metadata.width},
        {"height", metadata.height}
    };

    if (metadata.intrinsics.has_value()) {
        j["intrinsics"] = *metadata.intrinsics;
    }
}

EVRGB_API void to_json(nlohmann::json& j, const CameraMetadata& metadata)
{
    toJson(j, metadata);
}

void fromJson(const nlohmann::json& j, CameraMetadata& metadata)
{
    metadata.manufacturer = j.value("manufacturer", "");
    metadata.model = j.value("model", "");
    metadata.serial = j.value("serial", "");
    metadata.width = j.value("width", 0u);
    metadata.height = j.value("height", 0u);

    if (j.contains("intrinsics") && !j.at("intrinsics").is_null()) {
        metadata.intrinsics = j.at("intrinsics").get<CameraIntrinsics>();
    } else {
        metadata.intrinsics.reset();
    }
}

EVRGB_API void from_json(const nlohmann::json& j, CameraMetadata& metadata)
{
    fromJson(j, metadata);
}

void toJson(nlohmann::json& j, const ComboMetadata& metadata)
{
    j = {
        {"arrangement", toString(metadata.arrangement)},
        {"rgb", metadata.rgb},
        {"dvs", metadata.dvs},
        {"calibration", metadata.calibration}
    };
}

EVRGB_API void to_json(nlohmann::json& j, const ComboMetadata& metadata)
{
    toJson(j, metadata);
}

void fromJson(const nlohmann::json& j, ComboMetadata& metadata)
{
    metadata.arrangement = arrangementFromString(j.value("arrangement", "STEREO"));

    if (j.contains("rgb")) {
        metadata.rgb = j.at("rgb").get<CameraMetadata>();
    } else {
        metadata.rgb = CameraMetadata{};
    }

    if (j.contains("dvs")) {
        metadata.dvs = j.at("dvs").get<CameraMetadata>();
    } else {
        metadata.dvs = CameraMetadata{};
    }

    metadata.calibration = std::monostate{};
    if (j.contains("calibration")) {
        metadata.calibration = j.at("calibration").get<ComboCalibrationInfo>();
    }
}

EVRGB_API void from_json(const nlohmann::json& j, ComboMetadata& metadata)
{
    fromJson(j, metadata);
}

}  // namespace evrgb
