@page parameter_tuning_tutorial 参数调整教程

本教程介绍如何调整 RGB 相机参数，以获得最佳图像质量与性能。

## 目录

- [RGB 相机曝光时间调整](#rgb-相机曝光时间调整)
- [参数查询和验证](#参数查询和验证)
- [常见参数问题](#常见参数问题)

## RGB 相机曝光时间调整

以下示例演示如何使用 Combo 接口调整 RGB 相机的曝光时间。此示例基于 `rgb_exposure_step_example` 示例程序。

```cpp
#include "core/combo.h"
#include <iostream>

// 辅助函数：打印操作状态
void printStatus(const char* action, const evrgb::CameraStatus& status) {
    if (status.success()) {
        std::cout << action << " -> 成功" << std::endl;
    } else {
        std::cout << action << " -> " << status.message
                  << " (错误码=0x" << std::hex << std::uppercase << status.code << std::dec << ")" << std::endl;
    }
}

int main() {
    // 使用 RGB 相机初始化 Combo
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();
    evrgb::Combo combo(rgb_cameras[0].serial_number, "");
    combo.init();
    
    auto rgb_camera = combo.getRgbCamera();
    
    // 1. 禁用自动曝光以进行手动控制
    rgb_camera->setEnumByName("ExposureAuto", "Off");
    rgb_camera->setEnumByName("ExposureMode", "Timed");
    
    // 2. 查询当前曝光时间范围
    evrgb::FloatProperty exposure{};
    rgb_camera->getFloat("ExposureTime", exposure);
    std::cout << "当前曝光: " << exposure.value << " us"
              << " (最小=" << exposure.min << ", 最大=" << exposure.max << ")" << std::endl;
    
    // 3. 设置新的曝光时间（限制在相机范围内）
    double target_exposure_us = 10000.0; // 10ms
    target_exposure_us = std::max(exposure.min, std::min(exposure.max, target_exposure_us));
    
    // 先尝试浮点数，如果失败则使用整数
    auto st = rgb_camera->setFloat("ExposureTime", target_exposure_us);
    if (!st.success()) {
        rgb_camera->setInt("ExposureTime", static_cast<int64_t>(target_exposure_us));
    }
    
    // 4. 验证设置
    rgb_camera->getFloat("ExposureTime", exposure);
    std::cout << "应用的曝光: " << exposure.value << " us" << std::endl;
    
    combo.destroy();
    return 0;
}
```

### 曝光时间调整要点

1. **禁用自动曝光**：在手动控制前将 `ExposureAuto` 设置为 "Off"
2. **设置曝光模式**：某些相机需要将 `ExposureMode` 设置为 "Timed"
3. **查询参数范围**：设置前始终检查最小/最大值
4. **处理不同类型**：某些相机将 `ExposureTime` 暴露为浮点数，其他为整数
5. **验证设置**：读取值以确认已正确应用

## 参数查询和验证

### 查询参数信息

```cpp
// 查询曝光时间范围和当前值
evrgb::FloatProperty exposure{};
auto st = rgb_camera->getFloat("ExposureTime", exposure);
if (st.success()) {
    std::cout << "当前曝光: " << exposure.value << " us"
              << " (最小=" << exposure.min << ", 最大=" << exposure.max
              << ", 增量=" << exposure.inc << ")" << std::endl;
}
```

### 其他参数

对于其他参数，如增益、白平衡、像素格式、分辨率和 DVS 相机参数，请参考您特定相机型号的制造商文档。EvRGB Combo SDK 提供通用接口来访问这些参数，但确切的参数名称和支持的值因相机型号而异。

## 常见参数问题

### Q: 参数设置失败

**常见原因**：
1. 参数值超出范围
2. 相机不支持该参数
3. 相机处于错误状态
4. 权限不足
5. 需要先禁用自动模式

**解决方法**：
1. 设置值前查询参数范围
2. 检查相机型号支持的功能
3. 重新初始化相机
4. 确保有足够的权限
5. 在手动控制前禁用自动参数

### Q: 无法禁用自动曝光

**解决步骤**：
1. 确保相机已初始化
2. 将 `ExposureAuto` 设置为 "Off"
3. 如需要，将 `ExposureMode` 设置为 "Timed"
4. 查看相机文档以了解型号特定要求

### Q: 参数设置后没有效果

**可能原因**：
1. 参数需要相机重启才能生效
2. 其他参数相互干扰
3. 硬件限制
4. 参数类型错误（浮点数 vs 整数）

**解决方法**：
1. 重启相机
2. 检查相关参数设置
3. 查看相机规格
4. 尝试浮点数和整数两种类型

## 最佳实践

### RGB 相机曝光优化

1. **场景光照**：根据场景光照条件调整曝光
2. **避免过曝/欠曝**：找到最佳曝光范围
3. **帧率考虑**：确保曝光时间不超过帧周期
4. **步进调整**：使用参数的增量值进行平滑调整

### 通用参数处理

1. **始终先查询**：设置前检查参数范围
2. **设置后验证**：读取值以确认已应用
3. **优雅处理错误**：检查返回状态并处理失败
4. **查阅文档**：参考制造商文档了解型号特定细节

## 下一步

掌握曝光时间调整后，你可以：

1. 学习高级同步特性
2. 了解数据录制与回放
3. 探索性能优化技巧
4. 查看更多应用示例

## 相关文档

- [设备枚举教程](ENUMERATION_TUTORIAL.md)
- [相机使用教程](CAMERA_USAGE_TUTORIAL.md)
- [录制教程](RECORDING_TUTORIAL.md)

*/