#include "recording/recorded_sync_reader.h"

#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <opencv2/opencv.hpp>
#else
#include <opencv4/opencv2/opencv.hpp>
#endif

namespace {

void overlayEvents(cv::Mat& frame, const std::shared_ptr<dvsense::Event2DVector>& events)
{
    if (!events || events->empty()) {
        return;
    }

    const cv::Vec3b on_color(0, 0, 255);
    const cv::Vec3b off_color(255, 0, 0);

    for (const auto& e : *events) {
        const int x = static_cast<int>(e.x);
        const int y = static_cast<int>(e.y);
        if (x < 0 || y < 0 || x >= frame.cols || y >= frame.rows) {
            continue;
        }
        frame.at<cv::Vec3b>(y, x) = e.polarity ? on_color : off_color;
    }
}

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

    evrgb::RecordedSyncReader reader({recording_dir});
    if (!reader.open()) {
        std::cerr << "Failed to open recording at " << recording_dir << std::endl;
        return 1;
    }

    replay_status.last_frame_ts_us = reader.getRecordingStartTimeUs().value_or(0);
    replay_status.current_ts_us = replay_status.last_frame_ts_us + replay_status.base_time_step_ms * 1000;
    std::shared_ptr<dvsense::Event2DVector> remianing_events = std::make_shared<dvsense::Event2DVector>(); // Buffer for events when frame changes
    cv::namedWindow("Recorded Replay", cv::WINDOW_AUTOSIZE);

    evrgb::RecordedSyncReader::Sample sample;
    reader.next(sample); // Preload first sample to set initial timestamps
    dvsense::Event2DVector::const_iterator it_start, it_end;
    cv::Mat view = sample.rgb.clone();

    while (true) {
        if (sample.rgb.empty()) {
            if (!reader.next(sample)) {
                break;
            }
            continue;
        }

        if (!sample.events->empty()) {
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
        }
        if (replay_status.current_ts_us >= sample.exposure_end_us){
            // Move to next frame
            remianing_events->insert(
                remianing_events->end(),
                it_start,
                it_end);
            view = sample.rgb.clone();
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
        

        if (remianing_events && !remianing_events->empty()) {
            overlayEvents(view, remianing_events);
            remianing_events->clear();
        }
        overlayEvents(view, std::make_shared<dvsense::Event2DVector>(it_start, it_end));


        std::string info = "Frame " + std::to_string(sample.frame_index) +
                           " | ts us: [" + std::to_string(sample.exposure_start_us) + ", " +
                           std::to_string(sample.exposure_end_us) + "]";
        cv::putText(view, info, {10, 25}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {255, 255, 255}, 1, cv::LINE_AA);

        cv::imshow("Recorded Replay", view);
        int key = cv::waitKey(30);
        // Toggle slow-mo with ' ' key
        if ((key & 0xff) == 'q' || (key & 0xff) == 27) {
            break;
        } else if ((key & 0xff) == ' ') {
            replay_status.slowmo_active = !replay_status.slowmo_active;
            std::cout << "Slow-mo " << (replay_status.slowmo_active ? "activated." : "deactivated.") << std::endl;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
