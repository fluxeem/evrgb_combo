@page calibration_tutorial 校准教程

本教程介绍如何使用 EvRGB Combo SDK 进行相机校准，包括 BEAM_SPLITTER 布局下的 AffineTransform 校准。

## 📋 目录

- [校准概述](#校准概述)
- [校准数据结构](#校准数据结构)
- [BEAM_SPLITTER 校准](#beam_splitter-校准)
- [校准数据保存与加载](#校准数据保存与加载)
- [完整示例](#完整示例)
- [常见问题](#常见问题)

---

## 校准概述

EvRGB Combo SDK 提供了相机校准功能，用于对齐 RGB 相机和 DVS 相机的坐标系。当前支持两种校准类型：

- **RigidTransform**: 用于 STEREO 布局，包含旋转矩阵和平移向量
- **AffineTransform**: 用于 BEAM_SPLITTER 布局，包含 2D 仿射变换矩阵

本教程重点介绍 BEAM_SPLITTER 布局下的 AffineTransform 校准。

---

## 校准数据结构

### CameraIntrinsics

相机内参描述相机的内部光学特性：

```cpp
struct CameraIntrinsics {
    double fx{0.0};  // x 方向焦距
    double fy{0.0};  // y 方向焦距
    double cx{0.0};  // 主点 x 坐标
    double cy{0.0};  // 主点 y 坐标
    double skew{0.0}; // 偏斜系数
    int width{0};    // 图像宽度
    int height{0};   // 图像高度
    std::vector<double> distortion; // 畸变系数
};
```

### AffineTransform

仿射变换用于 BEAM_SPLITTER 布局的对齐：

```cpp
struct AffineTransform {
    cv::Matx23d A{1.0, 0.0, 0.0,
                  0.0, 1.0, 0.0};

    cv::Matx23d matrix() const { return A; }
};
```

仿射变换矩阵格式：
```
| a b tx |
| c d ty |
```
其中 (tx, ty) 是平移向量，a, b, c, d 控制旋转和缩放。

### ComboCalibrationInfo

联合类型，可以是以下三种之一：

```cpp
using ComboCalibrationInfo = std::variant<std::monostate, RigidTransform, AffineTransform>;
```

---

## BEAM_SPLITTER 校准

### 为什么 BEAM_SPLITTER 使用 AffineTransform？

BEAM_SPLITTER 布局使用分光镜（光学棱镜/反射镜）让 RGB 相机和 DVS 相机共享相同的视野，两台相机观察的是同一个场景。这种硬件配置决定了校准需求的特点：

**物理特性**：
- **共享光路**：通过分光镜将光线分成两路，两台相机观察同一个场景
- **光轴平行**：两台相机的光轴几乎平行，只有轻微的角度差异
- **视角一致**：两幅图像的视角基本相同，差异主要来自光学系统的微小偏差

**为什么使用 2D 仿射变换**：

1. **变换维度简化**：由于两台相机观察的是同一个场景，视角差异很小，主要是在 2D 图像平面上的差异，不需要完整的 3D 变换

2. **参数数量少**：AffineTransform 只需要 6 个参数（2x3 矩阵），包含平移、旋转和缩放，相比 RigidTransform 的 6 个自由度（3D 旋转 + 3D 平移）更简洁

3. **校准简单**：
   - BEAM_SPLITTER 只需要 2D 仿射变换校准，参数少，调整直观
   - STEREO 需要完整的立体标定，包含 3D 旋转矩阵和 3D 平移向量，校准过程更复杂

4. **实际应用**：BEAM_SPLITTER 的主要应用是深度学习和算法研究，需要精确的空间对齐，2D 仿射变换足以满足需求

**与 STEREO 的对比**：

| 特性 | BEAM_SPLITTER | STEREO |
|------|--------------|--------|
| 光学配置 | 分光镜共享光路 | 两台相机独立视角 |
| 变换类型 | 2D 仿射变换 | 3D 刚体变换 |
| 校准复杂度 | 简单 | 复杂 |
| 主要参数 | 平移、旋转、缩放 | 3D 旋转矩阵、3D 平移向量 |
| 应用场景 | 深度学习、算法研究 | 实际应用、多视角 |

---

## 完整示例

完整的校准示例代码请参考：

`sample/user_examples/beam_splitter_align/beam_splitter_align.cpp`

该示例包含：
- 实时叠加显示
- 键盘交互调整（方向键平移，+/- 缩放）
- 录制功能
- 元数据保存和加载

### 运行示例

```bash
./beam_splitter_align [metadata_path]
```

### 交互控制

- **方向键**: 调整仿射变换平移
- **+**: 放大仿射变换 (2%)
- **-**: 缩小仿射变换 (2%)
- **空格**: 开始/停止录制
- **q / ESC**: 退出

---

## 常见问题

### Q: 如何判断校准是否准确？

A: 观察实时叠加视图中的事件是否与图像中的对应特征对齐。可以使用棋盘格或其他明显的特征点进行验证。

### Q: 校准参数的初始值应该是什么？

A: 通常从单位矩阵开始（tx=0, ty=0, scale=1.0），然后通过交互式调整逐步优化。

### Q: AffineTransform 和 RigidTransform 的区别是什么？

A:
- **AffineTransform**: 2D 变换，适用于 BEAM_SPLITTER 布局，包含旋转、缩放和平移
- **RigidTransform**: 3D 刚体变换，适用于 STEREO 布局，只包含旋转和平移

### Q: BEAM_SPLITTER 和 STEREO 布局应该如何选择？

A:
- **BEAM_SPLITTER**: 适用于需要精确空间对齐的场景，如深度学习和算法研究。需要分光镜硬件，校准简单。
- **STEREO**: 适用于实际应用和多视角场景。只需要两台相机和触发线，但需要完整的立体标定。

---

## 相关资源

- [API 参考: calib_info.h](../../../include/utils/calib_info.h)
- [示例: beam_splitter_align](../../../sample/user_examples/beam_splitter_align/)
- [教程: 相机使用教程](camera_usage_tutorial.md)