# 事件可视化教程

## 概述

`EventVisualizer` 类是 EvRGB Combo SDK 中的一个强大可视化组件，支持将 DVS 事件实时叠加显示在 RGB 图像上。它提供了灵活的显示模式、颜色自定义和坐标变换支持，非常适合相机对齐、数据分析和调试场景。

## 核心特性

- **双显示模式**：叠加（Overlay）和并排（Side-by-Side）可视化
- **自定义颜色**：独立配置 ON/OFF 事件的颜色
- **事件偏移调整**：精细调整事件位置
- **坐标变换**：支持仿射变换和相机内参
- **线程安全设计**：支持并发访问
- **高性能**：针对实时应用优化的渲染

## 基本用法

### 1. 创建 EventVisualizer

```cpp
#include "utils/event_visualizer.h"

// 使用 RGB 和事件帧尺寸创建可视化器
cv::Size rgb_size(1920, 1080);
cv::Size event_size(640, 480);

// 默认颜色：ON=红色，OFF=蓝色
evrgb::EventVisualizer visualizer(rgb_size, event_size);

// 或自定义颜色
cv::Vec3b on_color(0, 255, 0);    // ON 事件为绿色
cv::Vec3b off_color(255, 0, 255);  // OFF 事件为品红色
evrgb::EventVisualizer visualizer(rgb_size, event_size, on_color, off_color);
```

### 2. 更新 RGB 帧

```cpp
cv::Mat rgb_frame = ...; // 您的 RGB 图像（BGR 格式，CV_8UC3）

// 更新缓存的 RGB 帧
if (!visualizer.updateRgbFrame(rgb_frame)) {
    std::cerr << "更新 RGB 帧失败" << std::endl;
}
```

### 3. 可视化事件

```cpp
std::vector<dvsense::Event2D> events = ...; // 您的事件数据

// 将事件可视化到输出帧
cv::Mat output_frame;
if (visualizer.visualizeEvents(events, output_frame)) {
    cv::imshow("事件可视化", output_frame);
    cv::waitKey(1);
}
```

## 显示模式

### 叠加模式（默认）

事件直接叠加在 RGB 图像上，可以清晰看到事件与图像内容的空间关系。

```cpp
// 显式设置叠加模式
visualizer.setDisplayMode(evrgb::EventVisualizer::DisplayMode::Overlay);
```

### 并排模式

事件和 RGB 图像并排显示，便于对比观察。

```cpp
// 设置并排模式
visualizer.setDisplayMode(evrgb::EventVisualizer::DisplayMode::SideBySide);

// 或在两种模式间切换
auto current_mode = visualizer.toggleDisplayMode();
```

## 事件偏移调整

精细调整事件相对于 RGB 图像的位置：

```cpp
// 设置绝对偏移
cv::Point2i offset(10, 20);
visualizer.setEventOffset(offset);

// 或增量调整
cv::Point2i current = visualizer.eventOffset();
current.x += 5;
current.y += 5;
visualizer.setEventOffset(current);
```

## 颜色自定义

自定义 ON 和 OFF 事件的外观：

```cpp
// 设置两种颜色
cv::Vec3b on_color(0, 255, 0);    // 绿色
cv::Vec3b off_color(255, 0, 0);   // 红色
visualizer.setColors(on_color, off_color);

// 获取当前颜色
cv::Vec3b current_on = visualizer.onColor();
cv::Vec3b current_off = visualizer.offColor();
```

## 坐标变换

### 设置事件帧尺寸

如果事件帧尺寸发生变化，进行更新：

```cpp
cv::Size2i new_event_size(1280, 720);
visualizer.setEventSize(new_event_size);
```

### 仿射变换

应用仿射变换进行精确对齐：

```cpp
#include "utils/calib_info.h"

// 创建仿射变换矩阵
evrgb::AffineTransform affine;
affine.A(0, 0) = 1.0;  // X 轴缩放
affine.A(1, 1) = 1.0;  // Y 轴缩放
affine.A(0, 2) = 10.0; // X 轴平移
affine.A(1, 2) = 20.0; // Y 轴平移

// 应用标定
visualizer.setCalibration(affine);
```

### 相机内参

设置相机内参以进行归一化坐标映射：

