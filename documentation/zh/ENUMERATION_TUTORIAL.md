@page enumeration_tutorial 设备枚举教程

本教程介绍如何使用 EvRGB Combo SDK 枚举系统中可用的 RGB 与 DVS 相机设备。

## 概述

在使用 SDK 前，先确认系统中有哪些相机已连接。SDK 提供枚举接口获取所有可用设备的详细信息。

## 枚举示例代码

### 基本枚举示例

```cpp
#include "core/combo.h"
#include <iostream>

int main() {
    std::cout << "EvRGB Combo SDK - 相机枚举示例" << std::endl;
    
    // 枚举所有相机设备
    auto [rgb_camera_infos, dvs_camera_infos] = evrgb::enumerateAllCameras();

    // 显示 RGB 相机信息
    std::cout << "发现 " << rgb_camera_infos.size() << " 台 RGB 相机:" << std::endl;
    for (size_t i = 0; i < rgb_camera_infos.size(); ++i) {
        const auto& info = rgb_camera_infos[i];
        std::cout << "  相机 " << i << ":" << std::endl;
        std::cout << "    制造商: " << info.manufacturer << std::endl;
        std::cout << "    序列号: " << info.serial_number << std::endl;
        std::cout << "    分辨率: " << info.width << "x" << info.height << std::endl;
        std::cout << "    型号: " << info.model_name << std::endl;
    }   
   
    // 显示 DVS 相机信息
    std::cout << "发现 " << dvs_camera_infos.size() << " 台 DVS 相机:" << std::endl;
    for (size_t i = 0; i < dvs_camera_infos.size(); ++i) {
        const auto& info = dvs_camera_infos[i];
        std::cout << "  DVS " << i << ":" << std::endl;
        std::cout << "    制造商: " << info.manufacturer << std::endl;
        std::cout << "    序列号: " << info.serial << std::endl;
        std::cout << "    型号: " << info.model_name << std::endl;
    }

    return 0;
}
```

### 单独枚举示例

如果只需要枚举特定类型的相机，可以使用单独的枚举函数：

```cpp
#include "core/combo.h"
#include <iostream>

int main() {
    // 只枚举 RGB 相机
    auto rgb_cameras = evrgb::enumerateAllRgbCameras();
    std::cout << "RGB 相机数量: " << rgb_cameras.size() << std::endl;
    
    // 只枚举 DVS 相机
    auto dvs_cameras = evrgb::enumerateAllDvsCameras();
    std::cout << "DVS 相机数量: " << dvs_cameras.size() << std::endl;

    return 0;
}
```

## 构建和运行

### 构建示例

```bash
# 创建构建目录
mkdir -p build
cd build

# 配置 CMake（启用示例程序）
cmake .. -DBUILD_SAMPLES=ON

# 编译枚举示例
cmake --build . --target camera_enum_example --config Release
```

### 运行示例

```bash
# Linux/macOS
./bin/camera_enum_example

# Windows
.\bin\Release\camera_enum_example.exe
```

## 预期输出

成功运行后，你应该看到类似以下的输出：

```
EvRGB Combo SDK - 相机枚举示例
发现 1 台 RGB 相机:
  相机 0:
    制造商: Hikvision
    序列号: HK123456789
    分辨率: 1920x1080
    型号: MV-CE200-10UC
发现 1 台 DVS 相机:
  DVS 0:
    制造商: Dvsense
    序列号: DVS001
    型号: DVXplorer
```

## 设备信息结构

### RGB 相机信息

`evrgb::RgbCameraInfo` 结构包含以下字段：

- `manufacturer`: 制造商名称（如 "Hikvision"）
- `model_name`: 相机型号
- `serial_number`: 相机序列号（用于初始化）
- `width`: 图像宽度（像素）
- `height`: 图像高度（像素）

### DVS 相机信息

`evrgb::DvsCameraInfo` 结构包含以下字段：

- `manufacturer`: 制造商名称（如 "Dvsense"）
- `model_name`: 相机型号
- `serial`: 相机序列号（用于初始化）

## 常见问题

### Q: 没有检测到相机

**可能原因**：
1. 相机未正确连接
2. 驱动程序未安装
3. 权限不足（Linux 上可能需要 sudo）
4. 防火墙阻止（网络相机）

**解决方法**：
1. 检查物理连接
2. 重新安装相机驱动
3. Linux 上使用 sudo 运行或配置 udev 规则
4. 检查防火墙设置

### Q: 枚举返回空列表

**排查步骤**：
1. 确认相机驱动已正确安装
2. 检查相机电源和连接
3. 验证 SDK 依赖项是否正确安装
4. 查看 SDK 日志输出

### Q: 序列号不正确

**注意事项**：
- RGB 相机使用 `serial_number` 字段
- DVS 相机使用 `serial` 字段
- 序列号是区分大小写的

## 下一步

完成枚举后你可以：

1. 选择特定相机进行初始化
2. 查看相机使用教程学习相机操作
3. 参考参数调整教程配置参数
4. 学习数据录制与回放

## 相关文档

- [相机使用教程](CAMERA_USAGE_TUTORIAL.md)
- [参数调整教程](PARAMETER_TUNING_TUTORIAL.md)
- [安装指南](INSTALLATION.md)

*/