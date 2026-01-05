@page camera_usage_tutorial Combo 使用教程

本教程介绍如何使用 EvRGB Combo SDK 操作 RGB+DVS 组合相机。本项目推荐使用 Combo 接口来利用内置的时间同步机制，实现 RGB 图像和 DVS 事件的精确同步。

## 目录

- [Combo 基本操作](#combo-基本操作)
- [设置同步回调](#设置同步回调)
- [分别设置回调](#分别设置回调)
- [错误处理](#错误处理)
- [常见问题](#常见问题)

## Combo 基本操作

### 组合排列模式

EvRGB Combo SDK 支持两种 RGB+DVS 组合排列模式，分别适用于不同的应用场景：

#### 1. STEREO 模式（立体模式）
- **定义**：RGB 相机和 DVS 相机并排独立放置，通过外部触发信号进行时间同步
- **特点**：
  - 相机之间物理分离，视野独立
  - 需要外部触发线连接以实现精确同步
  - 适用于需要分别观察两个不同视角的场景
  - 需要通过标定确定两个相机之间的空间关系
- **使用场景**：
  - 双目视觉系统
  - 需要独立控制 RGB 和 DVS 视野的应用
  - 大视野覆盖场景
  - 需要灵活配置相机位置的实验环境

#### 2. BEAM_SPLITTER 模式（分束镜模式）
- **定义**：RGB 相机和 DVS 相机通过分束镜（光束分离器）共享同一光路，实现完美的空间对齐
- **特点**：
  - RGB 和 DVS 共享相同的视野和光路
  - 自动实现空间对齐，无需复杂的标定
  - 通过分束镜将光线分到两个相机
  - 保证像素级的空间对应关系
- **使用场景**：
  - 需要像素级空间对齐的深度学习应用
  - 事件相机与传统相机融合研究
  - 需要精确对齐 RGB 图像和事件数据的应用
  - 计算机视觉算法开发和测试

#### 如何选择排列模式
选择排列模式时，考虑以下因素：
- **空间对齐需求**：需要完美对齐选择 BEAM_SPLITTER，灵活视角选择 STEREO
- **硬件配置**：BEAM_SPLITTER 需要分束镜硬件，STEREO 只需要两个相机和触发线
- **应用场景**：深度学习和算法研究推荐 BEAM_SPLITTER，实际应用推荐 STEREO
- **标定复杂度**：BEAM_SPLITTER 标定简单，STEREO 需要完整的立体标定

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

    // 指定排列模式（默认为 STEREO）
    // evrgb::ComboArrangement::STEREO - 立体模式（相机并排）
    // evrgb::ComboArrangement::BEAM_SPLITTER - 分束镜模式（共享光路）
    evrgb::Combo combo(rgb_serial, dvs_serial, evrgb::ComboArrangement::STEREO);

    // 或者使用分束镜模式
    // evrgb::Combo combo(rgb_serial, dvs_serial, evrgb::ComboArrangement::BEAM_SPLITTER);

    // 获取当前排列模式
    evrgb::ComboArrangement arrangement = combo.getArrangement();
    std::cout << "排列模式: " << evrgb::toString(arrangement) << std::endl;

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

    // 创建 Combo 对象（默认使用 STEREO 模式）
    evrgb::Combo combo(rgb_cameras[0].serial_number, dvs_cameras[0].serial);

    // 如果需要使用分束镜模式，可以指定：
    // evrgb::Combo combo(rgb_cameras[0].serial_number, dvs_cameras[0].serial,
    //                     evrgb::ComboArrangement::BEAM_SPLITTER);

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

## Metadata 保存和加载

### Metadata 概述

EvRGB Combo SDK 提供了 metadata（元数据）管理功能，用于保存和加载相机配置、标定信息等关键数据。主要应用场景包括：

- **标定信息持久化**：保存相机标定结果，避免重复标定
- **配置管理**：记录 RGB 和 DVS 相机的参数设置
- **数据录制关联**：在录制数据时保存对应的 metadata，确保数据可追溯

### Metadata 数据结构

#### ComboMetadata（组合元数据）

```cpp
struct ComboMetadata {
    CameraMetadata rgb;                    // RGB 相机元数据
    CameraMetadata dvs;                    // DVS 相机元数据
    ComboArrangement arrangement;          // 排列模式（STEREO 或 BEAM_SPLITTER）
    ComboCalibrationInfo calibration;      // 标定信息
};
```

#### CameraMetadata（单相机元数据）

```cpp
struct CameraMetadata {
    std::string manufacturer;              // 制造商
    std::string model;                     // 相机型号
    std::string serial;                    // 序列号
    unsigned int width = 0;                // 图像宽度
    unsigned int height = 0;               // 图像高度
    std::optional<CameraIntrinsics> intrinsics;  // 相机内参（可选）
};
```

### Metadata 操作方法

#### 获取 Metadata

```cpp
// 获取当前 Combo 的所有元数据
evrgb::ComboMetadata metadata = combo.getMetadata();

// 转换为 JSON（需要包含 nlohmann/json.hpp）
nlohmann::json j = metadata;
std::cout << j.dump(2) << std::endl;
```

#### 保存 Metadata

```cpp
// 保存 metadata 到 JSON 文件
std::string metadata_path = "combo_metadata.json";
std::string error_message;

if (combo.saveMetadata(metadata_path, &error_message)) {
    std::cout << "Metadata 保存成功: " << metadata_path << std::endl;
} else {
    std::cerr << "Metadata 保存失败: " << error_message << std::endl;
}
```

#### 加载 Metadata

```cpp
// 从 JSON 文件加载 metadata 并应用到 Combo
std::string metadata_path = "combo_metadata.json";
std::string error_message;

if (combo.loadMetadata(metadata_path, &error_message)) {
    std::cout << "Metadata 加载成功" << std::endl;
    
    // 获取加载后的 metadata
    evrgb::ComboMetadata metadata = combo.getMetadata();
} else {
    std::cerr << "Metadata 加载失败: " << error_message << std::endl;
}
```

#### 应用 Metadata

```cpp
// 手动应用自定义 metadata
evrgb::ComboMetadata metadata;
// ... 设置 metadata 内容

if (combo.applyMetadata(metadata, true)) {
    std::cout << "Metadata 应用成功" << std::endl;
}
```

### 使用场景示例

**标定信息持久化**：

```cpp
// 标定完成后保存
combo.calibration_info = affine_transform;
combo.saveMetadata("calibration.json", nullptr);

// 启动时加载
combo.loadMetadata("calibration.json", nullptr);
```

**数据录制关联**：

```cpp
// 保存 metadata 到录制目录
std::string metadata_path = recording_dir + "/metadata.json";
combo.saveMetadata(metadata_path, nullptr);
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

    // 创建 Combo 对象（默认使用 STEREO 模式）
    // 如果需要使用分束镜模式，可以指定：
    // evrgb::Combo combo(rgb_cameras[0].serial_number, dvs_cameras[0].serial,
    //                     evrgb::ComboArrangement::BEAM_SPLITTER);
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

### Q: 如何选择合适的排列模式？

**STEREO 模式适用场景**：
- 需要灵活配置相机位置
- 需要观察两个不同的视角
- 实际应用环境，视野范围较大
- 已有立体标定经验

**BEAM_SPLITTER 模式适用场景**：
- 需要像素级空间对齐
- 深度学习模型训练和推理
- 算法开发和测试
- 需要精确对应 RGB 图像和事件数据

### Q: 排列模式是否可以在运行时更改？

**答案**：不可以。排列模式在创建 Combo 对象时指定，初始化后无法更改。如果需要更改排列模式，必须：
1. 销毁当前的 Combo 对象
2. 创建新的 Combo 对象并指定不同的排列模式
3. 重新初始化和启动

### Q: BEAM_SPLITTER 模式需要特殊的硬件吗？

**答案**：是的。BEAM_SPLITTER 模式需要：
- 分束镜（光束分离器）硬件
- 精确的光学对齐支架
- 两个相机的光轴需要对齐到分束镜
- 通常需要专业的光学调试

### Q: STEREO 模式的标定复杂吗？

**答案**：相对复杂。STEREO 模式需要进行完整的立体标定：
- 需要标定每个相机的内参
- 需要标定两个相机之间的外参（旋转和平移）
- 需要使用标定板进行多次采集
- SDK 提供了标定信息存储和加载功能（`calibration_info`）

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
5. 根据你的应用场景选择合适的排列模式（STEREO 或 BEAM_SPLITTER）
6. 学习如何进行相机标定，特别是 STEREO 模式下的立体标定
7. 探索标定信息的保存和加载功能（`saveMetadata` 和 `loadMetadata`）

## 相关文档

- [设备枚举教程](ENUMERATION_TUTORIAL.md) - 了解如何发现和识别相机设备
- [参数调整教程](PARAMETER_TUNING_TUTORIAL.md) - 优化相机参数以获得最佳效果
- [录制教程](RECORDING_TUTORIAL.md) - 学习数据录制和高级回放功能
- [示例指南](SAMPLES.md) - 查看完整的示例程序列表