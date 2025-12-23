#ifndef EVRGB_I_CAMERA_H_
#define EVRGB_I_CAMERA_H_

#include <string>
#include <vector>
#include <cstdint>
#include <opencv2/opencv.hpp>

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

class EVRGB_API ICamera {
public:
    virtual ~ICamera() = default;

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

    

    // Escape hatch
    virtual void* getNativeHandle() = 0;
};

class EVRGB_API IRgbCamera : public ICamera {
public:
    virtual ~IRgbCamera() = default;

    // RGB specific
    virtual bool getLatestImage(cv::Mat& image) = 0;
    virtual unsigned int getWidth() const = 0;
    virtual unsigned int getHeight() const = 0;
};

} // namespace evrgb

#endif // EVRGB_I_CAMERA_H_
