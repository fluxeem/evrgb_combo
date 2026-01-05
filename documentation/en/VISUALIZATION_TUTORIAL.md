# Event Visualization Tutorial

## Overview

The `EventVisualizer` class is a powerful visualization component in EvRGB Combo SDK that enables real-time display and analysis of DVS events overlaid on RGB images. It provides flexible display modes, color customization, and coordinate transformation support, making it ideal for camera alignment, data analysis, and debugging.

## Key Features

- **Dual Display Modes**: Overlay and Side-by-Side visualization
- **Customizable Colors**: Configure ON/OFF event colors independently
- **Event Offset Adjustment**: Fine-tune event positioning
- **Coordinate Transformation**: Support for affine transformations and camera intrinsics
- **Thread-Safe Design**: Safe for concurrent access
- **High Performance**: Optimized rendering for real-time applications

## Basic Usage

### 1. Creating an EventVisualizer

```cpp
#include "utils/event_visualizer.h"

// Create visualizer with RGB and event frame sizes
cv::Size rgb_size(1920, 1080);
cv::Size event_size(640, 480);

// Default colors: ON=red, OFF=blue
evrgb::EventVisualizer visualizer(rgb_size, event_size);

// Or customize colors
cv::Vec3b on_color(0, 255, 0);    // Green for ON events
cv::Vec3b off_color(255, 0, 255);  // Magenta for OFF events
evrgb::EventVisualizer visualizer(rgb_size, event_size, on_color, off_color);
```

### 2. Updating RGB Frame

```cpp
cv::Mat rgb_frame = ...; // Your RGB image (BGR format, CV_8UC3)

// Update the cached RGB frame
if (!visualizer.updateRgbFrame(rgb_frame)) {
    std::cerr << "Failed to update RGB frame" << std::endl;
}
```

### 3. Visualizing Events

```cpp
std::vector<dvsense::Event2D> events = ...; // Your event data

// Visualize events onto the output frame
cv::Mat output_frame;
if (visualizer.visualizeEvents(events, output_frame)) {
    cv::imshow("Event Visualization", output_frame);
    cv::waitKey(1);
}
```

## Display Modes

### Overlay Mode (Default)

Events are overlaid directly onto the RGB image, allowing you to see the spatial relationship between events and image content.

```cpp
// Set overlay mode explicitly
visualizer.setDisplayMode(evrgb::EventVisualizer::DisplayMode::Overlay);
```

### Side-by-Side Mode

Events and RGB image are displayed side-by-side for easier comparison.

```cpp
// Set side-by-side mode
visualizer.setDisplayMode(evrgb::EventVisualizer::DisplayMode::SideBySide);

// Or toggle between modes
auto current_mode = visualizer.toggleDisplayMode();
```

## Event Offset Adjustment

Fine-tune the position of events relative to the RGB image:

```cpp
// Set absolute offset
cv::Point2i offset(10, 20);
visualizer.setEventOffset(offset);

// Or adjust incrementally
cv::Point2i current = visualizer.eventOffset();
current.x += 5;
current.y += 5;
visualizer.setEventOffset(current);
```

## Color Customization

Customize the appearance of ON and OFF events:

```cpp
// Set both colors
cv::Vec3b on_color(0, 255, 0);    // Green
cv::Vec3b off_color(255, 0, 0);   // Red
visualizer.setColors(on_color, off_color);

// Get current colors
cv::Vec3b current_on = visualizer.onColor();
cv::Vec3b current_off = visualizer.offColor();
```

## Coordinate Transformation

### Setting Event Frame Size

Update the event frame size if it changes:

```cpp
cv::Size2i new_event_size(1280, 720);
visualizer.setEventSize(new_event_size);
```

### Affine Transformation

Apply affine transformations for precise alignment:

```cpp
#include "utils/calib_info.h"

// Create affine transformation matrix
evrgb::AffineTransform affine;
affine.A(0, 0) = 1.0;  // Scale X
affine.A(1, 1) = 1.0;  // Scale Y
affine.A(0, 2) = 10.0; // Translation X
affine.A(1, 2) = 20.0; // Translation Y

// Apply calibration
visualizer.setCalibration(affine);
```

### Camera Intrinsics

