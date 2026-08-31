# TinyReflection

TinyReflection is a simple reflection library for Modern C++.

这是一个基于现代c++（c++20）的反射库，通过结构化绑定、模板元编程、编译器特性来反射出结构体元信息并完成Json结构的序列化和反序列化。

## 📚 目录

- [✨ 特性](#-特性)
- [📦 使用](#-使用)
- [🔭 TODO](#-todo)
  - [🔴 P0 — 基础类型补齐](#-p0--基础类型补齐)
  - [🟠 P1 — 易用性增强](#-p1--易用性增强)
  - [🟡 P2 — 扩展性建设](#-p2--扩展性建设)
  - [🟢 P3 — 实用能力](#-p3--实用能力)
  - [🔵 P4 — 生态与前沿](#-p4--生态与前沿)


## ✨ 特性

- ✅ 基于 C++20 和 `结构化绑定` 的**零依赖**反射机制
- ✅ 支持结构体成员的**自动 JSON 序列化和反序列化**
  - 支持嵌套的json数组格式
  - 支持成员字段忽略处理
- ✅ 支持跨平台编译（`MSVC 19+`、`GCC 11.3+`）
- ✅ 支持以下成员类型：
  - `std::string`
  - `int`
  - `bool`
  - `float`
  - `double`
  - `char` 、`char*` 、`const char*`
  - `嵌套struct`
  - `std::vector`, `std::list`, `std::deque`
  - `std::map<std::string, T>`、`std::unordered_map<std::string, T>` 

## 📦 使用

TinyReflection是headonly的，直接将 `tinyrefl` 文件夹加入你的项目中并 `#include` 即可

通过执行build.py能够编译`test`下简单的测试文件，顺序阅读测试文件能够快速了解实现原理。

**使用示例：**

```c++
#include "tinyrefl/reflection_to_json.hpp"
#include "tinyrefl/reflection_from_json.hpp"

struct Inner {
    int id;
    string label;
};

struct Config {
    bool flag;
    double ratio;
    vector<int> values;
    Inner inner;
    vector<Inner> inner_list;
};

struct Complex {
    string name;
    Config config;
    vector<vector<int>> matrix;
    vector<vector<Inner>> inner_matrix;
    tinyrefl::ignore<std::shared_ptr<Inner>> ptr;  // 被忽略
};

int main() {
    const char* json = R"(
        {
            "name": "TestComplex",
            "config": {
                "flag": true,
                "ratio": 3.1415,
                "values": [10, 20, 30],
                "inner": {
                "id": 42,
                "label": "InnerLabel"
                },
                "inner_list": [
                { "id": 1, "label": "A" },
                { "id": 2, "label": "B" },
                { "id": 3, "label": "C" }
                ]
            },
            "matrix": [
                [1, 2, 3],
                [4, 5, 6]
            ],
            "inner_matrix": [
                [
                { "id": 101, "label": "X" },
                { "id": 102, "label": "Y" }
                ],
                [
                { "id": 201, "label": "Z" },
                { "id": 202, "label": "W" }
                ]
            ]
        }
    )";
    Complex obj;
    tinyrefl::reflection_from_json(obj, json);
    std::string out;
    tinyrefl::reflection_to_json(obj, out);
    std::cout << "after:\n" << out << std::endl;

    assert(obj.ptr.get() == nullptr);  // ptr 被忽略，保持默认值 nullptr
    return 0;
}
```

**输出：**

```json
{"name":"TestComplex","config":{"flag":true,"ratio":3.141500,"values":[10,20,30],"inner":{"id":42,"label":"InnerLabel"},"inner_list":[{"id":1,"label":"A"},{"id":2,"label":"B"},{"id":3,"label":"C"}]},"matrix":[[1,2,3],[4,5,6]],"inner_matrix":[[{"id":101,"label":"X"},{"id":102,"label":"Y"}],[{"id":201,"label":"Z"},{"id":202,"label":"W"}]]}
```


## 🔭 TODO

### 🔴 P0 — 基础类型补齐

- [ ] 支持 `std::optional<T>`：序列化时 `nullopt → null`，反序列化时 `null → nullopt`
- [ ] 支持 `enum` / `enum class`：提供整数模式和字符串模式（利用 `__PRETTY_FUNCTION__` 编译期提取枚举值名称）
- [ ] 支持 `std::array<T, N>` / `T[N]`：序列化为 JSON array，反序列化时做长度校验
- [ ] 支持 `std::variant<Ts...>`：采用 tagged union 的 JSON 表示（如 `{"type": "A", "value": {...}}`）

### 🟠 P1 — 易用性增强

- [ ] 缺失字段策略：提供宽松模式（保留默认值）和严格模式（缺少必填字段时报错），配合 `std::optional` 缺失字段自动设为 `nullopt`
- [ ] 字段重命名 / 别名：支持 C++ 成员名与 JSON key 不一致的场景（如 `snake_case` ↔ `camelCase`）
- [ ] 继承支持：提供轻量机制声明基类关系，序列化时先处理基类字段再处理派生类字段

### 🟡 P2 — 扩展性建设

- [ ] 自定义序列化钩子：通过 ADL 扩展点让用户为第三方类型（如 `std::chrono::time_point`、`glm::vec3` 等）提供序列化/反序列化实现
- [ ] 多序列化格式支持：将反射遍历与格式输出解耦，设计 `Serializer` concept，在 JSON 基础上扩展 MessagePack / CBOR 等二进制格式
- [ ] 编译期 JSON Schema 生成：利用已有反射信息自动生成 [JSON Schema](https://json-schema.org/)，用于接口文档化和数据校验

### 🟢 P3 — 实用能力

- [ ] Diff / Patch：支持结构体差异对比与差异应用，适用于配置热更新、状态同步等场景
- [ ] 性能优化：探索 SIMD JSON 解析（如 simdjson）、`constexpr` 友好结构的编译期 JSON 生成、`std::string_view` 字段的 Zero-copy 反序列化

### 🔵 P4 — 生态与前沿

- [ ] 包管理器发布：发布到 vcpkg / conan / xmake，提供 `TinyReflectionConfig.cmake` 和单头文件合并版本
- [ ] C++26 静态反射兼容：关注 P2996 提案进展，设计兼容层使底层实现可平滑切换到语言原生反射
