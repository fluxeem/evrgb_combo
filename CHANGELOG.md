# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial changelog setup

### Changed

### Deprecated

### Removed

### Fixed

### Security

## [1.0.1] - 2026-01-02

### Changed
- Updated version number to 1.0.1

## [1.0.0] - 2025-01-02

### Added
- Initial release of EvRGB Combo SDK
- Core `Combo` class for unified RGB+DVS camera management
- Internal time synchronization driver with nanosecond-level precision
- Thread-safe multi-threaded architecture
- Synchronized data recording (RGB MP4 + CSV + DVS raw)
- Data replay functionality for recorded sessions
- Cross-platform support (Linux and Windows)
- Comprehensive example programs:
  - `camera_enum_example` - Camera device enumeration
  - `combo_sync_display_example` - Real-time synchronized preview
  - `recorded_replay_example` - Recording data replay
  - `rgb_exposure_step_example` - RGB exposure adjustment
- Complete documentation system with tutorials and API reference
- CMake build system with preset configurations
- vcpkg integration for Windows dependency management

### Core Components
- **Combo Class** (`include/core/combo.h`) - Main management interface
- **TriggerBuffer Class** (`include/sync/trigger_buffer.h`) - Timestamp synchronization
- **EventVectorPool Class** (`include/sync/event_vector_pool.h`) - Memory management
- **SyncedDataRecorder Class** (`include/recording/synced_data_recorder.h`) - Data recording
- **RecordedSyncReader Class** (`include/recording/recorded_sync_reader.h`) - Data playback
- **Camera Interfaces** (`include/camera/`) - RGB and DVS camera abstractions

### Technical Features
- C++17 standard compliance
- RAII resource management
- Smart pointer memory management
- Exception-safe design
- Comprehensive error handling with `CameraStatus` structures
- Loguru-based logging system
- Hardware-level trigger signal synchronization
- Event vector memory pooling for performance optimization

### Documentation
- Complete tutorial system (English and Chinese)
- API reference documentation with Doxygen
- Installation guides for all supported platforms
- Sample code with detailed explanations
- Parameter tuning guides

### Build System
- Modern CMake 3.22+ configuration
- CMake Presets for different build environments
- CPack support for package generation
- FindMVS.cmake module for Hikrobot MVS SDK integration
- Automated dependency management

[Unreleased]: https://github.com/fluxeem/evrgb_combo/compare/v1.0.1...HEAD
[1.0.1]: https://github.com/fluxeem/evrgb_combo/releases/tag/v1.0.1
[1.0.0]: https://github.com/fluxeem/evrgb_combo/releases/tag/v1.0.0