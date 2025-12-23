@page camera_usage_tutorial 相机使用教程

本教程介绍如何使用 EvRGB Combo SDK 操作 RGB 相机、DVS 相机，以及组合相机。

## 目录

- [RGB 相机基本操作](#rgb-相机基本操作)
- [DVS 相机基本操作](#dvs-相机基本操作)
- [组合相机使用](#组合相机使用)
- [错误处理](#错误处理)
- [常见问题](#常见问题)

## RGB 相机基本操作

### 初始化和基本使用

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

int main() {
    // 枚举 RGB 相机
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) {
        std::cerr << "未找到 RGB 相机" << std::endl;
        return 1;
    }

    // 使用第一个可用的相机
    const std::string serial = cameras[0].serial_number;
    evrgb::HikvisionRgbCamera camera;

    // 初始化相机
    if (!camera.initialize(serial)) {
        std::cerr << "相机初始化失败" << std::endl;
        return 1;
    }

    std::cout << "相机初始化成功: " << serial << std::endl;

    // 启动相机
    if (!camera.start()) {
        std::cerr << "相机启动失败" << std::endl;
        return 1;
    }

    std::cout << "相机已启动，开始采集图像..." << std::endl;

    // 获取图像
    for (int i = 0; i < 10; ++i) {
        cv::Mat frame;
        if (camera.getLatestImage(frame)) {
            std::cout << "获取到图像 " << (i+1) << ": " 
                      << frame.cols << "x" << frame.rows << std::endl;
            
            // 在这里处理图像...
            // cv::imwrite("frame_" + std::to_string(i) + ".jpg", frame);
        } else {
            std::cout << "获取图像失败" << std::endl;
        }
        
        // 等待一段时间
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 停止和销毁
    camera.stop();
    camera.destroy();
    std::cout << "相机已停止并销毁" << std::endl;
    
    return 0;
}
```

### 设置图像回调

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) return 1;

    evrgb::HikvisionRgbCamera camera;
    if (!camera.initialize(cameras[0].serial_number)) return 1;

    // 设置图像回调函数
    camera.setImageCallback([](const cv::Mat& image, uint64_t timestamp_us) {
        static int frame_count = 0;
        frame_count++;
        
        std::cout << "收到图像 " << frame_count 
                  << " 时间戳: " << timestamp_us << " us"
                  << " 尺寸: " << image.cols << "x" << image.rows << std::endl;
        
        // 在这里处理图像...
    });

    if (!camera.start()) return 1;

    // 运行 5 秒
    std::this_thread::sleep_for(std::chrono::seconds(5));

    camera.stop();
    camera.destroy();
    return 0;
}
```

## DVS 相机基本操作

### 初始化和事件采集

```cpp
#include "camera/dvs_camera.h"
#include <iostream>

int main() {
    // 枚举 DVS 相机
    auto cameras = evrgb::enumerateAllDvsCameras();
    if (cameras.empty()) {
        std::cerr << "未找到 DVS 相机" << std::endl;
        return 1;
    }

    // 使用第一个可用的相机
    const std::string serial = cameras[0].serial;
    evrgb::DvsCamera camera;

    // 初始化相机
    if (!camera.initialize(serial)) {
        std::cerr << "DVS 相机初始化失败" << std::endl;
        return 1;
    }

    std::cout << "DVS 相机初始化成功: " << serial << std::endl;

    // 设置事件回调函数
    camera.setEventCallback([](const std::vector<dvsense::Event2D>& events) {
        if (!events.empty()) {
            std::cout << "收到 " << events.size() << " 个事件" << std::endl;
            
            // 统计正负事件
            int positive_events = 0;
            int negative_events = 0;
            for (const auto& event : events) {
                if (event.polarity) {
                    positive_events++;
                } else {
                    negative_events++;
                }
            }
            
            std::cout << "  正事件: " << positive_events 
                      << ", 负事件: " << negative_events << std::endl;
        }
    });

    // 启动相机
    if (!camera.start()) {
        std::cerr << "DVS 相机启动失败" << std::endl;
        return 1;
    }

    std::cout << "DVS 相机已启动，开始采集事件..." << std::endl;

    // 运行 5 秒
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 停止和销毁
    camera.stop();
    camera.destroy();
    std::cout << "DVS 相机已停止并销毁" << std::endl;
    
    return 0;
}
```

## 组合相机使用

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

### 检查相机状态

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

void printCameraStatus(const evrgb::CameraStatus& status) {
    if (status.success()) {
        std::cout << "操作成功" << std::endl;
    } else {
        std::cout << "操作失败: " << status.message
                  << " (错误码: 0x" << std::hex << status.code << std::dec << ")" << std::endl;
    }
}

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) return 1;

    evrgb::HikvisionRgbCamera camera;
    
    // 初始化并检查状态
    auto status = camera.initialize(cameras[0].serial_number);
    printCameraStatus(status);
    if (!status.success()) return 1;

    // 启动并检查状态
    status = camera.start();
    printCameraStatus(status);
    if (!status.success()) {
        camera.destroy();
        return 1;
    }

    // 获取图像并检查状态
    cv::Mat frame;
    if (camera.getLatestImage(frame)) {
        std::cout << "成功获取图像" << std::endl;
    } else {
        std::cout << "获取图像失败" << std::endl;
    }

    camera.stop();
    camera.destroy();
    return 0;
}
```

## 常见问题

### Q: 相机初始化失败

**可能原因**：
1. 相机序列号不正确
2. 相机被其他程序占用
3. 驱动程序问题
4. 权限不足

**解决方法**：
1. 使用枚举示例确认正确的序列号
2. 关闭其他可能使用相机的程序
3. 重新安装相机驱动
4. Linux 上使用 sudo 或配置设备权限

### Q: 无法获取图像

**排查步骤**：
1. 确认相机已启动
2. 检查相机连接
3. 验证相机参数设置
4. 查看错误日志

### Q: 回调函数未被调用

**可能原因**：
1. 相机未启动
2. 回调函数设置在启动之后
3. 数据缓冲区问题

**解决方法**：
1. 确保在启动前设置回调
2. 检查相机是否成功启动
3. 调整缓冲区大小

### Q: 组合相机同步问题

**注意事项**：
1. 确保 RGB 和 DVS 相机都正常工作
2. 检查时间同步设置
3. 验证触发连接

## 下一步

掌握基础操作后，你可以：

1. 学习参数调整教程
2. 了解数据录制功能
3. 探索高级同步特性
4. 查看更多示例代码

## 相关文档

- [设备枚举教程](ENUMERATION_TUTORIAL.md)
- [参数调整教程](PARAMETER_TUNING_TUTORIAL.md)
- [录制教程](RECORDING_TUTORIAL.md)