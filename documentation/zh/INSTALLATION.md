@page installation_guide 安装指南

本指南说明如何在 Linux 与 Windows 上安装并配置 EvRGB Combo SDK 及其依赖。

## 📋 目录

- [系统要求](#系统要求)
- [依赖项概述](#依赖项概述)
- [Linux (Ubuntu) 安装](#linux-ubuntu-安装)
- [Windows 安装](#windows-安装)
- [验证安装](#验证安装)
- [常见问题](#常见问题)

## 🔧 系统要求

### 最低要求
- **操作系统**: 
  - Linux: Ubuntu 18.04+ / CentOS 7+ / RHEL 7+
  - Windows: Windows 10+ (64位)
- **编译器**: 
  - GCC 7+ 或 Clang 8+ (Linux)
  - Visual Studio 2019+ (Windows)
- **CMake**: 3.22 或更高版本
- **内存**: 至少 4GB RAM
- **存储**: 至少 2GB 可用空间

### 推荐配置
- **操作系统**: Ubuntu 20.04+ / Windows 11
- **编译器**: GCC 9+ 或 Visual Studio 2022
- **内存**: 8GB+ RAM
- **存储**: 5GB+ 可用空间

## 📦 依赖项概述

EvRGB Combo SDK 依赖以下核心组件：

| 依赖项 | 版本要求 | 用途 | 安装难度 |
|--------|----------|------|----------|
| **OpenCV** | 4.x | 图像处理 | ⭐⭐ |
| **海康机器人 MVS SDK** | 4.6.0+ | RGB 相机控制 | ⭐⭐⭐ |
| **DvsenseDriver** | 最新版 | DVS 相机控制 | ⭐⭐⭐ |

## 🐧 Linux (Ubuntu) 安装

### 步骤 1: 安装基础工具

```bash
sudo apt update
sudo apt install -y build-essential cmake git

# 安装新版 CMake（如有需要）
wget https://github.com/Kitware/CMake/releases/download/v3.28.0/cmake-3.28.0-linux-x86_64.sh
chmod +x cmake-3.28.0-linux-x86_64.sh
sudo ./cmake-3.28.0-linux-x86_64.sh --skip-license --prefix=/usr/local
```

### 步骤 2: 安装依赖

```bash
# 安装 OpenCV
sudo apt install -y libopencv-dev python3-opencv

# 安装海康机器人 MVS SDK
# 请参考 https://www.hikrobotics.com/cn/machinevision/service/download?module=0

# 安装 DvsenseDriver  
# 请参考 https://sdk.dvsense.com/zh/html/install_zh.html
```

### 步骤 3: 编译 EvRGB Combo SDK

```bash
git clone https://github.com/tonywangziteng/EvRGB_combo_sdk.git
cd EvRGB_combo_sdk

mkdir build && cd build
cmake .. -DBUILD_SAMPLES=ON -DBUILD_TESTS=OFF
make -j$(nproc)

# 可选：安装
sudo make install
```

## 🪟 Windows 安装

### 步骤 1: 安装 Visual Studio

1. 下载 https://visualstudio.microsoft.com/zh-hans/downloads/
2. 安装时选择 **"使用 C++ 的桌面开发"** 工作负载
3. 确保包含：
   - MSVC v143 编译器
   - Windows 10/11 SDK
   - CMake 工具

### 步骤 2: 安装 CMake

1. 下载 https://cmake.org/download/
2. 安装时选择 **"Add CMake to the system PATH"**
3. 验证：
```cmd
cmake --version
```

### 步骤 3: 安装 OpenCV

#### 选项 A: 预编译版（推荐）
1. 下载 https://opencv.org/releases/
2. 解压至 `C:\opencv`（或自定义路径）
3. 设置环境变量：
   - `OpenCV_DIR = C:\opencv\build`
   - PATH 追加 `C:\opencv\build\x64\vc16\bin`

#### 选项 B: vcpkg
```cmd
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

.\vcpkg install opencv:x64-windows
```

### 步骤 4: 安装海康机器人 MVS SDK

1. 下载 https://www.hikrobotics.com/cn/machinevision/service/download?module=0
2. 使用默认选项安装
3. 记录安装路径（如 `C:\Program Files (x86)\MVS\Development\Libraries\win64`）

### 步骤 5: 安装 DvsenseDriver

1. 获取 Windows 版安装包
2. 运行安装程序
3. 记录安装路径

### 步骤 6: 编译 EvRGB Combo SDK

1. 克隆项目：
```cmd
git clone https://github.com/tonywangziteng/EvRGB_combo_sdk.git
cd EvRGB_combo_sdk
```

2. 配置 `CMakeUserPresets.json`：
   - 使用 vcpkg 时设置 `VCPKG_ROOT`
   - 必填：将 `CMAKE_PREFIX_PATH` 指向你的 DvsenseDriver 路径

示例：
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

3. 编译：
```cmd
cmake --preset win64_release_local
cmake --build --preset release_local
```

**注意**: MVS SDK 会自动读取环境变量，无需手动配置路径。

## ✅ 验证安装

### 运行示例程序

```bash
# Linux
cd build
./bin/camera_enum_example

# Windows
cd build
.\bin\Release\camera_enum_example.exe
```

### 预期输出
```
EvRGB Combo SDK - Camera Enumeration Example
============================================
Found 2 RGB camera(s):
  Camera 0: Hikvision (Serial: HK123456789)
  Camera 1: Hikvision (Serial: HK987654321)

Found 1 DVS camera(s):
  DVS 0: Dvsense (Serial: DVS001)
```

## ❓ 常见问题

### Q: 编译时提示找不到 OpenCV
**A**: 检查 OpenCV 安装路径是否正确，确保 `OpenCV_DIR` 环境变量指向正确的目录。

### Q: 海康相机无法识别
**A**: 
1. 确保相机驱动已正确安装
2. 检查 USB 或网络连接
3. 确认相机权限（Linux 上可能需要 sudo）
4. 检查防火墙设置（GigE 相机）

### Q: DVS 相机初始化失败
**A**:
1. 确认 DvsenseDriver 版本兼容性
2. 检查设备权限
3. 验证硬件连接

### Q: CMake 配置失败
**A**:
1. 确保 CMake 版本 >= 3.22
2. 检查所有依赖项路径是否正确
3. 查看 CMake 输出中的具体错误信息

### Q: 运行时出现库文件找不到
**A**:
- **Linux**: 检查 `LD_LIBRARY_PATH` 环境变量
- **Windows**: 检查 PATH 环境变量，确保包含所有 DLL 路径

## 📞 获取帮助

如果遇到安装问题：

1. 查看日志：检查构建和运行时的详细错误信息
2. GitHub Issues: https://github.com/tonywangziteng/EvRGB_combo_sdk/issues
3. 文档：查看项目文档目录中的其他文档

---

## 🔄 下一步

1. 阅读 API 文档（`docs/html/index.html`）
2. 查看示例代码（`sample/user_examples/`）
3. 参考录制教程（中/英）
4. 开始开发你的应用！

*/
