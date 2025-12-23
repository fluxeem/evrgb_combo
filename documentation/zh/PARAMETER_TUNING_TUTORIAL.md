@page parameter_tuning_tutorial 参数调整教程

本教程介绍如何调整 RGB 与 DVS 相机参数，以获得最佳图像质量与性能。

## 目录

- [RGB 相机参数调整](#rgb-相机参数调整)
- [DVS 相机参数调整](#dvs-相机参数调整)
- [参数查询和验证](#参数查询和验证)
- [常见参数问题](#常见参数问题)

## RGB 相机参数调整

### 曝光时间调整

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

void printStatus(const char* action, const evrgb::CameraStatus& status) {
    if (status.success()) {
        std::cout << action << " -> 成功" << std::endl;
    } else {
        std::cout << action << " -> " << status.message
                  << " (错误码=0x" << std::hex << std::uppercase << status.code << std::dec << ")" << std::endl;
    }
}

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) {
        std::cerr << "未找到 RGB 相机" << std::endl;
        return 1;
    }

    evrgb::HikvisionRgbCamera camera;
    if (!camera.initialize(cameras[0].serial_number)) {
        std::cerr << "相机初始化失败" << std::endl;
        return 1;
    }

    // 1. 禁用自动曝光
    auto st_auto = camera.setEnumByName("ExposureAuto", "Off");
    printStatus("设置自动曝光=关闭", st_auto);

    // 2. 设置曝光模式为定时
    auto st_mode = camera.setEnumByName("ExposureMode", "Timed");
    printStatus("设置曝光模式=定时", st_mode);

    // 3. 查询当前曝光时间
    evrgb::FloatProperty exposure{};
    auto st = camera.getFloat("ExposureTime", exposure);
    printStatus("获取曝光时间", st);
    if (st.success()) {
        std::cout << "当前曝光时间: " << exposure.value << " us"
                  << " (最小=" << exposure.min << ", 最大=" << exposure.max
                  << ", 步长=" << exposure.inc << ")" << std::endl;
    }

    // 4. 设置新的曝光时间（例如 10ms）
    double target_exposure_us = 10000.0;
    if (st.success()) {
        // 限制在相机支持范围内
        target_exposure_us = std::max(exposure.min, std::min(exposure.max, target_exposure_us));
        std::cout << "设置目标曝光时间: " << target_exposure_us << " us" << std::endl;
    }
    
    st = camera.setFloat("ExposureTime", target_exposure_us);
    if (!st.success()) {
        // 某些相机可能需要使用整数类型
        st = camera.setInt("ExposureTime", static_cast<int64_t>(target_exposure_us));
    }
    printStatus("设置曝光时间", st);

    // 5. 验证设置结果
    st = camera.getFloat("ExposureTime", exposure);
    if (st.success()) {
        std::cout << "实际曝光时间: " << exposure.value << " us" << std::endl;
    }

    camera.destroy();
    return 0;
}
```

### 增益调整

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) return 1;

    evrgb::HikvisionRgbCamera camera;
    if (!camera.initialize(cameras[0].serial_number)) return 1;

    // 禁用自动增益
    auto st = camera.setEnumByName("GainAuto", "Off");
    std::cout << "设置自动增益=关闭: " << (st.success() ? "成功" : "失败") << std::endl;

    // 查询当前增益
    evrgb::FloatProperty gain{};
    st = camera.getFloat("Gain", gain);
    if (st.success()) {
        std::cout << "当前增益: " << gain.value 
                  << " (范围: " << gain.min << " - " << gain.max << ")" << std::endl;
        
        // 设置增益为中间值
        double new_gain = (gain.min + gain.max) / 2.0;
        st = camera.setFloat("Gain", new_gain);
        std::cout << "设置增益=" << new_gain << ": " << (st.success() ? "成功" : "失败") << std::endl;
        
        // 验证设置
        if (camera.getFloat("Gain", gain)) {
            std::cout << "实际增益: " << gain.value << std::endl;
        }
    }

    camera.destroy();
    return 0;
}
```

### 白平衡调整

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) return 1;

    evrgb::HikvisionRgbCamera camera;
    if (!camera.initialize(cameras[0].serial_number)) return 1;

    // 设置白平衡模式
    auto st = camera.setEnumByName("BalanceWhiteAuto", "Once");
    std::cout << "执行一次白平衡: " << (st.success() ? "成功" : "失败") << std::endl;

    // 等待白平衡完成
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 查询白平衡参数
    evrgb::FloatProperty rb_ratio{}, gb_ratio{};
    if (camera.getFloat("BalanceRatioRed", rb_ratio) && 
        camera.getFloat("BalanceRatioGreen", gb_ratio)) {
        std::cout << "红/绿比: " << rb_ratio.value << std::endl;
        std::cout << "绿/蓝比: " << gb_ratio.value << std::endl;
    }

    // 手动设置白平衡
    st = camera.setEnumByName("BalanceWhiteAuto", "Off");
    st = camera.setFloat("BalanceRatioRed", 1.2);
    st = camera.setFloat("BalanceRatioGreen", 1.0);
    st = camera.setFloat("BalanceRatioBlue", 1.8);
    std::cout << "手动白平衡设置: " << (st.success() ? "成功" : "失败") << std::endl;

    camera.destroy();
    return 0;
}
```

### 图像格式和分辨率

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) return 1;

    evrgb::HikvisionRgbCamera camera;
    if (!camera.initialize(cameras[0].serial_number)) return 1;

    // 查询支持的图像格式
    evrgb::EnumProperty pixel_format{};
    if (camera.getEnum("PixelFormat", pixel_format)) {
        std::cout << "当前像素格式: " << pixel_format.value << std::endl;
        std::cout << "支持的格式: ";
        for (const auto& entry : pixel_format.entries) {
            std::cout << entry << " ";
        }
        std::cout << std::endl;
    }

    // 设置像素格式
    auto st = camera.setEnumByName("PixelFormat", "BGR8");
    std::cout << "设置 BGR8 格式: " << (st.success() ? "成功" : "失败") << std::endl;

    // 查询和设置分辨率
    evrgb::IntProperty width{}, height{};
    if (camera.getInt("Width", width) && camera.getInt("Height", height)) {
        std::cout << "当前分辨率: " << width.value << "x" << height.value << std::endl;
        
        // 设置新分辨率（如果支持）
        st = camera.setInt("Width", 1280);
        st = camera.setInt("Height", 720);
        std::cout << "设置 1280x720 分辨率: " << (st.success() ? "成功" : "失败") << std::endl;
    }

    camera.destroy();
    return 0;
}
```

