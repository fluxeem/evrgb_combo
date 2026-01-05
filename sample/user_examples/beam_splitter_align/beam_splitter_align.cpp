#include "core/combo.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <cstring>
#include <memory>
#include <vector>
#include <algorithm>
#include <variant>

#include <nlohmann/json.hpp>

#include "utils/event_visualizer.h"
#include "utils/calib_info.h"

#ifdef _WIN32
    #include "opencv2/opencv.hpp"
#else
    #include "opencv4/opencv2/opencv.hpp"
#endif

#include "recording/synced_data_recorder.h"

namespace {

/**
 * @brief Thread-safe renderer that overlays DVS events onto RGB frames using the shared SDK visualizer.
 */
class SyncedFrameRenderer {
public:
    SyncedFrameRenderer() = default;

    void update(const evrgb::RgbImageWithTimestamp& rgb_data,
                const std::vector<dvsense::Event2D>& events) {
        cv::Mat bgr = ensureBgrFrame(rgb_data.image);
        if (bgr.empty()) {
            return;
        }

        ensureVisualizer(bgr.size());
        if (!visualizer_) {
            return;
        }

        if (!visualizer_->updateRgbFrame(bgr)) {
            return;
        }

        cv::Mat final_view;
        if (!visualizer_->visualizeEvents(events, final_view)) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        latest_frame_ = std::move(final_view);
    }

    void setIntrinsics(const evrgb::CameraIntrinsics& rgb, const evrgb::CameraIntrinsics& dvs) {
        rgb_intrinsics_ = rgb;
        dvs_intrinsics_ = dvs;
        if (visualizer_) {
            visualizer_->setIntrinsics(rgb, dvs);
        }
    }

    evrgb::AffineTransform setCalibration(const evrgb::AffineTransform& calib) {
        calibration_ = calib;
        if (visualizer_) {
            visualizer_->setCalibration(calibration_);
        }
        return calibration_;
    }

    void setEventSize(const cv::Size& event_size) {
        if (event_size.width <= 0 || event_size.height <= 0) {
            return;
        }

        event_size_ = event_size;
        if (visualizer_) {
            visualizer_->setEventSize(event_size_);
        }
    }

    evrgb::AffineTransform nudgeTranslation(const cv::Point& delta) {
        calibration_.A(0, 2) += static_cast<double>(delta.x);
        calibration_.A(1, 2) += static_cast<double>(delta.y);
        return setCalibration(calibration_);
    }

    evrgb::AffineTransform scaleAffine(double factor) {
        calibration_.A(0, 0) *= factor; calibration_.A(0, 1) *= factor;
        calibration_.A(1, 0) *= factor; calibration_.A(1, 1) *= factor;
        return setCalibration(calibration_);
    }

    bool getLatestFrame(cv::Mat& out_frame) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (latest_frame_.empty()) {
            return false;
        }

        out_frame = latest_frame_.clone();
        return true;
    }

private:
    static double depthTo8UScale(int depth) {
        switch (depth) {
            case CV_16U: return 1.0 / 256.0;
            case CV_32F:
            case CV_64F: return 255.0;
            default:     return 1.0;
        }
    }

    static cv::Mat ensureBgrFrame(const cv::Mat& input) {
        if (input.empty()) return {};

        if (input.channels() == 3) {
            if (input.type() == CV_8UC3) {
                return input;
            }
            cv::Mat converted;
            input.convertTo(converted, CV_8UC3, depthTo8UScale(input.depth()));
            return converted;
        }

        cv::Mat temp;
        input.convertTo(temp, CV_8U, depthTo8UScale(input.depth()));

        if (temp.channels() == 1) {
            cv::Mat bgr;
            cv::cvtColor(temp, bgr, cv::COLOR_GRAY2BGR);
            return bgr;
        }

        return {};
    }

private:
    cv::Mat latest_frame_;
    std::mutex mutex_;
    cv::Size2i event_size_{};
    evrgb::AffineTransform calibration_{};
    std::optional<evrgb::CameraIntrinsics> rgb_intrinsics_;
    std::optional<evrgb::CameraIntrinsics> dvs_intrinsics_;
    std::unique_ptr<evrgb::EventVisualizer> visualizer_;

