
# 🧭 C++ 代码风格与命名规范

本文件定义本项目的 **标准代码风格与命名约定**。  
包括命名规则、文件结构、注释风格、空白规则等，  
供 **GitHub Copilot**、**团队成员**、**CI 工具** 使用与遵循。

Copilot 将根据本文件内容学习项目风格，  
从而在自动补全时遵守这些命名和格式规范。

---

## 1️⃣ 通用原则

1. 项目使用 **C++17 或更高标准**。
2. 所有代码使用 **UTF‑8 无 BOM** 编码。
3. 每个 `.h` 文件应有防重定义保护宏。
4. 每个类、函数和文件应有简短注释说明。
5. 优先使用现代 C++ 特性（`auto`、`constexpr`、`unique_ptr` 等），避免裸指针。
6. 所有命名应语义清晰，禁止使用无意义缩写。
7. 格式化统一使用 `.clang-format` 配置（见仓库根目录）。

---

## 2️⃣ 命名风格总表

| 类型 | 风格 | 示例 | 说明 |
|------|------|------|------|
| 命名空间 | 小写 + 下划线 | `namespace image_utils {}` | 不使用驼峰 |
| 类 / 结构体 / 枚举 | PascalCase | `class CameraManager;` | 每个词首字母大写 |
| 函数 / 方法 | camelCase | `openCamera();` | 动词打头 |
| 局部变量 | snake_case | `frame_index` | 小写 + 下划线 |
| 成员变量 | snake_case_ | `int frame_index_;` | 尾随下划线表示成员 |
| 常量 | ALL_CAPS | `const int MAX_COUNT = 10;` | 使用下划线分隔 |
| 枚举值 | ALL_CAPS | `enum class Mode { AUTO, MANUAL };` | 推荐 `enum class` |
| 模板参数 | PascalCase, 单词少 | `template<typename T>` | 避免 `T_type` |
| 文件名 / 目录名 | snake_case | `image_buffer.cpp` | 保持小写 |
| 测试文件 | `<target>_test.cpp` | `camera_test.cpp` | 与被测单元对应 |

---

## 3️⃣ 类与文件结构

### 文件组织
- 每个类对应一个 `.h` + `.cpp` 文件；
- 公有接口头文件放入 `include/`，实现文件放入 `src/`；
- 测试文件放入 `tests/`；
- 内部工具类放入 `detail/` 子目录。

### 头文件示范
```cpp
// camera_manager.h
#ifndef PROJECT_CAMERA_MANAGER_H_
#define PROJECT_CAMERA_MANAGER_H_

#include <string>

namespace vision {

class CameraManager {
public:
    CameraManager();
    ~CameraManager();

    bool open(const std::string& device);
    void close();
    int frameCount() const;

private:
    int frame_count_;
    std::string device_;
};

}  // namespace vision

#endif  // PROJECT_CAMERA_MANAGER_H_
```

### 源文件示范
```cpp
// camera_manager.cpp
#include "vision/camera_manager.h"

namespace vision {

CameraManager::CameraManager() : frame_count_(0) {}

bool CameraManager::open(const std::string& device) {
    device_ = device;
    return true;
}

void CameraManager::close() {
    device_.clear();
}

int CameraManager::frameCount() const {
    return frame_count_;
}

}  // namespace vision
```

---

## 4️⃣ 函数与命名指南

- 函数名应为**动宾结构**（动词 + 名词）。
  - ✅ `loadImage()`, `computeHash()`
  - ❌ `imageLoader()`, `hashCompute()`
- 返回布尔类型的函数：
  - `isXxx()`, `hasXxx()`, `canXxx()`, `shouldXxx()`
- 回调函数以 `on` 开头：
  - `onFrameCaptured()`, `onConnectionLost()`
- Getter / Setter:
  ```cpp
  void setWidth(int width) { width_ = width; }
  int width() const { return width_; }
  ```

---

## 5️⃣ 类成员规范

- 私有成员以下划线结尾：`count_`, `name_`
- 静态成员变量或常量使用 `ALL_CAPS`
- 优先使用构造函数初始化列表
- 禁止使用未初始化成员变量
- 析构函数必须显式 `virtual` 当类可被继承时

---

## 6️⃣ 注释规范

### 文件头注释
```cpp
// Copyright (c) 2025.
// Author: <your name or org>
// Description: 定义相机管理类，用于视频捕捉与帧读取。
```

### 类与函数注释
- 用 `///` 或 `//` 简要说明接口功能（用于 Doxygen）。
- 参数和返回值注明用途：

```cpp
/// 打开指定设备
/// @param device 设备名称
/// @return 返回是否打开成功
bool open(const std::string& device);
```

### 逻辑注释
- 数据结构或复杂逻辑前必须注明目的：
  ```cpp
  // 使用缓冲区队列管理最近的帧
  // 避免重复分配内存
  ```

---

## 7️⃣ 空白与缩进

- 使用 **4 个空格** 缩进，不使用制表符。
- 每行最长不得超过 **100 列**。
- 控制流括号风格（Allman）：
  ```cpp
  if (condition)
  {
      doSomething();
  }
  else
  {
      handleError();
  }
  ```
- 逻辑块之间空一行；函数之间空两行。

---

## 8️⃣ 错误处理与异常

- 禁止使用裸指针手动 `new/delete`。
- 优先使用 `unique_ptr`、`shared_ptr`。
- 对外接口发生错误时，使用异常或 error code。
- RAII 原则：资源在析构时自动释放。

---

## 9️⃣ 测试规范

- 所有单元测试文件放入 `tests/`。
- 每个测试文件对应一个目标源文件。
- 测试函数命名：`TEST(SuiteName, CaseDescription)`
- 示例：

```cpp
TEST(CameraManagerTest, OpenAndClose)
{
    vision::CameraManager cam;
    EXPECT_TRUE(cam.open("mock"));
    cam.close();
}
```

---

## 🔟 Copilot 提示区（供模型学习）

> ✨ 对 GitHub Copilot 的指导：
>
> - 遵循以上命名规则生成变量与函数。
> - 优先使用现代 C++（`auto`, `std::unique_ptr`, `constexpr`）。
> - 不自动生成裸指针、C 风格数组、宏定义逻辑。
> - 类成员变量使用尾部下划线。
> - 所有测试函数按 GoogleTest 风格生成。
> - 缩进采用 4 空格，花括号独立成行（Allman 风格）。
> - 函数注释使用 Doxygen 风格简要描述。
> - 文件与类名保持一一对应：`foo_bar.h` / `foo_bar.cpp`。

---

## ✅ 附录：快速风格示范

| 类型 | 示例 |
|------|------|
| 命名空间 | `namespace vision::core {}` |
| 类 | `class ImageBuffer {};` |
| 成员函数 | `bool loadImage(const std::string& path);` |
| 成员变量 | `int width_;` |
| 局部变量 | `int frame_count = 0;` |
| 常量 | `constexpr int MAX_BUFFER = 256;` |
| 枚举 | `enum class State { IDLE, RUNNING, STOPPED };` |
| 文件名 | `image_buffer.cpp`, `camera_manager.h` |
| 测试文件 | `camera_manager_test.cpp` |

---

> 💡 **一旦提交此文件，Copilot 会调整生成风格匹配你的约定。**  
> 请将此文件命名为 `CODE_STYLE.md` 并放于仓库根路径。
