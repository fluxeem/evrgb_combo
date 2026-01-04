#ifndef EVRGB_I_CAMERA_H_
#define EVRGB_I_CAMERA_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <opencv2/opencv.hpp>

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



/// Strongly typed property descriptions; implementations fill from the underlying SDK
struct IntProperty {
    int64_t value = 0;
    int64_t min = 0;
    int64_t max = 0;
    int64_t inc = 0;
};

/// One enum choice
struct EnumEntry {
    uint32_t value = 0;
    std::string name;
    bool available = false;
};

/// Enum value plus available entries
struct EnumProperty {
    uint32_t value = 0;
    std::vector<EnumEntry> entries;
};

/// Floating property with limits
struct FloatProperty {
    double value = 0.0;
    double min = 0.0;
    double max = 0.0;
    double inc = 0.0;
};

/// String property with max length
struct StringProperty {
    std::string value;
    uint32_t max_len = 0;
};

/// Result of a camera operation (SDK-agnostic)
struct CameraStatus {
    /// 0 = success; non-zero is provider-specific error code
    int32_t code = 0;
    /// Human-readable message (optional)
    std::string message;
    /// Convenience check for success
    bool success() const { return code == 0; }
};

/// Node access mode (SDK-agnostic); implementations map from vendor enums.
enum class NodeAccessMode : int32_t {
    Unknown = 0, ///< Not reported or unmapped
    NA,          ///< Not available / not implemented
    RO,          ///< Read-only
    WO,          ///< Write-only
    RW,          ///< Read-write
    Cycle        ///< Access mode can change depending on context/state
};

/// Node interface type (SDK-agnostic); implementations map from vendor enums.
enum class NodeInterfaceType : int32_t {
    Unknown = 0, ///< Not reported or unmapped
    Integer,     ///< Numeric integer node
    Boolean,     ///< Boolean node
    Command,     ///< Fire-and-forget command node
    Float,       ///< Floating-point node
    Enumeration, ///< Enum node (symbolic/value)
    String,      ///< String node
    Category     ///< Grouping node
};

class EVRGB_API IRgbCamera {
public:
    virtual ~IRgbCamera() = default;

    // Lifecycle
    virtual bool initialize(const std::string& serial_number = "") = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual void destroy() = 0;

    // Strongly typed property access (maps 1:1 to MV_CC_* APIs)
    virtual CameraStatus getInt(const std::string& key, IntProperty& out) = 0;
    virtual CameraStatus setInt(const std::string& key, int64_t value) = 0;

    virtual CameraStatus getEnum(const std::string& key, EnumProperty& out) = 0;
    virtual CameraStatus setEnum(const std::string& key, uint32_t value) = 0;
    virtual CameraStatus setEnumByName(const std::string& key, const std::string& name) = 0;

    virtual CameraStatus getFloat(const std::string& key, FloatProperty& out) = 0;
    virtual CameraStatus setFloat(const std::string& key, double value) = 0;

    virtual CameraStatus getBool(const std::string& key, bool& out) = 0;
    virtual CameraStatus setBool(const std::string& key, bool value) = 0;

    virtual CameraStatus getString(const std::string& key, StringProperty& out) = 0;
    virtual CameraStatus setString(const std::string& key, const std::string& value) = 0;

    // Node metadata / feature files
    virtual CameraStatus getNodeAccessMode(const std::string& key, NodeAccessMode& mode) = 0;
    virtual CameraStatus getNodeInterfaceType(const std::string& key, NodeInterfaceType& type) = 0;
    virtual CameraStatus loadFeatureFile(const std::string& file_path) = 0;
    virtual CameraStatus saveFeatureFile(const std::string& file_path) = 0;

    // shortcut
    virtual CameraStatus getDeviceModelName(StringProperty& out)
    {
        out = StringProperty{"unknown", 0};
        return CameraStatus{-1, "Not implemented"};
    }

    // Escape hatch
    virtual void* getNativeHandle() = 0;

    // RGB specific
    virtual bool getLatestImage(cv::Mat& image) = 0;
    virtual unsigned int getWidth() const = 0;
    virtual unsigned int getHeight() const = 0;

    // Intrinsic parameters (optional)
    virtual void setIntrinsics(const CameraIntrinsics& intrinsics) = 0;
    virtual std::optional<CameraIntrinsics> getIntrinsics() const = 0;
};

/// RGB camera information structure (device-level metadata)
struct EVRGB_API RgbCameraInfo {
    char manufacturer[64] = {};
    char serial_number[64] = {};
    unsigned int width = 0;
    unsigned int height = 0;
};

/// Factory function used to instantiate concrete RGB camera drivers
using RgbCameraFactoryFn = std::function<std::shared_ptr<IRgbCamera>()>;
using RgbEnumeratorFn = std::function<std::vector<RgbCameraInfo>()>;

// Discovery and creation helpers (shared across brands)
EVRGB_API std::vector<RgbCameraInfo> enumerateAllRgbCameras();
EVRGB_API void registerRgbCameraFactory(const std::string& manufacturer_prefix, RgbCameraFactoryFn creator);
EVRGB_API void registerRgbEnumerator(RgbEnumeratorFn enumerator);
EVRGB_API std::shared_ptr<IRgbCamera> createRgbCamera(const RgbCameraInfo& info);
EVRGB_API std::shared_ptr<IRgbCamera> createRgbCameraBySerial(const std::string& serial_number);

} // namespace evrgb

#endif // EVRGB_I_CAMERA_H_
