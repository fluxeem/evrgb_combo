@page enumeration_tutorial Camera Enumeration Tutorial

This tutorial introduces how to use EvRGB Combo SDK to enumerate available RGB and DVS camera devices in your system.

## Overview

Before using the SDK, you need to discover which camera devices are connected to your system. The SDK provides enumeration functions to get detailed information about all available devices.

## Enumeration Example Code

### Basic Enumeration Example

```cpp
#include "core/combo.h"
#include <iostream>

int main() {
    std::cout << "EvRGB Combo SDK - Camera Enumeration Example" << std::endl;
    
    // Enumerate all camera devices
    auto [rgb_camera_infos, dvs_camera_infos] = evrgb::enumerateAllCameras();

    // Display RGB camera information
    std::cout << "Found " << rgb_camera_infos.size() << " RGB cameras:" << std::endl;
    for (size_t i = 0; i < rgb_camera_infos.size(); ++i) {
        const auto& info = rgb_camera_infos[i];
        std::cout << "  Camera " << i << ":" << std::endl;
        std::cout << "    Manufacturer: " << info.manufacturer << std::endl;
        std::cout << "    Serial Number: " << info.serial_number << std::endl;
        std::cout << "    Resolution: " << info.width << "x" << info.height << std::endl;
        std::cout << "    Model: " << info.model_name << std::endl;
    }   
   
    // Display DVS camera information
    std::cout << "Found " << dvs_camera_infos.size() << " DVS cameras:" << std::endl;
    for (size_t i = 0; i < dvs_camera_infos.size(); ++i) {
        const auto& info = dvs_camera_infos[i];
        std::cout << "  DVS " << i << ":" << std::endl;
        std::cout << "    Manufacturer: " << info.manufacturer << std::endl;
        std::cout << "    Serial Number: " << info.serial << std::endl;
        std::cout << "    Model: " << info.model_name << std::endl;
    }

    return 0;
}
```

### Separate Enumeration Example

If you only need to enumerate specific types of cameras, you can use separate enumeration functions:

```cpp
#include "core/combo.h"
#include <iostream>

int main() {
    // Enumerate only RGB cameras
    auto rgb_cameras = evrgb::enumerateAllRgbCameras();
    std::cout << "RGB cameras count: " << rgb_cameras.size() << std::endl;
    
    // Enumerate only DVS cameras
    auto dvs_cameras = evrgb::enumerateAllDvsCameras();
    std::cout << "DVS cameras count: " << dvs_cameras.size() << std::endl;

    return 0;
}
```

## Build and Run

### Build the Example

```bash
# Create build directory
mkdir -p build
cd build

# Configure CMake (enable sample programs)
cmake .. -DBUILD_SAMPLES=ON

# Compile enumeration example
cmake --build . --target camera_enum_example --config Release
```

### Run the Example

```bash
# Linux/macOS
./bin/camera_enum_example

# Windows
.\bin\Release\camera_enum_example.exe
```

## Expected Output

When successfully run, you should see output similar to:

```
EvRGB Combo SDK - Camera Enumeration Example
Found 1 RGB cameras:
  Camera 0:
    Manufacturer: Hikvision
    Serial Number: HK123456789
    Resolution: 1920x1080
    Model: MV-CE200-10UC
Found 1 DVS cameras:
  DVS 0:
    Manufacturer: Dvsense
    Serial Number: DVS001
    Model: DVXplorer
```

## Device Information Structures

### RGB Camera Information

The `evrgb::RgbCameraInfo` structure contains the following fields:

- `manufacturer`: Manufacturer name (e.g., "Hikvision")
- `model_name`: Camera model
- `serial_number`: Camera serial number (used for initialization)
- `width`: Image width (pixels)
- `height`: Image height (pixels)

### DVS Camera Information

The `evrgb::DvsCameraInfo` structure contains the following fields:

- `manufacturer`: Manufacturer name (e.g., "Dvsense")
- `model_name`: Camera model
- `serial`: Camera serial number (used for initialization)

## Common Issues

### Q: No cameras detected

**Possible causes**:
1. Camera not properly connected
2. Driver not installed
3. Insufficient permissions (may need sudo on Linux)
4. Firewall blocking (network cameras)

**Solutions**:
1. Check physical connections
2. Reinstall camera drivers
3. Use sudo or configure udev rules on Linux
4. Check firewall settings

### Q: Enumeration returns empty list

**Troubleshooting steps**:
1. Verify camera drivers are properly installed
2. Check camera power and connections
3. Confirm SDK dependencies are correctly installed
4. Check SDK log output

### Q: Incorrect serial numbers

**Notes**:
- RGB cameras use the `serial_number` field
- DVS cameras use the `serial` field
- Serial numbers are case-sensitive

## Next Steps

After successfully enumerating cameras, you can:

1. Select specific cameras for initialization
2. Learn camera usage tutorials for camera operations
3. Check parameter tuning tutorials for camera configuration
4. Study data recording and playback features

## Related Documentation

- [Camera Usage Tutorial](CAMERA_USAGE_TUTORIAL.md)
- [Parameter Tuning Tutorial](PARAMETER_TUNING_TUTORIAL.md)
- [Installation Guide](INSTALLATION.md)

*/