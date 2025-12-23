# EvRGB Combo SDK

[English](https://sdk.fluxeem.com/evrgb_combo/en/docs/index.html) | [中文](https://sdk.fluxeem.com/evrgb_combo/zh/docs/index.html)


EvRGB Combo SDK 是为 RGB + DVS 硬件套装设计的 C++ 驱动库，提供统一接口与内置时间同步。核心 `Combo` 类同时管理两类相机，确保数据时间对齐。

## ✨ 关键特性

- 统一管理：单一接口管理 RGB 与 DVS，相机同步开箱即用
- 内部同步驱动：硬件级触发，同步精度可达纳秒级
- 线程安全：多线程架构，支持并发调用
- 录制与回放：支持同步录制（RGB MP4 + CSV + DVS 原始数据）与回放
- 跨平台：支持 Linux 与 Windows，提供完整构建脚本

## 🚀 快速开始

### 依赖
- CMake 3.22+
- C++17 编译器
- OpenCV 4.x
- Hikrobot MVS SDK（RGB 相机）
- DvsenseDriver（DVS 相机）

### 构建
```bash
git clone https://github.com/tonywangziteng/EvRGB_combo_sdk.git
cd EvRGB_combo_sdk
mkdir build && cd build
cmake .. -DBUILD_SAMPLES=ON
make -j$(nproc)      # Linux/macOS
# 或：cmake --build . --config Release  # Windows
```

### 运行示例
```bash
./bin/camera_enum_example            # 枚举相机
./bin/combo_sync_display_example     # 实时同步预览
./bin/recorded_replay_example [dir]  # 回放录制数据
```

## 📚 文档

| 主题 | 中文 |
|------|------|
| 安装指南 | https://sdk.fluxeem.com/evrgb_combo/zh/docs/index.html |
| 教程索引 | https://sdk.fluxeem.com/evrgb_combo/zh/docs/tutorial_index.html |
| 示例指南 | https://sdk.fluxeem.com/evrgb_combo/zh/docs/samples_guide.html |
| API 参考 | https://sdk.fluxeem.com/evrgb_combo/zh/docs/annotated.html |

## 🛠️ 构建选项

| 选项 | 默认 | 说明 |
|------|------|------|
| `BUILD_SAMPLES=ON/OFF` | `ON` | 构建示例程序 |
| `BUILD_TESTS=ON/OFF` | `OFF` | 构建测试程序 |
| `BUILD_DEV_SAMPLES=ON/OFF` | `OFF` | 构建内部调试示例 |
| `CMAKE_BUILD_TYPE=Release/Debug` | `Release` | 构建配置 |

## 📞 支持

- 问题反馈：<https://github.com/tonywangziteng/EvRGB_combo_sdk/issues>
- 更多文档：见本目录下中文教程
