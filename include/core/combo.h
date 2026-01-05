#ifndef EVRGB_COMBO_H_
#define EVRGB_COMBO_H_

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include "DvsenseDriver/camera/DvsCameraManager.hpp"
#include "DvsenseDriver/camera/DvsCamera.hpp"

#include "core/combo_types.h"
#include "camera/dvs_camera.h"
#include "camera/rgb_camera.h"
#include "sync/event_vector_pool.h"
#include "recording/synced_data_recorder.h"
#include "utils/calib_info.h"

#ifdef _WIN32
    #ifdef EVRGB_EXPORTS
        #define EVRGB_API __declspec(dllexport)
    #else
        #define EVRGB_API __declspec(dllimport)
    #endif
#else
    #define EVRGB_API   
#endif

namespace evrgb {

class TriggerBuffer;

/**
 * @brief Enumerate all RGB and DVS cameras.
 * 
 * @return std::tuple<std::vector<RgbCameraInfo>, std::vector<dvsense::CameraDescription>>
 */
EVRGB_API std::tuple<std::vector<RgbCameraInfo>, std::vector<dvsense::CameraDescription>> enumerateAllCameras();

class EVRGB_API Combo {
public:
    using Arrangement = ComboArrangement;

    Combo(std::string rgb_serial = "", std::string dvs_serial = "", Arrangement arrangement = Arrangement::STEREO, size_t max_buffer_size = 10);
    ~Combo();

    // Disable copy (manages device resources)
    Combo(const Combo&) = delete;
    Combo& operator=(const Combo&) = delete;

    // Allow move semantics
    Combo(Combo&&) = default;
    Combo& operator=(Combo&&) = default;

    // Combo Control
    /** */
    /**
     * @brief Initialize the combo camera with the given serial numbers.
     * 
     * @return true if initialization is successful, false otherwise.
     */
    bool init();

    /**
     * @brief Start the combo camera.
     * 
     * @return true if start is successful, false otherwise.
     */
    bool start();
    /**
     * @brief Stop the combo camera.
     * 
     * @return true if stop is successful, false otherwise.
     */
    bool stop();
    /**
     * @brief Destroy the combo camera.
     * 
     * @return true if destroy is successful, false otherwise.
     */
    bool destroy();

    /**
     * @brief Get the RGB camera interface
     * @return Shared pointer to the RGB camera interface
     */
    std::shared_ptr<IRgbCamera> getRgbCamera() const { return rgb_camera_; }

    /**
     * #if ENGLISH
     * @brief Get the managed DVS camera wrapper
     * @return Shared pointer to the wrapper DVS camera interface
     * #endif
     * 
     * #if CHINESE
     * @brief 获取封装的 DVS 相机对象
     * @return 封装 DVS 相机的共享指针
     * #endif
     */
    std::shared_ptr<DvsCamera> getDvsCamera() const { return dvs_camera_; }

    /**
     * #if ENGLISH
     * @brief Get the raw DVS camera handle from the wrapper
     * @return Shared pointer to the underlying DVS camera interface
     * #endif
     * 
     * #if CHINESE
     * @brief 获取底层原始 DVS 相机句柄
     * @return 底层 DVS 相机接口的共享指针
     * #endif
     */
    std::shared_ptr<dvsense::DvsCamera> getRawDvsCamera() const { return dvs_camera_ ? dvs_camera_->getDvsCamera() : nullptr; }

    /**
     * @brief Get the number of images in the RGB buffer.
     * 
     * @return Number of images currently in the buffer.
     */
    size_t getRgbBufferSize() const;

    /**
     * @brief Get the maximum size of the RGB buffer.
     * 
     * @return Maximum number of images that can be stored in the buffer.
     */
    size_t getMaxRgbBufferSize() const;

    /**
     * @brief Add a callback for DVS event stream.
     * @param cb The callback function.
     * @return Callback ID.
     */
    uint32_t addDvsEventCallback(dvsense::EventsStreamHandleCallback cb);

    /**
     * @brief Remove a DVS event stream callback.
     * @param callback_id The ID returned by addDvsEventCallback.
     * @return true if successful.
     */
    bool removeDvsEventCallback(uint32_t callback_id);

    /**
     * @brief Set the callback function for synchronized RGB images
     * @param callback Callback function to be called when a new image is available.
     *                 This callback is called before sync with trigger events.
     *                 There is no shutter timestamp information.
     */
    void setRgbImageCallback(RgbImageCallback callback);

    /**
     * @brief Set the callback function for synchronized RGB images and events
     * @param callback Callback function to be called when a synchronized image and events are available
     */
    void setSyncedCallback(SyncedCallback callback);

    /**
     * @brief Attach a recorder that will persist synced RGB/DVS data.
     * @param recorder Shared recorder instance; pass nullptr to detach.
     */
    void setSyncedDataRecorder(std::shared_ptr<SyncedDataRecorder> recorder);

    /**
     * @brief Get the currently attached recorder (if any).
     */
    std::shared_ptr<SyncedDataRecorder> getSyncedDataRecorder() const;