    void ensureVisualizer(const cv::Size& rgb_size) {
        if (rgb_size.width <= 0 || rgb_size.height <= 0) {
            return;
        }

        if (event_size_.width <= 0 || event_size_.height <= 0) {
            return;
        }

        if (!visualizer_) {
            visualizer_ = std::make_unique<evrgb::EventVisualizer>(rgb_size, event_size_);
            visualizer_->setFlipX(true);
            visualizer_->setCalibration(calibration_);
            if (rgb_intrinsics_.has_value() && dvs_intrinsics_.has_value()) {
                visualizer_->setIntrinsics(*rgb_intrinsics_, *dvs_intrinsics_);
            }
        }
    }
};

cv::Point getArrowDirection(int key) {
    switch (key) {
        case 2424832: // Left arrow (Windows)
        case 65361:   // Left arrow (Linux)
        case 63234:   // Left arrow (macOS)
            return {-1, 0};
        case 2555904: // Right arrow (Windows)
        case 65363:   // Right arrow (Linux)
        case 63235:   // Right arrow (macOS)
            return {1, 0};
        case 2490368: // Up arrow (Windows)
        case 65362:   // Up arrow (Linux)
        case 63232:   // Up arrow (macOS)
            return {0, -1};
        case 2621440: // Down arrow (Windows)
        case 65364:   // Down arrow (Linux)
        case 63233:   // Down arrow (macOS)
            return {0, 1};
        default:
            return {0, 0};
    }
}

} // namespace

