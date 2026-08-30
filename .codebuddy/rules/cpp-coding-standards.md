---
description: C++ 编码规范与跨平台开发规则，适用于所有 C/C++ 源文件的编写与审查
globs:
  - "**/*.cpp"
  - "**/*.cc"
  - "**/*.cxx"
  - "**/*.h"
  - "**/*.hpp"
  - "**/*.hxx"
  - "**/*.inl"
alwaysApply: false
---

# C++ 编码规范与跨平台开发规则

## 1. 编码风格 — 遵循 Google C++ Style Guide

本项目代码与注释规范全面遵循 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)，以下为核心要点：

### 1.1 命名规范

- **文件名**：全小写，单词间用下划线分隔，如 `my_useful_class.cpp`、`my_useful_class.h`。
- **类型名（类、结构体、枚举、类型别名、模板参数）**：大驼峰（PascalCase），如 `MyClassName`、`UrlTable`。
- **变量名**：全小写加下划线，如 `table_name`、`num_errors`。
- **类成员变量**：末尾加下划线，如 `table_name_`、`num_entries_`。
- **结构体成员变量**：不加末尾下划线，如 `table_name`。
- **常量**：以 `k` 开头的大驼峰，如 `kMaxBufferSize`、`kDaysInWeek`。
- **函数名**：大驼峰（PascalCase），如 `AddTableEntry()`、`OpenFileOrDie()`。
- **命名空间**：全小写加下划线，如 `my_project`。
- **枚举值**：与常量命名一致，以 `k` 开头，如 `kOk`、`kErrorOutOfMemory`。
- **宏**：全大写加下划线，如 `MY_MACRO_THAT_SCARES_SMALL_CHILDREN`。

### 1.2 格式规范

- **缩进**：使用 2 个空格，禁止使用 Tab。
- **行宽**：每行不超过 80 个字符。
- **花括号**：左花括号不单独占一行（K&R 风格）。
- **指针和引用**：`*` 和 `&` 紧贴类型名，如 `int* ptr`，`const string& name`。
- **函数声明/定义**：返回类型与函数名在同一行；参数过长时合理换行并对齐。
- **条件语句**：`if`、`else`、`for`、`while` 后始终使用花括号，即使只有一行。
- **switch 语句**：`case` 标签不缩进，case 内容缩进 2 空格；非空 case 必须有 `break` 或 `[[fallthrough]]`。

### 1.3 注释规范

- **文件头注释**：每个文件顶部包含版权声明和简要文件说明。
- **类注释**：每个类声明前用 `//` 注释说明用途和关键设计决策。
- **函数注释**：声明处说明功能、参数含义和返回值；定义处可补充实现细节。
- **行内注释**：使用 `//`，位于代码上方或行尾（至少隔 2 个空格）。
- **TODO 注释**：格式为 `// TODO(username): 描述`。
- **注释语言**：注释统一使用中文或英文（项目内保持一致），推荐英文。

### 1.4 头文件规范

- **头文件保护**：使用 `#pragma once`（三大编译器均支持），或使用 `#ifndef` 守卫（格式：`PROJECT_PATH_FILE_H_`）。
- **include 顺序**（每组之间空一行）：
  1. 对应的 `.h` 文件（如 `foo.cpp` 先 include `foo.h`）
  2. C 系统头文件
  3. C++ 标准库头文件
  4. 第三方库头文件
  5. 本项目头文件
- **前向声明**：能用前向声明代替 `#include` 时，优先使用前向声明。

### 1.5 其他关键规范

- **命名空间**：禁止使用 `using namespace std;`，应使用具体的 `using` 声明或完全限定名。
- **智能指针**：优先使用 `std::unique_ptr`，按需使用 `std::shared_ptr`，避免裸指针所有权。
- **const 正确性**：能加 `const` 的地方都加 `const`。
- **auto**：仅在类型明显或过于冗长时使用 `auto`。
- **异常**：本项目不使用 C++ 异常（与 Google Style 一致），使用返回值或错误码处理错误。
- **RTTI**：避免使用 `dynamic_cast`，优先使用虚函数或其他设计模式替代。

---

## 2. 跨平台开发规则

本项目目标平台覆盖以下三大编译器：
- **MSVC**（Microsoft Visual C++，Windows）
- **GCC / g++**（GNU Compiler Collection，Linux / macOS 等）
- **Clang / clang++**（LLVM，macOS / Linux / Windows）

### 2.1 编译器检测宏

使用标准的编译器预定义宏进行平台隔离：

```cpp
// 编译器检测
#if defined(_MSC_VER)
  // MSVC 编译器专用代码
#elif defined(__clang__)
  // Clang 编译器专用代码（注意：Clang 也定义了 __GNUC__，所以必须先检测 Clang）
#elif defined(__GNUC__)
  // GCC 编译器专用代码
#else
  #error "Unsupported compiler. This project requires MSVC, GCC, or Clang."
#endif
```

> ⚠️ **关键**：`__clang__` 的检测必须在 `__GNUC__` 之前，因为 Clang 同时定义了 `__GNUC__`。

### 2.2 操作系统检测宏

```cpp
// 操作系统检测
#if defined(_WIN32) || defined(_WIN64)
  #define PLATFORM_WINDOWS 1
#elif defined(__linux__)
  #define PLATFORM_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
  #include <TargetConditionals.h>
  #define PLATFORM_MACOS 1
#else
  #error "Unsupported platform."
#endif
```

