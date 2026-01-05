@page recording_tutorial 数据录制与回放教程

本教程介绍如何使用 EvRGB Combo SDK 录制同步的 RGB 图像和 DVS 事件数据，以及如何回放录制的数据进行深入分析。

## 目录

- [录制功能概述](#录制功能概述)
- [录制功能使用](#录制功能使用)
- [回放功能使用](#回放功能使用)
- [配置详解](#配置详解)
- [Metadata 管理](#metadata-管理)
- [常见问题](#常见问题)
- [下一步](#下一步)

## 录制功能概述

EvRGB Combo SDK 提供了强大的数据录制功能，可以将 RGB 图像和 DVS 事件数据同步录制到文件中，支持以下特性：

- **同步录制**：RGB 图像和 DVS 事件精确时间同步
- **多格式输出**：支持 MP4 视频、CSV 时间戳、原始事件数据
- **自动创建目录**：输出目录不存在时自动创建
- **线程安全**：录制过程不影响实时数据采集
- **Metadata 保存**：自动保存相机配置和标定信息

### 录制内容

录制功能会生成以下文件：

| 文件名 | 格式 | 说明 |
|--------|------|------|
| `combo_rgb.mp4` | MP4 视频 | RGB 图像序列，使用 OpenCV VideoWriter 编码 |
| `combo_timestamps.csv` | CSV 文本 | 包含帧序号、曝光开始时间、曝光结束时间 |
| `combo_events.raw` | 二进制文件 | DVS 原始事件数据，包含所有事件的时间戳和坐标 |
| `metadata.json` | JSON 文件 | 相机配置、标定信息等元数据（可选） |

## 录制功能使用

### 基本录制流程

```cpp
// 1. 创建 Combo 对象
evrgb::Combo combo(rgb_serial, dvs_serial, evrgb::ComboArrangement::STEREO, 100);

// 2. 配置录制器
evrgb::SyncedRecorderConfig recorder_cfg;
recorder_cfg.output_dir = "recordings";
recorder_cfg.fps = 30.0;
recorder_cfg.fourcc = "mp4v";

// 3. 创建并绑定录制器
auto recorder = std::make_shared<evrgb::SyncedDataRecorder>();
combo.setSyncedDataRecorder(recorder);

// 4. 初始化并启动 Combo
if (!combo.init() || !combo.start()) {
    return -1;
}

// 5. 开始录制
combo.startRecording(recorder_cfg);

// 6. 录制数据...
// ...

// 7. 停止录制
combo.stopRecording();

// 8. 可选：保存 metadata
combo.saveMetadata("recordings/metadata.json", nullptr);

// 9. 停止 Combo
combo.stop();
```

### 示例程序

参考示例代码 `sample/user_examples/combo_sync_display_example.cpp`，该示例提供了完整的录制功能演示，包括：

- 实时同步显示
- 交互式录制控制
- 事件叠加偏移调整
- 多种显示模式切换

运行示例程序：

```bash
./bin/combo_sync_display_example
```

交互控制：
- 空格键：开始/停止录制
- m 键：切换显示模式（叠加显示 / 并排显示）
- 方向键：调整 DVS 事件叠加的偏移量
- q 键 / ESC：退出程序

## 回放功能使用

### 回放功能特性

EvRGB Combo SDK 提供了强大的回放功能：

- **精确时间控制**：基于时间戳的精确数据同步
- **慢动作播放**：支持慢动作回放，便于观察事件细节
- **多种显示模式**：支持叠加显示和并排显示
- **Metadata 自动加载**：自动加载并应用相机配置和标定信息
- **交互控制**：支持实时调整显示参数

### 基本回放流程

```cpp
// 1. 创建回放器
evrgb::RecordedSyncReader reader({"recordings"});
if (!reader.open()) {
    return -1;
}

// 2. 创建可视化画布
evrgb::EventVisualizer canvas(
    reader.getRgbFrameSize(),
    reader.getEventFrameSize()
);

// 3. 加载并应用 Metadata
auto metadata = reader.getMetadata();
if (metadata.has_value()) {
    // 应用相机内参
    if (metadata->rgb.intrinsics && metadata->dvs.intrinsics) {
        canvas.setIntrinsics(*metadata->rgb.intrinsics, *metadata->dvs.intrinsics);
    }
    // 应用标定信息
    if (std::holds_alternative<evrgb::AffineTransform>(metadata->calibration)) {
        canvas.setCalibration(std::get<evrgb::AffineTransform>(metadata->calibration));
    }
    // 根据排列模式设置翻转
    if (metadata->arrangement == evrgb::ComboArrangement::BEAM_SPLITTER) {
        canvas.setFlipX(true);
    }
}

// 4. 回放数据
evrgb::RecordedSyncReader::Sample sample;
while (reader.next(sample)) {
    // 更新 RGB 帧
    canvas.updateRgbFrame(sample.rgb);

    // 可视化事件
    cv::Mat view;
    canvas.visualizeEvents(sample.events->begin(), sample.events->end(), view);

    // 显示
    cv::imshow("Replay", view);
    cv::waitKey(1);
}
```

### 示例程序

参考示例代码 `sample/user_examples/recorded_replay_example/recorded_replay_example.cpp`，该示例提供了完整的回放功能演示，包括：

- 精确的时间戳控制
- 慢动作播放功能
- 显示模式切换
- Metadata 自动加载和应用

运行示例程序：

```bash
./bin/recorded_replay_example recordings
```

交互控制：
- 空格键：切换慢动作模式
- m 键：切换显示模式（叠加显示 / 并排显示）
- f 键：翻转 X 轴（用于分束镜模式）
- q 键 / ESC：退出程序

## 配置详解

### SyncedRecorderConfig 结构

```cpp
struct SyncedRecorderConfig {
    std::string output_dir;  // 输出目录路径
    double fps;              // 视频帧率
    std::string fourcc;      // 视频编码格式（如 "mp4v", "avc1"）
};
```

### output_dir（输出目录）

- **类型**：`std::string`
- **说明**：录制文件的输出目录
- **注意事项**：
  - 目录不存在时会自动创建
  - 确保程序有写入权限
  - 建议使用绝对路径

### fps（帧率）

- **类型**：`double`
- **说明**：MP4 视频的帧率
- **推荐值**：
  - 30.0：标准帧率，适合大多数应用
  - 60.0：高帧率，适合快速运动场景
  - 120.0：超高帧率，适合慢动作分析
- **注意事项**：
  - 建议设置为接近 RGB 实际帧率的值
  - 过高的帧率会增加文件大小
  - 过低的帧率可能导致数据丢失

### fourcc（编码格式）

- **类型**：`std::string`
- **说明**：OpenCV 视频编码器的 FourCC 代码
- **常用值**：
  - `"mp4v"`：MPEG-4 编码（兼容性好）
  - `"avc1"`：H.264 编码（压缩率高）
  - `"xvid"`：XVID 编码（开源）
  - `"mjpa"`：Motion JPEG（无损）
- **注意事项**：
  - 取决于 OpenCV 编译时支持的编码器
  - 不同平台支持的编码器可能不同
  - 建议测试编码器兼容性

## Metadata 管理

### Metadata 概述

Metadata（元数据）包含了相机配置、标定信息等关键数据，用于确保回放时能够正确还原录制时的设置。

### Metadata 数据结构

```cpp
struct ComboMetadata {
    CameraMetadata rgb;                    // RGB 相机元数据
    CameraMetadata dvs;                    // DVS 相机元数据
    ComboArrangement arrangement;          // 排列模式
    ComboCalibrationInfo calibration;      // 标定信息
};

struct CameraMetadata {
    std::string manufacturer;              // 制造商
    std::string model;                     // 相机型号
    std::string serial;                    // 序列号
    unsigned int width = 0;                // 图像宽度
    unsigned int height = 0;               // 图像高度
    std::optional<CameraIntrinsics> intrinsics;  // 相机内参
};
```

### 保存 Metadata

```cpp
std::string metadata_path = "recordings/metadata.json";
std::string error_message;

if (combo.saveMetadata(metadata_path, &error_message)) {
    std::cout << "Metadata 保存成功" << std::endl;
} else {
    std::cerr << "Metadata 保存失败: " << error_message << std::endl;
}
```

### 加载 Metadata（回放时）

回放器会自动加载 metadata：

```cpp
evrgb::RecordedSyncReader reader({"recordings"});
if (!reader.open()) {
    return -1;
}

auto metadata = reader.getMetadata();
if (metadata.has_value()) {
    // 应用 metadata 到可视化器
    if (metadata->rgb.intrinsics && metadata->dvs.intrinsics) {
        canvas.setIntrinsics(*metadata->rgb.intrinsics, *metadata->dvs.intrinsics);
    }
}
```

### Metadata 使用场景

1. **标定信息持久化**：保存相机标定结果，避免重复标定
2. **配置管理**：记录 RGB 和 DVS 相机的参数设置
3. **数据录制关联**：在录制数据时保存对应的 metadata，确保数据可追溯
4. **回放还原**：回放时自动应用录制时的配置和标定信息

## 常见问题

### Q: 录制失败，无法创建文件

**可能原因**：
1. 输出目录路径不正确
2. 程序没有写入权限
3. 磁盘空间不足
4. OpenCV 编码器不支持指定的 fourcc

**解决方法**：
1. 检查 `output_dir` 路径是否正确
2. 确保程序有写入权限
3. 检查磁盘空间
4. 尝试使用不同的 fourcc 编码器

### Q: 录制的视频无法播放

**可能原因**：
1. OpenCV 编码器不支持
2. 视频播放器不支持该编码格式
3. fps 设置不正确

**解决方法**：
1. 尝试使用不同的 fourcc（如 `"mp4v"`）
2. 使用支持更多格式的播放器（如 VLC）
3. 检查 fps 设置是否合理

### Q: 录制时丢帧

**可能原因**：
1. 系统负载过高
2. 缓冲区大小不足
3. 磁盘写入速度慢
4. fps 设置过低

**解决方法**：
1. 增加缓冲区大小（`Combo` 构造参数中的 `max_buffer_size`）
2. 使用 SSD 提高磁盘写入速度
3. 调整 fps 设置
4. 优化系统资源使用

### Q: 回放时事件和图像不同步

**可能原因**：
1. Metadata 未正确加载
2. 标定信息缺失
3. 排列模式设置错误

**解决方法**：
1. 确保录制时保存了 metadata
2. 检查 metadata 中的标定信息是否完整
3. 验证排列模式（STEREO 或 BEAM_SPLITTER）是否正确

### Q: 如何选择合适的 fps？

**推荐方案**：
- **标准应用**：30 fps（平衡性能和质量）
- **快速运动场景**：60 fps（减少运动模糊）
- **慢动作分析**：120 fps 或更高（便于后期处理）
- **存储受限**：15-20 fps（节省空间）

### Q: 如何选择合适的 fourcc？

**推荐方案**：
- **最大兼容性**：`"mp4v"`（MPEG-4）
- **最佳压缩率**：`"avc1"`（H.264）
- **无损质量**：`"mjpa"`（Motion JPEG）
- **跨平台**：测试不同平台的编码器支持

## 下一步

掌握录制和回放功能后，你可以：

1. 学习相机标定教程，了解如何进行相机标定
2. 探索参数调整教程，优化相机性能
3. 查看可视化教程，学习事件数据的可视化技巧
4. 查看更多示例代码，了解实际应用场景

## 相关文档

- [相机使用教程](CAMERA_USAGE_TUTORIAL.md) - 学习 Combo 基本操作
- [设备枚举教程](ENUMERATION_TUTORIAL.md) - 了解如何发现和识别相机设备
- [参数调整教程](PARAMETER_TUNING_TUTORIAL.md) - 优化相机参数以获得最佳效果
- [标定教程](CALIBRATION_TUTORIAL.md) - 学习相机标定方法
- [可视化教程](VISUALIZATION_TUTORIAL.md) - 学习事件数据可视化技巧
- [示例指南](SAMPLES.md) - 查看完整的示例程序列表

*/
