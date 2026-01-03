#ifndef EVRGB_EVENT_VISUALIZER_H_
#define EVRGB_EVENT_VISUALIZER_H_

#include <cstdint>
#include <vector>
#include <mutex>

#ifdef _WIN32
    #ifdef EVRGB_EXPORTS
        #define EVRGB_API __declspec(dllexport)
    #else
        #define EVRGB_API __declspec(dllimport)
    #endif
#else
    #define EVRGB_API
#endif

#ifdef _WIN32
    #include <opencv2/opencv.hpp>
#else
    #include <opencv4/opencv2/opencv.hpp>
#endif

#include "DvsenseDriver/FileReader/DvsFileReader.h"

namespace evrgb {

class EVRGB_API EventVisualizer {
public:
    enum class DisplayMode {
        Overlay,
        SideBySide
    };

    EventVisualizer(const cv::Size2i& rgb_size,
                    const cv::Size2i& event_size,
                    const cv::Vec3b& on_color = {0, 0, 255},
                    const cv::Vec3b& off_color = {255, 0, 0});

    DisplayMode toggleDisplayMode();
    void setDisplayMode(DisplayMode mode);
    DisplayMode displayMode() const;

    void setEventSize(const cv::Size2i& size);
    cv::Size2i eventSize() const;
    cv::Size2i rgbSize() const;

    cv::Point2i eventOffset() const;
    cv::Point2i adjustEventOffset(const cv::Point2i& delta);
    void setEventOffset(const cv::Point2i& offset);

    void setColors(const cv::Vec3b& on_color, const cv::Vec3b& off_color);
    cv::Vec3b onColor() const;
    cv::Vec3b offColor() const;

    void setFlipX(bool flip);
    bool flipX() const;

    bool updateRgbFrame(const cv::Mat& rgb_frame);

    bool visualizeEvents(
        dvsense::Event2DVector::const_iterator begin,
        dvsense::Event2DVector::const_iterator end,
        cv::Mat& output_frame);

    bool visualizeEvents(
        const dvsense::Event2DVector& events,
        cv::Mat& output_frame);

private:
    bool overlayEvents(
        dvsense::Event2DVector::const_iterator begin,
        dvsense::Event2DVector::const_iterator end,
        cv::Mat& output_frame);

    float calcScaleFactor() const;
    cv::Point2i getEventOffset() const;
private:
    cv::Size2i rgb_size_;
    cv::Size2i event_size_;
    cv::Vec3b on_color_;
    cv::Vec3b off_color_;
    DisplayMode display_mode_{DisplayMode::Overlay};
    cv::Point2i manual_offset_{0, 0};
    bool flip_x_{false};
    cv::Mat rgb_frame_;
    
    // 线程安全
    mutable std::mutex mutex_;
    
    // 缓存计算结果以提高性能
    mutable float cached_scale_ = 0.0f;
    mutable bool scale_dirty_ = true;
    
    // 禁用拷贝和移动操作以确保线程安全
    EventVisualizer(const EventVisualizer&) = delete;
    EventVisualizer& operator=(const EventVisualizer&) = delete;
    EventVisualizer(EventVisualizer&&) = delete;
    EventVisualizer& operator=(EventVisualizer&&) = delete;
};

}  // namespace evrgb

#endif  // EVRGB_EVENT_VISUALIZER_H_
