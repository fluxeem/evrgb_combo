// Calibration data structures shared across the SDK.
#ifndef EVRGB_CALIB_INFO_H_
#define EVRGB_CALIB_INFO_H_

#include <array>
#include <string>
#include <vector>
#include <variant>

#ifdef _WIN32
    #ifdef EVRGB_EXPORTS
        #define EVRGB_API __declspec(dllexport)
    #else
        #define EVRGB_API __declspec(dllimport)
    #endif
#else
    #define EVRGB_API
#endif

#ifdef _WIN32
    #include <opencv2/opencv.hpp>
#else
    #include <opencv4/opencv2/opencv.hpp>
#endif

#include <nlohmann/json.hpp>

namespace evrgb {

/** Intrinsic parameters for a single camera. */
struct EVRGB_API CameraIntrinsics {
    double fx{0.0};
    double fy{0.0};
    double cx{0.0};
    double cy{0.0};
    double skew{0.0};
    int width{0};
    int height{0};
    std::vector<double> distortion;

    cv::Matx33d cameraMatrix() const;

    static CameraIntrinsics idealFromPhysical(double focal_length_mm,
                                              double pixel_size_um,
                                              int width,
                                              int height);
};

/** Rigid body transform (rotation + translation) used for STEREO alignment. */
struct EVRGB_API RigidTransform {
    cv::Matx33d R{cv::Matx33d::eye()};
    cv::Vec3d t{0.0, 0.0, 0.0};

    cv::Matx44d matrix() const;
};

/** Affine transform used for BEAM_SPLITTER alignment (2x3). */
struct EVRGB_API AffineTransform {
    cv::Matx23d A{1.0, 0.0, 0.0,
                  0.0, 1.0, 0.0};

    cv::Matx23d matrix() const { return A; }
};

using ComboCalibrationInfo = std::variant<std::monostate, RigidTransform, AffineTransform>;

// JSON serialization helpers (public API names)
EVRGB_API void toJson(nlohmann::json& j, const CameraIntrinsics& intrinsics);
EVRGB_API void fromJson(const nlohmann::json& j, CameraIntrinsics& intrinsics);
EVRGB_API void toJson(nlohmann::json& j, const ComboCalibrationInfo& calib);
EVRGB_API void fromJson(const nlohmann::json& j, ComboCalibrationInfo& calib);

// Standard nlohmann entry points to keep ADL working
EVRGB_API void to_json(nlohmann::json& j, const CameraIntrinsics& intrinsics);
EVRGB_API void from_json(const nlohmann::json& j, CameraIntrinsics& intrinsics);
EVRGB_API void to_json(nlohmann::json& j, const RigidTransform& transform);
EVRGB_API void from_json(const nlohmann::json& j, RigidTransform& transform);
EVRGB_API void to_json(nlohmann::json& j, const AffineTransform& transform);
EVRGB_API void from_json(const nlohmann::json& j, AffineTransform& transform);
EVRGB_API void to_json(nlohmann::json& j, const ComboCalibrationInfo& calib);
EVRGB_API void from_json(const nlohmann::json& j, ComboCalibrationInfo& calib);

// File IO helpers
EVRGB_API bool loadComboCalibration(const std::string& path, ComboCalibrationInfo& out, std::string* error_message = nullptr);
EVRGB_API bool saveComboCalibration(const ComboCalibrationInfo& calib, const std::string& path, std::string* error_message = nullptr);

}  // namespace evrgb

#endif  // EVRGB_CALIB_INFO_H_
