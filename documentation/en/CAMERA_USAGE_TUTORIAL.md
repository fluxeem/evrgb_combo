@page camera_usage_tutorial Camera Usage Tutorial

This tutorial introduces how to use EvRGB Combo SDK to operate RGB cameras, DVS cameras, and combo cameras.

## Table of Contents

- [RGB Camera Basic Operations](#rgb-camera-basic-operations)
- [DVS Camera Basic Operations](#dvs-camera-basic-operations)
- [Combo Camera Usage](#combo-camera-usage)
- [Error Handling](#error-handling)
- [Common Issues](#common-issues)

## RGB Camera Basic Operations

### Initialization and Basic Usage

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

int main() {
    // Enumerate RGB cameras
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) {
        std::cerr << "No RGB camera found" << std::endl;
        return 1;
    }

    // Use the first available camera
    const std::string serial = cameras[0].serial_number;
    evrgb::HikvisionRgbCamera camera;

    // Initialize camera
    if (!camera.initialize(serial)) {
        std::cerr << "Camera initialization failed" << std::endl;
        return 1;
    }

    std::cout << "Camera initialized successfully: " << serial << std::endl;

    // Start camera
    if (!camera.start()) {
        std::cerr << "Camera start failed" << std::endl;
        return 1;
    }

    std::cout << "Camera started, capturing images..." << std::endl;

    // Get images
    for (int i = 0; i < 10; ++i) {
        cv::Mat frame;
        if (camera.getLatestImage(frame)) {
            std::cout << "Got image " << (i+1) << ": " 
                      << frame.cols << "x" << frame.rows << std::endl;
            
            // Process image here...
            // cv::imwrite("frame_" + std::to_string(i) + ".jpg", frame);
        } else {
            std::cout << "Failed to get image" << std::endl;
        }
        
        // Wait for some time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop and destroy
    camera.stop();
    camera.destroy();
    std::cout << "Camera stopped and destroyed" << std::endl;
    
    return 0;
}
```

### Setting Image Callback

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) return 1;

    evrgb::HikvisionRgbCamera camera;
    if (!camera.initialize(cameras[0].serial_number)) return 1;

    // Set image callback function
    camera.setImageCallback([](const cv::Mat& image, uint64_t timestamp_us) {
        static int frame_count = 0;
        frame_count++;
        
        std::cout << "Received image " << frame_count 
                  << " timestamp: " << timestamp_us << " us"
                  << " size: " << image.cols << "x" << image.rows << std::endl;
        
        // Process image here...
    });

    if (!camera.start()) return 1;

    // Run for 5 seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));

    camera.stop();
    camera.destroy();
    return 0;
}
```

## DVS Camera Basic Operations

### Initialization and Event Capture

```cpp
#include "camera/dvs_camera.h"
#include <iostream>

int main() {
    // Enumerate DVS cameras
    auto cameras = evrgb::enumerateAllDvsCameras();
    if (cameras.empty()) {
        std::cerr << "No DVS camera found" << std::endl;
        return 1;
    }

    // Use the first available camera
    const std::string serial = cameras[0].serial;
    evrgb::DvsCamera camera;

    // Initialize camera
    if (!camera.initialize(serial)) {
        std::cerr << "DVS camera initialization failed" << std::endl;
        return 1;
    }

    std::cout << "DVS camera initialized successfully: " << serial << std::endl;

    // Set event callback function
    camera.setEventCallback([](const std::vector<dvsense::Event2D>& events) {
        if (!events.empty()) {
            std::cout << "Received " << events.size() << " events" << std::endl;
            
            // Count positive and negative events
            int positive_events = 0;
            int negative_events = 0;
            for (const auto& event : events) {
                if (event.polarity) {
                    positive_events++;
                } else {
                    negative_events++;
                }
            }
            
            std::cout << "  Positive events: " << positive_events 
                      << ", Negative events: " << negative_events << std::endl;
        }
    });

    // Start camera
    if (!camera.start()) {
        std::cerr << "DVS camera start failed" << std::endl;
        return 1;
    }

    std::cout << "DVS camera started, capturing events..." << std::endl;

    // Run for 5 seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Stop and destroy
    camera.stop();
    camera.destroy();
    std::cout << "DVS camera stopped and destroyed" << std::endl;
    
    return 0;
}
```

## Combo Camera Usage

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
    
    evrgb::Combo combo(rgb_serial, dvs_serial);
    
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

    evrgb::Combo combo(rgb_cameras[0].serial_number, dvs_cameras[0].serial);

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

## Error Handling

### Checking Camera Status

```cpp
#include "camera/rgb_camera.h"
#include <iostream>

void printCameraStatus(const evrgb::CameraStatus& status) {
    if (status.success()) {
        std::cout << "Operation successful" << std::endl;
    } else {
        std::cout << "Operation failed: " << status.message
                  << " (error code: 0x" << std::hex << status.code << std::dec << ")" << std::endl;
    }
}

int main() {
    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) return 1;

    evrgb::HikvisionRgbCamera camera;
    
    // Initialize and check status
    auto status = camera.initialize(cameras[0].serial_number);
    printCameraStatus(status);
    if (!status.success()) return 1;

    // Start and check status
    status = camera.start();
    printCameraStatus(status);
    if (!status.success()) {
        camera.destroy();
        return 1;
    }

    // Get image and check status
    cv::Mat frame;
    if (camera.getLatestImage(frame)) {
        std::cout << "Successfully got image" << std::endl;
    } else {
        std::cout << "Failed to get image" << std::endl;
    }

    camera.stop();
    camera.destroy();
    return 0;
}
```

## Common Issues

### Q: Camera initialization failed

**Possible causes**:
1. Incorrect camera serial number
2. Camera occupied by another program
3. Driver issues
4. Insufficient permissions

**Solutions**:
1. Use enumeration example to confirm correct serial number
2. Close other programs that might be using the camera
3. Reinstall camera drivers
4. Use sudo or configure device permissions on Linux

### Q: Unable to get images

**Troubleshooting steps**:
1. Confirm camera is started
2. Check camera connection
3. Verify camera parameter settings
4. Check error logs

### Q: Callback function not called

**Possible causes**:
1. Camera not started
2. Callback set after camera start
3. Data buffer issues

**Solutions**:
1. Ensure callback is set before starting
2. Check if camera started successfully
3. Adjust buffer size

### Q: Combo camera sync issues

**Notes**:
1. Ensure both RGB and DVS cameras work properly
2. Check time synchronization settings
3. Verify trigger connections

## Next Steps

After mastering basic camera operations, you can:

1. Learn parameter tuning tutorials
2. Understand data recording functionality
3. Explore advanced synchronization features
4. Check more example code

## Related Documentation

- [Camera Enumeration Tutorial](ENUMERATION_TUTORIAL.md)
- [Parameter Tuning Tutorial](PARAMETER_TUNING_TUTORIAL.md)
- [Recording Tutorial](RECORDING_TUTORIAL.md)

*/