#include "recording/recorded_sync_reader.h"

#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <opencv2/opencv.hpp>
#else
#include <opencv4/opencv2/opencv.hpp>
#endif

namespace {

class DisplayCanvas {
public: 
    enum DISPLAY_MODE {
        OVERLAY, 
        SIDE_BY_SIDE
    } display_mode = OVERLAY;    

    DisplayCanvas(const cv::Size2i& rgb_size, const cv::Size2i& event_size)
        : rgb_size_(rgb_size), event_size_(event_size){

        }

    DISPLAY_MODE toggleDisplayMode() {
        if (display_mode == OVERLAY) {
            display_mode = SIDE_BY_SIDE;
        } else {
            display_mode = OVERLAY;
        }
        return display_mode;
    }

    bool updateFrame(const cv::Mat& rgb_frame) {
        if (rgb_frame.size() != rgb_size_) {
            return false;
        }
        rgb_frame_ = rgb_frame.clone();
        return true;
    }

    bool visualizeEvents(
        dvsense::Event2DVector::const_iterator remaining_begin,
        dvsense::Event2DVector::const_iterator remaining_end,
        dvsense::Event2DVector::const_iterator new_begin,
        dvsense::Event2DVector::const_iterator new_end,
        cv::Mat& output_frame
    ) {
        if (rgb_frame_.empty()) {
            return false;
        }

        // init output frame
        if (display_mode == OVERLAY) {
            output_frame = rgb_frame_.clone();
        } else {
            output_frame.create(rgb_size_.height, rgb_size_.width * 2, rgb_frame_.type());
            output_frame.setTo(cv::Scalar::all(0));
            rgb_frame_.copyTo(output_frame(cv::Rect(0, 0, rgb_size_.width, rgb_size_.height)));
        }
        
        overlayEvents(remaining_begin, remaining_end, output_frame);
        overlayEvents(new_begin, new_end, output_frame);
        return true;
    }


private:

    bool overlayEvents(
        dvsense::Event2DVector::const_iterator begin,
        dvsense::Event2DVector::const_iterator end,
        cv::Mat& output_frame
    ) {
        if (rgb_frame_.empty()) {
            return false;
        }

        cv::Point2i offset = getEventOffset();
        float scale = calcScaleFactor();

        for (auto it = begin; it != end; ++it) {
            const auto& e = *it;
            int x = static_cast<int>(e.x * scale) + offset.x;
            int y = static_cast<int>(e.y * scale) + offset.y;
            if (x < 0 || y < 0 || x >= output_frame.cols || y >= output_frame.rows) {
                continue;
            }
            output_frame.at<cv::Vec3b>(y, x) = e.polarity ? on_color : off_color;
        }
        return true;
    }

    float calcScaleFactor() const {
        float scale_x = static_cast<float>(rgb_size_.width) / static_cast<float>(event_size_.width);
        float scale_y = static_cast<float>(rgb_size_.height) / static_cast<float>(event_size_.height);
        return std::min(scale_x, scale_y);
    }

    cv::Point2i getEventOffset() const {
        float scale = calcScaleFactor();
        int offset_x = (rgb_size_.width - static_cast<int>(event_size_.width * scale)) / 2;
        int offset_y = (rgb_size_.height - static_cast<int>(event_size_.height * scale)) / 2;
        if (display_mode == SIDE_BY_SIDE) {
            offset_x += rgb_size_.width;
        }
        return cv::Point2i(offset_x, offset_y);
    }

    const cv::Vec3b on_color{0, 0, 255};
    const cv::Vec3b off_color{255, 0, 0};

    cv::Size2i rgb_size_;
    cv::Size2i event_size_;

    cv::Mat rgb_frame_;
};

struct ReplayStatus {
    bool slowmo_active = false;
    dvsense::TimeStamp current_ts_us = 0;
    dvsense::TimeStamp last_frame_ts_us = 0;
    uint32_t base_time_step_ms = 33;
    uint16_t slowmo_factor = 10;
} replay_status;

}  // namespace

