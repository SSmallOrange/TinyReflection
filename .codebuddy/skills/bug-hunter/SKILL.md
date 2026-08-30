---
name: bug-hunter
description: >
  C++ 序列化/反序列化库的自动化 Bug 狩猎技能。适用于用户要求对 TinyReflection
  库进行测试、查找 bug、验证序列化/反序列化正确性的场景。当用户提到
  "找 bug"、"测试序列化"、"roundtrip 测试"、"反序列化问题"、"JSON 正确性"
  等关键词时触发。
---

# Bug Hunter — TinyReflection 序列化/反序列化 Bug 狩猎技能

## 目的

本技能为 TinyReflection 库提供系统化的 Bug 狩猎流程。通过分层测试策略，从序列化/反序列化的正确性入手，逐步覆盖边界条件、异常输入、类型组合等维度，高效发现和定位 bug。

## 触发条件

以下情况应激活本技能：

- 用户要求对库进行测试或找 bug
- 用户提到序列化/反序列化的正确性验证
- 用户提到 roundtrip（往返）测试
- 用户提到 JSON 解析/生成问题
- 用户提到边界条件、fuzzing、属性测试

## 工作流程

### Phase 1：环境准备

1. 确认构建系统可用：在项目根目录执行 `cmake -B build && cmake --build build`
2. 确认现有测试全部通过：执行 `cd build && ctest --output-on-failure`
3. 阅读 `test/CMakeLists.txt`，了解已注册的测试列表和 `add_tinyrefl_test` 辅助函数

### Phase 2：理解库的序列化/反序列化实现

在编写测试前，先阅读关键源文件以理解实现细节：

| 文件 | 关注点 |
|------|--------|
| `tinyrefl/reflection_to_json.hpp` | `to_json_value` 各类型重载、字符串转义逻辑、容器处理 |
| `tinyrefl/reflection_from_json.hpp` | SAX handler 栈机制、字段匹配逻辑、错误处理路径 |
| `tinyrefl/utils/reflection_utils.hpp` | 类型特征判断（`is_string_v`、`is_custom_type_v` 等）、`ignore<T>` 定义 |
| `tinyrefl/utils/reflection_get_tuple.hpp` | 成员探测、结构化绑定、偏移量计算 |

查阅 `references/known-pitfalls.md` 了解此类库的常见易错点。

### Phase 3：编写测试 — 分层策略

按以下优先级从高到低编写测试，参考 `references/test-strategies.md` 获取详细策略：

#### 层级 1：Roundtrip 正确性（最高优先级）

对每种支持的类型执行 **序列化 → 反序列化 → 逐字段比较** 验证：

- 基本类型：`int`、`int64_t`、`double`、`float`、`bool`、`char`、`std::string`
- 容器类型：`std::vector<T>`、`std::list<T>`、`std::deque<T>`、`std::map<K,V>`、`std::unordered_map<K,V>`
- 嵌套结构体：含自定义类型成员的聚合类型
- 组合类型：容器内嵌结构体、结构体内嵌容器

#### 层级 2：边界值和特殊值

- 数值极值：`INT_MAX`、`INT_MIN`、`INT64_MAX`、`INT64_MIN`、`DBL_MAX`、`DBL_MIN`、`0`、`-0.0`
- 浮点特殊值：`inf`、`-inf`、`NaN`（已知问题区域）
- 字符串边界：空字符串 `""`、含转义字符（`\n`、`\t`、`\"`、`\\`）、Unicode（中文、emoji）、含 null 字节
- 容器边界：空容器、单元素、大量元素
- 结构体边界：全默认值、全极值

#### 层级 3：JSON 格式合规性

- 序列化输出是否为合法 JSON（使用 RapidJSON DOM 解析验证）
- key 是否正确转义
- 嵌套深度是否正确
- 数组/对象括号是否匹配

#### 层级 4：反序列化鲁棒性

- 畸形 JSON：缺少引号、多余逗号、括号不匹配
- 类型不匹配：字符串字段给数字、数字字段给字符串
- 缺少字段：JSON 中少于结构体成员
- 多余字段：JSON 中包含结构体没有的字段
- 空输入：`""`、`"{}"`、`"null"`、`"[]"`
- 超大数值溢出

#### 层级 5：属性测试（Property-Based Testing）

使用确定性 PRNG（如 SplitMix64）批量生成随机数据，执行大量 roundtrip 验证（建议 2000+ 轮）：

```cpp
struct SplitMix64 {
    uint64_t state;
    explicit SplitMix64(uint64_t seed) : state(seed) {}
    uint64_t next() {
        state += 0x9e3779b97f4a7c15ULL;
        uint64_t z = state;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }
};
```

### Phase 4：测试文件约定

每个测试文件遵循以下模板：

```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tinyrefl/reflection_to_json.hpp"
#include "tinyrefl/reflection_from_json.hpp"
#include <string>
#include <vector>
// ... 其他需要的头文件

// 在文件顶部定义专用测试结构体
struct TestStruct {
    int field1;
    std::string field2;
};

TEST_CASE("描述测试意图") {
    // Arrange
    TestStruct original{42, "hello"};

    // Act: 序列化
    std::string json;
    tinyrefl::reflection_to_json(original, json);

    // Act: 反序列化
    TestStruct restored{};
    auto status = tinyrefl::reflection_from_json(restored, json);

    // Assert
    CHECK(status.ok);
    CHECK(original.field1 == restored.field1);
    CHECK(original.field2 == restored.field2);
}
```

文件命名：`test/test_<描述性名称>.cpp`

注册测试：在 `test/CMakeLists.txt` 中添加 `add_tinyrefl_test(test_<描述性名称>)`

### Phase 5：执行与验证

1. 构建新测试：`cmake --build build`
2. 运行单个测试：`./bin/test_<name>` 或 `cd build && ctest -R test_<name> --output-on-failure`
3. 运行全部测试：`cd build && ctest --output-on-failure`

### Phase 6：Bug 处理

发现 bug 时：

1. **最小化复现**：将失败用例精简到最小可复现代码
2. **定位根因**：阅读相关源文件，定位导致错误的具体代码
3. **修复 bug**：在库源文件中修改（不在测试中 workaround）
4. **回归验证**：确保修复后所有测试（包括新测试和旧测试）全部通过
5. **停止**：修复一个 bug 后停止本轮狩猎，等待用户 review

## 重要约束

- **不提交代码**：不执行 `git add`、`git commit` 或 `git push`
- **逐个修复**：每轮只修复一个 bug，修复后停止
- **保留测试**：即使 bug 已修复，测试文件也保留作为回归测试
- **不修改测试框架**：不修改 `doctest.h`
- **中文注释**：测试代码中的注释使用中文
