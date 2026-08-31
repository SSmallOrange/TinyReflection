#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

// ==================== 测试结构体 ====================

// 基础 map 成员
struct HasMap {
  int id;
  std::map<std::string, int> scores;
};

// unordered_map 成员
struct HasUnorderedMap {
  std::string name;
  std::unordered_map<std::string, std::string> metadata;
};

// map 值为 struct
struct Inner {
  int x;
  std::string y;
};

struct HasMapOfStruct {
  std::map<std::string, Inner> items;
};

// map 值为 vector
struct HasMapOfVector {
  std::map<std::string, std::vector<int>> groups;
};

// vector 内含 map 的 struct
struct ItemWithMap {
  std::string label;
  std::map<std::string, int> props;
};

struct HasVecOfStructWithMap {
  std::vector<ItemWithMap> items;
};

// 空 map
struct HasEmptyMap {
  std::map<std::string, int> data;
  int flag;
};

// 多个 map 字段
struct HasMultipleMaps {
  std::map<std::string, int> ints;
  std::map<std::string, std::string> strings;
  std::map<std::string, double> doubles;
};

// unsigned 整型成员
struct HasUnsigned {
  unsigned int a;
  uint16_t b;
  uint32_t c;
  uint64_t d;
};

// signed/unsigned 混合
struct MixedIntTypes {
  int8_t a;
  int16_t b;
  int32_t c;
  int64_t d;
  uint8_t e;
  uint16_t f;
  uint32_t g;
  uint64_t h;
};

// bool 与 int 交叉
struct BoolAndInt {
  bool flag;
  int count;
};

// float 精度
struct FloatStruct {
  float f;
  double d;
};

// ==================== 测试用例 ====================

TEST_CASE("map<string,int> roundtrip") {
  HasMap original;
  original.id = 1;
  original.scores["alice"] = 95;
  original.scores["bob"] = 87;
  original.scores["charlie"] = 100;

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  // 验证 JSON 包含 map 内容
  INFO("序列化结果: " << json);
  CHECK(json.find("\"alice\"") != std::string::npos);
  CHECK(json.find("95") != std::string::npos);

  HasMap restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.id == 1);

  // 关键验证：map 内容是否被正确反序列化
  CHECK(restored.scores.size() == 3);
  CHECK(restored.scores["alice"] == 95);
  CHECK(restored.scores["bob"] == 87);
  CHECK(restored.scores["charlie"] == 100);
}

TEST_CASE("unordered_map<string,string> roundtrip") {
  HasUnorderedMap original;
  original.name = "test";
  original.metadata["key1"] = "value1";
  original.metadata["key2"] = "value2";

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  HasUnorderedMap restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.name == "test");
  CHECK(restored.metadata.size() == 2);
  CHECK(restored.metadata["key1"] == "value1");
  CHECK(restored.metadata["key2"] == "value2");
}

TEST_CASE("map 值为嵌套 struct 的 roundtrip") {
  HasMapOfStruct original;
  original.items["first"] = {10, "hello"};
  original.items["second"] = {20, "world"};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  HasMapOfStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.items.size() == 2);
  CHECK(restored.items["first"].x == 10);
  CHECK(restored.items["first"].y == "hello");
  CHECK(restored.items["second"].x == 20);
  CHECK(restored.items["second"].y == "world");
}

TEST_CASE("map 值为 vector 的 roundtrip") {
  HasMapOfVector original;
  original.groups["a"] = {1, 2, 3};
  original.groups["b"] = {4, 5};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  HasMapOfVector restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.groups.size() == 2);
  CHECK(restored.groups["a"] == std::vector<int>{1, 2, 3});
  CHECK(restored.groups["b"] == std::vector<int>{4, 5});
}

TEST_CASE("vector 内 struct 含 map 的 roundtrip") {
  HasVecOfStructWithMap original;
  original.items.push_back({"item1", {{"p1", 10}, {"p2", 20}}});
  original.items.push_back({"item2", {{"p3", 30}}});

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  HasVecOfStructWithMap restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.items.size() == 2);
  CHECK(restored.items[0].label == "item1");
  CHECK(restored.items[0].props.size() == 2);
  CHECK(restored.items[0].props["p1"] == 10);
  CHECK(restored.items[0].props["p2"] == 20);
  CHECK(restored.items[1].label == "item2");
  CHECK(restored.items[1].props["p3"] == 30);
}

TEST_CASE("空 map roundtrip") {
  HasEmptyMap original;
  original.data.clear();
  original.flag = 42;

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  HasEmptyMap restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.data.empty());
  CHECK(restored.flag == 42);
}

