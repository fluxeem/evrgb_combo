@page camera_usage_tutorial Combo Usage Tutorial

This tutorial introduces how to use EvRGB Combo SDK to operate RGB+DVS combo cameras. This project recommends using the Combo interface to leverage the built-in time synchronization mechanism, achieving precise synchronization between RGB images and DVS events.

## Table of Contents

- [Combo Basic Operations](#combo-basic-operations)
- [Setting Synced Callback](#setting-synced-callback)
- [Separate Callbacks](#separate-callbacks)
- [Error Handling](#error-handling)
- [Common Issues](#common-issues)

## Combo Basic Operations

### Combo Arrangement Modes

EvRGB Combo SDK supports two RGB+DVS arrangement modes, each suitable for different application scenarios:

#### 1. STEREO Mode
- **Definition**: RGB camera and DVS camera are placed side-by-side independently, synchronized through external trigger signals
- **Features**:
  - Cameras are physically separated with independent fields of view
  - Requires external trigger line connection for precise synchronization
  - Suitable for scenarios requiring observation of two different perspectives
  - Requires calibration to determine spatial relationship between cameras
- **Use Cases**:
  - Stereo vision systems
  - Applications requiring independent control of RGB and DVS fields of view
  - Large field-of-view coverage scenarios
  - Experimental environments requiring flexible camera positioning

#### 2. BEAM_SPLITTER Mode
- **Definition**: RGB camera and DVS camera share the same optical path through a beam splitter, achieving perfect spatial alignment
- **Features**:
  - RGB and DVS share the same field of view and optical path
  - Automatic spatial alignment without complex calibration
  - Light is split to both cameras through a beam splitter
  - Ensures pixel-level spatial correspondence
- **Use Cases**:
  - Deep learning applications requiring pixel-level spatial alignment
  - Event camera and traditional camera fusion research
  - Applications requiring precise alignment of RGB images and event data
  - Computer vision algorithm development and testing

#### How to Choose Arrangement Mode
Consider the following factors when choosing arrangement mode:
- **Spatial Alignment Requirements**: BEAM_SPLITTER for perfect alignment, STEREO for flexible perspectives
- **Hardware Configuration**: BEAM_SPLITTER requires beam splitter hardware, STEREO only needs two cameras and trigger cable
- **Application Scenarios**: BEAM_SPLITTER recommended for deep learning and algorithm research, STEREO for practical applications
- **Calibration Complexity**: BEAM_SPLITTER has simple calibration, STEREO requires complete stereo calibration

### Basic Combo Operations

```cpp
#include "core/combo.h"
#include <iostream>

int main() {
    // Enumerate cameras
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();

    if (rgb_cameras.empty()) {
        std::cerr << "No RGB camera found!" << std::endl;
        return -1;
    }
    if (dvs_cameras.empty()) {
        std::cerr << "No DVS camera found!" << std::endl;
        return -1;
    }

    // Create Combo object
    std::string rgb_serial = rgb_cameras[0].serial_number;
    std::string dvs_serial = dvs_cameras[0].serial;

    // Specify arrangement mode (default is STEREO)
    // evrgb::ComboArrangement::STEREO - Stereo mode (cameras side-by-side)
    // evrgb::ComboArrangement::BEAM_SPLITTER - Beam splitter mode (shared optical path)
    evrgb::Combo combo(rgb_serial, dvs_serial, evrgb::ComboArrangement::STEREO);

    // Or use beam splitter mode
    // evrgb::Combo combo(rgb_serial, dvs_serial, evrgb::ComboArrangement::BEAM_SPLITTER);

    // Get current arrangement mode
    evrgb::ComboArrangement arrangement = combo.getArrangement();
    std::cout << "Arrangement mode: " << evrgb::toString(arrangement) << std::endl;

    std::cout << "Using RGB: " << rgb_serial << std::endl;
    std::cout << "Using DVS: " << dvs_serial << std::endl;

    // Set synced callback
    combo.setSyncedCallback([](const evrgb::RgbImageWithTimestamp& rgb, 
                               const std::vector<dvsense::Event2D>& events) {
        std::cout << "Synced data: RGB time " << rgb.timestamp_us 
                  << " us, event count " << events.size() << std::endl;
        
        // Process synced RGB image and DVS events
        if (!rgb.image.empty()) {
            std::cout << "  RGB image size: " << rgb.image.cols 
                      << "x" << rgb.image.rows << std::endl;
        }
        
        if (!events.empty()) {
            // Analyze event time range
            auto min_time = std::min_element(events.begin(), events.end(),
                [](const auto& a, const auto& b) { return a.timestamp < b.timestamp; });
            auto max_time = std::max_element(events.begin(), events.end(),
                [](const auto& a, const auto& b) { return a.timestamp < b.timestamp; });
            
            if (min_time != events.end() && max_time != events.end()) {
                std::cout << "  Event time range: [" << min_time->timestamp 
                          << ", " << max_time->timestamp << "] us" << std::endl;
            }
        }
    });

    // Initialize and start
    if (!combo.init()) {
        std::cerr << "Combo initialization failed" << std::endl;
        return -1;
    }

    if (!combo.start()) {
        std::cerr << "Combo start failed" << std::endl;
        return -1;
    }

    std::cout << "Combo started, capturing synced data..." << std::endl;

    // Run for 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Stop
    combo.stop();
    std::cout << "Combo stopped" << std::endl;
    
    return 0;
}
```

### Separate Callbacks

```cpp
#include "core/combo.h"
#include <iostream>

int main() {
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();
    if (rgb_cameras.empty() || dvs_cameras.empty()) return -1;

    // Create Combo object (default uses STEREO mode)
    evrgb::Combo combo(rgb_cameras[0].serial_number, dvs_cameras[0].serial);

    // If you need to use beam splitter mode, specify:
    // evrgb::Combo combo(rgb_cameras[0].serial_number, dvs_cameras[0].serial,
    //                     evrgb::ComboArrangement::BEAM_SPLITTER);

    // Set RGB image callback
    combo.setRgbImageCallback([](const evrgb::RgbImageWithTimestamp& rgb) {
        std::cout << "RGB image: time " << rgb.timestamp_us 
                  << " us, size " << rgb.image.cols << "x" << rgb.image.rows << std::endl;
    });

    // Set DVS event callback
    combo.setDvsEventCallback([](const std::vector<dvsense::Event2D>& events) {
        if (!events.empty()) {
            std::cout << "DVS events: count " << events.size() << std::endl;
        }
    });

    if (!combo.init() || !combo.start()) return -1;

    // Run for 5 seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));

    combo.stop();
    return 0;
}
```

## Metadata Saving and Loading

### Metadata Overview

EvRGB Combo SDK provides metadata management functionality for saving and loading camera configurations, calibration information, and other critical data. Main use cases include:

- **Calibration persistence**: Save calibration results to avoid repeated calibration
- **Configuration management**: Record parameter settings for RGB and DVS cameras
- **Data recording association**: Save corresponding metadata when recording data to ensure data traceability

### Metadata Data Structures

#### ComboMetadata (Combo Metadata)

```cpp
struct ComboMetadata {
    CameraMetadata rgb;                    // RGB camera metadata
    CameraMetadata dvs;                    // DVS camera metadata
    ComboArrangement arrangement;          // Arrangement mode (STEREO or BEAM_SPLITTER)
    ComboCalibrationInfo calibration;      // Calibration information
};
```

#### CameraMetadata (Single Camera Metadata)

```cpp
struct CameraMetadata {
    std::string manufacturer;              // Manufacturer
    std::string model;                     // Camera model
    std::string serial;                    // Serial number
    unsigned int width = 0;                // Image width
    unsigned int height = 0;               // Image height
    std::optional<CameraIntrinsics> intrinsics;  // Camera intrinsics (optional)
};
```

### Metadata Operations

#### Getting Metadata

```cpp
// Get all metadata for the current Combo
evrgb::ComboMetadata metadata = combo.getMetadata();

// Convert to JSON (requires nlohmann/json.hpp)
nlohmann::json j = metadata;
std::cout << j.dump(2) << std::endl;
```

#### Saving Metadata

```cpp
// Save metadata to a JSON file
std::string metadata_path = "combo_metadata.json";
std::string error_message;

if (combo.saveMetadata(metadata_path, &error_message)) {
    std::cout << "Metadata saved successfully: " << metadata_path << std::endl;
} else {
    std::cerr << "Failed to save metadata: " << error_message << std::endl;
}
```

#### Loading Metadata

```cpp
// Load metadata from a JSON file and apply it to the Combo
std::string metadata_path = "combo_metadata.json";
std::string error_message;

if (combo.loadMetadata(metadata_path, &error_message)) {
    std::cout << "Metadata loaded successfully" << std::endl;
    
    // Get loaded metadata
    evrgb::ComboMetadata metadata = combo.getMetadata();
} else {
    std::cerr << "Failed to load metadata: " << error_message << std::endl;
}
```

#### Applying Metadata

```cpp
// Manually apply custom metadata
evrgb::ComboMetadata metadata;
// ... Set metadata content

if (combo.applyMetadata(metadata, true)) {
    std::cout << "Metadata applied successfully" << std::endl;
}
```

### Usage Examples

**Calibration persistence**:

```cpp
// Save after calibration
combo.calibration_info = affine_transform;
combo.saveMetadata("calibration.json", nullptr);

// Load on startup
combo.loadMetadata("calibration.json", nullptr);
```

**Data recording association**:

```cpp
// Save metadata to recording directory
std::string metadata_path = recording_dir + "/metadata.json";
combo.saveMetadata(metadata_path, nullptr);
```

## Error Handling

### Checking Combo Status

```cpp
#include "core/combo.h"
#include <iostream>

void printComboStatus(const evrgb::CameraStatus& status) {
    if (status.success()) {
        std::cout << "Operation successful" << std::endl;
    } else {
        std::cout << "Operation failed: " << status.message
                  << " (error code: 0x" << std::hex << status.code << std::dec << ")" << std::endl;
    }
}

int main() {
    // Enumerate cameras
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();
    if (rgb_cameras.empty()) {
        std::cerr << "No RGB camera found" << std::endl;
        return 1;
    }
    if (dvs_cameras.empty()) {
        std::cerr << "No DVS camera found" << std::endl;
        return 1;
    }

    // Create Combo object (default uses STEREO mode)
    // If you need to use beam splitter mode, specify:
    // evrgb::Combo combo(rgb_cameras[0].serial_number, dvs_cameras[0].serial,
    //                     evrgb::ComboArrangement::BEAM_SPLITTER);
    evrgb::Combo combo(rgb_cameras[0].serial_number, dvs_cameras[0].serial);

    // Initialize and check status
    if (!combo.init()) {
        std::cerr << "Combo initialization failed" << std::endl;
        return 1;
    }
    std::cout << "Combo initialized successfully" << std::endl;

    // Start and check status
    if (!combo.start()) {
        std::cerr << "Combo start failed" << std::endl;
        combo.destroy();
        return 1;
    }
    std::cout << "Combo started successfully" << std::endl;

    // Run for some time
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Stop and destroy
    combo.stop();
    combo.destroy();
    std::cout << "Combo stopped and destroyed" << std::endl;

    return 0;
}
```

## Common Issues

### Q: How to choose the appropriate arrangement mode?

**STEREO Mode Use Cases**:
- Need flexible camera positioning
- Need to observe two different perspectives
- Real-world application environments with large field of view
- Have experience with stereo calibration

**BEAM_SPLITTER Mode Use Cases**:
- Need pixel-level spatial alignment
- Deep learning model training and inference
- Algorithm development and testing
- Need precise correspondence between RGB images and event data

### Q: Can arrangement mode be changed at runtime?

**Answer**: No. Arrangement mode is specified when creating the Combo object and cannot be changed after initialization. If you need to change the arrangement mode, you must:
1. Destroy the current Combo object
2. Create a new Combo object with a different arrangement mode
3. Re-initialize and start

### Q: Does BEAM_SPLITTER mode require special hardware?

**Answer**: Yes. BEAM_SPLITTER mode requires:
- Beam splitter hardware
- Precise optical alignment mount
- Optical axes of both cameras aligned to the beam splitter
- Usually requires professional optical debugging

### Q: Is STEREO mode calibration complex?

**Answer**: Relatively complex. STEREO mode requires complete stereo calibration:
- Need to calibrate intrinsic parameters of each camera
- Need to calibrate extrinsic parameters (rotation and translation) between two cameras
- Need to use calibration board for multiple captures
- SDK provides calibration information storage and loading functionality (`calibration_info`)

### Q: Combo initialization failed

**Possible causes**:
1. Incorrect RGB or DVS camera serial number
2. Camera occupied by another program
3. Driver issues
4. Insufficient permissions
5. RGB and DVS cameras not properly connected via trigger signal

**Solutions**:
1. Use enumeration example to confirm correct serial numbers
2. Close other programs that might be using the cameras
3. Reinstall camera drivers
4. Use sudo or configure device permissions on Linux
5. Ensure trigger signal lines are properly connected between RGB and DVS cameras

### Q: Combo camera sync issues

**Notes**:
1. Ensure both RGB and DVS cameras work properly
2. Check time synchronization settings
3. Verify trigger connections
4. Check timestamp differences in logs

**Troubleshooting steps**:
1. Verify that both RGB and DVS cameras work individually
2. Verify trigger signal line connections are correct
3. Check timestamp alignment within Combo
4. Verify timestamps in callback functions are within reasonable ranges

### Q: Callback function not called

**Possible causes**:
1. Combo not started
2. Callback set after Combo start
3. Data buffer issues
4. Sync time window settings inappropriate

**Solutions**:
1. Ensure callback is set before starting
2. Check if Combo started successfully
3. Adjust buffer size
4. Check sync time window parameters

### Q: Synced data discontinuous

**Possible causes**:
1. RGB frame rate too low
2. Event data volume too high
3. System load too high
4. Buffer overflow

**Solutions**:
1. Adjust RGB frame rate
2. Optimize event processing logic
3. Increase buffer size
4. Check system resource usage

## Next Steps

After mastering basic Combo operations, you can:

1. Learn parameter tuning tutorials to optimize camera performance
2. Understand data recording functionality to save synchronized data
3. Explore advanced synchronization features like slow-motion replay
4. Check more example code to understand real-world applications
5. Choose the appropriate arrangement mode (STEREO or BEAM_SPLITTER) based on your application scenario
6. Learn how to perform camera calibration, especially stereo calibration in STEREO mode
7. Explore calibration information saving and loading functionality (`saveMetadata` and `loadMetadata`)

## Related Documentation

- [Camera Enumeration Tutorial](ENUMERATION_TUTORIAL.md) - Learn how to discover and identify camera devices
- [Parameter Tuning Tutorial](PARAMETER_TUNING_TUTORIAL.md) - Optimize camera parameters for best results
- [Recording Tutorial](RECORDING_TUTORIAL.md) - Learn data recording and advanced playback features
- [Samples Guide](SAMPLES.md) - View complete list of example programs

*/