Set camera intrinsics for normalized coordinate mapping:

```cpp
// Define camera intrinsics
evrgb::CameraIntrinsics rgb_intrinsics;
rgb_intrinsics.fx = 1000.0;
rgb_intrinsics.fy = 1000.0;
rgb_intrinsics.cx = 960.0;
rgb_intrinsics.cy = 540.0;

evrgb::CameraIntrinsics dvs_intrinsics;
dvs_intrinsics.fx = 500.0;
dvs_intrinsics.fy = 500.0;
dvs_intrinsics.cx = 320.0;
dvs_intrinsics.cy = 240.0;

// Set intrinsics
visualizer.setIntrinsics(rgb_intrinsics, dvs_intrinsics);
```

### Horizontal Flip

Enable horizontal flipping for event coordinates:

```cpp
// Enable X-axis flipping
visualizer.setFlipX(true);

// Check current flip state
bool is_flipped = visualizer.flipX();
```

## Complete Example

Here's a complete example showing how to use EventVisualizer with Combo:

```cpp
#include "core/combo.h"
#include "utils/event_visualizer.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <mutex>
#include <memory>

class SyncedFrameRenderer {
public:
    void update(const evrgb::RgbImageWithTimestamp& rgb_data,
                const std::vector<dvsense::Event2D>& events) {
        cv::Mat bgr = ensureBgrFrame(rgb_data.image);
        if (bgr.empty()) return;

        ensureVisualizer(bgr.size());
        if (!visualizer_) return;

        visualizer_->updateRgbFrame(bgr);

        cv::Mat output_frame;
        if (visualizer_->visualizeEvents(events, output_frame)) {
            std::lock_guard<std::mutex> lock(mutex_);
            latest_frame_ = output_frame.clone();
        }
    }

    void toggleDisplayMode() {
        if (visualizer_) {
            visualizer_->toggleDisplayMode();
        }
    }

    void adjustEventOffset(const cv::Point& delta) {
        if (visualizer_) {
            cv::Point2i current = visualizer_->eventOffset();
            current.x += delta.x;
            current.y += delta.y;
            visualizer_->setEventOffset(current);
        }
    }

    bool getLatestFrame(cv::Mat& out_frame) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (latest_frame_.empty()) return false;
        out_frame = latest_frame_.clone();
        return true;
    }

private:
    static cv::Mat ensureBgrFrame(const cv::Mat& input) {
        if (input.empty()) return {};
        if (input.channels() == 3 && input.type() == CV_8UC3) {
            return input;
        }
        cv::Mat converted;
        input.convertTo(converted, CV_8UC3);
        return converted;
    }

    void ensureVisualizer(const cv::Size& rgb_size) {
        if (visualizer_) return;
        cv::Size event_size(640, 480);
        visualizer_ = std::make_unique<evrgb::EventVisualizer>(
            rgb_size, event_size,
            cv::Vec3b(0, 255, 0),  // ON events: green
            cv::Vec3b(255, 0, 0)    // OFF events: red
        );
    }

private:
    std::mutex mutex_;
    cv::Mat latest_frame_;
    std::unique_ptr<evrgb::EventVisualizer> visualizer_;
};

int main() {
    // Enumerate cameras
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();

    if (rgb_cameras.empty() || dvs_cameras.empty()) {
        std::cerr << "No cameras found!" << std::endl;
        return -1;
    }

    // Create combo
    evrgb::Combo combo(
        rgb_cameras[0].serial_number,
        dvs_cameras[0].serial,
        evrgb::Combo::Arrangement::STEREO,
        100
    );

    // Create renderer
    auto renderer = std::make_shared<SyncedFrameRenderer>();

    // Set callback
    combo.setSyncedCallback(
        [renderer](const evrgb::RgbImageWithTimestamp& rgb,
                   const std::vector<dvsense::Event2D>& events) {
            renderer->update(rgb, events);
        }
    );

    // Initialize and start
    if (!combo.init() || !combo.start()) {
        std::cerr << "Failed to start combo" << std::endl;
        return -1;
    }

    // Create window
    cv::namedWindow("Event Visualization", cv::WINDOW_AUTOSIZE);
    std::cout << "Controls:" << std::endl;
    std::cout << "  - Arrow Keys: Adjust event offset" << std::endl;
    std::cout << "  - 'm': Toggle display mode" << std::endl;
    std::cout << "  - 'q' or ESC: Quit" << std::endl;

    // Main loop
    cv::Mat frame;
    while (true) {
        if (renderer->getLatestFrame(frame)) {
            cv::imshow("Event Visualization", frame);
        }

        int key = cv::waitKeyEx(33);

        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }

        if (key == 'm' || key == 'M') {
            renderer->toggleDisplayMode();
        }

        // Handle arrow keys for offset adjustment
        cv::Point delta(0, 0);
        if (key == 2424832) delta.x = -1;      // Left
        else if (key == 2555904) delta.x = 1;   // Right
        else if (key == 2490368) delta.y = -1;  // Up
        else if (key == 2621440) delta.y = 1;   // Down

        if (delta.x != 0 || delta.y != 0) {
            renderer->adjustEventOffset(delta);
        }
    }

    combo.stop();
    cv::destroyAllWindows();

    return 0;
}
```

