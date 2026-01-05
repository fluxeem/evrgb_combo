# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- EventVisualizer class for real-time event visualization with overlay/side-by-side modes
- Beam splitter alignment example with interactive calibration tools
- Camera intrinsics support with CameraIntrinsics structure and factory pattern
- Calibration information system (CalibInfo, RigidTransform, AffineTransform)
- Metadata persistence system (CameraMetadata, ComboMetadata) with JSON serialization
- Recording metadata support in SyncedDataRecorder and RecordedSyncReader
- Device model infrastructure for RGB and DVS cameras
- Camera arrangement modes (STEREO, BEAM_SPLITTER) with arrangement-specific calibration
- nlohmann_json dependency as Git submodule
- Comprehensive documentation: Event Visualization Tutorial, Calibration Tutorial, updated tutorials

### Changed
- Camera management refactoring with shared pointers and factory pattern
- Combo interface enhancements with consolidated metadata handling
- Recording system improvements with metadata support
- Documentation restructuring and tutorial reorganization
- Build system updates for new dependencies and source files

### Fixed
- JSON serialization linking issues
- Interface compatibility issues
- Documentation inconsistencies

### Removed
- Unused combo_threads.h header file

## [1.1.0] - 2026-01-05

### Added
- New calibration information support for both STEREO and BEAM_SPLITTER configurations
- CalibInfo class for managing calibration data
- Beam splitter alignment example for precise optical setup
- Enhanced documentation with calibration tutorial
- Support for hardware configuration identification and management

### Changed
- Updated version to 1.1.0
- Improved hardware configuration handling
- Enhanced documentation structure

### Fixed
- Minor documentation improvements

## [1.0.1] - 2026-01-02

### Added
- Comprehensive changelog system following Keep a Changelog standard
- Chinese version of changelog in documentation/zh/CHANGELOG.md
- Enhanced RecordedSyncReader interface with new functionality
- Improved documentation cross-references

### Changed
- Updated version number from 1.0.0 to 1.0.1
- Refactored sample code structure:
  - Moved example implementations to subdirectories
  - Removed duplicate example files from root sample directory
- Updated documentation references in README.md and HOME_PAGE.md
- Enhanced PARAMETER_TUNING_TUTORIAL.md content for both English and Chinese versions
- Improved RecordedSyncReader implementation with additional features

### Fixed
- Documentation structure and cross-references
- Sample code organization and build configuration

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
