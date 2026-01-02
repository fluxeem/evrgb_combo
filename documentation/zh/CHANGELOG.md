# 更新日志

本项目的所有重要变更都将记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
并且本项目遵循 [语义化版本](https://semver.org/lang/zh-CN/spec/v2.0.0.html)。

## [未发布]

### 新增
- 初始变更日志设置

### 变更

### 已弃用

### 移除

### 修复

### 安全

## [1.0.0] - 2025-01-02

### 新增
- EvRGB Combo SDK 初始发布
- 核心 `Combo` 类，用于统一管理 RGB+DVS 相机
- 内部时间同步驱动，支持纳秒级精度
- 线程安全的多线程架构
- 同步数据录制功能（RGB MP4 + CSV + DVS raw）
- 录制数据回放功能
- 跨平台支持（Linux 和 Windows）
- 完整的示例程序：
  - `camera_enum_example` - 相机设备枚举
  - `combo_sync_display_example` - 实时同步预览
  - `recorded_replay_example` - 录制数据回放
  - `rgb_exposure_step_example` - RGB 曝光调整
- 完整的文档系统，包含教程和 API 参考
- CMake 构建系统，支持预设配置
- Windows 依赖管理的 vcpkg 集成

### 核心组件
- **Combo 类** (`include/core/combo.h`) - 主要管理接口
- **TriggerBuffer 类** (`include/sync/trigger_buffer.h`) - 时间戳同步
- **EventVectorPool 类** (`include/sync/event_vector_pool.h`) - 内存管理
- **SyncedDataRecorder 类** (`include/recording/synced_data_recorder.h`) - 数据录制
- **RecordedSyncReader 类** (`include/recording/recorded_sync_reader.h`) - 数据回放
- **相机接口** (`include/camera/`) - RGB 和 DVS 相机抽象

### 技术特性
- 符合 C++17 标准
- RAII 资源管理
- 智能指针内存管理
- 异常安全设计
- 使用 `CameraStatus` 结构体的全面错误处理
- 基于 Loguru 的日志系统
- 硬件级触发信号同步
- 性能优化的事件向量内存池

### 文档
- 完整的教程系统（英文和中文）
- Doxygen API 参考文档
- 所有支持平台的安装指南
- 带详细解释的示例代码
- 参数调整指南

### 构建系统
- 现代 CMake 3.22+ 配置
- 不同构建环境的 CMake 预设
- CPack 包生成支持
- 海康机器人 MVS SDK 集成的 FindMVS.cmake 模块
- 自动化依赖管理

[未发布]: https://github.com/fluxeem/evrgb_combo/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/fluxeem/evrgb_combo/releases/tag/v1.0.0