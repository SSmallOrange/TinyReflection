# TODO List — 已知缺口与待办

> 约定：优先级沿用 README 的 P0–P4（P0 阻断 / P1 近期必做 / P2 规划内 / P3 锦上添花）；状态用 `[ ]` 待办、`[x]` 已完成。
> 关联位置统一用 `文件: 符号` 形式引用，避免行号漂移。

---

## 一、反序列化能力缺口

### [P1] [ ] 0 成员聚合体不支持（含 `optional<Empty>`）

- **现状**：`struct Empty {};` 这类 0 成员聚合体从未被支持过，`optional_reflection_design.md` 中"现有机制天然支持"的判断不成立。三个层面的阻断：
  1. `tinyrefl/utils/reflection_get_tuple.hpp: count_serializable_impl` 中 `(expr + ...)` 一元 fold 对空 pack 是硬错误（编译失败）；
  2. `tinyrefl/utils/reflection_get_tuple.hpp: get_member_references_tuple` 主模板对 `N = 0` 触发 `static_assert("Too few structural member parameters")`，`struct_members_to_tuple<Empty>()` 返回 `void`；
  3. 即便修复前两处，`tinyrefl/thirdparty/frozen/unordered_map.h` 中 `storage_size = next_highest_power_of_two(0) * 2 = 0`，`bits/pmh.h: lookup()` 里 `hasher(key, seed) % M` 对 `M = 0` 取模是 UB——空 struct 的 handler 一旦收到未知 key（如 `{"e":{"x":1}}`）就会触发 `Key()` → `find()` 崩溃。
- **影响**：`optional<Empty>`（设计文档计划覆盖的边界用例）与 plain `Empty` 成员均无法序列化/反序列化；本次实现已从 `test_optional.cpp` 移除该用例。
- **建议**：① fold 改为带初值 `(0 + ... + ...)`；② 为 `get_member_references_tuple<T, 0>` 增加特化，返回 `std::tuple<>`；③ 评估 frozen 空表查找的除零 UB——可考虑对 0 成员类型不 push handler（在三个 reader handler 的 optional/custom 分支特判），或修复/替换 frozen 空表路径。
- **关联**：`tinyrefl/utils/reflection_get_tuple.hpp`、`tinyrefl/thirdparty/frozen/unordered_map.h`、`tinyrefl/thirdparty/frozen/bits/pmh.h`、`test/test_optional.cpp`。

### [P1] [ ] map/unordered_map 值位置 enum 反序列化缺失

- **现状**：`tinyrefl/reflection_from_json.hpp: AssociativeReaderHandleImp::String()` 只处理 `is_char_v` / `is_string_v` 内层，plain `enum` 从字符串（`"Red"`）静默丢失；`assign_value()` 也没有 plain enum 的整数 `enum_cast` 校验路径（`is_json_compatible_v<Color, int>` 为 false → 静默丢）。本次按设计文档只补齐了 `optional<char>` / `optional<string>` 的字符串分支与 `optional<enum>` 的整数分支，因此：plain enum 字符串/整数均缺失、`optional<enum>` 字符串缺失。
- **影响**：与序列化默认 `as_string` 模式不对称——`map<string, Color>`、`map<string, optional<Color>>` 序列化输出 `"Red"`，反序列化读不回来（roundtrip 丢数据）。
- **建议**：统一补齐 `String()` 的 enum / `optional<enum>` 分支与 `assign_value()` 的 enum / `optional<enum>` 校验，与 `ReaderHandlerImp` 保持同一套语义。
- **关联**：`tinyrefl/reflection_from_json.hpp: AssociativeReaderHandleImp`、`test/test_optional.cpp`（目前 map enum 仅测整数路径）。

### [P2] [ ] `optional<const char*>` / `optional<char*>` 从字符串被静默丢弃

- **现状**：三个 reader handler 的 `String()` 事件中，optional 分支对内层非 `string` / `char` / `enum` 的情况直接 `char_handled = true`（或 `return true`）吞掉，`optional<const char*>` 这类内层不走 `assign_if_match<const char*>` 路径。
- **影响**：成员 / 序列元素 / map 值三层位置的 `optional<const char*>` 均无法从 JSON 字符串赋值（optional 支持是新能力，此为主动取舍而非回归）。
- **建议**：要么在 optional 分支中对 `char*` 内层显式复用 `assign(str, length)` 语义，要么在文档中明确列为不支持类型。
- **关联**：`tinyrefl/reflection_from_json.hpp` 三处 `String()`。

### [P2] [ ] `optional<T>` 内层为不可序列化类型时判定错误

- **现状**：`tinyrefl/utils/reflection_utils.hpp: is_serializable_v` 只检查外层，`optional<unique_ptr<T>>` / `optional<ignore<T>>` 会返回 `true` 被纳入序列化；随后 `to_json_value` 对 `*object`（`unique_ptr<T>&`）没有任何匹配重载 → 编译错误，诊断不友好。
- **影响**：用户写 `std::optional<std::unique_ptr<X>>` 成员会得到模板深度展开的编译错误，而非"该类型不可序列化"的明确提示。
- **建议**：`is_serializable_v<std::optional<T>>` 递归判定内层（`is_serializable_v<T>`），使这类成员在 `is_member_serializable` 阶段即被排除；或提供 static_assert 给出可读诊断。
- **关联**：`tinyrefl/utils/reflection_utils.hpp: is_serializable_v`。

