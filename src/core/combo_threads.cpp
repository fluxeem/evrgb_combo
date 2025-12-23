#include "core/combo.h"
#include "utils/evrgb_logger.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include "sync/trigger_buffer.h"

namespace evrgb {

void Combo::startRgbCaptureThread()
{
    if (rgb_capture_thread_running_) {
        LOG_WARN("RGB capture thread is already running");
        return;
    }

    rgb_capture_thread_running_ = true;
    rgb_capture_thread_ = std::thread(&Combo::rgbCaptureLoop, this);
    LOG_INFO("RGB capture thread started");
}

void Combo::stopRgbCaptureThread()
{
    if (!rgb_capture_thread_running_) {
        LOG_DEBUG("RGB capture thread is not running");
        return;
    }

    LOG_INFO("Stopping RGB capture thread...");
    rgb_capture_thread_running_ = false;

    if (rgb_capture_thread_.joinable()) {
        rgb_capture_thread_.join();
    }

    LOG_INFO("RGB capture thread stopped");
}

void Combo::rgbCaptureLoop()
{
    LOG_INFO("RGB capture loop started");

    cv::Mat frame;
    static uint32_t image_counter = 0;

    while (rgb_capture_thread_running_) {
        if (rgb_camera_) {
            if (rgb_camera_->getLatestImage(frame)) {
                if (!frame.empty()) {
                    std::lock_guard<std::mutex> lock(rgb_buffer_mutex_);
                    if (rgb_image_callback_) {
                        rgb_image_callback_(frame);
                    }
                    rgb_image_buffer_.push(ImageWithIndex(frame, image_counter++));

                    if (rgb_image_buffer_.size() > max_rgb_buffer_size_) {
                        rgb_image_buffer_.pop();
                    }
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    LOG_INFO("RGB capture loop ended");
}

void Combo::startSyncThread()
{
    if (sync_thread_running_) {
        LOG_WARN("Synchronization thread is already running");
        return;
    }

    sync_thread_running_ = true;
    sync_thread_ = std::thread(&Combo::syncLoop, this);
    LOG_INFO("Synchronization thread started");
}

void Combo::stopSyncThread()
{
    if (!sync_thread_running_) {
        LOG_DEBUG("Synchronization thread is not running");
        return;
    }

    LOG_INFO("Stopping synchronization thread...");
    sync_thread_running_ = false;

    if (sync_thread_.joinable()) {
        sync_thread_.join();
    }

    LOG_INFO("Synchronization thread stopped");
}

void Combo::syncLoop()
{
    LOG_DEBUG("Synchronization loop started");

    cv::Mat image;
    uint64_t exposure_start_ts = 0;
    uint64_t exposure_end_ts = 0;
    uint32_t image_index = 0;

    while (sync_thread_running_) {
        if (synchronizeImageAndTrigger(image, exposure_start_ts, exposure_end_ts, image_index)) {
            RgbImageWithTimestamp image_with_ts(image, exposure_start_ts, exposure_end_ts, image_index);

            std::lock_guard<std::mutex> cb_lock(sync_callback_mutex_);

            if (synced_callback_) {
                auto frame_events = event_vector_pool_.acquire();
                {
                    std::lock_guard<std::mutex> event_lock(event_buffer_mutex_);

                    // last_frame_end_ts_ is empty for the first frame
                    uint64_t start_ts = (last_frame_end_ts_ > 0) ? last_frame_end_ts_ : exposure_start_ts;

                    // Find the range of events that fall within the exposure period
                    auto it_end = std::upper_bound(event_buffer_.begin(), event_buffer_.end(), exposure_end_ts,
                        [](uint64_t ts, const dvsense::Event2D& e) {
                            return ts < e.timestamp;
                        });

                    if (it_end != event_buffer_.begin()) {
                        frame_events->insert(frame_events->end(), event_buffer_.begin(), it_end);
                        event_buffer_.erase(event_buffer_.begin(), it_end);
                    }  

                    last_frame_end_ts_ = exposure_end_ts;
                }

                {
                    std::lock_guard<std::mutex> queue_lock(callback_queue_mutex_);
                    callback_queue_.push({image_with_ts, frame_events});
                }
                callback_cv_.notify_one();
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    LOG_DEBUG("Synchronization loop ended");
}

bool Combo::synchronizeImageAndTrigger(cv::Mat& image, uint64_t& exposure_start_ts, uint64_t& exposure_end_ts, uint32_t& image_index)
{
    std::lock_guard<std::mutex> lock(rgb_buffer_mutex_);

    if (rgb_image_buffer_.empty()) {
        return false;
    }

    if (!trigger_buffer_ || trigger_buffer_->empty()) {
        return false;
    }

    ImageWithIndex image_with_idx = rgb_image_buffer_.front();
    rgb_image_buffer_.pop();

    TriggerPair trigger_pair;
    if (!trigger_buffer_->getOldestTrigger(trigger_pair)) {
        rgb_image_buffer_.push(image_with_idx);
        return false;
    }

    if (trigger_pair.end_trigger.has_value()) {
        if (trigger_pair.start_trigger.has_value()) {
            exposure_start_ts = trigger_pair.start_trigger->timestamp_us;
        } else {
            // The timestamp of the first trigger is missing, use end trigger timestamp as start
            exposure_start_ts = trigger_pair.end_trigger->timestamp_us;
        }
        
        exposure_end_ts = trigger_pair.end_trigger->timestamp_us;
        image = image_with_idx.image;
        image_index = image_with_idx.index;
        return true;
    }

    rgb_image_buffer_.push(image_with_idx);
    return false;
}

void Combo::startCallbackThread()
{
    if (callback_thread_running_) {
        LOG_WARN("Callback thread is already running");
        return;
    }

    callback_thread_running_ = true;
    callback_thread_ = std::thread(&Combo::callbackLoop, this);
    LOG_INFO("Callback thread started");
}

void Combo::stopCallbackThread()
{
    if (!callback_thread_running_) {
        LOG_DEBUG("Callback thread is not running");
        return;
    }

    LOG_INFO("Stopping callback thread...");
    {
        std::lock_guard<std::mutex> lock(callback_queue_mutex_);
        callback_thread_running_ = false;
    }
    callback_cv_.notify_all();

    if (callback_thread_.joinable()) {
        callback_thread_.join();
    }

    std::lock_guard<std::mutex> lock(callback_queue_mutex_);
    while (!callback_queue_.empty()) {
        auto item = callback_queue_.front();
        callback_queue_.pop();
        event_vector_pool_.release(item.events);
    }

    LOG_INFO("Callback thread stopped");
}

void Combo::callbackLoop()
{
    LOG_INFO("Callback loop started");

    while (true) {
        SyncedFrameData data;
        {
            std::unique_lock<std::mutex> lock(callback_queue_mutex_);
            callback_cv_.wait(lock, [this] {
                return !callback_queue_.empty() || !callback_thread_running_;
            });

            if (!callback_thread_running_ && callback_queue_.empty()) {
                break;
            }

            if (callback_queue_.empty()) {
                continue;
            }

            data = callback_queue_.front();
            callback_queue_.pop();
        }

        auto recorder = getSyncedDataRecorder();

        if (recorder && recorder->isActive()) {
            recorder->record(data.image_data, *data.events);
        }

        if (synced_callback_) {
            synced_callback_(data.image_data, *data.events);
        }

        event_vector_pool_.release(data.events);
    }

    LOG_INFO("Callback loop ended");
}

}  // namespace evrgb
