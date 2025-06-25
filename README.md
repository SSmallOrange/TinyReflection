# TinyReflection

TinyReflection is a simple reflection library for Modern C++.

这是一个基于现代c++（c++20）实现的反射库，核心通过结构化绑定（聚合结构体）、编译器特性来反射出结构体元信息并完成序列化

## ✨ 特性

- ✅ 基于 C++20 `Concepts` 和 `结构化绑定` 的零依赖反射机制
- ✅ 支持结构体成员的自动 JSON 序列化（TODO：反序列化）
- ✅ 支持以下成员类型：
  - `std::string`
    - `int`
    - `float`
    - `double`
    - `char`
    - `char*` / `const char*`
    - `嵌套struct`
    - `std::vector`, `std::list`, `std::deque`
    - `std::map<std::string, T>`、`std::unordered_map<std::string, T>` 

## 📦 使用

TinyReflection是headonly的，直接将 `tinyrefl` 文件夹加入你的项目中并 `#include` 即可

目前只支持`MSVC19+`，通过执行build.py能够编译`test`下简单的测试文件，顺序阅读测试文件能够快速了解实现原理。

**使用示例：**

```c++
#include "tinyrefl/reflection_to_json.hpp"

struct Person {
    std::string name;
    int age;
    std::map<std::string, std::string> meta;
};

int main() {
    Person p{"Alice", 30, {{"role", "admin"}}};
    std::string json;
    tinyrefl::reflection_to_json(p, json);
    std::cout << json << std::endl;
}

```

**输出：**

```json
{"name":"Alice","age":30,"meta":{"role":"admin"}}
```

## 🚧 待完善

- ❌ 支持的类型较少。
- ❌ 只支持聚合类型，即能够支持大括号初始化的类型，当类型内含有：const成员、自定义构造、虚函数等元素时都会破坏类型的聚合特性。
- 对于关联容器，只支持字符串类型作为容器的key值。

## 🔭 TODO

- 支持跨平台编译（GCC、Clang）
- 支持反序列化。
- 支持更多类型：`T[N]`、`std::array<T>`等。