TEST_CASE("多个 map 字段 roundtrip") {
  HasMultipleMaps original;
  original.ints["a"] = 1;
  original.ints["b"] = 2;
  original.strings["x"] = "hello";
  original.doubles["pi"] = 3.14;

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  HasMultipleMaps restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.ints.size() == 2);
  CHECK(restored.ints["a"] == 1);
  CHECK(restored.ints["b"] == 2);
  CHECK(restored.strings["x"] == "hello");
  CHECK(restored.doubles["pi"] == doctest::Approx(3.14));
}

// ==================== 整型类型 roundtrip ====================

TEST_CASE("unsigned 整型 roundtrip") {
  HasUnsigned original{100u, 200u, 300u, 400u};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  HasUnsigned restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.a == 100u);
  CHECK(restored.b == 200u);
  CHECK(restored.c == 300u);
  CHECK(restored.d == 400u);
}

TEST_CASE("混合有符号/无符号整型 roundtrip") {
  MixedIntTypes original{-1, -100, -1000, -10000, 1, 100, 1000, 10000};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  MixedIntTypes restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.a == -1);
  CHECK(restored.b == -100);
  CHECK(restored.c == -1000);
  CHECK(restored.d == -10000);
  CHECK(restored.e == 1);
  CHECK(restored.f == 100);
  CHECK(restored.g == 1000);
  CHECK(restored.h == 10000);
}

TEST_CASE("unsigned 极值 roundtrip") {
  HasUnsigned original{
      UINT32_MAX,  // 4294967295
      UINT16_MAX,  // 65535
      UINT32_MAX,
      UINT64_MAX  // 18446744073709551615
  };

  std::string json;
  tinyrefl::reflection_to_json(original, json);
  INFO("序列化结果: " << json);

  HasUnsigned restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.a == UINT32_MAX);
  CHECK(restored.b == UINT16_MAX);
  CHECK(restored.c == UINT32_MAX);
  CHECK(restored.d == UINT64_MAX);
}

// ==================== bool/int 交叉赋值 ====================

TEST_CASE("bool 和 int 从 JSON 反序列化") {
  // 正常 roundtrip
  BoolAndInt original{true, 42};
  std::string json;
  tinyrefl::reflection_to_json(original, json);

  BoolAndInt restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.flag == true);
  CHECK(restored.count == 42);
}

TEST_CASE("JSON 中 bool 字段给 int 值不会崩溃") {
  // 手工构造: flag 给 int 值, count 给 bool 值
  const char* json = R"({"flag":1,"count":true})";
  BoolAndInt restored{};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok);
  // 这里检查是否不崩溃，具体值取决于 is_json_compatible_v 的行为
}

// ==================== float 精度 ====================

TEST_CASE("float/double roundtrip 精度") {
  FloatStruct original{3.14f, 2.718281828459045};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  FloatStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.f == doctest::Approx(3.14f));
  CHECK(restored.d == doctest::Approx(2.718281828459045));
}

TEST_CASE("float 极小值 roundtrip") {
  FloatStruct original{FLT_MIN, DBL_MIN};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  FloatStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);
  CHECK(restored.f == doctest::Approx(FLT_MIN));
  CHECK(restored.d == doctest::Approx(DBL_MIN));
}

// ==================== 属性测试 (随机 roundtrip) ====================

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
  int32_t next_int() { return static_cast<int32_t>(next()); }
  double next_double() {
    return static_cast<double>(next() >> 11) / (1ULL << 53);
  }
  std::string next_string(int max_len = 20) {
    auto len = static_cast<int>(next() % static_cast<uint64_t>(max_len + 1));
    std::string s;
    s.reserve(static_cast<size_t>(len));
    for (int i = 0; i < len; ++i) {
      // 可打印 ASCII（避免控制字符简化比较）
      s.push_back(static_cast<char>(32 + next() % 95));
    }
    return s;
  }
};

struct RandomTestStruct {
  int a;
  double b;
  std::string c;
  bool d;
  std::vector<int> e;
};

TEST_CASE("属性测试: 随机 struct roundtrip 2000轮") {
  SplitMix64 rng(12345);

  for (int round = 0; round < 2000; ++round) {
    RandomTestStruct original;
    original.a = rng.next_int();
    original.b = rng.next_double() * 1000.0 - 500.0;
    original.c = rng.next_string(30);
    original.d = (rng.next() & 1) != 0;
    int vec_len = rng.next() % 10;
    original.e.clear();
    for (int i = 0; i < vec_len; ++i) {
      original.e.push_back(rng.next_int());
    }

    std::string json;
    tinyrefl::reflection_to_json(original, json);

    RandomTestStruct restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());

    INFO("Round " << round << " JSON: " << json);
    REQUIRE(status.ok);
    CHECK(restored.a == original.a);
    CHECK(restored.b == doctest::Approx(original.b));
    CHECK(restored.c == original.c);
    CHECK(restored.d == original.d);
    REQUIRE(restored.e.size() == original.e.size());
    for (size_t i = 0; i < original.e.size(); ++i) {
      CHECK(restored.e[i] == original.e[i]);
    }
  }
}
