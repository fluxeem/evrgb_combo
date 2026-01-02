@page parameter_tuning_tutorial Parameter Tuning Tutorial

This tutorial introduces how to adjust RGB camera parameters to achieve optimal image quality and performance.

## Table of Contents

- [RGB Camera Exposure Time Adjustment](#rgb-camera-exposure-time-adjustment)
- [Parameter Query and Verification](#parameter-query-and-verification)
- [Common Parameter Issues](#common-parameter-issues)

## RGB Camera Exposure Time Adjustment

The following example demonstrates how to adjust the exposure time of an RGB camera using the Combo interface. This is based on the `rgb_exposure_step_example` sample program.

```cpp
#include "core/combo.h"
#include <iostream>

// Helper function to print operation status
void printStatus(const char* action, const evrgb::CameraStatus& status) {
    if (status.success()) {
        std::cout << action << " -> OK" << std::endl;
    } else {
        std::cout << action << " -> " << status.message
                  << " (code=0x" << std::hex << std::uppercase << status.code << std::dec << ")" << std::endl;
    }
}

int main() {
    // Initialize Combo with RGB camera
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();
    evrgb::Combo combo(rgb_cameras[0].serial_number, "");
    combo.init();
    
    auto rgb_camera = combo.getRgbCamera();
    
    // 1. Disable auto exposure for manual control
    rgb_camera->setEnumByName("ExposureAuto", "Off");
    rgb_camera->setEnumByName("ExposureMode", "Timed");
    
    // 2. Query current exposure time range
    evrgb::FloatProperty exposure{};
    rgb_camera->getFloat("ExposureTime", exposure);
    std::cout << "Current exposure: " << exposure.value << " us"
              << " (min=" << exposure.min << ", max=" << exposure.max << ")" << std::endl;
    
    // 3. Set new exposure time (clamp to camera limits)
    double target_exposure_us = 10000.0; // 10ms
    target_exposure_us = std::max(exposure.min, std::min(exposure.max, target_exposure_us));
    
    // Try float first, fallback to integer if needed
    auto st = rgb_camera->setFloat("ExposureTime", target_exposure_us);
    if (!st.success()) {
        rgb_camera->setInt("ExposureTime", static_cast<int64_t>(target_exposure_us));
    }
    
    // 4. Verify the setting
    rgb_camera->getFloat("ExposureTime", exposure);
    std::cout << "Applied exposure: " << exposure.value << " us" << std::endl;
    
    combo.destroy();
    return 0;
}
```

### Key Points for Exposure Time Adjustment

1. **Disable Auto Exposure**: Set `ExposureAuto` to "Off" before manual control
2. **Set Exposure Mode**: Some cameras require `ExposureMode` to be "Timed"
3. **Query Parameter Range**: Always check min/max values before setting
4. **Handle Different Types**: Some cameras expose `ExposureTime` as float, others as integer
5. **Verify Settings**: Read back the value to confirm it was applied correctly

## Parameter Query and Verification

### Querying Parameter Information

```cpp
// Query exposure time range and current value
evrgb::FloatProperty exposure{};
auto st = rgb_camera->getFloat("ExposureTime", exposure);
if (st.success()) {
    std::cout << "Current exposure: " << exposure.value << " us"
              << " (min=" << exposure.min << ", max=" << exposure.max
              << ", inc=" << exposure.inc << ")" << std::endl;
}
```

### Other Parameters

For other parameters such as gain, white balance, pixel format, resolution, and DVS camera parameters, please refer to the manufacturer's documentation for your specific camera model. The EvRGB Combo SDK provides generic interfaces to access these parameters, but the exact parameter names and supported values vary by camera model.

## Common Parameter Issues

### Q: Parameter setting failed

**Common causes**:
1. Parameter value out of range
2. Camera doesn't support the parameter
3. Camera is in error state
4. Insufficient permissions
5. Auto mode needs to be disabled first

**Solutions**:
1. Query parameter range before setting value
2. Check camera model supported features
3. Reinitialize camera
4. Ensure sufficient permissions
5. Disable auto parameters before manual control

### Q: Unable to disable auto exposure

**Solution steps**:
1. Ensure camera is initialized
2. Set `ExposureAuto` to "Off"
3. Set `ExposureMode` to "Timed" if required
4. Check camera documentation for model-specific requirements

### Q: No effect after parameter setting

**Possible causes**:
1. Parameter requires camera restart to take effect
2. Other parameters are interfering
3. Hardware limitations
4. Wrong parameter type (float vs integer)

**Solutions**:
1. Restart camera
2. Check related parameter settings
3. Review camera specifications
4. Try both float and integer types

## Best Practices

### RGB Camera Exposure Optimization

1. **Scene Lighting**: Adjust exposure based on scene lighting conditions
2. **Avoid Over/Under Exposure**: Find the optimal exposure range
3. **Frame Rate Consideration**: Ensure exposure time doesn't exceed frame period
4. **Step Adjustment**: Use the parameter's increment value for smooth adjustments

### General Parameter Handling

1. **Always Query First**: Check parameter range before setting
2. **Verify After Setting**: Read back to confirm the value was applied
3. **Handle Errors Gracefully**: Check return status and handle failures
4. **Consult Documentation**: Refer to manufacturer docs for model-specific details

## Next Steps

After mastering exposure time adjustment, you can:

1. Learn advanced synchronization features
2. Understand data recording and playback
3. Explore performance optimization techniques
4. Check more application examples

## Related Documentation

- [Camera Enumeration Tutorial](ENUMERATION_TUTORIAL.md)
- [Camera Usage Tutorial](CAMERA_USAGE_TUTORIAL.md)
- [Recording Tutorial](RECORDING_TUTORIAL.md)

*/