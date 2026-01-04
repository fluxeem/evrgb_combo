#include "core/combo.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <cstring>
#include <memory>
#include <vector>
#include <algorithm>

#include "utils/event_visualizer.h"

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
    void update(const evrgb::RgbImageWithTimestamp& rgb_data,
                const std::vector<dvsense::Event2D>& events) {
        cv::Mat bgr = ensureBgrFrame(rgb_data.image);
        if (bgr.empty()) {
            return;
        }

        event_size_ = inferEventFrameSize(events, event_size_);
        ensureVisualizer(bgr.size());
        if (!visualizer_) {
            return;
        }

        visualizer_->setEventSize(event_size_);
        visualizer_->setEventOffset(manual_offset_);

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

    void toggleDisplayMode() {
        if (!visualizer_) {
            return;
        }
        const auto mode = visualizer_->toggleDisplayMode();
        std::cout << "Display Mode: "
                  << (mode == evrgb::EventVisualizer::DisplayMode::SideBySide ? "Side-by-Side" : "Overlay Only")
                  << std::endl;
    }

    cv::Point adjustEventOffset(const cv::Point& delta) {
        manual_offset_.x += delta.x;
        manual_offset_.y += delta.y;
        if (visualizer_) {
            visualizer_->setEventOffset(manual_offset_);
        }
        return manual_offset_;
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

    static cv::Size2i inferEventFrameSize(const std::vector<dvsense::Event2D>& events, cv::Size2i current) {
        int max_x = current.width;
        int max_y = current.height;
        for (const auto& e : events) {
            max_x = std::max(max_x, static_cast<int>(e.x) + 1);
            max_y = std::max(max_y, static_cast<int>(e.y) + 1);
        }
        return {max_x, max_y};
    }

    void ensureVisualizer(const cv::Size& rgb_size) {
        if (visualizer_) {
            return;
        }
        if (rgb_size.width <= 0 || rgb_size.height <= 0) {
            return;
        }
        visualizer_ = std::make_unique<evrgb::EventVisualizer>(rgb_size, event_size_);
        visualizer_->setEventOffset(manual_offset_);
    }

private:
    cv::Mat latest_frame_;
    std::mutex mutex_;
    cv::Size2i event_size_{};
    cv::Point2i manual_offset_{0, 0};
    std::unique_ptr<evrgb::EventVisualizer> visualizer_;
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
int main() {
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

    auto recorder = std::make_shared<evrgb::SyncedDataRecorder>();
    combo.setSyncedDataRecorder(recorder);
    std::cout << "Press SPACE to toggle recording (stored under: " << recorder_cfg.output_dir << ")" << std::endl;

    auto renderer = std::make_shared<SyncedFrameRenderer>();
    const std::string window_name = "Combo Synced View";
    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);
    std::cout << "Window created. Press 'q' or ESC to exit." << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  - Arrow Keys: Shift event overlay" << std::endl;
    std::cout << "  - 'm': Toggle display mode (Overlay / Side-by-Side)" << std::endl;
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

    std::cout << "Combo started. Adjust the overlay alignment with the arrow keys." << std::endl;

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

        if (key == 'm' || key == 'M') {
            renderer->toggleDisplayMode();
            continue;
        }

        const cv::Point delta = getArrowDirection(key);
        if (delta.x != 0 || delta.y != 0) {
            const cv::Point updated = renderer->adjustEventOffset(delta);
            std::cout << "Event offset -> x: " << updated.x << ", y: " << updated.y << std::endl;
        }
    }

    combo.stop();
    cv::destroyWindow(window_name);

    if (recorder && recorder->isActive()) {
        combo.stopRecording();
    }

    std::cout << "Exit." << std::endl;
    return 0;
}
