# 序列化/反序列化库测试策略详解

## 1. Roundtrip 测试（往返测试）

Roundtrip 测试是序列化库最基本也最有效的测试方法。核心思想：

```
原始对象 → 序列化 → JSON 字符串 → 反序列化 → 还原对象 → 与原始对象逐字段比较
```

### 1.1 基本类型 Roundtrip

针对每种基本类型单独测试，确保序列化/反序列化的无损性：

| 类型 | 关键测试值 |
|------|-----------|
| `int` | `0`, `1`, `-1`, `INT_MAX`, `INT_MIN`, `42`, `-42` |
| `int64_t` | `0`, `INT64_MAX`, `INT64_MIN`, `INT_MAX + 1LL`, `INT_MIN - 1LL` |
| `double` | `0.0`, `-0.0`, `1.5`, `-1.5`, `DBL_MAX`, `DBL_MIN`, `DBL_EPSILON`, `1e308`, `5e-324` |
| `float` | `0.0f`, `1.5f`, `FLT_MAX`, `FLT_MIN`, `FLT_EPSILON` |
| `bool` | `true`, `false` |
| `char` | `'a'`, `'\0'`, `'\n'`, `'\t'`, `'\"'`, `'\\'`, 中文字符（如果支持）|
| `std::string` | `""`, `"hello"`, `"含中文"`, `"with\"quote"`, `"with\\slash"`, 含控制字符 |

### 1.2 容器类型 Roundtrip

| 容器 | 关键测试场景 |
|------|-------------|
| `vector<int>` | 空 `{}`、单元素 `{1}`、多元素 `{1,2,3}`、大量元素 |
| `vector<string>` | 含空字符串、含特殊字符的字符串 |
| `vector<CustomType>` | 嵌套自定义结构体 |
| `list<T>` / `deque<T>` | 同 vector 策略 |
| `map<string, int>` | 空 map、单键值对、多键值对、key 含特殊字符 |
| `map<string, CustomType>` | value 为自定义类型 |
| `unordered_map<string, T>` | 同 map，但需注意序列化顺序不确定 |

### 1.3 嵌套结构 Roundtrip

```
struct Inner { int x; string y; };
struct Outer { Inner inner; vector<Inner> list; map<string, Inner> dict; };
```

关注嵌套深度为 2-3 层的情况，超深嵌套可能暴露栈溢出或递归问题。

## 2. 序列化输出验证

不仅验证 roundtrip 正确性，还需验证序列化输出本身的正确性：

### 2.1 JSON 合法性检查

使用独立 JSON 解析器（如 RapidJSON Document）解析序列化输出，确保是合法 JSON：

```cpp
rapidjson::Document doc;
doc.Parse(json.c_str());
CHECK_FALSE(doc.HasParseError());
```

### 2.2 结构正确性检查

验证 JSON 结构与预期一致：

- 对象的 key 是否与结构体成员名匹配
- 数组长度是否与容器大小一致
- 嵌套层次是否正确

### 2.3 值精度检查

- 整数：精确匹配
- 浮点数：在 roundtrip 后可能存在精度损失，使用 epsilon 比较
- 字符串：转义序列是否正确还原

## 3. 反序列化鲁棒性测试

### 3.1 畸形输入

```cpp
// 语法错误
const char* malformed[] = {
    "",                          // 空输入
    "{",                         // 未闭合对象
    "}",                         // 多余闭合
    "{\"a\":}",                  // 缺少值
    "{\"a\":1,}",                // 尾部逗号
    "{\"a\":1,,\"b\":2}",        // 双逗号
    "{'a':1}",                   // 单引号 key
    "{a:1}",                     // 无引号 key
    "[1,2,3]",                   // 顶层是数组而非对象
    "null",                      // 顶层是 null
    "42",                        // 顶层是数字
    "\"hello\"",                 // 顶层是字符串
    "{\"a\": 1} extra",          // 尾部多余内容
};
```

对于畸形输入，核心要求是：**不崩溃、不挂起、返回错误状态**。

### 3.2 类型不匹配

- 对 int 字段输入 `"hello"`
- 对 string 字段输入 `42`
- 对 bool 字段输入 `"true"`（字符串而非布尔）
- 对数组字段输入 `{}`（对象而非数组）
- 对对象字段输入 `[]`（数组而非对象）

### 3.3 字段缺失与多余

- JSON 中缺少结构体某个字段 → 该字段应保持默认值
- JSON 中有结构体没有的字段 → 应忽略或报错（取决于库的设计）

### 3.4 数值溢出

- `int` 字段给 `999999999999999` → 是否正确处理溢出
- `float` 字段给 `1e999` → 是否正确处理无穷大

## 4. 属性测试（Property-Based Testing）

属性测试的核心思想：**对于任意有效输入，序列化再反序列化应还原为相同的值**。

### 4.1 随机数据生成

使用确定性 PRNG 生成随机数据：

```cpp
int rand_int(SplitMix64& rng) {
    return static_cast<int>(rng.next());
}

double rand_double(SplitMix64& rng) {
    // 注意：避免生成 inf/nan，除非专门测试
    uint64_t bits = rng.next();
    double d;
    memcpy(&d, &bits, sizeof(d));
    if (std::isnan(d) || std::isinf(d)) d = 0.0;
    return d;
}

std::string rand_string(SplitMix64& rng, size_t max_len = 32) {
    size_t len = rng.next() % (max_len + 1);
    std::string s(len, ' ');
    for (auto& c : s) c = 32 + (rng.next() % 95); // 可打印 ASCII
    return s;
}
```

### 4.2 批量 Roundtrip

```cpp
TEST_CASE("Property: roundtrip 不变性") {
    SplitMix64 rng(12345);
    for (int i = 0; i < 2000; ++i) {
        MyStruct original;
        original.x = rand_int(rng);
        original.y = rand_string(rng);
        // ... 填充其他字段

        std::string json;
        tinyrefl::reflection_to_json(original, json);

        MyStruct restored{};
        auto st = tinyrefl::reflection_from_json(restored, json);

        CAPTURE(i);
        CAPTURE(json);
        CHECK(st.ok);
        CHECK(original.x == restored.x);
        CHECK(original.y == restored.y);
    }
}
```

## 5. 特殊场景测试

### 5.1 `ignore<T>` / `skip<T>` 字段

- 含 `ignore<int>` 成员的结构体序列化时应跳过该字段
- 反序列化时 `ignore<int>` 字段应保持默认值

### 5.2 空结构体

- 没有成员的结构体序列化应产生 `{}`
- `{}` 反序列化为空结构体应成功

### 5.3 单成员结构体

- 仅有一个成员的结构体

### 5.4 大结构体

- 成员数量接近库支持的上限（由 `GET_MEMBER_TUPLE_HELPER` 宏的最大参数数决定）

### 5.5 `const char*` 成员

- 这是一个危险类型，序列化可能正常但反序列化后指针悬空
- 需要特别关注库如何处理此类型

## 6. 独立 Oracle 验证

使用第三方 JSON 库（如 RapidJSON DOM、nlohmann/json）作为 oracle：

1. 序列化：库输出 JSON → 第三方库解析 → 验证字段值
2. 反序列化：第三方库构建 JSON → 库解析 → 验证字段值

这可以发现"roundtrip 看似正确但序列化/反序列化各自都有互相抵消的 bug"的情况。
