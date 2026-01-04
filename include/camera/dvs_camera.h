// Copyright (c) 2025.
// Author: EvRGB Team
// Description: Define the DVS camera class for device management and control.

#ifndef EVRGB_DVS_CAMERA_H_
#define EVRGB_DVS_CAMERA_H_

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <optional>
#include "DvsenseDriver/camera/DvsCameraManager.hpp"
#include "DvsenseDriver/camera/DvsCamera.hpp"
#include "DvsenseBase/EventBase/EventTypes.hpp"

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


// Forward declaration of TriggerSignal to avoid circular dependency
namespace evrgb {
    struct TriggerSignal;
}

namespace evrgb
{

// Enumerate all DVS cameras (wrapper over DvsenseDriver)
EVRGB_API std::vector<dvsense::CameraDescription> enumerateAllDvsCameras();

// DVS camera control class
class EVRGB_API DvsCamera
{
public:
    /// DVS camera state enumeration
    enum class CameraState
    {
        UNINITIALIZED = 0,
        INITIALIZED,
        STARTED,
        STOPPED,
        ERROR_STATUS
    };

    /**
     * @brief Default constructor
     */
    DvsCamera();
    
    /**
     * @brief Construct a DVS camera object and optionally initialize by serial
     * @param serial_number Camera serial number to initialize, empty to skip
     */
    explicit DvsCamera(const std::string& serial_number);
    
    /**
     * @brief Destructor, releases camera resources
     */
    ~DvsCamera();

    // Disable copy (manages device resources)
    DvsCamera(const DvsCamera&) = delete;
    DvsCamera& operator=(const DvsCamera&) = delete;

    // Allow move semantics
    DvsCamera(DvsCamera&&) = default;
    DvsCamera& operator=(DvsCamera&&) = default;

    /**
     * @brief Initialize the DVS camera
     * @param serial_number Camera serial number, empty to use first available
     * @return true on success, false on error
     */
    bool initialize(const std::string& serial_number = "");
    
    /**
     * @brief Start camera capture
     * @return true on success
     */
    bool start();
    
    /**
     * @brief Stop camera capture
     * @return true on success
     */
    bool stop();
    
    /**
     * @brief Destroy camera resources and reset state
     */
    void destroy();

    /// ---------------- state query ----------------

    /**
     * @brief Get current camera state
     * @return Current CameraState
     */
    CameraState getState() const 
    { 
        return camera_state_; 
    }
    
    /**
     * @brief Check whether camera is connected (handle valid)
     * @return true if connected
     */
    bool isConnected() const 
    { 
        return dvs_camera_ != nullptr; 
    }

    /**
     * @brief Get the internal DVS camera object
     * @return Shared pointer to the DVS camera
     */
    std::shared_ptr<dvsense::DvsCamera> getDvsCamera() const
    {
        return dvs_camera_;
    }

    /**
     * @brief Enable Trigger In signal processing for DVS camera
     * @return true if successful, false otherwise
     */
    bool enableTriggerInProcessing();

    /**
     * @brief Disable Trigger In signal processing for DVS camera
     * @return true if successful, false otherwise
     */
    bool disableTriggerInProcessing();

    /**
     * @brief Add callback function for trigger in events
     * @param callback Function to call when a trigger in event is received
     * @return ID of the callback, used for removing it later
     */
    uint32_t addTriggerInCallback(std::function<void(const dvsense::EventTriggerIn)> callback);
    
    /**
     * @brief Remove a specific trigger in callback by ID
     * @param callback_id ID of the callback to remove
     * @return true on success, false on failure
     */
    bool removeTriggerInCallback(uint32_t callback_id);

    /**
     * @brief Remove all trigger in callbacks
     */
    void removeAllTriggerInCallbacks();

    /**
     * @brief Start recording raw DVS events to a file
     * @param file_path Output file path
     * @return true on success
     */
    bool startRecording(const std::string& file_path);

    /**
     * @brief Stop recording raw DVS events
     * @return true on success
     */
    bool stopRecording();

    /**
     * @brief Whether raw recording is currently active
     */
    bool isRecording() const { return recording_.load(); }

    /**
     * #if ENGLISH
     * @brief Get the device model name
     * @param model_name Output string for model name
     * @return true on success, false on error
     * #endif
     * 
     * #if CHINESE
     * @brief 获取设备型号名称
     * @param model_name 输出的型号名称字符串
     * @return 如果成功返回true，否则返回false
     * #endif
     */
    bool getDeviceModelName(std::string& model_name);

    /**
     * #if ENGLISH
     * @brief Set camera intrinsics calibration
     * @param intrinsics Camera intrinsics parameters
     * #endif
     * 
     * #if CHINESE
     * @brief 设置相机内参标定
     * @param intrinsics 相机内参参数
     * #endif
     */
    void setIntrinsics(const CameraIntrinsics& intrinsics);
    
    /**
     * #if ENGLISH
     * @brief Get camera intrinsics calibration
     * @return Optional camera intrinsics parameters
     * #endif
     * 
     * #if CHINESE
     * @brief 获取相机内参标定
     * @return 可选的相机内参参数
     * #endif
     */
    std::optional<CameraIntrinsics> getIntrinsics() const;

private:
    std::string serial_number_;
    std::shared_ptr<dvsense::DvsCamera> dvs_camera_;
    CameraState camera_state_ = CameraState::UNINITIALIZED;
    std::atomic<bool> recording_{false};
    
    // Trigger in tool and callback IDs to manage the callbacks
    std::shared_ptr<dvsense::CameraTool> trigger_in_tool_;
    std::vector<uint32_t> trigger_in_callback_ids_;
    bool trigger_in_processing_enabled_ = false;

    std::optional<CameraIntrinsics> intrinsics_;
    
    /**
     * @brief Update internal camera state (debug-logged)
     * @param new_state New camera state
     */
    void setState(CameraState new_state);
};

}  // namespace evrgb

#endif  // EVRGB_DVS_CAMERA_H_