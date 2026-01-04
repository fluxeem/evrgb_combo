#ifndef EVRGB_SYNCED_DATA_RECORDER_H_
#define EVRGB_SYNCED_DATA_RECORDER_H_

#include <fstream>
#include <chrono>
#include <mutex>
#include <string>
#include <vector>

#ifdef _WIN32
    #include "opencv2/opencv.hpp"
#else
    #include "opencv4/opencv2/opencv.hpp"
#endif

#include "core/combo_types.h"

#ifdef _WIN32
    #ifdef EVRGB_EXPORTS
        #define EVRGB_API __declspec(dllexport)
    #else
        #define EVRGB_API __declspec(dllimport)
    #endif
#else
    #define EVRGB_API   
#endif

namespace evrgb {

struct SyncedRecorderConfig {
    std::string output_dir;   // Directory to store MP4/CSV/DVS raw
    double fps = 30.0;
    std::string fourcc = "mp4v";
    std::string arrangement;  // Optional combo arrangement (e.g., "STEREO", "BEAM_SPLITTER")
    std::string rgb_serial;
    std::string dvs_serial;
    std::string rgb_model;
    std::string dvs_model;
};

class EVRGB_API SyncedDataRecorder {
public:
    SyncedDataRecorder() = default;
    explicit SyncedDataRecorder(SyncedRecorderConfig config);

    bool start(const SyncedRecorderConfig& config);
    bool record(const RgbImageWithTimestamp& rgb, const std::vector<dvsense::Event2D>& events);
    void stop();

    bool isActive() const;
    bool requiresDvsRawRecording() const;
    const std::string& dvsRawPath() const;
    const std::string& rgbPath() const;
    const std::string& csvPath() const;

private:
    cv::Mat toBgr8(const cv::Mat& input) const;
    bool ensureVideoWriter(const cv::Mat& frame);
    void writeCsvRow(const RgbImageWithTimestamp& rgb);

    SyncedRecorderConfig config_{};
    std::string rgb_path_;
    std::string csv_path_;
    std::string dvs_raw_path_;
    std::string metadata_path_;
    cv::VideoWriter writer_;
    std::ofstream csv_stream_;
    cv::Size frame_size_{0, 0};
    std::size_t frame_count_ = 0;
    std::size_t event_count_ = 0;
    bool started_ = false;
    mutable std::mutex mutex_;
};

}  // namespace evrgb

#endif  // EVRGB_SYNCED_DATA_RECORDER_H_
