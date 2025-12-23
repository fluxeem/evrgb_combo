#include "recording/recorded_sync_reader.h"

#include <iostream>

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

}  // namespace

int main(int argc, char* argv[])
{
    const std::string recording_dir = (argc > 1) ? argv[1] : "recordings";

    evrgb::RecordedSyncReader reader({recording_dir});
    if (!reader.open()) {
        std::cerr << "Failed to open recording at " << recording_dir << std::endl;
        return 1;
    }

    cv::namedWindow("Recorded Replay", cv::WINDOW_AUTOSIZE);

    evrgb::RecordedSyncReader::Sample sample;
    while (reader.next(sample)) {
        if (sample.rgb.empty()) {
            continue;
        }

        cv::Mat view = sample.rgb.clone();
        overlayEvents(view, sample.events);

        std::cout << "Frame " << sample.frame_index
                  << " | ts us: [" << sample.exposure_start_us << ", " << sample.exposure_end_us << "]"
                  << " | events: " << (sample.events ? sample.events->size() : 0)
                  << std::endl;

        std::string info = "Frame " + std::to_string(sample.frame_index) +
                           " | ts us: [" + std::to_string(sample.exposure_start_us) + ", " +
                           std::to_string(sample.exposure_end_us) + "]";
        cv::putText(view, info, {10, 25}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {255, 255, 255}, 1, cv::LINE_AA);

        cv::imshow("Recorded Replay", view);
        int key = cv::waitKey(30);
        if ((key & 0xff) == 'q' || (key & 0xff) == 27) {
            break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