## DVS 相机参数调整

### 偏置参数调整

```cpp
#include "camera/dvs_camera.h"
#include <iostream>

int main() {
    auto cameras = evrgb::enumerateAllDvsCameras();
    if (cameras.empty()) {
        std::cerr << "未找到 DVS 相机" << std::endl;
        return 1;
    }

    evrgb::DvsCamera camera;
    if (!camera.initialize(cameras[0].serial)) {
        std::cerr << "DVS 相机初始化失败" << std::endl;
        return 1;
    }

    // 设置偏置参数
    evrgb::CameraStatus status;
    
    // 1. 设置灵敏度
    status = camera.setFloat("sensitivity", 0.5);
    std::cout << "设置灵敏度=0.5: " << (status.success() ? "成功" : "失败") << std::endl;

    // 2. 设置阈值
    status = camera.setFloat("threshold", 0.3);
    std::cout << "设置阈值=0.3: " << (status.success() ? "成功" : "失败") << std::endl;

    // 3. 设置 refractory period
    status = camera.setFloat("refractory_period", 1.0);
    std::cout << "设置不应期=1.0: " << (status.success() ? "成功" : "失败") << std::endl;

    // 4. 设置带宽
    status = camera.setFloat("bandwidth", 0.9);
    std::cout << "设置带宽=0.9: " << (status.success() ? "成功" : "失败") << std::endl;

    camera.destroy();
    return 0;
}
```

### 事件过滤配置

```cpp
#include "camera/dvs_camera.h"
#include <iostream>

int main() {
    auto cameras = evrgb::enumerateAllDvsCameras();
    if (cameras.empty()) return 1;

    evrgb::DvsCamera camera;
    if (!camera.initialize(cameras[0].serial)) return 1;

    // 设置区域过滤
    auto status = camera.setBool("region_filter", true);
    std::cout << "启用区域过滤: " << (status.success() ? "成功" : "失败") << std::endl;

    // 设置噪声过滤
    status = camera.setBool("noise_filter", true);
    std::cout << "启用噪声过滤: " << (status.success() ? "成功" : "失败") << std::endl;

    // 设置 ROI（感兴趣区域）
    status = camera.setInt("roi_x", 100);
    status = camera.setInt("roi_y", 100);
    status = camera.setInt("roi_width", 300);
    status = camera.setInt("roi_height", 300);
    std::cout << "设置 ROI: " << (status.success() ? "成功" : "失败") << std::endl;

    camera.destroy();
    return 0;
}
```