### 2.3 跨平台编码准则

- **优先使用标准 C++**：能用 C++ 标准库（C++17 及以上）解决的问题，禁止使用平台特定 API。例如：
  - 文件系统操作使用 `<filesystem>`（`std::filesystem`）。
  - 线程使用 `<thread>`、`<mutex>`、`<condition_variable>`。
  - 时间使用 `<chrono>`。
- **平台隔离封装**：当必须使用平台特定 API 时，封装到独立的平台抽象层（PAL）文件中：
  ```
  src/platform/
  ├── platform.h          // 统一接口声明
  ├── platform_win.cpp    // Windows (MSVC) 实现
  ├── platform_linux.cpp  // Linux (GCC) 实现
  └── platform_macos.cpp  // macOS (Clang) 实现
  ```
- **条件编译最小化**：`#if` / `#ifdef` 块应尽量短小，避免大段条件编译导致可读性下降。将平台差异代码提取为独立函数。
- **禁止平台特定类型泄露**：平台头文件（如 `<windows.h>`）不得出现在公共头文件中，仅限在 `.cpp` 实现文件中 include。

### 2.4 编译器差异注意事项

| 场景 | MSVC | GCC | Clang | 建议做法 |
|------|------|-----|-------|----------|
| 导出符号 | `__declspec(dllexport/dllimport)` | `__attribute__((visibility("default")))` | 同 GCC | 定义统一的 `EXPORT_API` 宏 |
| 对齐 | `__declspec(align(N))` | `__attribute__((aligned(N)))` | 同 GCC | 使用 C++11 `alignas(N)` |
| 不可达代码 | `__assume(0)` | `__builtin_unreachable()` | 同 GCC | 封装为 `UNREACHABLE()` 宏 |
| 强制内联 | `__forceinline` | `__attribute__((always_inline))` | 同 GCC | 封装为 `FORCE_INLINE` 宏 |
| 废弃声明 | `__declspec(deprecated)` | `__attribute__((deprecated))` | 同 GCC | 使用 C++14 `[[deprecated]]` |
| 打包结构体 | `#pragma pack(push, 1)` | `__attribute__((packed))` | 同 GCC | 封装为统一宏或使用 `#pragma pack`（三者均支持） |
| 禁用特定警告 | `#pragma warning(disable: XXXX)` | `#pragma GCC diagnostic ignored` | `#pragma clang diagnostic ignored` | 封装为 `DISABLE_WARNING_XXX` 宏对 |

### 2.5 推荐的跨平台宏定义模板

在项目中维护一个公共的平台宏头文件（如 `platform_macros.h`）：

```cpp
#pragma once

// ============================================================
// 编译器检测
// ============================================================
#if defined(_MSC_VER)
  #define COMPILER_MSVC 1
#elif defined(__clang__)
  #define COMPILER_CLANG 1
#elif defined(__GNUC__)
  #define COMPILER_GCC 1
#endif

// ============================================================
// 符号导出
// ============================================================
#if defined(COMPILER_MSVC)
  #define EXPORT_API __declspec(dllexport)
  #define IMPORT_API __declspec(dllimport)
#else
  #define EXPORT_API __attribute__((visibility("default")))
  #define IMPORT_API
#endif

// ============================================================
// 强制内联
// ============================================================
#if defined(COMPILER_MSVC)
  #define FORCE_INLINE __forceinline
#else
  #define FORCE_INLINE inline __attribute__((always_inline))
#endif

// ============================================================
// 不可达代码标记
// ============================================================
#if defined(COMPILER_MSVC)
  #define UNREACHABLE() __assume(0)
#else
  #define UNREACHABLE() __builtin_unreachable()
#endif

// ============================================================
// 警告控制
// ============================================================
#if defined(COMPILER_MSVC)
  #define DISABLE_WARNING_PUSH __pragma(warning(push))
  #define DISABLE_WARNING_POP  __pragma(warning(pop))
  #define DISABLE_WARNING(warningNumber) __pragma(warning(disable: warningNumber))
#elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
  #define DISABLE_WARNING_PUSH _Pragma("GCC diagnostic push")
  #define DISABLE_WARNING_POP _Pragma("GCC diagnostic pop")
  #define DISABLE_WARNING(warningName) _Pragma(#warningName)
#endif
```

### 2.6 构建系统要求

- 使用 **CMake**（≥ 3.16）作为构建系统，确保三个编译器下均可构建。
- 在 CMakeLists.txt 中设置统一的警告级别：
  ```cmake
  if(MSVC)
    add_compile_options(/W4 /WX)
  else()
    add_compile_options(-Wall -Wextra -Werror -Wpedantic)
  endif()
  ```
- CI 流水线必须覆盖 MSVC、GCC、Clang 三个编译器的构建和测试。

---

## 3. 代码审查检查清单

在提交代码或进行 Code Review 时，确认以下要点：

- [ ] 命名是否符合 Google C++ Style Guide
- [ ] 缩进为 2 空格，行宽不超过 80 字符
- [ ] 头文件 include 顺序正确
- [ ] 无 `using namespace std;`
- [ ] 新增代码在三个编译器下均可编译
- [ ] 平台特定代码已正确使用条件编译宏隔离
- [ ] 平台特定代码封装在 PAL 层，未泄露到公共接口
- [ ] 优先使用 C++ 标准库而非平台 API
- [ ] 智能指针使用正确，无裸指针所有权
- [ ] const 正确性
