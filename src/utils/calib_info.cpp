#include "utils/calib_info.h"

#include <fstream>
#include <type_traits>
#include <variant>

namespace {

using Matrix3x3 = cv::Matx33d;
using Matrix2x3 = cv::Matx23d;
using Vector3 = cv::Vec3d;

nlohmann::json mat33ToJson(const Matrix3x3& m)
{
    nlohmann::json rows = nlohmann::json::array();
    for (int r = 0; r < 3; ++r) {
        rows.push_back({m(r, 0), m(r, 1), m(r, 2)});
    }
    return rows;
}

Matrix3x3 jsonToMat33(const nlohmann::json& j)
{
    Matrix3x3 m = Matrix3x3::eye();
    if (!j.is_array() || j.size() != 3) {
        return m;
    }

    try {
        for (int r = 0; r < 3; ++r) {
            const auto& row = j.at(r);
            if (!row.is_array() || row.size() != 3) {
                return m;
            }
            for (int c = 0; c < 3; ++c) {
                m(r, c) = row.at(c).get<double>();
            }
        }
    } catch (...) {
        m = Matrix3x3::eye();
    }
    return m;
}

nlohmann::json mat23ToJson(const Matrix2x3& m)
{
    nlohmann::json rows = nlohmann::json::array();
    for (int r = 0; r < 2; ++r) {
        rows.push_back({m(r, 0), m(r, 1), m(r, 2)});
    }
    return rows;
}

Matrix2x3 jsonToMat23(const nlohmann::json& j)
{
    Matrix2x3 m{1.0, 0.0, 0.0,
                0.0, 1.0, 0.0};
    if (!j.is_array() || j.size() != 2) {
        return m;
    }

    try {
        for (int r = 0; r < 2; ++r) {
            const auto& row = j.at(r);
            if (!row.is_array() || row.size() != 3) {
                return m;
            }
            for (int c = 0; c < 3; ++c) {
                m(r, c) = row.at(c).get<double>();
            }
        }
    } catch (...) {
        m = Matrix2x3{1.0, 0.0, 0.0,
                      0.0, 1.0, 0.0};
    }
    return m;
}

void setError(const std::string& message, std::string* out)
{
    if (out) {
        *out = message;
    }
}

}  // namespace

namespace evrgb {

cv::Matx33d CameraIntrinsics::cameraMatrix() const
{
    return cv::Matx33d(
        fx,   skew, cx,
        0.0,  fy,   cy,
        0.0,  0.0,  1.0);
}

cv::Matx44d RigidTransform::matrix() const
{
    cv::Matx44d T = cv::Matx44d::eye();
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            T(r, c) = R(r, c);
        }
    }
    T(0, 3) = t[0];
    T(1, 3) = t[1];
    T(2, 3) = t[2];
    return T;
}

CameraIntrinsics CameraIntrinsics::idealFromPhysical(double focal_length_mm,
                                                     double pixel_size_um,
                                                     int width,
                                                     int height)
{
    CameraIntrinsics intr;
    intr.fx = focal_length_mm / (pixel_size_um * 1e-3);
    intr.fy = focal_length_mm / (pixel_size_um * 1e-3);
    intr.cx = (static_cast<double>(width) - 1.0) * 0.5;
    intr.cy = (static_cast<double>(height) - 1.0) * 0.5;
    intr.skew = 0.0;
    intr.width = width;
    intr.height = height;
    intr.distortion.clear();
    return intr;
}

void toJson(nlohmann::json& j, const CameraIntrinsics& intrinsics)
{
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

void fromJson(const nlohmann::json& j, CameraIntrinsics& intrinsics)
{
    intrinsics.fx = j.value("fx", 0.0);
    intrinsics.fy = j.value("fy", 0.0);
    intrinsics.cx = j.value("cx", 0.0);
    intrinsics.cy = j.value("cy", 0.0);
    intrinsics.skew = j.value("skew", 0.0);
    intrinsics.width = j.value("width", 0);
    intrinsics.height = j.value("height", 0);
    intrinsics.distortion = j.value("distortion", std::vector<double>{});
}

EVRGB_API void to_json(nlohmann::json& j, const CameraIntrinsics& intrinsics)
{
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

EVRGB_API void from_json(const nlohmann::json& j, CameraIntrinsics& intrinsics)
{
    fromJson(j, intrinsics);
}

EVRGB_API void to_json(nlohmann::json& j, const RigidTransform& rt)
{
    j = {
        {"rotation", mat33ToJson(rt.R)},
        {"translation", nlohmann::json::array({rt.t[0], rt.t[1], rt.t[2]})}
    };
}

EVRGB_API void from_json(const nlohmann::json& j, RigidTransform& rt)
{
    if (j.contains("rotation")) {
        rt.R = jsonToMat33(j.at("rotation"));
    }
    if (j.contains("translation")) {
        const auto& t = j.at("translation");
        if (t.is_array() && t.size() == 3) {
            try {
                rt.t[0] = t.at(0).get<double>();
                rt.t[1] = t.at(1).get<double>();
                rt.t[2] = t.at(2).get<double>();
            } catch (...) {
                rt.t = Vector3{0.0, 0.0, 0.0};
            }
        }
    }
}

EVRGB_API void to_json(nlohmann::json& j, const AffineTransform& a)
{
    j = mat23ToJson(a.A);
}

EVRGB_API void from_json(const nlohmann::json& j, AffineTransform& a)
{
    // Support legacy formats: either direct 2x3 array or object with "matrix".
    if (j.is_array()) {
        a.A = jsonToMat23(j);
        return;
    }

    if (j.contains("matrix")) {
        a.A = jsonToMat23(j.at("matrix"));
        return;
    }

    a.A = jsonToMat23(j);
}

EVRGB_API void to_json(nlohmann::json& j, const ComboCalibrationInfo& calib)
{
    j = nlohmann::json::object();
    std::visit([
        &j](auto&& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, RigidTransform>) {
                j["stereo_extrinsics"] = value;
            } else if constexpr (std::is_same_v<T, AffineTransform>) {
                j["beam_splitter_affine"] = value;
            }
        },
        calib);
}

EVRGB_API void from_json(const nlohmann::json& j, ComboCalibrationInfo& calib)
{
    calib = std::monostate{};
    if (j.contains("stereo_extrinsics")) {
        calib = j.at("stereo_extrinsics").get<RigidTransform>();
    } else if (j.contains("beam_splitter_affine")) {
        calib = j.at("beam_splitter_affine").get<AffineTransform>();
    }
}

EVRGB_API void toJson(nlohmann::json& j, const ComboCalibrationInfo& calib)
{
    to_json(j, calib);
}

EVRGB_API void fromJson(const nlohmann::json& j, ComboCalibrationInfo& calib)
{
    from_json(j, calib);
}

bool loadComboCalibration(const std::string& path, ComboCalibrationInfo& out, std::string* error_message)
{
    std::ifstream in(path);
    if (!in.is_open()) {
        setError("Failed to open calibration file: " + path, error_message);
        return false;
    }

    try {
        nlohmann::json j;
        in >> j;
        out = j.get<ComboCalibrationInfo>();
        return true;
    } catch (const std::exception& ex) {
        setError(ex.what(), error_message);
        return false;
    }
}

bool saveComboCalibration(const ComboCalibrationInfo& calib, const std::string& path, std::string* error_message)
{
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        setError("Failed to open calibration file for writing: " + path, error_message);
        return false;
    }

    try {
        nlohmann::json j = calib;
        out << j.dump(2);
        return true;
    } catch (const std::exception& ex) {
        setError(ex.what(), error_message);
        return false;
    }
}

}  // namespace evrgb
