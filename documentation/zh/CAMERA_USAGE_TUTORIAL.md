@page camera_usage_tutorial Combo 使用教程

本教程介绍如何使用 EvRGB Combo SDK 操作 RGB+DVS 组合相机。本项目推荐使用 Combo 接口来利用内置的时间同步机制，实现 RGB 图像和 DVS 事件的精确同步。

## 目录

- [Combo 基本操作](#combo-基本操作)
- [设置同步回调](#设置同步回调)
- [分别设置回调](#分别设置回调)
- [错误处理](#错误处理)
- [常见问题](#常见问题)

## Combo 基本操作

### 基本组合操作

```cpp
#include "core/combo.h"
#include <iostream>

int main() {
    // 枚举相机
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();

    if (rgb_cameras.empty()) {
        std::cerr << "未找到 RGB 相机!" << std::endl;
        return -1;
    }
    if (dvs_cameras.empty()) {
        std::cerr << "未找到 DVS 相机!" << std::endl;
        return -1;
    }

    // 创建 Combo 对象
    std::string rgb_serial = rgb_cameras[0].serial_number;
    std::string dvs_serial = dvs_cameras[0].serial;
    
    evrgb::Combo combo(rgb_serial, dvs_serial);
    
    std::cout << "使用 RGB: " << rgb_serial << std::endl;
    std::cout << "使用 DVS: " << dvs_serial << std::endl;

    // 设置同步回调
    combo.setSyncedCallback([](const evrgb::RgbImageWithTimestamp& rgb, 
                               const std::vector<dvsense::Event2D>& events) {
        std::cout << "同步数据: RGB 时间 " << rgb.timestamp_us 
                  << " us, 事件数量 " << events.size() << std::endl;
        
        // 处理同步的 RGB 图像和 DVS 事件
        if (!rgb.image.empty()) {
            std::cout << "  RGB 图像尺寸: " << rgb.image.cols 
                      << "x" << rgb.image.rows << std::endl;
        }
        
        if (!events.empty()) {
            // 分析事件时间范围
            auto min_time = std::min_element(events.begin(), events.end(),
                [](const auto& a, const auto& b) { return a.timestamp < b.timestamp; });
            auto max_time = std::max_element(events.begin(), events.end(),
                [](const auto& a, const auto& b) { return a.timestamp < b.timestamp; });
            
            if (min_time != events.end() && max_time != events.end()) {
                std::cout << "  事件时间范围: [" << min_time->timestamp 
                          << ", " << max_time->timestamp << "] us" << std::endl;
            }
        }
    });

    // 初始化和启动
    if (!combo.init()) {
        std::cerr << "Combo 初始化失败" << std::endl;
        return -1;
    }

    if (!combo.start()) {
        std::cerr << "Combo 启动失败" << std::endl;
        return -1;
    }

    std::cout << "Combo 已启动，采集同步数据..." << std::endl;

    // 运行 10 秒
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // 停止
    combo.stop();
    std::cout << "Combo 已停止" << std::endl;
    
    return 0;
}
```

### 分别设置回调

```cpp
#include "core/combo.h"
#include <iostream>

int main() {
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();
    if (rgb_cameras.empty() || dvs_cameras.empty()) return -1;

    evrgb::Combo combo(rgb_cameras[0].serial_number, dvs_cameras[0].serial);

    // 设置 RGB 图像回调
    combo.setRgbImageCallback([](const evrgb::RgbImageWithTimestamp& rgb) {
        std::cout << "RGB 图像: 时间 " << rgb.timestamp_us 
                  << " us, 尺寸 " << rgb.image.cols << "x" << rgb.image.rows << std::endl;
    });

    // 设置 DVS 事件回调
    combo.setDvsEventCallback([](const std::vector<dvsense::Event2D>& events) {
        if (!events.empty()) {
            std::cout << "DVS 事件: 数量 " << events.size() << std::endl;
        }
    });

    if (!combo.init() || !combo.start()) return -1;

    // 运行 5 秒
    std::this_thread::sleep_for(std::chrono::seconds(5));

    combo.stop();
    return 0;
}
```

## 错误处理

### 检查 Combo 状态

```cpp
#include "core/combo.h"
#include <iostream>

void printComboStatus(const evrgb::CameraStatus& status) {
    if (status.success()) {
        std::cout << "操作成功" << std::endl;
    } else {
        std::cout << "操作失败: " << status.message
                  << " (错误码: 0x" << std::hex << status.code << std::dec << ")" << std::endl;
    }
}

int main() {
    // 枚举相机
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();
    if (rgb_cameras.empty()) {
        std::cerr << "未找到 RGB 相机" << std::endl;
        return 1;
    }
    if (dvs_cameras.empty()) {
        std::cerr << "未找到 DVS 相机" << std::endl;
        return 1;
    }

    // 创建 Combo 对象
    evrgb::Combo combo(rgb_cameras[0].serial_number, dvs_cameras[0].serial);

    // 初始化并检查状态
    if (!combo.init()) {
        std::cerr << "Combo 初始化失败" << std::endl;
        return 1;
    }
    std::cout << "Combo 初始化成功" << std::endl;

    // 启动并检查状态
    if (!combo.start()) {
        std::cerr << "Combo 启动失败" << std::endl;
        combo.destroy();
        return 1;
    }
    std::cout << "Combo 启动成功" << std::endl;

    // 运行一段时间
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 停止并销毁
    combo.stop();
    combo.destroy();
    std::cout << "Combo 已停止并销毁" << std::endl;

    return 0;
}
```

## 常见问题

### Q: Combo 初始化失败

**可能原因**：
1. RGB 或 DVS 相机序列号不正确
2. 相机被其他程序占用
3. 驱动程序问题
4. 权限不足
5. RGB 和 DVS 相机未正确连接触发信号

**解决方法**：
1. 使用枚举示例确认正确的序列号
2. 关闭其他可能使用相机的程序
3. 重新安装相机驱动
4. Linux 上使用 sudo 或配置设备权限
5. 确保 RGB 和 DVS 相机的触发线正确连接

### Q: 组合相机同步问题

**注意事项**：
1. 确保 RGB 和 DVS 相机都正常工作
2. 检查时间同步设置
3. 验证触发连接
4. 查看日志中的时间戳差异

**排查步骤**：
1. 检查 RGB 和 DVS 相机是否都能单独正常工作
2. 验证触发信号线连接是否正确
3. 查看 Combo 内部的时间戳对齐情况
4. 检查回调函数中的时间戳是否在合理范围内

### Q: 回调函数未被调用

**可能原因**：
1. Combo 未启动
2. 回调函数设置在启动之后
3. 数据缓冲区问题
4. 同步时间窗口设置不当

**解决方法**：
1. 确保在启动前设置回调
2. 检查 Combo 是否成功启动
3. 调整缓冲区大小
4. 检查同步时间窗口参数

### Q: 同步数据不连续

**可能原因**：
1. RGB 帧率设置过低
2. 事件数据量过大
3. 系统负载过高
4. 缓冲区溢出

**解决方法**：
1. 调整 RGB 帧率
2. 优化事件处理逻辑
3. 增加缓冲区大小
4. 检查系统资源使用情况

## 下一步

掌握 Combo 基础操作后，你可以：

1. 学习参数调整教程，优化相机性能
2. 了解数据录制功能，保存同步数据
3. 探索高级同步特性，如慢动作回放
4. 查看更多示例代码，了解实际应用场景

## 相关文档

- [设备枚举教程](ENUMERATION_TUTORIAL.md) - 了解如何发现和识别相机设备
- [参数调整教程](PARAMETER_TUNING_TUTORIAL.md) - 优化相机参数以获得最佳效果
- [录制教程](RECORDING_TUTORIAL.md) - 学习数据录制和高级回放功能
- [示例指南](SAMPLES.md) - 查看完整的示例程序列表