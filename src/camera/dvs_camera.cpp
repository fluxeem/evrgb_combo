#include "camera/dvs_camera.h"
#include "utils/evrgb_logger.h"
#include <algorithm>

namespace evrgb
{

dvsense::DvsCameraManager dvs_camera_manager_;

std::vector<dvsense::CameraDescription> enumerateAllDvsCameras()
{
	return dvs_camera_manager_.getCameraDescs();
}

DvsCamera::DvsCamera()
    : serial_number_("")
    , dvs_camera_(nullptr)
    , camera_state_(CameraState::UNINITIALIZED)
{
    LOG_DEBUG("DvsCamera default constructor");
}

DvsCamera::DvsCamera(const std::string& serial_number)
    : serial_number_(serial_number)
    , dvs_camera_(nullptr)
    , camera_state_(CameraState::UNINITIALIZED)
{
    LOG_DEBUG("DvsCamera constructor with serial number: %s", serial_number.c_str());
    if (!initialize(serial_number))
    {
        LOG_ERROR("Failed to initialize DVS camera with serial number: %s", serial_number.c_str());
    }
}

DvsCamera::~DvsCamera()
{
    LOG_DEBUG("DvsCamera destructor");
    destroy();
}

bool DvsCamera::initialize(const std::string& serial_number)
{
    // If already initialized, destroy first
    if (camera_state_ != CameraState::UNINITIALIZED)
    {
        LOG_DEBUG("DVS camera already initialized, destroying first");
        destroy();
    }

    std::string target_serial = serial_number.empty() ? serial_number_ : serial_number;
    serial_number_ = target_serial;

    // If serial number is empty, use the first available camera
    if (target_serial.empty())
    {
        auto cameras = enumerateAllDvsCameras();
        if (cameras.empty())
        {
            LOG_ERROR("No DVS cameras found");
            setState(CameraState::ERROR_STATUS);
            return false;
        }
        target_serial = cameras[0].serial;
        serial_number_ = target_serial;
        LOG_INFO("No serial number specified, using first available DVS camera: %s", target_serial.c_str());
    }

    dvs_camera_ = dvs_camera_manager_.openCamera(target_serial);
    if (!dvs_camera_) {
        LOG_ERROR("Failed to create DVS camera with serial number: %s", target_serial.c_str());
        setState(CameraState::ERROR_STATUS);
        return false;
    } else {
        LOG_INFO("Created DVS camera with serial number: %s", target_serial.c_str());
    }

    setState(CameraState::INITIALIZED);
    return true;
}

bool DvsCamera::start()
{
    if (camera_state_ != CameraState::INITIALIZED && camera_state_ != CameraState::STOPPED) {
        LOG_ERROR("DVS camera must be initialized or stopped before starting (current state: %d)", 
                  static_cast<int>(camera_state_));
        return false;
    }

    if (!dvs_camera_) {
        LOG_ERROR("Invalid DVS camera handle");
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    int start_result = dvs_camera_->start();
    if (start_result != 0) {
        LOG_ERROR("DVS camera start failed, error code: %d", start_result);
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    setState(CameraState::STARTED);
    LOG_INFO("DVS camera started successfully");
    return true;
}

bool DvsCamera::stop()
{
    if (camera_state_ != CameraState::STARTED) {
        LOG_ERROR("DVS camera is not started (current state: %d)", static_cast<int>(camera_state_));
        return false;
    }

    if (!dvs_camera_) {
        LOG_ERROR("Invalid DVS camera handle");
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    // Disable trigger input processing before stopping
    if (trigger_in_processing_enabled_) {
        disableTriggerInProcessing();
    }

    int stop_result = dvs_camera_->stop();
    if (stop_result != 0) {
        LOG_ERROR("DVS camera stop failed, error code: %d", stop_result);
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    setState(CameraState::STOPPED);
    LOG_INFO("DVS camera stopped successfully");
    return true;
}

void DvsCamera::destroy()
{
    if (camera_state_ == CameraState::STARTED) {
        LOG_DEBUG("DVS camera is started, stopping first");
        stop();
    }

    if (trigger_in_processing_enabled_) {
        disableTriggerInProcessing();
    }

    // Release DVS camera resources
    dvs_camera_.reset();
    
    setState(CameraState::UNINITIALIZED);
    LOG_DEBUG("DVS camera destroyed successfully");
}

uint32_t DvsCamera::addTriggerInCallback(std::function<void(const dvsense::EventTriggerIn)> callback)
{
    if (!dvs_camera_) {
        LOG_ERROR("DVS camera not initialized, cannot add callback");
        return 0;
    }
    
    // Register the callback with the camera
    uint32_t callback_id = dvs_camera_->addTriggerInCallback(callback);
    
    // Store the callback ID so we can remove it later
    trigger_in_callback_ids_.push_back(callback_id);
    
    return callback_id;
}

bool DvsCamera::removeTriggerInCallback(uint32_t callback_id)
{
    if (!dvs_camera_) {
        LOG_ERROR("DVS camera not initialized, cannot remove callback");
        return false;
    }

    // Find and remove the callback ID from our list
    auto it = std::find(trigger_in_callback_ids_.begin(), trigger_in_callback_ids_.end(), callback_id);
    if (it != trigger_in_callback_ids_.end()) {
        bool result = dvs_camera_->removeTriggerInCallback(callback_id);
        if (result) {
            trigger_in_callback_ids_.erase(it);
        }
        return result;
    }
    
    LOG_WARN("Callback ID %u not found for removal", callback_id);
    return false;
}

void DvsCamera::removeAllTriggerInCallbacks()
{
    if (!dvs_camera_) {
        LOG_WARN("DVS camera not initialized, cannot remove callbacks");
        return;
    }

    // Remove all registered callback IDs
    for (uint32_t callback_id : trigger_in_callback_ids_) {
        dvs_camera_->removeTriggerInCallback(callback_id);
    }
    
    trigger_in_callback_ids_.clear();
}

bool DvsCamera::startRecording(const std::string& file_path)
{
    if (!dvs_camera_) {
        LOG_ERROR("DVS camera not initialized, cannot start recording");
        return false;
    }

    if (recording_.load()) {
        return true;
    }

    int rc = dvs_camera_->startRecording(file_path);
    if (rc != 0) {
        LOG_ERROR("DVS startRecording failed (code=%d) for %s", rc, file_path.c_str());
        return false;
    }

    recording_.store(true);
    LOG_DEBUG("DVS raw recording started: %s", file_path.c_str());
    return true;
}

bool DvsCamera::stopRecording()
{
    if (!dvs_camera_) {
        LOG_ERROR("DVS camera not initialized, cannot stop recording");
        return false;
    }

    if (!recording_.load()) {
        return true;
    }

    int rc = dvs_camera_->stopRecording();
    if (rc != 0) {
        LOG_WARN("DVS stopRecording returned code=%d", rc);
        return false;
    }

    recording_.store(false);
    LOG_DEBUG("DVS raw recording stopped");
    return true;
}



bool DvsCamera::enableTriggerInProcessing()
{
    if (!isConnected() || !dvs_camera_) {
        LOG_ERROR("DVS camera not initialized or available");
        return false;
    }

    // Check if already enabled
    if (trigger_in_processing_enabled_) {
        LOG_WARN("Trigger In processing is already enabled");
        return true;
    }

    try {
        // Get Trigger In tool from DVS camera
        trigger_in_tool_ = dvs_camera_->getTool(dvsense::ToolType::TOOL_TRIGGER_IN);
        if (!trigger_in_tool_) {
            LOG_ERROR("Failed to get Trigger In tool from DVS camera");
            return false;
        }
        
        // Enable Trigger In tool
        trigger_in_tool_->setParam("enable", true);
        trigger_in_processing_enabled_ = true;
        
        LOG_INFO("Trigger In processing enabled successfully");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to enable Trigger In processing: %s", e.what());
        trigger_in_processing_enabled_ = false;
        return false;
    }
}

bool DvsCamera::disableTriggerInProcessing()
{
    if (!isConnected() || !dvs_camera_) {
        LOG_ERROR("DVS camera not initialized or available");
        return false;
    }

    if (!trigger_in_processing_enabled_) {
        LOG_WARN("Trigger In processing is not enabled");
        return true;
    }

    try {
        // Remove all registered callbacks
        for (uint32_t callback_id : trigger_in_callback_ids_) {
            dvs_camera_->removeTriggerInCallback(callback_id);
        }
        trigger_in_callback_ids_.clear();
        
        if (trigger_in_tool_) {
            trigger_in_tool_.reset();  // Release tool
        }
        
        trigger_in_processing_enabled_ = false;
        LOG_INFO("Trigger In processing disabled successfully");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception when disabling Trigger In processing: %s", e.what());
        return false;
    }
}

void DvsCamera::setState(CameraState newState)
{
    if (camera_state_ != newState) {
        LOG_DEBUG("DVS camera state changed: %d -> %d", 
                  static_cast<int>(camera_state_), static_cast<int>(newState));
        camera_state_ = newState;
    }
}

bool DvsCamera::getDeviceModelName(std::string& model_name)
{
    if (!dvs_camera_) {
        LOG_ERROR("DVS camera not initialized, cannot get model name");
        return false;
    }

    model_name = dvs_camera_->getDescription().product;
    return true;
}

}  // namespace evrgb