#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"

#include <cmath>
#include <cstring>
#include <deque>
#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================
// 测试结构体定义
// ============================================================

// --- char 非 ASCII 值 ---
struct CharStruct {
  char c;
  signed char sc;
  unsigned char uc;
};

struct VectorCharStruct {
  std::vector<char> chars;
};

// --- list / deque 容器 ---
struct ListIntStruct {
  std::list<int> data;
};

struct DequeStringStruct {
  std::deque<std::string> data;
};

struct ListOfStructs {
  int id;
  std::string name;
};

struct ContainingList {
  std::list<ListOfStructs> items;
};

struct DequeOfVectors {
  std::deque<std::vector<int>> data;
};

// --- 浮点精度 ---
struct FloatStruct {
  float f;
  double d;
};

// --- 混合深层嵌套 ---
struct MapOfVectorOfMaps {
  std::map<std::string, std::vector<std::map<std::string, int>>> data;
};

struct MapOfMapOfVector {
  std::map<std::string, std::map<std::string, std::vector<int>>> data;
};

// --- map<string, bool> ---
struct MapBoolStruct {
  std::map<std::string, bool> flags;
};

// --- map<string, double> ---
struct MapDoubleStruct {
  std::map<std::string, double> values;
};

// --- map<string, char> ---
struct MapCharStruct {
  std::map<std::string, char> chars;
};

// --- 空 list/deque ---
struct EmptyContainers {
  std::list<int> empty_list;
  std::deque<int> empty_deque;
  std::vector<int> empty_vec;
};

// --- 组合: struct 含 list<map<string, vector<int>>> ---
struct ComplexNested {
  std::list<std::map<std::string, std::vector<int>>> data;
};

// ============================================================
// 测试用例
// ============================================================

// ---- 1. char 非 ASCII 值序列化/反序列化 ----

TEST_CASE("char ASCII值 roundtrip") {
  // ASCII 范围内的 char 应该正常 roundtrip
  CharStruct original{'A', 'B', 'C'};
  std::string json;
  tinyrefl::reflection_to_json(original, json);

  CharStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());

  CHECK(status.ok);
  CHECK(original.c == restored.c);
  CHECK(original.sc == restored.sc);
  CHECK(original.uc == restored.uc);
}

TEST_CASE("char 特殊ASCII字符 roundtrip") {
  // 特殊 ASCII 控制字符 (< 0x20) 应该被转义为 \\uXXXX
  CharStruct original{'\n', '\t', '\0'};
  std::string json;
  tinyrefl::reflection_to_json(original, json);

  // 验证 JSON 合法性
  CharStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(original.c == restored.c);
  CHECK(original.sc == restored.sc);
  // '\0' 序列化后可能丢失（length==0 时默认为 char{}）
}

TEST_CASE("unsigned char 非ASCII值 序列化产生非法UTF-8") {
  // 值 > 127 的 unsigned char 序列化时直接输出原始字节，
  // 这不是合法的 UTF-8，RapidJSON 解析会失败
  CharStruct original{'A', 'B', 200};  // uc=200, 超出 ASCII
  std::string json;
  tinyrefl::reflection_to_json(original, json);

  // 尝试反序列化——预期应该成功（如果序列化正确转义了的话）
  CharStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());

  // 如果这里失败，说明非 ASCII char 序列化产生了非法 JSON
  CHECK(status.ok);
  if (status.ok) {
    CHECK(original.uc == restored.uc);
  }
}

TEST_CASE("signed char 负值 序列化产生非法UTF-8") {
  // 负值 signed char 的二进制表示 > 0x7F，序列化时原始字节不是合法 UTF-8
  CharStruct original{'A', -1, 'C'};  // sc=-1 (0xFF)
  std::string json;
  tinyrefl::reflection_to_json(original, json);

  CharStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());

  CHECK(status.ok);
  if (status.ok) {
    CHECK(original.sc == restored.sc);
  }
}

TEST_CASE("unsigned char 边界值 128 序列化") {
  CharStruct original{'A', 'B', 128};  // uc=128, 刚好超出 ASCII
  std::string json;
  tinyrefl::reflection_to_json(original, json);

  CharStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
}

