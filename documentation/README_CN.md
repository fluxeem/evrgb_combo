# EvRGB Combo SDK

[English](README.md) | [中文](documentation/README_CN.md)

EvRGB Combo SDK 是一个专为 RGB+DVS 硬件套装设计的 C++ 驱动库，提供内部时间同步的统一接口。核心 `Combo` 类管理 RGB 相机和 DVS（事件相机）的组合，确保这两种不同类型相机之间的精确时间同步。

## ✨ 主要特性

- **🔗 统一的 RGB+DVS 管理**：单一接口管理两种相机类型，内置精确时间同步
- **⚡ 内部同步驱动**：硬件级触发信号同步，确保纳秒级时间戳对齐
- **🧵 线程安全设计**：多线程架构，支持并发访问
- **📹 数据录制与回放**：同步录制（RGB MP4 + CSV + DVS raw）和回放功能
- **🔧 跨平台支持**：支持 Linux 和 Windows，提供完善的构建系统

## 🚀 快速开始

### 前置要求
- **CMake** 3.22 或更高版本
- **C++17** 兼容编译器
- [OpenCV](https://opencv.org/) 4.x
- [海康机器人 MVS SDK](https://www.hikrobotics.com/cn/machinevision/service/download?module=0) 用于 RGB 相机
- [DvsenseDriver](https://sdk.dvsense.com/) 用于 DVS 相机

### 构建
```bash
# 克隆并构建
git clone https://github.com/tonywangziteng/EvRGB_combo_sdk.git
cd EvRGB_combo_sdk
mkdir build && cd build
cmake .. -DBUILD_SAMPLES=ON
make -j$(nproc)  # Linux/macOS
# 或: cmake --build . --config Release  # Windows
```

### 运行示例
```bash
# 枚举相机
./bin/camera_enum_example

# 实时同步预览
./bin/combo_sync_display_example

# 回放录制数据
./bin/recorded_replay_example [recording_dir]
```

## 📚 文档

| 主题 | 英文 |
|-------|---------|
| **安装** | [📖 安装指南](https://sdk.fluxeem.com/evrgb_combo/en/docs/index.html) |
| **教程** | [📚 教程索引](https://sdk.fluxeem.com/evrgb_combo/en/docs/tutorial_index.html) |
| **示例** | [🔧 示例指南](https://sdk.fluxeem.com/evrgb_combo/en/docs/samples_guide.html) |
| **API 参考** | [📖 生成的文档](https://sdk.fluxeem.com/evrgb_combo/en/docs/annotated.html) |
| **更新日志** | [📝 版本历史](CHANGELOG.md) |

## 🏗️ 项目结构

```
EvRGB_combo_sdk/
├── include/           # 头文件
│   ├── core/         # 核心 Combo 类和类型
│   ├── camera/       # RGB 和 DVS 相机接口
│   ├── sync/         # 同步组件
│   ├── recording/    # 数据录制/回放
│   └── utils/        # 工具和日志
├── src/              # 源代码实现
├── sample/           # 示例程序
├── test/             # 测试程序
├── documentation/    # 详细文档
└── external/         # 外部依赖 (loguru)
```

## 🎯 核心组件

### **Combo 类** (`include/core/combo.h`)
RGB+DVS 硬件套件的统一管理接口：
- 同时管理 RGB 和 DVS 相机
- 内部时间同步驱动
- 硬件级触发信号同步
- 同步数据处理的回调函数
- 数据录制和回放功能

### **TriggerBuffer 类** (`include/sync/trigger_buffer.h`)
管理触发信号的时间戳同步 - 实现 RGB+DVS 内部同步的关键组件。

### **SyncedDataRecorder 类** (`include/recording/synced_data_recorder.h`)
处理同步数据录制到文件（RGB MP4 + CSV + DVS raw）。

## 🛠️ 构建选项

| 选项 | 默认值 | 描述 |
|--------|---------|-------------|
| `BUILD_SAMPLES=ON/OFF` | `ON` | 构建示例程序 |
| `BUILD_TESTS=ON/OFF` | `OFF` | 构建测试程序 |
| `BUILD_DEV_SAMPLES=ON/OFF` | `OFF` | 构建开发测试示例 |
| `CMAKE_BUILD_TYPE=Release/Debug` | `Release` | 构建配置 |

## 📄 许可证

本项目采用 [LICENSE](LICENSE) 文件中指定的条款进行许可。

## 🤝 贡献

欢迎贡献！请随时提交 Pull Request。

## 📞 支持

- 🐛 **报告问题**：[GitHub Issues](https://github.com/tonywangziteng/EvRGB_combo_sdk/issues)
- 📖 **文档**：详细指南请参阅 [documentation](documentation/) 文件夹

---

**注意**：详细的安装说明、API 文档和教程，请参考 [documentation](documentation/) 文件夹。