```cpp
// 定义相机内参
evrgb::CameraIntrinsics rgb_intrinsics;
rgb_intrinsics.fx = 1000.0;
rgb_intrinsics.fy = 1000.0;
rgb_intrinsics.cx = 960.0;
rgb_intrinsics.cy = 540.0;

evrgb::CameraIntrinsics dvs_intrinsics;
dvs_intrinsics.fx = 500.0;
dvs_intrinsics.fy = 500.0;
dvs_intrinsics.cx = 320.0;
dvs_intrinsics.cy = 240.0;

// 设置内参
visualizer.setIntrinsics(rgb_intrinsics, dvs_intrinsics);
```

### 水平翻转

启用事件坐标的水平翻转：

```cpp
// 启用 X 轴翻转
visualizer.setFlipX(true);

// 检查当前翻转状态
bool is_flipped = visualizer.flipX();
```

## 完整示例

下面是一个完整的示例，展示如何将 EventVisualizer 与 Combo 结合使用：

```cpp
#include "core/combo.h"
#include "utils/event_visualizer.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <mutex>
#include <memory>

class SyncedFrameRenderer {
public:
    void update(const evrgb::RgbImageWithTimestamp& rgb_data,
                const std::vector<dvsense::Event2D>& events) {
        cv::Mat bgr = ensureBgrFrame(rgb_data.image);
        if (bgr.empty()) return;

        ensureVisualizer(bgr.size());
        if (!visualizer_) return;

        visualizer_->updateRgbFrame(bgr);

        cv::Mat output_frame;
        if (visualizer_->visualizeEvents(events, output_frame)) {
            std::lock_guard<std::mutex> lock(mutex_);
            latest_frame_ = output_frame.clone();
        }
    }

    void toggleDisplayMode() {
        if (visualizer_) {
            visualizer_->toggleDisplayMode();
        }
    }

    void adjustEventOffset(const cv::Point& delta) {
        if (visualizer_) {
            cv::Point2i current = visualizer_->eventOffset();
            current.x += delta.x;
            current.y += delta.y;
            visualizer_->setEventOffset(current);
        }
    }

    bool getLatestFrame(cv::Mat& out_frame) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (latest_frame_.empty()) return false;
        out_frame = latest_frame_.clone();
        return true;
    }

private:
    static cv::Mat ensureBgrFrame(const cv::Mat& input) {
        if (input.empty()) return {};
        if (input.channels() == 3 && input.type() == CV_8UC3) {
            return input;
        }
        cv::Mat converted;
        input.convertTo(converted, CV_8UC3);
        return converted;
    }

    void ensureVisualizer(const cv::Size& rgb_size) {
        if (visualizer_) return;
        cv::Size event_size(640, 480);
        visualizer_ = std::make_unique<evrgb::EventVisualizer>(
            rgb_size, event_size,
            cv::Vec3b(0, 255, 0),  // ON 事件：绿色
            cv::Vec3b(255, 0, 0)    // OFF 事件：红色
        );
    }

private:
    std::mutex mutex_;
    cv::Mat latest_frame_;
    std::unique_ptr<evrgb::EventVisualizer> visualizer_;
};

int main() {
    // 枚举相机
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();

    if (rgb_cameras.empty() || dvs_cameras.empty()) {
        std::cerr << "未找到相机！" << std::endl;
        return -1;
    }

    // 创建 combo
    evrgb::Combo combo(
        rgb_cameras[0].serial_number,
        dvs_cameras[0].serial,
        evrgb::Combo::Arrangement::STEREO,
        100
    );

    // 创建渲染器
    auto renderer = std::make_shared<SyncedFrameRenderer>();

    // 设置回调
    combo.setSyncedCallback(
        [renderer](const evrgb::RgbImageWithTimestamp& rgb,
                   const std::vector<dvsense::Event2D>& events) {
            renderer->update(rgb, events);
        }
    );

    // 初始化并启动
    if (!combo.init() || !combo.start()) {
        std::cerr << "启动 combo 失败" << std::endl;
        return -1;
    }

    // 创建窗口
    cv::namedWindow("事件可视化", cv::WINDOW_AUTOSIZE);
    std::cout << "控制说明：" << std::endl;
    std::cout << "  - 方向键：调整事件偏移" << std::endl;
    std::cout << "  - 'm'：切换显示模式" << std::endl;
    std::cout << "  - 'q' 或 ESC：退出" << std::endl;

    // 主循环
    cv::Mat frame;
    while (true) {
        if (renderer->getLatestFrame(frame)) {
            cv::imshow("事件可视化", frame);
        }

        int key = cv::waitKeyEx(33);

        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }

        if (key == 'm' || key == 'M') {
            renderer->toggleDisplayMode();
        }

        // 处理方向键进行偏移调整
        cv::Point delta(0, 0);
        if (key == 2424832) delta.x = -1;      // 左
        else if (key == 2555904) delta.x = 1;   // 右
        else if (key == 2490368) delta.y = -1;  // 上
        else if (key == 2621440) delta.y = 1;   // 下

        if (delta.x != 0 || delta.y != 0) {
            renderer->adjustEventOffset(delta);
        }
    }

    combo.stop();
    cv::destroyAllWindows();

    return 0;
}
```

