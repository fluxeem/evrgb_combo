# RGB Camera Enumeration Tests and Examples

This directory contains tests and examples for the RGB camera enumeration functionality using Google Test framework.

## Files Overview

### Tests (`/test/`)
- `test_camera_enumeration.cpp` - Google Test based test suite for camera enumeration
- `CMakeLists.txt` - Build configuration for tests (requires Google Test)
- `CMakeLists_with_fetch.txt` - Alternative build config that auto-downloads Google Test

### Examples (`/sample/`)
- `camera_enum_example.cpp` - Simple example showing how to enumerate cameras
- `CMakeLists.txt` - Build configuration for examples

## Prerequisites

### Option 1: Install Google Test
```bash
# Ubuntu/Debian
sudo apt-get install libgtest-dev libgmock-dev

# Or build from source
git clone https://github.com/google/googletest.git
cd googletest && mkdir build && cd build
cmake .. && make && sudo make install
```

### Option 2: Use FetchContent (Automatic Download)
If Google Test is not installed, you can use the alternative CMakeLists.txt:
```bash
cp test/CMakeLists_with_fetch.txt test/CMakeLists.txt
```

## Building

### Basic Build (Library only)
```bash
mkdir -p build
cd build
cmake ..
make
```

### Build with Tests
```bash
mkdir -p build
cd build
cmake -DBUILD_TESTS=ON ..
make
```

### Build with Tests and Samples
```bash
mkdir -p build
cd build
cmake -DBUILD_TESTS=ON -DBUILD_SAMPLES=ON ..
make
```

### Build Options
- `BUILD_TESTS=ON/OFF` - Enable/disable test compilation (default: OFF)
- `BUILD_SAMPLES=ON/OFF` - Enable/disable sample compilation (default: ON)

When enabled, this will create executables in `build/bin/`:
- `test_camera_enumeration` - Google Test based test suite (if BUILD_TESTS=ON)
- `camera_enum_example` - Simple example (if BUILD_SAMPLES=ON)

## Running

### Run the simple example:
```bash
./build/bin/camera_enum_example
```

### Run the comprehensive test suite:
```bash
./build/bin/test_camera_enumeration
```

### Run tests via CTest:
```bash
cd build
ctest
```

## Test Suite Features

The Google Test based test suite (`test_camera_enumeration`) includes:

1. **RgbCameraTest.BasicEnumeration**
   - Tests that enumeration function runs without crashing
   - Verifies return value is a valid vector

2. **RgbCameraTest.MultipleEnumerationConsistency**
   - Performs multiple enumerations to check consistency
   - Verifies that the same number of cameras are found each time

3. **RgbCameraTest.EnumerationPerformance**
   - Measures enumeration performance
   - Ensures enumeration completes within reasonable time (< 2s per call)

4. **RgbCameraTest.CameraInfoStructure**
   - Validates basic structure integrity
   - Checks that strings are properly null-terminated
   - Verifies width/height are non-negative

5. **RgbCameraTest.CameraInfoNotEmpty**
   - If cameras are found, validates they have basic information
   - Handles the case where no cameras are connected

## Expected Output

### When cameras are connected:
```
[==========] Running 5 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 5 tests from RgbCameraTest
[ RUN      ] RgbCameraTest.BasicEnumeration
Found 2 camera(s)
[       OK ] RgbCameraTest.BasicEnumeration (45 ms)
[ RUN      ] RgbCameraTest.MultipleEnumerationConsistency
[       OK ] RgbCameraTest.MultipleEnumerationConsistency (150 ms)
[ RUN      ] RgbCameraTest.EnumerationPerformance
[       OK ] RgbCameraTest.EnumerationPerformance (225 ms)
[ RUN      ] RgbCameraTest.CameraInfoStructure
[       OK ] RgbCameraTest.CameraInfoStructure (45 ms)
[ RUN      ] RgbCameraTest.CameraInfoNotEmpty
[       OK ] RgbCameraTest.CameraInfoNotEmpty (45 ms)
[----------] 5 tests from RgbCameraTest (510 ms total)

[----------] Global test environment tear-down
[==========] 5 tests from 1 test suite ran. (510 ms total)
[  PASSED  ] 5 tests.
```

### When no cameras are connected:
```
[==========] Running 5 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 5 tests from RgbCameraTest
[ RUN      ] RgbCameraTest.BasicEnumeration
No camera devices found!
[       OK ] RgbCameraTest.BasicEnumeration (10 ms)
...
[  PASSED  ] 5 tests.
```

## Troubleshooting

If tests fail or no cameras are found:

1. **Check camera connections** - Ensure cameras are properly connected via USB or network
2. **Verify drivers** - Make sure Hikvision MVS SDK is properly installed
3. **Check permissions** - Camera access may require special permissions on some systems
4. **Network cameras** - For GigE cameras, ensure network configuration is correct

## API Usage

Basic usage of the enumeration API:

```cpp
#include "rgb_camera.h"

// Enumerate all cameras
auto cameras = evrgb::enumerateAllCameras();

// Process each camera
for (const auto& camera : cameras) {
    std::cout << "Found: " << camera.manufacturer 
              << " (" << camera.serialNumber << ")" << std::endl;
}
```
