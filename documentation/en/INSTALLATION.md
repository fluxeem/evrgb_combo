@page installation_guide Installation Guide


This guide walks you through installing and configuring EvRGB Combo SDK and its dependencies on Linux and Windows.

## 📋 Table of Contents

- [System Requirements](#-system-requirements)
- [Dependencies Overview](#-dependencies-overview)
- [Linux (Ubuntu) Install](#-linux-ubuntu-install)
- [Windows Install](#-windows-install)
- [Verify Installation](#-verify-installation)
- [FAQ](#-faq)

## 🔧 System Requirements

### Minimum
- **OS**:
  - Linux: Ubuntu 18.04+ / CentOS 7+ / RHEL 7+
  - Windows: Windows 10+ (64-bit)
- **Compiler**:
  - GCC 7+ or Clang 8+ (Linux)
  - Visual Studio 2019+ (Windows)
- **CMake**: 3.22 or newer
- **Memory**: 4GB RAM+
- **Storage**: 2GB free space+

### Recommended
- **OS**: Ubuntu 20.04+ / Windows 11
- **Compiler**: GCC 9+ or Visual Studio 2022
- **Memory**: 8GB RAM+
- **Storage**: 5GB free space+

## 📦 Dependencies Overview

EvRGB Combo SDK depends on these core components:

| Dependency | Version | Purpose | Difficulty |
|------------|---------|---------|------------|
| **OpenCV** | 4.x | Image processing | ⭐⭐ |
| **Hikrobot MVS SDK** | 4.6.0+ | RGB camera control | ⭐⭐⭐ |
| **DvsenseDriver** | Latest | DVS camera control | ⭐⭐⭐ |

## 🐧 Linux (Ubuntu) Install

### Step 1: Base tools

```bash
sudo apt update
sudo apt install -y build-essential cmake git

# Install newer CMake if needed
wget https://github.com/Kitware/CMake/releases/download/v3.28.0/cmake-3.28.0-linux-x86_64.sh
chmod +x cmake-3.28.0-linux-x86_64.sh
sudo ./cmake-3.28.0-linux-x86_64.sh --skip-license --prefix=/usr/local
```

### Step 2: Install dependencies

```bash
# OpenCV
sudo apt install -y libopencv-dev python3-opencv

# Hikrobot MVS SDK
# See: https://www.hikrobotics.com/cn/machinevision/service/download?module=0

# DvsenseDriver
# See: https://sdk.dvsense.com/zh/html/install_zh.html
```

### Step 3: Build EvRGB Combo SDK

```bash
git clone https://github.com/tonywangziteng/EvRGB_combo_sdk.git
cd EvRGB_combo_sdk

mkdir build && cd build
cmake .. -DBUILD_SAMPLES=ON -DBUILD_TESTS=OFF
make -j$(nproc)

# Optional install
sudo make install
```

## 🪟 Windows Install

### Step 1: Install Visual Studio

1. Download Visual Studio: https://visualstudio.microsoft.com/zh-hans/downloads/
2. Pick the **"Desktop development with C++"** workload.
3. Include:
   - MSVC v143 toolset
   - Windows 10/11 SDK
   - CMake tools

### Step 2: Install CMake

1. Download: https://cmake.org/download/
2. Choose **"Add CMake to the system PATH"** during setup.
3. Verify:
```cmd
cmake --version
```

### Step 3: Install OpenCV

#### Option A: Prebuilt (recommended)
1. Download OpenCV for Windows from https://opencv.org/releases/
2. Extract to `C:\opencv` (or another path).
3. Set env vars:
   - `OpenCV_DIR = C:\opencv\build`
   - Add to PATH: `C:\opencv\build\x64\vc16\bin`

#### Option B: vcpkg
```cmd
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

.\vcpkg install opencv:x64-windows
```

### Step 4: Install Hikrobot MVS SDK

1. Download: https://www.hikrobotics.com/cn/machinevision/service/download?module=0
2. Run the installer with defaults.
3. Note the install path (e.g., `C:\Program Files (x86)\MVS\Development\Libraries\win64`).

### Step 5: Install DvsenseDriver

1. Get the Windows package from the vendor.
2. Run the installer.
3. Note the install path.

### Step 6: Build EvRGB Combo SDK

1. Clone the repo:
```cmd
git clone https://github.com/tonywangziteng/EvRGB_combo_sdk.git
cd EvRGB_combo_sdk
```

2. Configure `CMakeUserPresets.json`:
   - Set `VCPKG_ROOT` if you use vcpkg for OpenCV.
   - Set `CMAKE_PREFIX_PATH` to your DvsenseDriver path (required).

Example:
```json
{
    "version": 8,
    "configurePresets": [
        {
            "name": "win64_release_local",
            "inherits": "win64_release",
            "environment": {
                "VCPKG_ROOT": "E:/Libs/vcpkg"
            },
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "CMAKE_PREFIX_PATH": "E:/Libs/DvsenseDriver;$env{CMAKE_PREFIX_PATH}"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "release_local",
            "inherits": "release",
            "configurePreset": "win64_release_local",
            "configuration": "Release"
        }
    ]
}
```

3. Build:
```cmd
cmake --preset win64_release_local
cmake --build --preset release_local
```

**Note**: MVS SDK reads its own env vars automatically; no manual path setup is required.

## ✅ Verify Installation

### Run sample

```bash
# Linux
cd build
./bin/camera_enum_example

# Windows
cd build
.\bin\Release\camera_enum_example.exe
```

### Expected output
```
EvRGB Combo SDK - Camera Enumeration Example
============================================
Found 2 RGB camera(s):
  Camera 0: Hikvision (Serial: HK123456789)
  Camera 1: Hikvision (Serial: HK987654321)

Found 1 DVS camera(s):
  DVS 0: Dvsense (Serial: DVS001)
```

## ❓ FAQ

**Q: OpenCV not found while configuring?**  
A: Check `OpenCV_DIR` is set correctly.

**Q: Hikrobot camera not detected?**  
A: Ensure the driver is installed, check USB/network, confirm permissions (sudo on Linux), and firewall for GigE.

**Q: DVS init fails?**  
A: Check DvsenseDriver version and device permissions and connections.

**Q: CMake configuration fails?**  
A: Verify CMake >= 3.22 and all dependency paths.

**Q: Runtime missing libraries?**  
A: On Linux, check `LD_LIBRARY_PATH`; on Windows, check PATH for DLLs.

## 📞 Getting Help

1. Review build/runtime logs.
2. Open a GitHub issue: https://github.com/tonywangziteng/EvRGB_combo_sdk/issues
3. See other docs in this directory.

---

## 🔄 Next Steps

1. Read the API docs (`docs/html/index.html`).
2. Browse examples in `sample/user_examples/`.
3. See the recording tutorial (English/Chinese) for data capture.
4. Start building your app!

*/