## 参数查询和验证

### 通用参数查询函数

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

void printFloatProperty(evrgb::HikvisionRgbCamera& camera, const std::string& name) {
    evrgb::FloatProperty prop;
    if (camera.getFloat(name, prop)) {
        std::cout << name << ": " << prop.value 
                  << " (范围: " << prop.min << " - " << prop.max
                  << ", 步长: " << prop.inc << ")" << std::endl;
    } else {
        std::cout << name << ": 无法获取" << std::endl;
    }
}

void printIntProperty(evrgb::HikvisionRgbCamera& camera, const std::string& name) {
    evrgb::IntProperty prop;
    if (camera.getInt(name, prop)) {
        std::cout << name << ": " << prop.value 
                  << " (范围: " << prop.min << " - " << prop.max
                  << ", 步长: " << prop.inc << ")" << std::endl;
    } else {
        std::cout << name << ": 无法获取" << std::endl;
    }
}

void printEnumProperty(evrgb::HikvisionRgbCamera& camera, const std::string& name) {
    evrgb::EnumProperty prop;
    if (camera.getEnum(name, prop)) {
        std::cout << name << ": " << prop.value << std::endl;
        std::cout << "  可选值: ";
        for (const auto& entry : prop.entries) {
            std::cout << entry << " ";
        }
        std::cout << std::endl;
    } else {
        std::cout << name << ": 无法获取" << std::endl;
    }
}

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) return 1;

    evrgb::HikvisionRgbCamera camera;
    if (!camera.initialize(cameras[0].serial_number)) return 1;

    std::cout << "=== RGB 相机参数信息 ===" << std::endl;

    // 浮点参数
    printFloatProperty(camera, "ExposureTime");
    printFloatProperty(camera, "Gain");
    printFloatProperty(camera, "AcquisitionFrameRate");

    // 整数参数
    printIntProperty(camera, "Width");
    printIntProperty(camera, "Height");
    printIntProperty(camera, "PayloadSize");

    // 枚举参数
    printEnumProperty(camera, "PixelFormat");
    printEnumProperty(camera, "ExposureAuto");
    printEnumProperty(camera, "GainAuto");

    camera.destroy();
    return 0;
}
```

## 常见参数问题

### Q: 参数设置失败

**常见原因**：
1. 参数值超出范围
2. 相机不支持该参数
3. 相机处于错误状态
4. 权限不足

**解决方法**：
1. 先查询参数范围，再设置值
2. 检查相机型号支持的功能
3. 重新初始化相机
4. 确保有足够的权限

### Q: 自动参数无法禁用

**解决步骤**：
1. 确保相机已启动
2. 按正确顺序禁用自动参数
3. 检查相机是否支持手动模式
4. 查看相机文档

### Q: 参数设置后没有效果

**可能原因**：
1. 参数需要相机重启才能生效
2. 其他参数相互影响
3. 硬件限制

**解决方法**：
1. 重启相机
2. 检查相关参数设置
3. 查看相机规格说明

## 最佳实践

### RGB 相机参数优化

1. **曝光时间**：根据场景光照调整，避免过曝与欠曝
2. **增益**：在曝光不足时适度提高，但会带来噪声
3. **白平衡**：稳定光源下执行一次自动白平衡
4. **帧率**：依据算力与需求设定

### DVS 相机参数优化

1. **灵敏度**：依场景动态调整，避免噪声事件过多
2. **阈值**：平衡事件数量与信息质量
3. **过滤**：合理使用滤波降低噪声
4. **带宽**：按应用场景调整事件输出速率

## 下一步

掌握参数调整后，你可以：

1. 学习高级同步特性
2. 了解数据录制与回放
3. 探索性能优化技巧
4. 查看更多应用示例

## 相关文档

- [设备枚举教程](ENUMERATION_TUTORIAL.md)
- [相机使用教程](CAMERA_USAGE_TUTORIAL.md)
- [录制教程](RECORDING_TUTORIAL.md)

*/