// ============================
// ============ main ==========
// ============================
int main(int argc, char** argv) {
    const std::string metadata_path = (argc > 1) ? argv[1] : "combo_metadata.json";

    // Enumerate cameras
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();

    if (rgb_cameras.empty()) {
        std::cerr << "No RGB camera found!" << std::endl;
        return -1;
    }
    if (dvs_cameras.empty()) {
        std::cerr << "No DVS camera found!" << std::endl;
        return -1;
    }

    std::string rgb_serial = rgb_cameras[0].serial_number;
    std::string dvs_serial = dvs_cameras[0].serial;

    evrgb::Combo combo(rgb_serial, dvs_serial,  evrgb::Combo::Arrangement::BEAM_SPLITTER, 100);

    evrgb::SyncedRecorderConfig recorder_cfg;
    recorder_cfg.output_dir = "recordings";
    recorder_cfg.fps = 30.0;
    recorder_cfg.fourcc = "mp4v";

    bool loaded_from_file = combo.loadMetadata(metadata_path, nullptr);
    if (loaded_from_file) {
        std::cout << "Loaded combo metadata from: " << metadata_path << std::endl;
    } else {
        std::cout << "Metadata not loaded, using defaults (path attempted: " << metadata_path << ")" << std::endl;
    }

    // Fetch current metadata (after load attempt) to configure renderer and defaults.
    evrgb::ComboMetadata current_meta = combo.getMetadata();

    evrgb::CameraIntrinsics dvs_intrinsics{};
    evrgb::CameraIntrinsics rgb_intrinsics{};
    if (current_meta.rgb.intrinsics && current_meta.dvs.intrinsics) {
        rgb_intrinsics = *current_meta.rgb.intrinsics;
        dvs_intrinsics = *current_meta.dvs.intrinsics;
    } else {
        std::cerr << "Intrinsics not found in metadata, using ideal pinhole model based on physical parameters." << std::endl;
        dvs_intrinsics = evrgb::CameraIntrinsics::idealFromPhysical(
            16.0,    // focal length in mm
            4.86, // pixel size in um
            combo.getRawDvsCamera()->getWidth(),
            combo.getRawDvsCamera()->getHeight()
        );

        rgb_intrinsics = evrgb::CameraIntrinsics::idealFromPhysical(
            16.0,    // focal length in mm
            4.80, // pixel size in um
            combo.getRgbCamera()->getWidth(),
            combo.getRgbCamera()->getHeight()
        );

        combo.getDvsCamera()->setIntrinsics(dvs_intrinsics);
        combo.getRgbCamera()->setIntrinsics(rgb_intrinsics);
    }

    auto recorder = std::make_shared<evrgb::SyncedDataRecorder>();
    combo.setSyncedDataRecorder(recorder);
    std::cout << "Press SPACE to toggle recording (stored under: " << recorder_cfg.output_dir << ")" << std::endl;

    auto renderer = std::make_shared<SyncedFrameRenderer>();
    renderer->setEventSize(cv::Size(combo.getRawDvsCamera()->getWidth(), combo.getRawDvsCamera()->getHeight()));
    const std::string window_name = "Combo Synced View";
    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);
    std::cout << "Window created. Press 'q' or ESC to exit." << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  - Arrow Keys: Adjust affine translation" << std::endl;
    std::cout << "  - '+': Scale up affine" << std::endl;
    std::cout << "  - '-': Scale down affine" << std::endl;
    std::cout << "  - SPACE: Start/Stop recording" << std::endl;

    combo.setSyncedCallback(
        [renderer](const evrgb::RgbImageWithTimestamp& rgb, const std::vector<dvsense::Event2D>& events) {
            renderer->update(rgb, events);
        }
    );

    if (!combo.init()) {
        std::cerr << "Combo init failed" << std::endl;
        return -1;
    }

    if (!combo.start()) {
        std::cerr << "Combo start failed" << std::endl;
        return -1;
    }

    {
        auto meta = combo.getMetadata();
        nlohmann::json j = meta;
        std::cout << "Combo metadata:\n" << j.dump(2) << std::endl;
    }

    renderer->setIntrinsics(rgb_intrinsics, dvs_intrinsics);

    evrgb::AffineTransform affine_calib{};
    if (std::holds_alternative<evrgb::AffineTransform>(combo.calibration_info)) {
        affine_calib = std::get<evrgb::AffineTransform>(combo.calibration_info);
    }
    renderer->setCalibration(affine_calib);
    combo.calibration_info = affine_calib;

    std::cout << "Combo started. Adjust the affine alignment with the arrow keys and +/- for scale." << std::endl;

    // Main thread: fetch rendered frame and display
    cv::Mat frame_to_show;
    while (true) {
        cv::Mat latest;
        if (renderer->getLatestFrame(latest)) {
            frame_to_show = std::move(latest);
        }

        if (!frame_to_show.empty()) {
            cv::imshow(window_name, frame_to_show);
        }

        const int key = cv::waitKeyEx(33);

        if (key == -1) {
            continue;
        }

        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }

        if (key == ' ') {
            if (recorder->isActive()) {
                combo.stopRecording();
                std::cout << "Recording stopped." << std::endl;
            } else {
                if (!combo.startRecording(recorder_cfg)) {
                    std::cout << "Recorder start failed." << std::endl;
                } else {
                    std::cout << "Recording started (dir: " << recorder_cfg.output_dir << ")" << std::endl;
                }
            }
            continue;
        }

        const cv::Point delta = getArrowDirection(key);
        if (delta.x != 0 || delta.y != 0) {
            auto calib = renderer->nudgeTranslation(delta);
            combo.calibration_info = calib;
            std::cout << "Affine translation -> tx: " << calib.A(0, 2) << ", ty: " << calib.A(1, 2) << std::endl;
            continue;
        }

        if (key == '+' || key == '=') {
            auto calib = renderer->scaleAffine(1.02);
            combo.calibration_info = calib;
            std::cout << "Affine scaled up (2%)" << std::endl;
            continue;
        }

        if (key == '-' || key == '_') {
            auto calib = renderer->scaleAffine(0.98);
            combo.calibration_info = calib;
            std::cout << "Affine scaled down (2%)" << std::endl;
            continue;
        }
    }

    combo.stop();
    cv::destroyWindow(window_name);

    if (recorder && recorder->isActive()) {
        combo.stopRecording();
    }

    {
        std::string err;
        if (!combo.saveMetadata(metadata_path, &err)) {
            std::cerr << "Failed to save combo metadata to '" << metadata_path << "': " << err << std::endl;
        } else {
            std::cout << "Saved combo metadata to: " << metadata_path << std::endl;
        }
    }

    std::cout << "Exit." << std::endl;
    return 0;
}
