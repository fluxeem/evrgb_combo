#include "recording/recorded_sync_reader.h"

#include "utils/event_visualizer.h"

#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <opencv2/opencv.hpp>
#else
#include <opencv4/opencv2/opencv.hpp>
#endif

namespace {

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

    evrgb::EventVisualizer canvas(
        reader.getRgbFrameSize(),
        reader.getEventFrameSize()
    );

    replay_status.last_frame_ts_us = reader.getRecordingStartTimeUs().value_or(0);
    replay_status.current_ts_us = replay_status.last_frame_ts_us + replay_status.base_time_step_ms * 1000;
    std::shared_ptr<dvsense::Event2DVector> remaining_events = std::make_shared<dvsense::Event2DVector>(); // Buffer for events when frame changes
    cv::namedWindow("Recorded Replay", cv::WINDOW_AUTOSIZE);

    evrgb::RecordedSyncReader::Sample sample;
    reader.next(sample); // Preload first sample to set initial timestamps
    dvsense::Event2DVector::const_iterator it_start, it_end;
    canvas.updateRgbFrame(sample.rgb);

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
            it_start = it_end = remaining_events->end();
        }
        if (replay_status.current_ts_us >= sample.exposure_end_us){
            // Move to next frame
            if (has_events) {
                remaining_events->insert(
                    remaining_events->end(),
                    it_start,
                    it_end);
            }
            canvas.updateRgbFrame(sample.rgb);
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
        if (!remaining_events->empty()){
            remaining_events->insert(
                remaining_events->end(),
                it_start,
                it_end);
            canvas.visualizeEvents(remaining_events->begin(), remaining_events->end(), view);
            remaining_events->clear();
        } else {
            canvas.visualizeEvents(it_start, it_end, view);
        }

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
                      << (mode == evrgb::EventVisualizer::DisplayMode::Overlay ? "OVERLAY." : "SIDE_BY_SIDE.") << std::endl;
        } else if ((key & 0xff) == ' ') {
            replay_status.slowmo_active = !replay_status.slowmo_active;
            std::cout << "Slow-mo " << (replay_status.slowmo_active ? "activated." : "deactivated.") << std::endl;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
