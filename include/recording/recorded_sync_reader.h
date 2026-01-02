// Reader for synchronized RGB + DVS recordings produced by SyncedDataRecorder.
// Provides zero-copy style access: each call to next() returns references into
// buffers owned by OpenCV (for RGB) and the dvsense driver (for DVS events).
// Callers must fully consume a Sample before invoking next() again.

#ifndef EVRGB_RECORDED_SYNC_READER_H_
#define EVRGB_RECORDED_SYNC_READER_H_

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <optional>

#ifdef _WIN32
#include <opencv2/opencv.hpp>
#else
#include <opencv4/opencv2/opencv.hpp>
#endif

#include "DvsenseDriver/FileReader/DvsFileReader.h"

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

class EVRGB_API RecordedSyncReader {
public:
    struct Paths {
        std::string recording_dir;
        std::string video = "combo_rgb.mp4";
        std::string csv = "combo_timestamps.csv";
        std::string events = "combo_events.raw";
    };

    struct Sample {
        int frame_index = -1;
        uint64_t exposure_start_us = 0;
        uint64_t exposure_end_us = 0;
        cv::Mat rgb;  // BGR frame view; buffer reused between calls.

        std::shared_ptr<dvsense::Event2DVector> events;  // Owns event storage; data() points to driver buffer.
    };

    explicit RecordedSyncReader(Paths paths);

    // Open all sources (CSV, MP4, DVS raw). Returns false on failure.
    bool open();

    // Reset iteration to the first frame.
    void reset();

    // Advance to the next synchronized sample. Returns false at end or on error.
    bool next(Sample& out);

    /**
     * @brief Get the Recording Start Time Us object    
     * 
     * @return dvsense::TimeStamp  The start timestamp of the recording in microseconds.
     */
    std::optional<dvsense::TimeStamp> getRecordingStartTimeUs() const;

    /**
     * \~ english @brief Get the RGB frame size.
     * \~ chinese @brief 获取 RGB 帧大小。
     * 
     * 
     * \~ english @return cv::Size2i The size of the RGB frames.
     * \~ chinese @return cv::Size2i RGB 帧的尺寸。
     */
    const cv::Size2i getRgbFrameSize() const;

    /**
     * \~ english @brief Get the Event frame size.
     * \~ chinese @brief 获取事件帧大小。
     * 
     * 
     * \~ english @return cv::Size2i The size of the Event frames.
     * \~ chinese @return cv::Size2i 事件帧的尺寸。
     */
    const cv::Size2i getEventFrameSize() const;

    /**
     * \~ english @brief Get the Frame Count object
     * \~ chinese @brief 获取帧数对象
     * 
     * 
     * \~ english @return size_t  The total number of frames in the recording.
     * \~ chinese @return size_t  录制中的总帧数。
     */
    size_t frameCount() const;
    bool isOpen() const;

private:
    struct FrameMeta {
        int frame_index = -1;
        uint64_t start_us = 0;
        uint64_t end_us = 0;
    };

    std::filesystem::path resolve(const std::string& name) const;
    bool loadCsv();
    bool openVideo();
    bool openEvents();

    Paths paths_{};
    std::vector<FrameMeta> frames_{};
    size_t cursor_ = 0;

    cv::VideoCapture cap_{};
    dvsense::DvsFile dvs_reader_{};
    bool opened_ = false;
};

}  // namespace evrgb

#endif  // EVRGB_RECORDED_SYNC_READER_H_