## Advanced Usage

### Thread-Safe Implementation

The `EventVisualizer` class is thread-safe and can be safely used in multi-threaded environments:

```cpp
// Thread 1: Update RGB frame
std::thread rgb_thread([&]() {
    while (running) {
        cv::Mat rgb_frame = getRgbFrame();
        visualizer.updateRgbFrame(rgb_frame);
    }
});

// Thread 2: Visualize events
std::thread event_thread([&]() {
    while (running) {
        auto events = getEvents();
        cv::Mat output;
        visualizer.visualizeEvents(events, output);
        displayFrame(output);
    }
});

rgb_thread.join();
event_thread.join();
```

### Integration with Calibration

For precise camera alignment, combine EventVisualizer with calibration data:

```cpp
// Load calibration from combo
auto meta = combo.getMetadata();
if (std::holds_alternative<evrgb::AffineTransform>(meta.calibration)) {
    auto affine = std::get<evrgb::AffineTransform>(meta.calibration);
    visualizer.setCalibration(affine);
}

// Set intrinsics if available
if (meta.rgb.intrinsics && meta.dvs.intrinsics) {
    visualizer.setIntrinsics(
        *meta.rgb.intrinsics,
        *meta.dvs.intrinsics
    );
}
```

## Common Issues

### Q: Why are events not visible on the RGB image?

**A**: Check the following:
1. Ensure RGB frame size matches the size passed to constructor
2. Verify event coordinates are within valid range
3. Check if event offset is too large
4. Confirm display mode is set correctly (try Side-by-Side)

### Q: How do I improve visualization performance?

**A**:
- Use appropriate frame sizes (avoid unnecessarily large resolutions)
- Minimize frequency of `updateRgbFrame()` calls
- Consider downsizing frames for preview purposes
- Use thread-safe patterns to avoid blocking

### Q: Can I use EventVisualizer with recorded data?

**A**: Yes, EventVisualizer works with both real-time and recorded data. See the `recorded_replay_example` for details.

### Q: How do I handle different image formats?

**A**: EventVisualizer expects BGR format (CV_8UC3). Convert your images:
```cpp
cv::Mat bgr;
if (input.channels() == 1) {
    cv::cvtColor(input, bgr, cv::COLOR_GRAY2BGR);
} else if (input.channels() == 4) {
    cv::cvtColor(input, bgr, cv::COLOR_BGRA2BGR);
} else {
    bgr = input;
}
```

## Related Examples

- `combo_sync_display_example` - Basic synchronized display with EventVisualizer
- `beam_splitter_align` - Advanced alignment with affine transformations
- `recorded_replay_example` - Visualization of recorded data

## API Reference

For complete API documentation, see:
- [EventVisualizer Class Reference](../../../docs/html/classevrgb_1_1_event_visualizer.html)
- [Combo Class Reference](../../../docs/html/classevrgb_1_1_combo.html)

## Next Steps

- Explore the [Camera Usage Tutorial](CAMERA_USAGE_TUTORIAL.md) for basic camera operations
- Learn about [Calibration](CALIBRATION_TUTORIAL.md) for precise coordinate alignment
- Check [Recording Tutorial](RECORDING_TUTORIAL.md) for data recording and playback