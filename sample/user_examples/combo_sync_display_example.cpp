#include "core/combo.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <cstring>
#include <memory>
#include <vector>
#include <algorithm>
#include <atomic>

#ifdef _WIN32
    #include "opencv2/opencv.hpp"
#else
    #include "opencv4/opencv2/opencv.hpp"
#endif

#include "recording/synced_data_recorder.h"

namespace {

/**
 * @brief Thread-safe renderer that overlays DVS events onto RGB frames.
 */
class SyncedFrameRenderer {
public:
    enum class DisplayMode {
        OverlayOnly,
        SideBySide
    };

    void update(const evrgb::RgbImageWithTimestamp& rgb_data,
                const std::vector<dvsense::Event2D>& events) {
        cv::Mat bgr = ensureBgrFrame(rgb_data.image);
        if (bgr.empty()) {
            return;
        }

        const cv::Size event_size = updateEventFrameSize(events);
        const cv::Point manual_offset = getEventOffset();

        cv::Mat final_view;
        if (display_mode_ == DisplayMode::SideBySide) {
            // 1. Original RGB (Left)
            cv::Mat left_view = bgr.clone();

            // 2. Create Event-only frame on black background (Right)
            cv::Mat right_view = cv::Mat::zeros(bgr.size(), bgr.type());
            overlayEvents(right_view, events, event_size, manual_offset);

            // 3. Combine side-by-side
            cv::hconcat(left_view, right_view, final_view);
        } else {
            // Overlay only
            final_view = bgr.clone();
            overlayEvents(final_view, events, event_size, manual_offset);
        }

        std::lock_guard<std::mutex> lock(mutex_);
        latest_frame_ = std::move(final_view);
    }

    void toggleDisplayMode() {
        if (display_mode_ == DisplayMode::OverlayOnly) {
            display_mode_ = DisplayMode::SideBySide;
            std::cout << "Display Mode: Side-by-Side" << std::endl;
        } else {
            display_mode_ = DisplayMode::OverlayOnly;
            std::cout << "Display Mode: Overlay Only" << std::endl;
        }
    }

    cv::Point adjustEventOffset(const cv::Point& delta) {
        const int new_x = (event_offset_x_ += delta.x);
        const int new_y = (event_offset_y_ += delta.y);
        return {new_x, new_y};
    }

    cv::Point getEventOffset() const {
        return {event_offset_x_.load(), event_offset_y_.load()};
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

    static void overlayEvents(cv::Mat& frame,
                              const std::vector<dvsense::Event2D>& events,
                              const cv::Size& event_frame_size,
                              const cv::Point& manual_offset) {
        if (events.empty()) {
            return;
        }

        const int effective_width = event_frame_size.width > 0 ? event_frame_size.width : frame.cols;
        const int effective_height = event_frame_size.height > 0 ? event_frame_size.height : frame.rows;
        const int offset_x = (frame.cols - effective_width) / 2;
        const int offset_y = (frame.rows - effective_height) / 2;

        for (const auto& e : events) {
            int x = static_cast<int>(e.x);
            int y = static_cast<int>(e.y);
            const int px = x + offset_x + manual_offset.x;
            const int py = y + offset_y + manual_offset.y;
            if (px < 0 || py < 0 || px >= frame.cols || py >= frame.rows) {
                continue;
            }

            frame.at<cv::Vec3b>(py, px) =
                e.polarity ? cv::Vec3b(0, 0, 255)
                           : cv::Vec3b(255, 0, 0);
        }
    }

    cv::Size updateEventFrameSize(const std::vector<dvsense::Event2D>& events) {
        for (const auto& e : events) {
            event_frame_size_.width = std::max(event_frame_size_.width, static_cast<int>(e.x) + 1);
            event_frame_size_.height = std::max(event_frame_size_.height, static_cast<int>(e.y) + 1);
        }
        return event_frame_size_;
    }

private:
    cv::Mat latest_frame_;
    std::mutex mutex_;
    cv::Size event_frame_size_;
    std::atomic<int> event_offset_x_{0};
    std::atomic<int> event_offset_y_{0};
    std::atomic<DisplayMode> display_mode_{DisplayMode::OverlayOnly};
};

/**
 * @brief Handles window creation and displaying frames.
 */
class FrameViewer {
public:
    explicit FrameViewer(const std::string& window_name)
        : window_name_(window_name) {}

    void createWindow() {
        cv::namedWindow(window_name_, cv::WINDOW_AUTOSIZE);
        std::cout << "Window created. Press 'q' or ESC to exit." << std::endl;
        std::cout << "Controls:" << std::endl;
        std::cout << "  - Arrow Keys: Shift event overlay" << std::endl;
        std::cout << "  - 'm': Toggle display mode (Overlay / Side-by-Side)" << std::endl;
        std::cout << "  - SPACE: Start/Stop recording" << std::endl;
    }

    void destroyWindow() {
        cv::destroyWindow(window_name_);
    }

    int show(const cv::Mat& frame, int wait_ms = 1) {
        if (!frame.empty()) {
            cv::imshow(window_name_, frame);
        }

        return cv::waitKeyEx(wait_ms);
    }

private:
    std::string window_name_;
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

    std::cout << "Using RGB: " << rgb_serial << std::endl;
    std::cout << "Using DVS: " << dvs_serial << std::endl;

    evrgb::Combo combo(rgb_serial, dvs_serial, 100);

    evrgb::SyncedRecorderConfig recorder_cfg;
    recorder_cfg.output_dir = "recordings";
    recorder_cfg.fps = 30.0;
    recorder_cfg.fourcc = "mp4v";

    auto recorder = std::make_shared<evrgb::SyncedDataRecorder>();
    combo.setSyncedDataRecorder(recorder);
    std::cout << "Press SPACE to toggle recording (stored under: " << recorder_cfg.output_dir << ")" << std::endl;

    auto renderer = std::make_shared<SyncedFrameRenderer>();
    auto viewer = std::make_shared<FrameViewer>("Combo Synced View");
    viewer->createWindow();

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

        const int key = viewer->show(frame_to_show, 33);

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
    viewer->destroyWindow();

    if (recorder && recorder->isActive()) {
        combo.stopRecording();
    }

    std::cout << "Exit." << std::endl;
    return 0;
}