int main(int argc, char* argv[])
{
    const std::string recording_dir = (argc > 1) ? argv[1] : "recordings";

    std::cout << "Usage: " << argv[0] << " [recording_dir]\n"
              << "Controls: q/Esc to quit, m to toggle display mode, space to toggle slow-mo."
              << std::endl;

    evrgb::RecordedSyncReader reader({recording_dir});
    if (!reader.open()) {
        std::cerr << "Failed to open recording at " << recording_dir << std::endl;
        return 1;
    }

    DisplayCanvas canvas(
        reader.getRgbFrameSize(),
        reader.getEventFrameSize()
    );

    replay_status.last_frame_ts_us = reader.getRecordingStartTimeUs().value_or(0);
    replay_status.current_ts_us = replay_status.last_frame_ts_us + replay_status.base_time_step_ms * 1000;
    std::shared_ptr<dvsense::Event2DVector> remianing_events = std::make_shared<dvsense::Event2DVector>(); // Buffer for events when frame changes
    cv::namedWindow("Recorded Replay", cv::WINDOW_AUTOSIZE);

    evrgb::RecordedSyncReader::Sample sample;
    reader.next(sample); // Preload first sample to set initial timestamps
    dvsense::Event2DVector::const_iterator it_start, it_end;
    canvas.updateFrame(sample.rgb);

    while (true) {
        if (sample.rgb.empty()) {
            if (!reader.next(sample)) {
                break;
            }
            continue;
        }

        bool has_events = sample.events && !sample.events->empty();
        if (has_events) {
            it_start = std::lower_bound(
                sample.events->begin(), sample.events->end(),
                replay_status.last_frame_ts_us,
                [](const dvsense::Event2D& e, dvsense::TimeStamp ts) {
                    return e.timestamp < ts;
                });
            it_end = std::upper_bound(
                sample.events->begin(), sample.events->end(),
                replay_status.current_ts_us,
                [](dvsense::TimeStamp ts, const dvsense::Event2D& e) {
                    return ts < e.timestamp;
                });
        } else {
            it_start = it_end = remianing_events->end();
        }
        if (replay_status.current_ts_us >= sample.exposure_end_us){
            // Move to next frame
            if (has_events) {
                remianing_events->insert(
                    remianing_events->end(),
                    it_start,
                    it_end);
            }
            canvas.updateFrame(sample.rgb);
            if (!reader.next(sample)) {
                break;
            }
            continue;
        }
     
        // Advance time
        replay_status.last_frame_ts_us = replay_status.current_ts_us;

        const uint32_t step_us = replay_status.base_time_step_ms * 1000 /
            (replay_status.slowmo_active ? replay_status.slowmo_factor : 1);
        replay_status.current_ts_us += step_us;
        
        cv::Mat view;
        canvas.visualizeEvents(
            remianing_events->begin(),
            remianing_events->end(),
            it_start,
            it_end,
            view);
        remianing_events->clear();


        std::string info = "Frame " + std::to_string(sample.frame_index) +
                           " | ts us: [" + std::to_string(sample.exposure_start_us) + ", " +
                           std::to_string(sample.exposure_end_us) + "]";
        cv::putText(view, info, {10, 25}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {255, 255, 255}, 1, cv::LINE_AA);

        cv::imshow("Recorded Replay", view);
        int key = cv::waitKey(30);
        // Toggle slow-mo with ' ' key
        if ((key & 0xff) == 'q' || (key & 0xff) == 27) {
            break;
        } if ((key & 0xff) == 'm' || (key & 0xff) == 'M') {
            auto mode = canvas.toggleDisplayMode();
            std::cout << "Display mode switched to "
                      << (mode == DisplayCanvas::OVERLAY ? "OVERLAY." : "SIDE_BY_SIDE.") << std::endl;
        } else if ((key & 0xff) == ' ') {
            replay_status.slowmo_active = !replay_status.slowmo_active;
            std::cout << "Slow-mo " << (replay_status.slowmo_active ? "activated." : "deactivated.") << std::endl;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
