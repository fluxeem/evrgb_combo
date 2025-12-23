@page parameter_tuning_tutorial Parameter Tuning Tutorial

This tutorial introduces how to adjust various parameters of RGB and DVS cameras to achieve optimal image quality and performance.

## Table of Contents

- [RGB Camera Parameter Tuning](#rgb-camera-parameter-tuning)
- [DVS Camera Parameter Tuning](#dvs-camera-parameter-tuning)
- [Parameter Query and Verification](#parameter-query-and-verification)
- [Common Parameter Issues](#common-parameter-issues)

## RGB Camera Parameter Tuning

### Exposure Time Adjustment

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

void printStatus(const char* action, const evrgb::CameraStatus& status) {
    if (status.success()) {
        std::cout << action << " -> Success" << std::endl;
    } else {
        std::cout << action << " -> " << status.message
                  << " (error code=0x" << std::hex << std::uppercase << status.code << std::dec << ")" << std::endl;
    }
}

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) {
        std::cerr << "No RGB camera found" << std::endl;
        return 1;
    }

    evrgb::HikvisionRgbCamera camera;
    if (!camera.initialize(cameras[0].serial_number)) {
        std::cerr << "Camera initialization failed" << std::endl;
        return 1;
    }

    // 1. Disable auto exposure
    auto st_auto = camera.setEnumByName("ExposureAuto", "Off");
    printStatus("Set Auto Exposure=Off", st_auto);

    // 2. Set exposure mode to Timed
    auto st_mode = camera.setEnumByName("ExposureMode", "Timed");
    printStatus("Set Exposure Mode=Timed", st_mode);

    // 3. Query current exposure time
    evrgb::FloatProperty exposure{};
    auto st = camera.getFloat("ExposureTime", exposure);
    printStatus("Get Exposure Time", st);
    if (st.success()) {
        std::cout << "Current exposure time: " << exposure.value << " us"
                  << " (min=" << exposure.min << ", max=" << exposure.max
                  << ", step=" << exposure.inc << ")" << std::endl;
    }

    // 4. Set new exposure time (e.g., 10ms)
    double target_exposure_us = 10000.0;
    if (st.success()) {
        // Limit to camera supported range
        target_exposure_us = std::max(exposure.min, std::min(exposure.max, target_exposure_us));
        std::cout << "Setting target exposure time: " << target_exposure_us << " us" << std::endl;
    }
    
    st = camera.setFloat("ExposureTime", target_exposure_us);
    if (!st.success()) {
        // Some cameras might require integer type
        st = camera.setInt("ExposureTime", static_cast<int64_t>(target_exposure_us));
    }
    printStatus("Set Exposure Time", st);

    // 5. Verify setting result
    st = camera.getFloat("ExposureTime", exposure);
    if (st.success()) {
        std::cout << "Actual exposure time: " << exposure.value << " us" << std::endl;
    }

    camera.destroy();
    return 0;
}
```

### Gain Adjustment

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) return 1;

    evrgb::HikvisionRgbCamera camera;
    if (!camera.initialize(cameras[0].serial_number)) return 1;

    // Disable auto gain
    auto st = camera.setEnumByName("GainAuto", "Off");
    std::cout << "Set Auto Gain=Off: " << (st.success() ? "Success" : "Failed") << std::endl;

    // Query current gain
    evrgb::FloatProperty gain{};
    st = camera.getFloat("Gain", gain);
    if (st.success()) {
        std::cout << "Current gain: " << gain.value 
                  << " (range: " << gain.min << " - " << gain.max << ")" << std::endl;
        
        // Set gain to middle value
        double new_gain = (gain.min + gain.max) / 2.0;
        st = camera.setFloat("Gain", new_gain);
        std::cout << "Set gain=" << new_gain << ": " << (st.success() ? "Success" : "Failed") << std::endl;
        
        // Verify setting
        if (camera.getFloat("Gain", gain)) {
            std::cout << "Actual gain: " << gain.value << std::endl;
        }
    }

    camera.destroy();
    return 0;
}
```

### White Balance Adjustment

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) return 1;

    evrgb::HikvisionRgbCamera camera;
    if (!camera.initialize(cameras[0].serial_number)) return 1;

    // Set white balance mode
    auto st = camera.setEnumByName("BalanceWhiteAuto", "Once");
    std::cout << "Execute white balance once: " << (st.success() ? "Success" : "Failed") << std::endl;

    // Wait for white balance to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Query white balance parameters
    evrgb::FloatProperty rb_ratio{}, gb_ratio{};
    if (camera.getFloat("BalanceRatioRed", rb_ratio) && 
        camera.getFloat("BalanceRatioGreen", gb_ratio)) {
        std::cout << "Red/Green ratio: " << rb_ratio.value << std::endl;
        std::cout << "Green/Blue ratio: " << gb_ratio.value << std::endl;
    }

    // Manual white balance setting
    st = camera.setEnumByName("BalanceWhiteAuto", "Off");
    st = camera.setFloat("BalanceRatioRed", 1.2);
    st = camera.setFloat("BalanceRatioGreen", 1.0);
    st = camera.setFloat("BalanceRatioBlue", 1.8);
    std::cout << "Manual white balance setting: " << (st.success() ? "Success" : "Failed") << std::endl;

    camera.destroy();
    return 0;
}
```

### Image Format and Resolution

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) return 1;

    evrgb::HikvisionRgbCamera camera;
    if (!camera.initialize(cameras[0].serial_number)) return 1;

    // Query supported image formats
    evrgb::EnumProperty pixel_format{};
    if (camera.getEnum("PixelFormat", pixel_format)) {
        std::cout << "Current pixel format: " << pixel_format.value << std::endl;
        std::cout << "Supported formats: ";
        for (const auto& entry : pixel_format.entries) {
            std::cout << entry << " ";
        }
        std::cout << std::endl;
    }

    // Set pixel format
    auto st = camera.setEnumByName("PixelFormat", "BGR8");
    std::cout << "Set BGR8 format: " << (st.success() ? "Success" : "Failed") << std::endl;

    // Query and set resolution
    evrgb::IntProperty width{}, height{};
    if (camera.getInt("Width", width) && camera.getInt("Height", height)) {
        std::cout << "Current resolution: " << width.value << "x" << height.value << std::endl;
        
        // Set new resolution (if supported)
        st = camera.setInt("Width", 1280);
        st = camera.setInt("Height", 720);
        std::cout << "Set 1280x720 resolution: " << (st.success() ? "Success" : "Failed") << std::endl;
    }

    camera.destroy();
    return 0;
}
```

