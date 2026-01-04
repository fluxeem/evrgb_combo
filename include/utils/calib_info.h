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
                                              double pixel_size_mm,
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

/** Full calibration description of the RGB+DVS combo. */
struct EVRGB_API ComboCalibration {
    std::string arrangement{"UNKNOWN"};
    std::variant<std::monostate, RigidTransform, AffineTransform> extrinsics{};
};

// JSON serialization helpers
void to_json(nlohmann::json& j, const CameraIntrinsics& intrinsics);
void from_json(const nlohmann::json& j, CameraIntrinsics& intrinsics);

void to_json(nlohmann::json& j, const ComboCalibration& calib);
void from_json(const nlohmann::json& j, ComboCalibration& calib);

// File IO helpers
EVRGB_API bool loadComboCalibration(const std::string& path, ComboCalibration& out, std::string* error_message = nullptr);
EVRGB_API bool saveComboCalibration(const ComboCalibration& calib, const std::string& path, std::string* error_message = nullptr);

}  // namespace evrgb

#endif  // EVRGB_CALIB_INFO_H_
