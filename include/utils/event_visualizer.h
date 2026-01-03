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

    /** 
     * #if ENGLISH
     * @brief Construct a new Event Visualizer object
     * 
     * @param rgb_size Size of the RGB frame
     * @param event_size Size of the event frame
     * @param on_color Color for ON events
     * @param off_color Color for OFF events
     * #endif
     * 
     * #if CHINESE
     * @brief 构造函数
     * 
     * @param rgb_size RGB帧的尺寸
     * @param event_size 事件帧的尺寸
     * @param on_color ON事件的颜色
     * @param off_color OFF事件的颜色
     * #endif
     */
    EventVisualizer(const cv::Size2i& rgb_size,
                    const cv::Size2i& event_size,
                    const cv::Vec3b& on_color = {0, 0, 255},
                    const cv::Vec3b& off_color = {255, 0, 0});

    /**
     * #if ENGLISH
     * @brief Toggle between Overlay and SideBySide display modes.
     * @return The new display mode after toggling.
     * #endif
     *
     * #if CHINESE
     * @brief 在叠加与并排显示模式之间切换。
     * @return 切换后的显示模式。
     * #endif
     */
    DisplayMode toggleDisplayMode();

    /**
     * #if ENGLISH
     * @brief Set the display mode explicitly.
     * @param mode Desired display mode.
     * #endif
     *
     * #if CHINESE
     * @brief 显式设置显示模式。
     * @param mode 目标显示模式。
     * #endif
     */
    void setDisplayMode(DisplayMode mode);

    /**
     * #if ENGLISH
     * @brief Get the current display mode.
     * @return Current display mode.
     * #endif
     *
     * #if CHINESE
     * @brief 获取当前显示模式。
     * @return 当前的显示模式。
     * #endif
     */
    DisplayMode displayMode() const;

    /**
     * #if ENGLISH
     * @brief Update the event frame size.
     * @param size New event frame dimensions.
     * #endif
     *
     * #if CHINESE
     * @brief 更新事件帧尺寸。
     * @param size 新的事件帧大小。
     * #endif
     */
    void setEventSize(const cv::Size2i& size);

    /**
     * #if ENGLISH
     * @brief Get the configured event frame size.
     * @return Event frame dimensions.
     * #endif
     *
     * #if CHINESE
     * @brief 获取当前设置的事件帧尺寸。
     * @return 事件帧大小。
     * #endif
     */
    cv::Size2i eventSize() const;

    /**
     * #if ENGLISH
     * @brief Get the configured RGB frame size.
     * @return RGB frame dimensions.
     * #endif
     *
     * #if CHINESE
     * @brief 获取当前设置的 RGB 帧尺寸。
     * @return RGB 帧大小。
     * #endif
     */
    cv::Size2i rgbSize() const;

    /**
     * #if ENGLISH
     * @brief Get the current manual event offset.
     * @return Offset applied to event coordinates.
     * #endif
     *
     * #if CHINESE
     * @brief 获取当前的事件手动偏移。
     * @return 应用于事件坐标的偏移量。
     * #endif
     */
    cv::Point2i eventOffset() const;

    /**
     * #if ENGLISH
     * @brief Set the event offset directly.
     * @param offset Absolute offset for event coordinates.
     * #endif
     *
     * #if CHINESE
     * @brief 直接设置事件偏移。
     * @param offset 事件坐标的绝对偏移量。
     * #endif
     */
    void setEventOffset(const cv::Point2i& offset);

    /**
     * #if ENGLISH
     * @brief Set colors for ON and OFF events.
     * @param on_color BGR color for ON events.
     * @param off_color BGR color for OFF events.
     * #endif
     *
     * #if CHINESE
     * @brief 设置 ON / OFF 事件的颜色。
     * @param on_color ON 事件的 BGR 颜色。
     * @param off_color OFF 事件的 BGR 颜色。
     * #endif
     */
    void setColors(const cv::Vec3b& on_color, const cv::Vec3b& off_color);

    /**
     * #if ENGLISH
     * @brief Get the current ON event color.
     * @return BGR color used for ON events.
     * #endif
     *
     * #if CHINESE
     * @brief 获取当前 ON 事件的颜色。
     * @return 用于 ON 事件的 BGR 颜色。
     * #endif
     */
    cv::Vec3b onColor() const;

    /**
     * #if ENGLISH
     * @brief Get the current OFF event color.
     * @return BGR color used for OFF events.
     * #endif
     *
     * #if CHINESE
     * @brief 获取当前 OFF 事件的颜色。
     * @return 用于 OFF 事件的 BGR 颜色。
     * #endif
     */
    cv::Vec3b offColor() const;

    /**
     * #if ENGLISH
     * @brief Enable or disable horizontal flipping for event coordinates.
     * @param flip true to mirror events along the X axis; false to keep original orientation.
     * #endif
     *
     * #if CHINESE
     * @brief 设置事件帧是否在 X 方向翻转。
     * @param flip true 表示沿 X 轴镜像事件，false 表示保持原始方向。
     * #endif
     */
    void setFlipX(bool flip);

    /**
     * #if ENGLISH
     * @brief Check whether X-axis flipping is enabled for events.
     * @return true if events are mirrored horizontally; false otherwise.
     * #endif
     *
     * #if CHINESE
     * @brief 查询事件是否启用 X 轴翻转。
     * @return true 表示事件被水平镜像，false 表示未翻转。
     * #endif
     */
    bool flipX() const;

    /**
     * #if ENGLISH
     * @brief Update the cached RGB frame used for visualization.
     *
     * The frame is copied internally and resized or reused as needed when
     * generating combined outputs.
     *
     * @param rgb_frame Latest RGB frame.
     * @return true on success; false if the input is empty or has an invalid size.
     * #endif
     *
     * #if CHINESE
     * @brief 更新用于可视化的缓存 RGB 帧。
     *
     * 该帧会在内部复制，并在生成组合输出时进行复用或调整尺寸。
     *
     * @param rgb_frame 最新的 RGB 帧。
     * @return true 表示更新成功；false 表示输入为空或尺寸无效。
     * #endif
     */
    bool updateRgbFrame(const cv::Mat& rgb_frame);

    /**
     * #if ENGLISH
     * @brief Visualize a range of events into the output frame.
     * @param begin Iterator to the first event to render.
     * @param end Iterator past the last event to render.
     * @param output_frame Destination image that receives the visualization.
     * @return true on success; false if sizes are mismatched or buffers are invalid.
     * #endif
     *
     * #if CHINESE
     * @brief 将一段事件数据可视化到输出图像。
     * @param begin 指向首个要渲染事件的迭代器。
     * @param end 指向最后一个事件之后位置的迭代器。
     * @param output_frame 用于接收可视化结果的图像。
     * @return true 表示成功；false 表示尺寸不匹配或缓冲区无效。
     * #endif
     */
    bool visualizeEvents(
        dvsense::Event2DVector::const_iterator begin,
        dvsense::Event2DVector::const_iterator end,
        cv::Mat& output_frame);

    /**
     * #if ENGLISH
     * @brief Visualize an entire event container into the output frame.
     * @param events Collection of events to render.
     * @param output_frame Destination image that receives the visualization.
     * @return true on success; false if sizes are mismatched or buffers are invalid.
     * #endif
     *
     * #if CHINESE
     * @brief 将完整的事件容器可视化到输出图像。
     * @param events 要渲染的事件集合。
     * @param output_frame 用于接收可视化结果的图像。
     * @return true 表示成功；false 表示尺寸不匹配或缓冲区无效。
     * #endif
     */
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