---

## 二、成员数探测（`Any` 探针）同类隐患

### [P2] [ ] 贪婪模板构造类型作为成员未处理

- **现状**：本次为 `std::optional` 修了 `Any` 探测歧义（`reflection_get_tuple.hpp: Any::probe_ok`），但 `std::variant`、`std::function` 等同样带贪婪模板构造（`variant(U&&)`、`function(F&&)`）的类型作为 struct 成员时，成员数探测存在同根源的歧义，同样会被判为 0 成员或编译失败（`std::variant` 同时是 README P0 规划类型）。
- **影响**：README P0 "支持 `std::variant<Ts...>`" 落地时会直接撞上该问题，需在 `probe_ok` 中泛化（用 `is_template_instant_of` 检测 + 同样的"构造路径可行则让位"判定），或明确维护一份"不支持类型清单"。
- **关联**：`tinyrefl/utils/reflection_get_tuple.hpp: Any::probe_ok`、README P0。

### [P3] [ ] `probe_ok` 的编译期开销观察

- **现状**：`probe_ok` 对 optional 目标会求值 `is_constructible_v` / `is_convertible_v`（非 optional 走 `if constexpr` 短路零开销）。
- **建议**：若后续大量使用 optional 成员导致编译变慢，可考虑缓存判定结果或更换探测策略；暂不处理。

---

## 三、语义与错误处理

### [P1] [ ] 缺失字段策略（README P1 既有项）

- **现状**：非 optional 成员遇到 `null` 时 `ReaderHandlerImp::Null()` 静默忽略；缺失字段保留默认值。严格模式（非 optional 必填、optional 缺失 → `nullopt`）以本次 optional 支持为前置，尚未实现。
- **关联**：`optional_reflection_design.md` 第 1/7 节、README P1。

### [P2] [ ] 无效 enum 值的静默丢弃无诊断

- **现状**：无效整数/名称被丢弃且无任何报错或状态回传；成员位置 plain enum 保持原值、`optional<enum>` 保持 `nullopt`，元素位置直接跳过。语义大体一致但细节有别。
- **建议**：与缺失字段策略一并设计，评估是否在 `Status` 中上报丢弃事件（如警告列表）。
- **关联**：`tinyrefl/reflection_from_json.hpp` 各 `assign_if_match` / `assign_value`、`test/test_enum.cpp`。

### [P3] [ ] `RawNumber`（`kParseNumbersAsStrings`）路径不支持 optional

- **现状**：rapidjson 开启该 flag 时数字走 `RawNumber` → `assign_if_match<const char*>`，optional 标量与 `const char*` 不兼容被静默丢弃（与 plain 标量现状一致，非新增回归）。
- **建议**：若未来正式支持该解析 flag，需在 optional 分支补齐；暂列观察项。

---

## 四、跨平台与工程

### [P1] [ ] GCC / MSVC 三平台编译验证

- **现状**：本次实现仅在 Apple Clang（libc++）验证。`Any::probe_ok` 依赖 requires 子句中引用类内成员函数模板与 `if constexpr` 短路，MSVC 对 requires 子句的历史兼容性、libstdc++ 下 `is_constructible_v` / `is_convertible_v` 的判定结果均未实测（设计上两条路径互为补集、自适应，但需编译验证）。
- **建议**：CI 或手动在 GCC、MSVC 下跑 `test_optional` 及全量测试。
- **关联**：`tinyrefl/utils/reflection_get_tuple.hpp: Any::probe_ok`。

### [P2] [ ] README 同步更新

- **现状**：README P0 的 "支持 `std::optional<T>`" 仍为未勾选状态。
- **建议**：实现合入后勾选该项，并补充使用示例与限制说明（不支持 `optional<optional<T>>`、0 成员聚合体、`optional<const char*>` 从字符串等）。
- **关联**：`README.md` 🔭 TODO → P0。

### [P3] [ ] 测试补强

- [ ] optional trait 断言（`is_optional_v` / `optional_inner_type_t` / `is_custom_type_v<optional>`）目前放在 `test_optional.cpp`，可同步补进 `test_check_type.cpp`（trait 的"家"）；
- [ ] move-only 内层（`optional<unique_ptr<T>>` 等）无测试覆盖（当前因二.4 的判定问题会编译失败，修复后补测试）；
- [ ] `test_pref_reflection` 性能测试未覆盖 optional 场景。

---

## 五、明确不支持（设计决策存档）

- **`optional<optional<T>>`**：无实际意义，不支持。序列化侧递归 optional 重载实际可工作，但反序列化侧未显式处理内层为 optional 的事件路径，不作为承诺能力。
- **map 键非字符串类型**：既有限制（`to_json_value` associative 分支 static_assert "Only string keys are supported"），与本次无关，存档备查。
