# TinyReflection

TinyReflection is a simple reflection library for Modern C++.

这是一个基于现代c++（c++20）的反射库，通过结构化绑定、模板元编程、编译器特性来反射出结构体元信息并完成Json结构的序列化和反序列化。

## 📚 目录

- [✨ 特性](#-特性)
- [📦 使用](#-使用)
- [🔭 TODO](#-TODO)


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

- 支持更多类型：`T[N]`、`std::array<T>`、`std::optional`、`std::variant`等
- 支持自定义结构体序列化方案
