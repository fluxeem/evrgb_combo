// Copyright (c) 2025.
// Author: EvRGB Team
// Description: Define the RGB camera class for device management and control.

#ifndef EVRGB_RGB_CAMERA_H_
#define EVRGB_RGB_CAMERA_H_

#include <vector>
#include <string>
#include <optional>
#ifdef _WIN32
    #include "opencv2/opencv.hpp"
#else
    #include "opencv4/opencv2/opencv.hpp"
#endif

#include "camera/i_camera.h"

#ifdef _WIN32
    #ifdef EVRGB_EXPORTS
        #define EVRGB_API __declspec(dllexport)
    #else
        #define EVRGB_API __declspec(dllimport)
    #endif
#else
    #define EVRGB_API   
#endif

namespace evrgb
{

/// @brief RGB camera information structure
struct EVRGB_API RgbCameraInfo
{
    /// Manufacturer name (null-terminated)
    char manufacturer[64] = {};

    /// Camera serial number (null-terminated)
    char serial_number[64] = {};

    /// Image width in pixels
    unsigned int width = 0;

    /// Image height in pixels
    unsigned int height = 0;
};

/**
 * @brief Enumerate all available RGB cameras
 * @return A vector of RgbCameraInfo describing each discovered camera
 */
EVRGB_API std::vector<RgbCameraInfo> enumerateAllRgbCameras();

/**
 * @brief Hikvision RGB camera implementation
 *
 * Provides a simple four-stage state machine for camera lifecycle:
 * initialize, start, stop and destroy.
 */
class EVRGB_API HikvisionRgbCamera : public IRgbCamera
{
public:
    /// Camera state enumeration
    enum class CameraState
    {
        UNINITIALIZED = 0,  ///< not initialized
        INITIALIZED,        ///< initialized
        STARTED,            ///< streaming/started
        STOPPED,            ///< stopped
        ERROR_STATUS               ///< error
    };

    /**
     * @brief Default constructor
     */
    HikvisionRgbCamera();
    
    /**
     * @brief Construct a camera object and optionally initialize by serial
     * @param serial_number Camera serial number to initialize, empty to skip
     */
    explicit HikvisionRgbCamera(const std::string& serial_number);
    
    /**
     * @brief Destructor, calls destroy() to clean up resources
     */
    ~HikvisionRgbCamera() override;

    /// ---------------- camera operations ----------------

    /**
     * @brief Initialize the camera
     * @param serial_number Camera serial number, empty to use first available
     * @return true on success, false on error
     */
    bool initialize(const std::string& serial_number = "") override;
    
    /**
     * @brief Start camera capture
     * @return true on success
     */
    bool start() override;
    
    /**
     * @brief Stop camera capture
     * @return true on success
     */
    bool stop() override;
    
    /**
     * @brief Destroy camera resources and reset state
     */
    void destroy() override;

    /**
     * @brief Get one frame from the camera (blocking)
     * @param image Output image
     * @return true on success
     */
    bool getLatestImage(cv::Mat& image) override;

    unsigned int getWidth() const override;
    unsigned int getHeight() const override;

    // Property access (typed)
    CameraStatus getInt(const std::string& key, IntProperty& out) override;
    CameraStatus setInt(const std::string& key, int64_t value) override;

    CameraStatus getEnum(const std::string& key, EnumProperty& out) override;
    CameraStatus setEnum(const std::string& key, uint32_t value) override;
    CameraStatus setEnumByName(const std::string& key, const std::string& name) override;

    CameraStatus getFloat(const std::string& key, FloatProperty& out) override;
    CameraStatus setFloat(const std::string& key, double value) override;

    CameraStatus getBool(const std::string& key, bool& out) override;
    CameraStatus setBool(const std::string& key, bool value) override;

    CameraStatus getString(const std::string& key, StringProperty& out) override;
    CameraStatus setString(const std::string& key, const std::string& value) override;

    CameraStatus getNodeAccessMode(const std::string& key, NodeAccessMode& mode) override;
    CameraStatus getNodeInterfaceType(const std::string& key, NodeInterfaceType& type) override;
    CameraStatus loadFeatureFile(const std::string& file_path) override;
    CameraStatus saveFeatureFile(const std::string& file_path) override;

    

    void* getNativeHandle() override;

    /**
     * @brief Get current camera state
     * @return Current CameraState
     */
    CameraState getState() const { return camera_state_; }
    
    /**
     * @brief Check whether camera is connected (handle valid)
     * @return true if connected
     */
    bool isConnected() const { return camera_handle_ != nullptr; }

    /// Disable copy and assignment
    HikvisionRgbCamera(const HikvisionRgbCamera&) = delete;
    HikvisionRgbCamera& operator=(const HikvisionRgbCamera&) = delete;

private:
    void* camera_handle_ = nullptr;
    CameraState camera_state_ = CameraState::UNINITIALIZED;
    std::string serial_number_;
    
    unsigned int width_ = 0;
    unsigned int height_ = 0;

    /**
     * @brief Find whether a camera with given serial exists
     * @param serial_number Serial number to search for
     * @return true if camera found
     */
    bool findCameraBySerial(const std::string& serial_number);
    
    /**
     * @brief Update internal camera state (debug-logged)
     * @param new_state New camera state
     */
    void setState(CameraState new_state);
};

} // namespace evrgb

#endif // EVRGB_RGB_CAMERA_H_