## DVS Camera Parameter Tuning

### Bias Parameter Adjustment

```cpp
#include "camera/dvs_camera.h"
#include <iostream>

int main() {
    auto cameras = evrgb::enumerateAllDvsCameras();
    if (cameras.empty()) {
        std::cerr << "No DVS camera found" << std::endl;
        return 1;
    }

    evrgb::DvsCamera camera;
    if (!camera.initialize(cameras[0].serial)) {
        std::cerr << "DVS camera initialization failed" << std::endl;
        return 1;
    }

    // Set bias parameters
    evrgb::CameraStatus status;
    
    // 1. Set sensitivity
    status = camera.setFloat("sensitivity", 0.5);
    std::cout << "Set sensitivity=0.5: " << (status.success() ? "Success" : "Failed") << std::endl;

    // 2. Set threshold
    status = camera.setFloat("threshold", 0.3);
    std::cout << "Set threshold=0.3: " << (status.success() ? "Success" : "Failed") << std::endl;

    // 3. Set refractory period
    status = camera.setFloat("refractory_period", 1.0);
    std::cout << "Set refractory period=1.0: " << (status.success() ? "Success" : "Failed") << std::endl;

    // 4. Set bandwidth
    status = camera.setFloat("bandwidth", 0.9);
    std::cout << "Set bandwidth=0.9: " << (status.success() ? "Success" : "Failed") << std::endl;

    camera.destroy();
    return 0;
}
```

### Event Filter Configuration

```cpp
#include "camera/dvs_camera.h"
#include <iostream>

int main() {
    auto cameras = evrgb::enumerateAllDvsCameras();
    if (cameras.empty()) return 1;

    evrgb::DvsCamera camera;
    if (!camera.initialize(cameras[0].serial)) return 1;

    // Set region filter
    auto status = camera.setBool("region_filter", true);
    std::cout << "Enable region filter: " << (status.success() ? "Success" : "Failed") << std::endl;

    // Set noise filter
    status = camera.setBool("noise_filter", true);
    std::cout << "Enable noise filter: " << (status.success() ? "Success" : "Failed") << std::endl;

    // Set ROI (Region of Interest)
    status = camera.setInt("roi_x", 100);
    status = camera.setInt("roi_y", 100);
    status = camera.setInt("roi_width", 300);
    status = camera.setInt("roi_height", 300);
    std::cout << "Set ROI: " << (status.success() ? "Success" : "Failed") << std::endl;

    camera.destroy();
    return 0;
}
```