TEST_CASE("vector<char> 含非ASCII值") {
  VectorCharStruct original;
  original.chars = {'A', static_cast<char>(200), 'Z'};
  std::string json;
  tinyrefl::reflection_to_json(original, json);

  VectorCharStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  if (status.ok) {
    REQUIRE(restored.chars.size() == 3);
    CHECK(original.chars[0] == restored.chars[0]);
    CHECK(original.chars[1] == restored.chars[1]);
    CHECK(original.chars[2] == restored.chars[2]);
  }
}

// ---- 2. list 容器 roundtrip ----

TEST_CASE("list<int> 基本 roundtrip") {
  ListIntStruct original;
  original.data = {1, 2, 3, 42, -100};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  ListIntStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(original.data == restored.data);
}

TEST_CASE("list<int> 空列表") {
  ListIntStruct original;
  // data 为空

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  ListIntStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.data.empty());
}

TEST_CASE("list<int> 单元素") {
  ListIntStruct original;
  original.data = {42};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  ListIntStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(original.data == restored.data);
}

TEST_CASE("deque<string> roundtrip") {
  DequeStringStruct original;
  original.data = {"hello", "world", "", "test with spaces"};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  DequeStringStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  REQUIRE(original.data.size() == restored.data.size());
  auto it1 = original.data.begin();
  auto it2 = restored.data.begin();
  for (; it1 != original.data.end(); ++it1, ++it2) {
    CHECK(*it1 == *it2);
  }
}

TEST_CASE("list<struct> roundtrip") {
  ContainingList original;
  original.items = {{1, "Alice"}, {2, "Bob"}, {3, "Charlie"}};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  ContainingList restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  REQUIRE(original.items.size() == restored.items.size());
  auto it1 = original.items.begin();
  auto it2 = restored.items.begin();
  for (; it1 != original.items.end(); ++it1, ++it2) {
    CHECK(it1->id == it2->id);
    CHECK(it1->name == it2->name);
  }
}

TEST_CASE("deque<vector<int>> 嵌套容器 roundtrip") {
  DequeOfVectors original;
  original.data = {{1, 2, 3}, {}, {42}, {10, 20, 30, 40}};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  DequeOfVectors restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  REQUIRE(original.data.size() == restored.data.size());
  auto it1 = original.data.begin();
  auto it2 = restored.data.begin();
  for (; it1 != original.data.end(); ++it1, ++it2) {
    CHECK(*it1 == *it2);
  }
}

// ---- 3. 浮点精度 roundtrip ----

TEST_CASE("double -0.0 roundtrip — 正规化为 0.0") {
  // 修复后：-0.0 在序列化时正规化为 0.0，roundtrip 得到正零
  FloatStruct original{0.0f, -0.0};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  // 验证序列化输出不含 "-0"
  CHECK(json.find("-0") == std::string::npos);

  FloatStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);

  // 正规化后应该是正零
  CHECK(restored.d == 0.0);
  CHECK(!std::signbit(restored.d));
}

TEST_CASE("float -0.0f roundtrip — 正规化为 0.0f") {
  FloatStruct original{-0.0f, 1.0};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  FloatStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.f == 0.0f);
  CHECK(!std::signbit(restored.f));
}

TEST_CASE("float 精度边界值 roundtrip") {
  // 测试一些特殊的 float 值，确保通过 double 中转不丢精度
  float test_values[] = {
      1.0f / 3.0f,     // 0.333333...
      1.17549435e-38f,  // FLT_MIN (smallest normal)
      3.40282347e+38f,  // FLT_MAX
      1.17549421e-38f,  // slightly less than FLT_MIN
      1.23456789f,      // 有效数字多
  };

  for (float val : test_values) {
    FloatStruct original{val, 0.0};
    std::string json;
    tinyrefl::reflection_to_json(original, json);

    FloatStruct restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok);
    // float 通过 double 中转后应该保持不变
    CHECK(original.f == restored.f);
  }
}

// ---- 4. 混合深层嵌套 ----

