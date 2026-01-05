# EvRGB Combo SDK

[English](README.md) | [中文](documentation/README_CN.md)

EvRGB Combo SDK is a C++ driver library designed for RGB+DVS hardware suites, providing a unified interface with internal time synchronization. The core `Combo` class manages RGB cameras and DVS (Event Cameras) combination, ensuring precise time synchronization between these different camera types.

## ✨ Key Features

- **🔗 Unified RGB+DVS Management**: Single interface for managing both camera types with built-in precise time synchronization
- **⚡ Internal Synchronization Driver**: Hardware-level trigger signal synchronization ensuring nanosecond-level timestamp alignment
- **🧵 Thread-Safe Design**: Multi-threaded architecture supporting concurrent access
- **📹 Data Recording & Playback**: Synchronized recording (RGB MP4 + CSV + DVS raw) and replay functionality
- **🔧 Cross-Platform Support**: Linux and Windows with comprehensive build system

## 🔧 Hardware Configurations

The EvRGB Combo SDK supports two different hardware configurations to accommodate various application scenarios:

### **STEREO Configuration**
- **Setup**: RGB camera and DVS camera are placed side-by-side
- **Characteristics**: 
  - Has a baseline distance between the two cameras
  - Suitable for stereo vision applications requiring depth perception
  - May introduce parallax effects for objects at different distances
  - Easier to implement and maintain

### **BEAM_SPLITTER Configuration**
- **Setup**: Uses a beam splitter (optical prism/mirror) to allow both cameras to share the same field of view
- **Characteristics**:
  - Zero baseline - both cameras see the exact same scene
  - No parallax effects, eliminating depth-related visual differences
  - Perfect for pixel-level RGB-DVS data alignment
  - Ideal for applications requiring precise temporal and spatial alignment
  - Requires careful calibration of the beam splitter assembly

## 🚀 Quick Start

### Prerequisites
- **CMake** 3.22 or newer
- **C++17** compatible compiler
- [OpenCV](https://opencv.org/) 4.x
- [Hikrobot MVS SDK](https://www.hikrobotics.com/en/machinevision/service/download?module=0) for RGB cameras
- [DvsenseDriver](https://sdk.dvsense.com/) for DVS cameras

### Build
```bash
# Clone and build
git clone https://github.com/tonywangziteng/EvRGB_combo_sdk.git
cd EvRGB_combo_sdk
mkdir build && cd build
cmake .. -DBUILD_SAMPLES=ON
make -j$(nproc)  # Linux/macOS
# or: cmake --build . --config Release  # Windows
```

### Run Examples
```bash
# Enumerate cameras
./bin/camera_enum_example

# Live synchronized preview
./bin/combo_sync_display_example

# Replay recorded data
./bin/recorded_replay_example [recording_dir]
```

## 📚 Documentation

| Topic | English |
|-------|---------|
| **Installation** | [📖 Installation Guide](https://sdk.fluxeem.com/evrgb_combo/en/docs/index.html) |
| **Tutorials** | [📚 Tutorial Index](https://sdk.fluxeem.com/evrgb_combo/en/docs/tutorial_index.html) |
| **Samples** | [🔧 Samples Guide](https://sdk.fluxeem.com/evrgb_combo/en/docs/samples_guide.html) |
| **API Reference** | [📖 Generated Docs](https://sdk.fluxeem.com/evrgb_combo/en/docs/annotated.html) |
| **Changelog** | [📝 Version History](CHANGELOG.md) |

## 🏗️ Project Structure

```
EvRGB_combo_sdk/
├── include/           # Header files
│   ├── core/         # Core Combo class and types
│   ├── camera/       # RGB and DVS camera interfaces
│   ├── sync/         # Synchronization components
│   ├── recording/    # Data recording/playback
│   └── utils/        # Utilities and logging
├── src/              # Source implementations
├── sample/           # Example programs
├── test/             # Test programs
├── documentation/    # Detailed documentation
└── external/         # External dependencies (loguru)
```

## 🎯 Core Components

### **Combo Class** (`include/core/combo.h`)
The unified management interface for RGB+DVS hardware suites:
- Simultaneous RGB and DVS camera management
- Internal time synchronization driver
- Hardware-level trigger signal synchronization
- Callback functions for synchronized data processing
- Data recording and playback capabilities

### **TriggerBuffer Class** (`include/sync/trigger_buffer.h`)
Manages timestamp synchronization of trigger signals - the key component for RGB+DVS internal synchronization.

### **SyncedDataRecorder Class** (`include/recording/synced_data_recorder.h`)
Handles synchronized data recording to files (RGB MP4 + CSV + DVS raw).

## 🛠️ Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_SAMPLES=ON/OFF` | `ON` | Build example programs |
| `BUILD_TESTS=ON/OFF` | `OFF` | Build test programs |
| `BUILD_DEV_SAMPLES=ON/OFF` | `OFF` | Build development testing samples |
| `CMAKE_BUILD_TYPE=Release/Debug` | `Release` | Build configuration |

## 📄 License

This project is licensed under the terms specified in the [LICENSE](LICENSE) file.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📞 Support

- 🐛 **Report Issues**: [GitHub Issues](https://github.com/tonywangziteng/EvRGB_combo_sdk/issues)
- 📖 **Documentation**: See the [documentation](documentation/) folder for detailed guides

---

**Note**: For detailed installation instructions, API documentation, and tutorials, please refer to the [documentation](documentation/) folder.