## Parameter Query and Verification

### Generic Parameter Query Functions

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

void printFloatProperty(evrgb::HikvisionRgbCamera& camera, const std::string& name) {
    evrgb::FloatProperty prop;
    if (camera.getFloat(name, prop)) {
        std::cout << name << ": " << prop.value 
                  << " (range: " << prop.min << " - " << prop.max
                  << ", step: " << prop.inc << ")" << std::endl;
    } else {
        std::cout << name << ": Unable to get" << std::endl;
    }
}

void printIntProperty(evrgb::HikvisionRgbCamera& camera, const std::string& name) {
    evrgb::IntProperty prop;
    if (camera.getInt(name, prop)) {
        std::cout << name << ": " << prop.value 
                  << " (range: " << prop.min << " - " << prop.max
                  << ", step: " << prop.inc << ")" << std::endl;
    } else {
        std::cout << name << ": Unable to get" << std::endl;
    }
}

void printEnumProperty(evrgb::HikvisionRgbCamera& camera, const std::string& name) {
    evrgb::EnumProperty prop;
    if (camera.getEnum(name, prop)) {
        std::cout << name << ": " << prop.value << std::endl;
        std::cout << "  Available values: ";
        for (const auto& entry : prop.entries) {
            std::cout << entry << " ";
        }
        std::cout << std::endl;
    } else {
        std::cout << name << ": Unable to get" << std::endl;
    }
}

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) return 1;

    evrgb::HikvisionRgbCamera camera;
    if (!camera.initialize(cameras[0].serial_number)) return 1;

    std::cout << "=== RGB Camera Parameter Information ===" << std::endl;

    // Float parameters
    printFloatProperty(camera, "ExposureTime");
    printFloatProperty(camera, "Gain");
    printFloatProperty(camera, "AcquisitionFrameRate");

    // Integer parameters
    printIntProperty(camera, "Width");
    printIntProperty(camera, "Height");
    printIntProperty(camera, "PayloadSize");

    // Enum parameters
    printEnumProperty(camera, "PixelFormat");
    printEnumProperty(camera, "ExposureAuto");
    printEnumProperty(camera, "GainAuto");

    camera.destroy();
    return 0;
}
```

## Common Parameter Issues

### Q: Parameter setting failed

**Common causes**:
1. Parameter value out of range
2. Camera doesn't support the parameter
3. Camera is in error state
4. Insufficient permissions

**Solutions**:
1. Query parameter range before setting value
2. Check camera model supported features
3. Reinitialize camera
4. Ensure sufficient permissions

### Q: Unable to disable auto parameters

**Solution steps**:
1. Ensure camera is started
2. Disable auto parameters in correct order
3. Check if camera supports manual mode
4. Check camera documentation

### Q: No effect after parameter setting

**Possible causes**:
1. Parameter requires camera restart to take effect
2. Other parameters are interfering
3. Hardware limitations

**Solutions**:
1. Restart camera
2. Check related parameter settings
3. Review camera specifications

## Best Practices

### RGB Camera Parameter Optimization

1. **Exposure Time**: Adjust based on scene lighting, avoid overexposure and underexposure
2. **Gain**: Increase appropriately when exposure time is insufficient, but introduces noise
3. **White Balance**: Execute auto white balance once under stable lighting
4. **Frame Rate**: Set based on processing capability and requirements

### DVS Camera Parameter Optimization

1. **Sensitivity**: Adjust based on scene dynamics, avoid too many noise events
2. **Threshold**: Balance event count and information quality
3. **Filtering**: Use filtering functions appropriately to reduce noise
4. **Bandwidth**: Adjust event output rate based on application scenario

## Next Steps

After mastering parameter tuning, you can:

1. Learn advanced synchronization features
2. Understand data recording and playback
3. Explore performance optimization techniques
4. Check more application examples

## Related Documentation

- [Camera Enumeration Tutorial](ENUMERATION_TUTORIAL.md)
- [Camera Usage Tutorial](CAMERA_USAGE_TUTORIAL.md)
- [Recording Tutorial](RECORDING_TUTORIAL.md)

*/