## 高级用法

### 线程安全实现

`EventVisualizer` 类是线程安全的，可以在多线程环境中安全使用：

```cpp
// 线程 1：更新 RGB 帧
std::thread rgb_thread([&]() {
    while (running) {
        cv::Mat rgb_frame = getRgbFrame();
        visualizer.updateRgbFrame(rgb_frame);
    }
});

// 线程 2：可视化事件
std::thread event_thread([&]() {
    while (running) {
        auto events = getEvents();
        cv::Mat output;
        visualizer.visualizeEvents(events, output);
        displayFrame(output);
    }
});

rgb_thread.join();
event_thread.join();
```

### 与标定集成

进行精确的相机对齐时，将 EventVisualizer 与标定数据结合：

```cpp
// 从 combo 加载标定
auto meta = combo.getMetadata();
if (std::holds_alternative<evrgb::AffineTransform>(meta.calibration)) {
    auto affine = std::get<evrgb::AffineTransform>(meta.calibration);
    visualizer.setCalibration(affine);
}

// 如果有内参则设置
if (meta.rgb.intrinsics && meta.dvs.intrinsics) {
    visualizer.setIntrinsics(
        *meta.rgb.intrinsics,
        *meta.dvs.intrinsics
    );
}
```

## 常见问题

### Q: 为什么在 RGB 图像上看不到事件？

**A**: 检查以下几点：
1. 确保 RGB 帧尺寸与构造函数传入的尺寸匹配
2. 验证事件坐标在有效范围内
3. 检查事件偏移是否过大
4. 确认显示模式设置正确（尝试并排模式）

### Q: 如何提高可视化性能？

**A**:
- 使用适当的帧尺寸（避免不必要的过大分辨率）
- 减少 `updateRgbFrame()` 调用频率
- 考虑为预览目的缩小帧尺寸
- 使用线程安全模式避免阻塞

### Q: EventVisualizer 可以用于录制数据吗？

**A**: 可以，EventVisualizer 同时支持实时和录制数据的可视化。详情请参考 `recorded_replay_example`。

### Q: 如何处理不同的图像格式？

**A**: EventVisualizer 期望 BGR 格式（CV_8UC3）。转换您的图像：
```cpp
cv::Mat bgr;
if (input.channels() == 1) {
    cv::cvtColor(input, bgr, cv::COLOR_GRAY2BGR);
} else if (input.channels() == 4) {
    cv::cvtColor(input, bgr, cv::COLOR_BGRA2BGR);
} else {
    bgr = input;
}
```

## 相关示例

- `combo_sync_display_example` - 使用 EventVisualizer 的基本同步显示
- `beam_splitter_align` - 带仿射变换的高级对齐
- `recorded_replay_example` - 录制数据的可视化

## API 参考

完整的 API 文档请参考：
- [EventVisualizer 类参考](../../../docs/html/classevrgb_1_1_event_visualizer.html)
- [Combo 类参考](../../../docs/html/classevrgb_1_1_combo.html)

## 下一步

- 探索[相机使用教程](CAMERA_USAGE_TUTORIAL.md)了解基本相机操作
- 学习[标定教程](CALIBRATION_TUTORIAL.md)进行精确的坐标对齐
- 查看[录制教程](RECORDING_TUTORIAL.md)了解数据录制和回放