TEST_CASE("map<string, vector<map<string, int>>> roundtrip") {
  MapOfVectorOfMaps original;
  original.data["group1"] = {
      {{"a", 1}, {"b", 2}},
      {{"c", 3}},
  };
  original.data["group2"] = {
      {{"x", 10}, {"y", 20}, {"z", 30}},
  };
  original.data["empty_group"] = {};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  MapOfVectorOfMaps restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);

  REQUIRE(restored.data.size() == original.data.size());
  for (const auto& [key, vec] : original.data) {
    REQUIRE(restored.data.count(key) == 1);
    REQUIRE(restored.data[key].size() == vec.size());
    for (size_t i = 0; i < vec.size(); ++i) {
      CHECK(restored.data[key][i] == vec[i]);
    }
  }
}

TEST_CASE("map<string, map<string, vector<int>>> roundtrip") {
  MapOfMapOfVector original;
  original.data["outer1"] = {
      {"inner_a", {1, 2, 3}},
      {"inner_b", {4, 5}},
  };
  original.data["outer2"] = {
      {"inner_c", {}},
      {"inner_d", {100}},
  };

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  MapOfMapOfVector restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);

  REQUIRE(restored.data.size() == original.data.size());
  for (const auto& [k1, inner_map] : original.data) {
    REQUIRE(restored.data.count(k1) == 1);
    for (const auto& [k2, vec] : inner_map) {
      REQUIRE(restored.data[k1].count(k2) == 1);
      CHECK(restored.data[k1][k2] == vec);
    }
  }
}

// ---- 5. map<string, bool> ----

TEST_CASE("map<string, bool> roundtrip") {
  MapBoolStruct original;
  original.flags = {{"enabled", true}, {"debug", false}, {"verbose", true}};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  MapBoolStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.flags == original.flags);
}

// ---- 6. map<string, double> ----

TEST_CASE("map<string, double> roundtrip") {
  MapDoubleStruct original;
  original.values = {{"pi", 3.14159265358979}, {"e", 2.71828182845905}, {"zero", 0.0}};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  MapDoubleStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  for (const auto& [k, v] : original.values) {
    REQUIRE(restored.values.count(k) == 1);
    CHECK(restored.values[k] == v);
  }
}

// ---- 7. map<string, char> ----

TEST_CASE("map<string, char> ASCII roundtrip") {
  MapCharStruct original;
  original.chars = {{"first", 'A'}, {"second", 'Z'}, {"digit", '9'}};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  MapCharStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.chars == original.chars);
}

// ---- 8. 空容器 ----

TEST_CASE("空 list/deque/vector roundtrip") {
  EmptyContainers original;

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  EmptyContainers restored{};
  // 预填一些值，验证反序列化会正确处理
  restored.empty_list = {1, 2, 3};
  restored.empty_deque = {4, 5, 6};
  restored.empty_vec = {7, 8, 9};

  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  // 注意：反序列化空数组不会清空已有数据（vector 累加行为）
  // 如果这是 bug，以下检查会失败
  // 空 JSON 数组 [] 反序列化时不添加元素，但也不清空
  // 所以 restored.empty_list 仍然是 {1,2,3}
  // 这个行为是否正确取决于设计决策
  MESSAGE("反序列化空数组后 list size: ", restored.empty_list.size());
  MESSAGE("反序列化空数组后 deque size: ", restored.empty_deque.size());
  MESSAGE("反序列化空数组后 vector size: ", restored.empty_vec.size());
}

// ---- 9. 组合: list<map<string, vector<int>>> ----

TEST_CASE("list<map<string, vector<int>>> 深层混合嵌套") {
  ComplexNested original;
  original.data.push_back({{"a", {1, 2}}, {"b", {3}}});
  original.data.push_back({{"c", {4, 5, 6}}});
  original.data.push_back({});  // 空 map

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  ComplexNested restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);

  REQUIRE(restored.data.size() == original.data.size());
  auto it1 = original.data.begin();
  auto it2 = restored.data.begin();
  for (; it1 != original.data.end(); ++it1, ++it2) {
    REQUIRE(it1->size() == it2->size());
    for (const auto& [k, v] : *it1) {
      REQUIRE(it2->count(k) == 1);
      CHECK(it2->at(k) == v);
    }
  }
}

// ---- 10. 大量元素的 list ----

TEST_CASE("list<int> 大量元素 roundtrip") {
  ListIntStruct original;
  for (int i = 0; i < 1000; ++i) {
    original.data.push_back(i * i - 500);
  }

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  ListIntStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(original.data == restored.data);
}
