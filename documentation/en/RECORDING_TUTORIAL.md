@page recording_tutorial Data Recording and Replay Tutorial

This tutorial introduces how to record synchronized RGB images and DVS event data using EvRGB Combo SDK, and how to replay recorded data for in-depth analysis.

## Table of Contents

- [Recording Overview](#recording-overview)
- [Using Recording Features](#using-recording-features)
- [Using Replay Features](#using-replay-features)
- [Configuration Details](#configuration-details)
- [Metadata Management](#metadata-management)
- [Common Issues](#common-issues)
- [Next Steps](#next-steps)

## Recording Overview

EvRGB Combo SDK provides powerful data recording capabilities that can synchronize RGB images and DVS event data to files, supporting the following features:

- **Synchronized Recording**: Precise time synchronization between RGB images and DVS events
- **Multiple Output Formats**: MP4 video, CSV timestamps, and raw event data
- **Automatic Directory Creation**: Output directory is created automatically if it doesn't exist
- **Thread-Safe**: Recording process does not affect real-time data acquisition
- **Metadata Saving**: Automatically saves camera configuration and calibration information

### Recording Content

The recording feature generates the following files:

| Filename | Format | Description |
|----------|--------|-------------|
| `combo_rgb.mp4` | MP4 Video | RGB image sequence, encoded using OpenCV VideoWriter |
| `combo_timestamps.csv` | CSV Text | Contains frame index, exposure start time, and exposure end time |
| `combo_events.raw` | Binary File | DVS raw event data, containing all event timestamps and coordinates |
| `metadata.json` | JSON File | Camera configuration, calibration information, and other metadata (optional) |

## Using Recording Features

### Basic Recording Workflow

```cpp
// 1. Create Combo object
evrgb::Combo combo(rgb_serial, dvs_serial, evrgb::ComboArrangement::STEREO, 100);

// 2. Configure recorder
evrgb::SyncedRecorderConfig recorder_cfg;
recorder_cfg.output_dir = "recordings";
recorder_cfg.fps = 30.0;
recorder_cfg.fourcc = "mp4v";

// 3. Create and bind recorder
auto recorder = std::make_shared<evrgb::SyncedDataRecorder>();
combo.setSyncedDataRecorder(recorder);

// 4. Initialize and start Combo
if (!combo.init() || !combo.start()) {
    return -1;
}

// 5. Start recording
combo.startRecording(recorder_cfg);

// 6. Recording data...
// ...

// 7. Stop recording
combo.stopRecording();

// 8. Optional: Save metadata
combo.saveMetadata("recordings/metadata.json", nullptr);

// 9. Stop Combo
combo.stop();
```

### Sample Program

Refer to the sample code `sample/user_examples/combo_sync_display_example.cpp`, which provides a complete demonstration of recording features, including:

- Real-time synchronized display
- Interactive recording control
- Event overlay offset adjustment
- Multiple display mode switching

Run the sample program:

```bash
./bin/combo_sync_display_example
```

Interactive controls:
- SPACE: Start/stop recording
- m key: Toggle display mode (overlay / side-by-side)
- Arrow keys: Adjust DVS event overlay offset
- q key / ESC: Exit program

## Using Replay Features

### Replay Features

EvRGB Combo SDK provides powerful replay capabilities:

- **Precise Time Control**: Precise data synchronization based on timestamps
- **Slow-Motion Playback**: Supports slow-motion replay for detailed event observation
- **Multiple Display Modes**: Supports overlay and side-by-side display
- **Automatic Metadata Loading**: Automatically loads and applies camera configuration and calibration information
- **Interactive Control**: Supports real-time adjustment of display parameters

### Basic Replay Workflow

```cpp
// 1. Create replay reader
evrgb::RecordedSyncReader reader({"recordings"});
if (!reader.open()) {
    return -1;
}

// 2. Create visualization canvas
evrgb::EventVisualizer canvas(
    reader.getRgbFrameSize(),
    reader.getEventFrameSize()
);

// 3. Load and apply metadata
auto metadata = reader.getMetadata();
if (metadata.has_value()) {
    // Apply camera intrinsics
    if (metadata->rgb.intrinsics && metadata->dvs.intrinsics) {
        canvas.setIntrinsics(*metadata->rgb.intrinsics, *metadata->dvs.intrinsics);
    }
    // Apply calibration information
    if (std::holds_alternative<evrgb::AffineTransform>(metadata->calibration)) {
        canvas.setCalibration(std::get<evrgb::AffineTransform>(metadata->calibration));
    }
    // Set flip based on arrangement mode
    if (metadata->arrangement == evrgb::ComboArrangement::BEAM_SPLITTER) {
        canvas.setFlipX(true);
    }
}

// 4. Replay data
evrgb::RecordedSyncReader::Sample sample;
while (reader.next(sample)) {
    // Update RGB frame
    canvas.updateRgbFrame(sample.rgb);

    // Visualize events
    cv::Mat view;
    canvas.visualizeEvents(sample.events->begin(), sample.events->end(), view);

    // Display
    cv::imshow("Replay", view);
    cv::waitKey(1);
}
```

### Sample Program

Refer to the sample code `sample/user_examples/recorded_replay_example/recorded_replay_example.cpp`, which provides a complete demonstration of replay features, including:

- Precise timestamp control
- Slow-motion playback
- Display mode switching
- Automatic metadata loading and application

Run the sample program:

```bash
./bin/recorded_replay_example recordings
```

Interactive controls:
- SPACE: Toggle slow-motion mode
- m key: Toggle display mode (overlay / side-by-side)
- f key: Flip X axis (for beam splitter mode)
- q key / ESC: Exit program

## Configuration Details

### SyncedRecorderConfig Structure

```cpp
struct SyncedRecorderConfig {
    std::string output_dir;  // Output directory path
    double fps;              // Video frame rate
    std::string fourcc;      // Video codec format (e.g., "mp4v", "avc1")
};
```

### output_dir (Output Directory)

- **Type**: `std::string`
- **Description**: Output directory for recorded files
- **Notes**:
  - Directory is created automatically if it doesn't exist
  - Ensure the program has write permissions
  - Absolute paths are recommended

### fps (Frame Rate)

- **Type**: `double`
- **Description**: Frame rate for MP4 video
- **Recommended Values**:
  - 30.0: Standard frame rate, suitable for most applications
  - 60.0: High frame rate, suitable for fast motion scenes
  - 120.0: Ultra-high frame rate, suitable for slow-motion analysis
- **Notes**:
  - Recommended to set close to the actual RGB frame rate
  - Higher frame rates increase file size
  - Lower frame rates may cause data loss

### fourcc (Codec Format)

- **Type**: `std::string`
- **Description**: OpenCV video encoder FourCC code
- **Common Values**:
  - `"mp4v"`: MPEG-4 encoding (good compatibility)
  - `"avc1"`: H.264 encoding (high compression rate)
  - `"xvid"`: XVID encoding (open source)
  - `"mjpa"`: Motion JPEG (lossless)
- **Notes**:
  - Depends on encoders supported by OpenCV build
  - Different platforms may support different encoders
  - Test encoder compatibility

## Metadata Management

### Metadata Overview

Metadata contains key data such as camera configuration and calibration information, ensuring correct restoration of recording settings during replay.

### Metadata Data Structure

```cpp
struct ComboMetadata {
    CameraMetadata rgb;                    // RGB camera metadata
    CameraMetadata dvs;                    // DVS camera metadata
    ComboArrangement arrangement;          // Arrangement mode
    ComboCalibrationInfo calibration;      // Calibration information
};

struct CameraMetadata {
    std::string manufacturer;              // Manufacturer
    std::string model;                     // Camera model
    std::string serial;                    // Serial number
    unsigned int width = 0;                // Image width
    unsigned int height = 0;               // Image height
    std::optional<CameraIntrinsics> intrinsics;  // Camera intrinsics
};
```

### Saving Metadata

```cpp
std::string metadata_path = "recordings/metadata.json";
std::string error_message;

if (combo.saveMetadata(metadata_path, &error_message)) {
    std::cout << "Metadata saved successfully" << std::endl;
} else {
    std::cerr << "Failed to save metadata: " << error_message << std::endl;
}
```

### Loading Metadata (During Replay)

The replay reader automatically loads metadata:

```cpp
evrgb::RecordedSyncReader reader({"recordings"});
if (!reader.open()) {
    return -1;
}

auto metadata = reader.getMetadata();
if (metadata.has_value()) {
    // Apply metadata to visualizer
    if (metadata->rgb.intrinsics && metadata->dvs.intrinsics) {
        canvas.setIntrinsics(*metadata->rgb.intrinsics, *metadata->dvs.intrinsics);
    }
}
```

### Metadata Use Cases

1. **Calibration Persistence**: Save camera calibration results to avoid repeated calibration
2. **Configuration Management**: Record RGB and DVS camera parameter settings
3. **Data Recording Association**: Save corresponding metadata when recording data to ensure data traceability
4. **Replay Restoration**: Automatically apply recording configuration and calibration information during replay

## Common Issues

### Q: Recording failed, unable to create files

**Possible Causes**:
1. Incorrect output directory path
2. Program lacks write permissions
3. Insufficient disk space
4. OpenCV encoder doesn't support specified fourcc

**Solutions**:
1. Check if `output_dir` path is correct
2. Ensure the program has write permissions
3. Check disk space
4. Try a different fourcc encoder

### Q: Recorded video cannot be played

**Possible Causes**:
1. OpenCV encoder not supported
2. Video player doesn't support the codec format
3. Incorrect fps setting

**Solutions**:
1. Try a different fourcc (e.g., `"mp4v"`)
2. Use a player that supports more formats (e.g., VLC)
3. Check if fps setting is reasonable

### Q: Frame drops during recording

**Possible Causes**:
1. High system load
2. Insufficient buffer size
3. Slow disk write speed
4. Low fps setting

**Solutions**:
1. Increase buffer size (`max_buffer_size` in `Combo` constructor)
2. Use SSD to improve disk write speed
3. Adjust fps setting
4. Optimize system resource usage

### Q: Events and images out of sync during replay

**Possible Causes**:
1. Metadata not loaded correctly
2. Missing calibration information
3. Incorrect arrangement mode setting

**Solutions**:
1. Ensure metadata was saved during recording
2. Check if calibration information in metadata is complete
3. Verify arrangement mode (STEREO or BEAM_SPLITTER) is correct

### Q: How to choose appropriate fps?

**Recommendations**:
- **Standard Applications**: 30 fps (balances performance and quality)
- **Fast Motion Scenes**: 60 fps (reduces motion blur)
- **Slow-Motion Analysis**: 120 fps or higher (facilitates post-processing)
- **Storage Limited**: 15-20 fps (saves space)

### Q: How to choose appropriate fourcc?

**Recommendations**:
- **Maximum Compatibility**: `"mp4v"` (MPEG-4)
- **Best Compression**: `"avc1"` (H.264)
- **Lossless Quality**: `"mjpa"` (Motion JPEG)
- **Cross-Platform**: Test encoder support on different platforms

## Next Steps

After mastering recording and replay features, you can:

1. Learn the camera calibration tutorial to understand how to perform camera calibration
2. Explore the parameter tuning tutorial to optimize camera performance
3. Check the visualization tutorial to learn event data visualization techniques
4. View more sample code to understand real-world application scenarios

## Related Documentation

- [Camera Usage Tutorial](CAMERA_USAGE_TUTORIAL.md) - Learn Combo basic operations
- [Device Enumeration Tutorial](ENUMERATION_TUTORIAL.md) - Learn how to discover and identify camera devices
- [Parameter Tuning Tutorial](PARAMETER_TUNING_TUTORIAL.md) - Optimize camera parameters for best results
- [Calibration Tutorial](CALIBRATION_TUTORIAL.md) - Learn camera calibration methods
- [Visualization Tutorial](VISUALIZATION_TUTORIAL.md) - Learn event data visualization techniques
- [Samples Guide](SAMPLES.md) - View complete sample program list

*/
