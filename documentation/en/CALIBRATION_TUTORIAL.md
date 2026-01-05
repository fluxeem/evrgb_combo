@page calibration_tutorial Calibration Tutorial

This tutorial introduces how to use EvRGB Combo SDK for camera calibration, including AffineTransform calibration for BEAM_SPLITTER layout.

## 📋 Table of Contents

- [Calibration Overview](#calibration-overview)
- [Calibration Data Structures](#calibration-data-structures)
- [BEAM_SPLITTER Calibration](#beam_splitter-calibration)
- [Saving and Loading Calibration Data](#saving-and-loading-calibration-data)
- [Complete Example](#complete-example)
- [FAQ](#faq)

---

## Calibration Overview

EvRGB Combo SDK provides camera calibration functionality to align the coordinate systems of RGB and DVS cameras. Currently, two types of calibration are supported:

- **RigidTransform**: Used for STEREO layout, contains rotation matrix and translation vector
- **AffineTransform**: Used for BEAM_SPLITTER layout, contains 2D affine transformation matrix

This tutorial focuses on AffineTransform calibration for BEAM_SPLITTER layout.

---

## Calibration Data Structures

### CameraIntrinsics

Camera intrinsics describe the internal optical characteristics of the camera:

```cpp
struct CameraIntrinsics {
    double fx{0.0};  // Focal length in x direction
    double fy{0.0};  // Focal length in y direction
    double cx{0.0};  // Principal point x coordinate
    double cy{0.0};  // Principal point y coordinate
    double skew{0.0}; // Skew coefficient
    int width{0};    // Image width
    int height{0};   // Image height
    std::vector<double> distortion; // Distortion coefficients
};
```

### AffineTransform

Affine transformation used for BEAM_SPLITTER layout alignment:

```cpp
struct AffineTransform {
    cv::Matx23d A{1.0, 0.0, 0.0,
                  0.0, 1.0, 0.0};

    cv::Matx23d matrix() const { return A; }
};
```

Affine transformation matrix format:
```
| a b tx |
| c d ty |
```
where (tx, ty) is the translation vector, and a, b, c, d control rotation and scaling.

### ComboCalibrationInfo

Variant type that can be one of the following:

```cpp
using ComboCalibrationInfo = std::variant<std::monostate, RigidTransform, AffineTransform>;
```

---

## BEAM_SPLITTER Calibration

### Why BEAM_SPLITTER Uses AffineTransform?

BEAM_SPLITTER layout uses a beam splitter (optical prism/mirror) to allow both RGB and DVS cameras to share the same field of view. Both cameras observe the same scene. This hardware configuration determines the characteristics of calibration requirements:

**Physical Properties**:
- **Shared Optical Path**: Light is split into two paths through a beam splitter, allowing both cameras to observe the same scene
- **Parallel Optical Axes**: The optical axes of both cameras are almost parallel, with only slight angular differences
- **Consistent Perspective**: Both images have basically the same perspective, with differences mainly from minor deviations in the optical system

**Why Use 2D Affine Transformation**:

1. **Simplified Transformation Dimension**: Since both cameras observe the same scene with minimal perspective difference, the difference is primarily in the 2D image plane, requiring no complete 3D transformation

2. **Fewer Parameters**: AffineTransform only needs 6 parameters (2x3 matrix), including translation, rotation, and scaling. This is more concise than RigidTransform's 6 degrees of freedom (3D rotation + 3D translation)

3. **Numerical Stability**: In BEAM_SPLITTER configuration, the deviation between the two cameras is very small (typically only a few pixels of translation and slight rotation). In this case, traditional 3D extrinsic calibration methods (such as solving the essential matrix or PnP problem) are prone to numerical instability, leading to optimization failure or inaccurate results. 2D affine transformation is more suitable for handling such small deviations and can provide more stable and reliable calibration results.

4. **Simple Calibration**:
   - BEAM_SPLITTER only needs 2D affine transformation calibration with fewer parameters and intuitive adjustment
   - STEREO requires complete stereo calibration including 3D rotation matrix and 3D translation vector, making the calibration process more complex

5. **Practical Applications**: BEAM_SPLITTER is mainly used for deep learning and algorithm research, requiring precise spatial alignment. 2D affine transformation is sufficient to meet these needs

**Comparison with STEREO**:

| Feature | BEAM_SPLITTER | STEREO |
|---------|--------------|--------|
| Optical Configuration | Beam splitter shared optical path | Two cameras with independent perspectives |
| Transformation Type | 2D affine transformation | 3D rigid body transformation |
| Calibration Complexity | Simple | Complex |
| Main Parameters | Translation, rotation, scaling | 3D rotation matrix, 3D translation vector |
| Application Scenarios | Deep learning, algorithm research | Practical applications, multi-view |

---

## Complete Example

For a complete calibration example, please refer to:

`sample/user_examples/beam_splitter_align/beam_splitter_align.cpp`

This example includes:
- Real-time overlay display
- Keyboard interaction adjustment (arrow keys for translation, +/- for scaling)
- Recording functionality
- Metadata saving and loading

### Run the Example

```bash
./beam_splitter_align [metadata_path]
```

### Interactive Controls

- **Arrow Keys**: Adjust affine transformation translation
- **+**: Scale up affine transformation (2%)
- **-**: Scale down affine transformation (2%)
- **Space**: Start/Stop recording
- **q / ESC**: Exit

---

## FAQ

### Q: How to judge if calibration is accurate?

A: Observe whether events in the real-time overlay view align with corresponding features in the image. You can use a checkerboard or other obvious feature points for verification.

### Q: What should be the initial values for calibration parameters?

A: Usually start from the identity matrix (tx=0, ty=0, scale=1.0), then gradually optimize through interactive adjustment.

### Q: What is the difference between AffineTransform and RigidTransform?

A:
- **AffineTransform**: 2D transformation for BEAM_SPLITTER layout, includes rotation, scaling, and translation
- **RigidTransform**: 3D rigid body transformation for STEREO layout, only includes rotation and translation

### Q: How to choose between BEAM_SPLITTER and STEREO layouts?

A:
- **BEAM_SPLITTER**: Suitable for scenarios requiring precise spatial alignment, such as deep learning and algorithm research. Requires beam splitter hardware, simple calibration.
- **STEREO**: Suitable for practical applications and multi-view scenarios. Only needs two cameras and trigger cable, but requires complete stereo calibration.

---

## Related Resources

- [API Reference: calib_info.h](../../../include/utils/calib_info.h)
- [Example: beam_splitter_align](../../../sample/user_examples/beam_splitter_align/)
- [Tutorial: Camera Usage Tutorial](camera_usage_tutorial.md)