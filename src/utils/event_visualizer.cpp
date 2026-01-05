#include "utils/event_visualizer.h"

#include <algorithm>
#include <cmath>

#include "DvsenseDriver/FileReader/DvsFileReader.h"

namespace evrgb {

EventVisualizer::EventVisualizer(const cv::Size2i& rgb_size,
                                 const cv::Size2i& event_size,
                                 const cv::Vec3b& on_color,
                                 const cv::Vec3b& off_color)
    : rgb_size_(rgb_size),
      event_size_(event_size),
      on_color_(on_color),
      off_color_(off_color) {}

EventVisualizer::DisplayMode EventVisualizer::toggleDisplayMode() {
    display_mode_ = (display_mode_ == DisplayMode::Overlay)
        ? DisplayMode::SideBySide
        : DisplayMode::Overlay;
    return display_mode_;
}

void EventVisualizer::setDisplayMode(DisplayMode mode) {
    display_mode_ = mode;
}

EventVisualizer::DisplayMode EventVisualizer::displayMode() const {
    return display_mode_;
}

void EventVisualizer::setEventSize(const cv::Size2i& size) {
    event_size_ = size;
    refreshFusedTransform();
}

cv::Size2i EventVisualizer::eventSize() const {
    return event_size_;
}

cv::Size2i EventVisualizer::rgbSize() const {
    return rgb_size_;
}

cv::Point2i EventVisualizer::eventOffset() const {
    return manual_offset_;
}

void EventVisualizer::setEventOffset(const cv::Point2i& offset) {
    manual_offset_ = offset;
}

void EventVisualizer::setColors(const cv::Vec3b& on_color, const cv::Vec3b& off_color) {
    on_color_ = on_color;
    off_color_ = off_color;
}

cv::Vec3b EventVisualizer::onColor() const { return on_color_; }
cv::Vec3b EventVisualizer::offColor() const { return off_color_; }

void EventVisualizer::setFlipX(bool flip) {
    flip_x_ = flip;
    refreshFusedTransform();
}

bool EventVisualizer::flipX() const {
    return flip_x_;
}

bool EventVisualizer::updateRgbFrame(const cv::Mat& rgb_frame) {
    if (rgb_frame.empty()) {
        return false;
    }

    if (rgb_frame.size() != rgb_size_) {
        return false;
    }

    rgb_frame_ = rgb_frame.clone();
    return true;
}

void EventVisualizer::setCalibration(const ComboCalibrationInfo& calib)
{
    calibration_ = calib;
    refreshFusedTransform();
}

void EventVisualizer::setIntrinsics(const CameraIntrinsics& rgb, const CameraIntrinsics& dvs)
{
    rgb_intrinsics_ = rgb;
    dvs_intrinsics_ = dvs;
    refreshFusedTransform();
}

bool EventVisualizer::visualizeEvents(
    dvsense::Event2DVector::const_iterator begin,
    dvsense::Event2DVector::const_iterator end,
    cv::Mat& output_frame) {

    if (rgb_frame_.empty()) {
        return false;
    }

    const bool has_events = (begin != end);

    // Prepare output frame
    if (display_mode_ == DisplayMode::Overlay) {
        output_frame = rgb_frame_.clone();
    } else {
        output_frame.create(rgb_size_.height, rgb_size_.width * 2, rgb_frame_.type());
        output_frame.setTo(cv::Scalar::all(0));
        rgb_frame_.copyTo(output_frame(cv::Rect(0, 0, rgb_size_.width, rgb_size_.height)));
    }

    if (!has_events) {
        return true;
    }

    if (event_size_.width <= 0 || event_size_.height <= 0) {
        return false;
    }

    overlayEvents(begin, end, output_frame);
    return true;
}

bool EventVisualizer::visualizeEvents(
    const dvsense::Event2DVector& events,
    cv::Mat& output_frame) {
    return visualizeEvents(events.begin(), events.end(), output_frame);
}

bool EventVisualizer::overlayEvents(
    dvsense::Event2DVector::const_iterator begin,
    dvsense::Event2DVector::const_iterator end,
    cv::Mat& output_frame) {
    if (rgb_frame_.empty()) {
        return false;
    }

    if (begin == end) {
        return true;
    }

    const bool use_affine = (display_mode_ == DisplayMode::Overlay) && useAffineMapping();
    const cv::Point2i base_offset = computeBaseOffset();

    if (use_affine) {
        for (auto it = begin; it != end; ++it) {
            const auto& e = *it;
            const double tx = fused_affine_(0, 0) * static_cast<double>(e.x)
                             + fused_affine_(0, 1) * static_cast<double>(e.y)
                             + fused_affine_(0, 2) + manual_offset_.x;
            const double ty = fused_affine_(1, 0) * static_cast<double>(e.x)
                             + fused_affine_(1, 1) * static_cast<double>(e.y)
                             + fused_affine_(1, 2) + manual_offset_.y;

            const int x = static_cast<int>(std::lround(tx));
            const int y = static_cast<int>(std::lround(ty));
            if (x < 0 || y < 0 || x >= output_frame.cols || y >= output_frame.rows) {
                continue;
            }
            output_frame.at<cv::Vec3b>(y, x) = e.polarity ? on_color_ : off_color_;
        }
        return true;
    }

    const cv::Point2i offset = getEventOffset();
    const float scale = calcScaleFactor();

    for (auto it = begin; it != end; ++it) {
        const auto& e = *it;
        const float source_x = (flip_x_ && event_size_.width > 0)
            ? (static_cast<float>(event_size_.width - 1) - e.x)
            : e.x;
        const int x = static_cast<int>(source_x * scale) + offset.x;
        const int y = static_cast<int>(e.y * scale) + offset.y;
        if (x < 0 || y < 0 || x >= output_frame.cols || y >= output_frame.rows) {
            continue;
        }
        output_frame.at<cv::Vec3b>(y, x) = e.polarity ? on_color_ : off_color_;
    }
    return true;
}

float EventVisualizer::calcScaleFactor() const {
    if (event_size_.width <= 0 || event_size_.height <= 0) {
        return 1.0f;
    }

    const float scale_x = static_cast<float>(rgb_size_.width) / static_cast<float>(event_size_.width);
    const float scale_y = static_cast<float>(rgb_size_.height) / static_cast<float>(event_size_.height);
    return std::min(scale_x, scale_y);
}

cv::Point2i EventVisualizer::getEventOffset() const {
    const float scale = calcScaleFactor();
    const int offset_x = (event_size_.width > 0)
        ? (rgb_size_.width - static_cast<int>(event_size_.width * scale)) / 2
        : 0;
    const int offset_y = (event_size_.height > 0)
        ? (rgb_size_.height - static_cast<int>(event_size_.height * scale)) / 2
        : 0;

    const int base_x = (display_mode_ == DisplayMode::SideBySide)
        ? (offset_x + rgb_size_.width)
        : offset_x;

    return {base_x + manual_offset_.x, offset_y + manual_offset_.y};
}

bool EventVisualizer::useAffineMapping() const
{
    return std::holds_alternative<AffineTransform>(calibration_);
}

cv::Point2i EventVisualizer::computeBaseOffset() const
{
    if (display_mode_ == DisplayMode::SideBySide) {
        return {rgb_size_.width, 0};
    }
    return {0, 0};
}

bool EventVisualizer::intrinsicsReady() const
{
    return rgb_intrinsics_.has_value() && dvs_intrinsics_.has_value();
}

bool EventVisualizer::eventSizeValid() const
{
    return event_size_.width > 0 && event_size_.height > 0;
}

void EventVisualizer::refreshFusedTransform()
{
    fused_affine_ = cv::Matx23d(1.0, 0.0, 0.0,
                                0.0, 1.0, 0.0);

    if (!intrinsicsReady() || !useAffineMapping() || !eventSizeValid()) {
        return;
    }

    const auto affine = std::get<AffineTransform>(calibration_);

    // Build K_rgb * (A * K_dvs^{-1}) including optional horizontal flip.
    const auto& rgb_intr = *rgb_intrinsics_;
    const auto& dvs_intr = *dvs_intrinsics_;

    const double flip_sign = flip_x_ ? -1.0 : 1.0;
    const double flip_bias = flip_x_ ? static_cast<double>(event_size_.width - 1) : 0.0;

    const cv::Matx33d K_rgb = rgb_intr.cameraMatrix();

    const cv::Matx33d K_dvs_inv(
        flip_sign / dvs_intr.fx, 0.0, (flip_bias - dvs_intr.cx) / dvs_intr.fx,
        0.0, 1.0 / dvs_intr.fy, -dvs_intr.cy / dvs_intr.fy,
        0.0, 0.0, 1.0);

    const cv::Matx33d A = cv::Matx33d(
        affine.A(0, 0), affine.A(0, 1), affine.A(0, 2),
        affine.A(1, 0), affine.A(1, 1), affine.A(1, 2),
        0.0,            0.0,            1.0);

    const cv::Matx33d M = A * (K_rgb * K_dvs_inv);

    fused_affine_(0, 0) = M(0, 0);
    fused_affine_(0, 1) = M(0, 1);
    fused_affine_(0, 2) = M(0, 2);

    fused_affine_(1, 0) = M(1, 0);
    fused_affine_(1, 1) = M(1, 1);
    fused_affine_(1, 2) = M(1, 2);

    const auto base_offset = computeBaseOffset();
    fused_affine_(0, 2) += base_offset.x;
    fused_affine_(1, 2) += base_offset.y;

    const bool finite = std::isfinite(fused_affine_(0, 0)) && std::isfinite(fused_affine_(0, 1)) &&
                        std::isfinite(fused_affine_(0, 2)) && std::isfinite(fused_affine_(1, 0)) &&
                        std::isfinite(fused_affine_(1, 1)) && std::isfinite(fused_affine_(1, 2));
    if (!finite) {
        fused_affine_ = cv::Matx23d(1.0, 0.0, 0.0,
                                    0.0, 1.0, 0.0);
    }

}

}  // namespace evrgb