    /**
     * @brief Start recording (RGB MP4 + CSV + DVS raw) with the given config.
     * @return true on success
     */
    bool startRecording(const SyncedRecorderConfig& config);

    /**
     * @brief Stop recording (RGB MP4 + CSV + DVS raw).
     * @return true on success
     */
    bool stopRecording();

    /**
     * #if ENGLISH
     * @brief Get the arrangement mode of the combo camera.
     * @return Arrangement mode.
     * #endif
     * 
     * #if CHINESE
     * @brief 获取组合相机的排列模式。
     * @return 排列模式。
     * #endif
     */
    Arrangement getArrangement() const { return arrangement_; }

    /**
     * @brief Gather all available combo metadata (devices, arrangement, calibration).
     */
    ComboMetadata getMetadata() const;

    /**
     * @brief Apply provided metadata to the combo (arrangement, calibration, intrinsics when available).
     */
    bool applyMetadata(const ComboMetadata& metadata, bool apply_intrinsics = true);

    /**
     * @brief Persist metadata as JSON to disk.
     */
    bool saveMetadata(const std::string& path, std::string* error_message = nullptr) const;

    /**
     * @brief Load metadata from disk and apply it.
     */
    bool loadMetadata(const std::string& path, std::string* error_message = nullptr);

    ComboCalibrationInfo calibration_info{};  ///< Calibration information between RGB and DVS cameras

private:
    std::string rgb_serial_;
    std::string dvs_serial_;
    std::string rgb_model_;
    std::string dvs_model_;
    std::shared_ptr<IRgbCamera> rgb_camera_;                // managed RGB camera instance
    std::shared_ptr<DvsCamera> dvs_camera_; // managed DVS camera (if initialized)
    bool rgb_initialized_ = false;
    bool dvs_initialized_ = false;

    Arrangement arrangement_;
    
    // RGB image buffer with index
    struct ImageWithIndex {
        cv::Mat image;
        uint32_t index;
        
        ImageWithIndex(const cv::Mat& img, uint32_t idx) 
            : image(img), index(idx) {}
    };
    std::queue<ImageWithIndex> rgb_image_buffer_;
    size_t max_rgb_buffer_size_ = 10;  // Maximum number of images to store in buffer
    mutable std::mutex rgb_buffer_mutex_;  // Mutex for protecting the RGB image buffer
    bool rgb_capture_thread_running_ = false;  // Flag to control the RGB capture thread
    std::thread rgb_capture_thread_;  // Thread for capturing RGB images
    
    // Trigger synchronization components
    std::unique_ptr<TriggerBuffer> trigger_buffer_;
    
    // Synchronization thread and related components
    bool sync_thread_running_ = false;
    std::thread sync_thread_;
    std::function<void(const cv::Mat&)> rgb_image_callback_;
    mutable std::mutex sync_callback_mutex_;

    // Internal event buffering
    std::deque<dvsense::Event2D> event_buffer_;
    mutable std::mutex event_buffer_mutex_;
    uint64_t last_frame_end_ts_ = 0;
    SyncedCallback synced_callback_;
    uint32_t internal_event_callback_id_ = 0;
    
    // Callback worker components
    struct SyncedFrameData {
        RgbImageWithTimestamp image_data;
        std::shared_ptr<std::vector<dvsense::Event2D>> events;
    };
    
    std::queue<SyncedFrameData> callback_queue_;
    std::mutex callback_queue_mutex_;
    std::condition_variable callback_cv_;
    std::thread callback_thread_;
    bool callback_thread_running_ = false;

    // Event vector pool
    static constexpr size_t kDefaultEventPoolPreallocation = 8;
    static constexpr size_t kDefaultEventPoolCapacity = 256 * 1024;
    EventVectorPool event_vector_pool_;

    // Recorder integration
    std::shared_ptr<SyncedDataRecorder> recorder_;
    mutable std::mutex recorder_mutex_;

    // Internal helpers for raw DVS recording
    bool startDvsRawRecording();
    bool stopDvsRawRecording();

    bool updateRgbImage(const cv::Mat& new_image);
    void clearRgbBuffer();
    bool addTriggerSignal(const TriggerSignal& trigger);
    void clearTriggerBuffer();

    // Private helper methods
    void startRgbCaptureThread();  // Start the RGB capture thread
    void stopRgbCaptureThread();   // Stop the RGB capture thread
    void rgbCaptureLoop();         // Main loop for capturing RGB images
    void startSyncThread();        // Start the synchronization thread
    void stopSyncThread();         // Stop the synchronization thread
    void syncLoop();               // Main loop for synchronization

    void startCallbackThread();
    void stopCallbackThread();
    void callbackLoop();
    
    bool synchronizeImageAndTrigger(cv::Mat& image, uint64_t& exposure_start_ts, uint64_t& exposure_end_ts, uint32_t& image_index);
};

}  // namespace evrgb

#endif  // EVRGB_COMBO_H_