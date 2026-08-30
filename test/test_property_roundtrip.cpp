#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"

#include "tinyrefl/thirdparty/rapidjson/document.h"

#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <list>
#include <map>
#include <string>
#include <vector>

// ===== 测试结构体定义 =====

struct CharStruct {
  char ch;
};

struct BasicStruct {
  int i;
  double d;
  bool b;
  std::string s;
};

struct FloatStruct {
  float f;
  double d;
};

struct NestedInner {
  int x;
  std::string y;
};

struct NestedOuter {
  NestedInner inner;
  std::vector<NestedInner> list;
};

struct VectorStruct {
  std::vector<int> vi;
  std::vector<std::string> vs;
};

struct MapStruct {
  std::map<std::string, int> m;
};

struct Int64Struct {
  int64_t val;
};

struct MultiFieldStruct {
  int a;
  std::string b;
  double c;
  bool d;
};

// ===== 辅助：验证 JSON 合法性 =====
static bool is_valid_json(const std::string& json) {
  ::rapidjson::Document doc;
  doc.Parse(json.c_str());
  return !doc.HasParseError();
}

// ===== SplitMix64 PRNG =====
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

// ======================================================
// 层级 1：Roundtrip 正确性
// ======================================================

TEST_CASE("Roundtrip: 基本类型结构体") {
  BasicStruct original{42, 3.14, true, "hello world"};
  std::string json;
  tinyrefl::reflection_to_json(original, json);

  BasicStruct restored{};
  auto st = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(st.ok);
  CHECK(original.i == restored.i);
  CHECK(original.d == doctest::Approx(restored.d));
  CHECK(original.b == restored.b);
  CHECK(original.s == restored.s);
}

TEST_CASE("Roundtrip: 嵌套结构体") {
  NestedOuter original{
      {10, "inner"}, {{1, "a"}, {2, "b"}, {3, "c"}}};
  std::string json;
  tinyrefl::reflection_to_json(original, json);

  NestedOuter restored{};
  auto st = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(st.ok);
  CHECK(original.inner.x == restored.inner.x);
  CHECK(original.inner.y == restored.inner.y);
  REQUIRE(original.list.size() == restored.list.size());
  for (size_t i = 0; i < original.list.size(); ++i) {
    CHECK(original.list[i].x == restored.list[i].x);
    CHECK(original.list[i].y == restored.list[i].y);
  }
}

TEST_CASE("Roundtrip: vector 容器") {
  VectorStruct original{{1, 2, 3, 4, 5}, {"hello", "world", ""}};
  std::string json;
  tinyrefl::reflection_to_json(original, json);

  VectorStruct restored{};
  auto st = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(st.ok);
  CHECK(original.vi == restored.vi);
  CHECK(original.vs == restored.vs);
}

TEST_CASE("Roundtrip: 空容器") {
  VectorStruct original{{}, {}};
  std::string json;
  tinyrefl::reflection_to_json(original, json);

  VectorStruct restored{};
  auto st = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(st.ok);
  CHECK(restored.vi.empty());
  CHECK(restored.vs.empty());
}

TEST_CASE("Roundtrip: int64_t 极值") {
  SUBCASE("INT64_MAX") {
    Int64Struct original{INT64_MAX};
    std::string json;
    tinyrefl::reflection_to_json(original, json);

    Int64Struct restored{};
    auto st = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(st.ok);
    CHECK(original.val == restored.val);
  }

  SUBCASE("INT64_MIN") {
    Int64Struct original{INT64_MIN};
    std::string json;
    tinyrefl::reflection_to_json(original, json);

    Int64Struct restored{};
    auto st = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(st.ok);
    CHECK(original.val == restored.val);
  }
}

// ======================================================
// 层级 2：边界值和特殊值 — char 转义问题
// ======================================================

TEST_CASE("Bug 验证: char 类型序列化应产生合法 JSON") {
  // char 中含有 JSON 特殊字符时，应正确转义
  SUBCASE("双引号 char") {
    CharStruct original{'"'};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("反斜杠 char") {
    CharStruct original{'\\'};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("换行符 char") {
    CharStruct original{'\n'};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("制表符 char") {
    CharStruct original{'\t'};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("空字符 char") {
    CharStruct original{'\0'};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("回车符 char") {
    CharStruct original{'\r'};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }
}

TEST_CASE("Bug 验证: char roundtrip 正确性") {
  // 普通可打印 char 的 roundtrip
  SUBCASE("普通字符 'A'") {
    CharStruct original{'A'};
    std::string json;
    tinyrefl::reflection_to_json(original, json);

    CharStruct restored{};
    auto st = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(st.ok);
    CHECK(original.ch == restored.ch);
  }

  // 特殊字符 roundtrip — 需要序列化正确转义，反序列化正确还原
  SUBCASE("双引号 roundtrip") {
    CharStruct original{'"'};
    std::string json;
    tinyrefl::reflection_to_json(original, json);

    CharStruct restored{};
    auto st = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(st.ok);
    CHECK(original.ch == restored.ch);
  }

  SUBCASE("反斜杠 roundtrip") {
    CharStruct original{'\\'};
    std::string json;
    tinyrefl::reflection_to_json(original, json);

    CharStruct restored{};
    auto st = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(st.ok);
    CHECK(original.ch == restored.ch);
  }
}

// ======================================================
// 层级 2：边界值和特殊值 — map key 转义问题
// ======================================================

TEST_CASE("Bug 验证: map key 含特殊字符时应产生合法 JSON") {
  SUBCASE("key 含双引号") {
    MapStruct original;
    original.m["key\"quote"] = 42;
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("key 含反斜杠") {
    MapStruct original;
    original.m["key\\slash"] = 42;
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("key 含换行符") {
    MapStruct original;
    original.m["key\nline"] = 42;
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }
}

// ======================================================
// 层级 2：边界值和特殊值 — 浮点特殊值
// ======================================================

TEST_CASE("Bug 验证: 浮点特殊值序列化应产生合法 JSON") {
  SUBCASE("正无穷大 float") {
    FloatStruct original{std::numeric_limits<float>::infinity(), 0.0};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("负无穷大 float") {
    FloatStruct original{-std::numeric_limits<float>::infinity(), 0.0};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("NaN float") {
    FloatStruct original{std::numeric_limits<float>::quiet_NaN(), 0.0};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("正无穷大 double") {
    FloatStruct original{0.0f, std::numeric_limits<double>::infinity()};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("负无穷大 double") {
    FloatStruct original{0.0f, -std::numeric_limits<double>::infinity()};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("NaN double") {
    FloatStruct original{0.0f, std::numeric_limits<double>::quiet_NaN()};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    INFO("序列化结果: ", json);
    CHECK(is_valid_json(json));
  }
}

// ======================================================
// 层级 3：JSON 格式合规性
// ======================================================

TEST_CASE("JSON 合规性: 序列化输出均为合法 JSON") {
  SUBCASE("空 vector 结构体") {
    VectorStruct original{{}, {}};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("嵌套结构体") {
    NestedOuter original{{42, "test"}, {{1, "a"}}};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    CHECK(is_valid_json(json));
  }

  SUBCASE("含转义字符的字符串") {
    BasicStruct original{0, 0.0, false, "line1\nline2\ttab\"quote\\slash"};
    std::string json;
    tinyrefl::reflection_to_json(original, json);
    CHECK(is_valid_json(json));
  }
}

// ======================================================
// 层级 4：反序列化鲁棒性
// ======================================================

TEST_CASE("反序列化鲁棒性: 畸形 JSON 不崩溃") {
  const char* malformed[] = {
      "",
      "{",
      "}",
      "{\"a\":}",
      "{\"a\":1,}",
      "[1,2,3]",
      "null",
      "42",
      "\"hello\"",
  };

  for (const char* input : malformed) {
    BasicStruct restored{};
    // 不应崩溃，返回错误即可
    auto st = tinyrefl::reflection_from_json(restored, input);
    // 只要不崩溃就算通过，大多数应返回 !ok
    (void)st;
    // 确认没有崩溃地走到这里
    CHECK(true);
  }
}

TEST_CASE("反序列化鲁棒性: 缺失字段保持默认值") {
  MultiFieldStruct restored{};
  restored.a = 999;
  restored.b = "default";
  restored.c = 1.23;
  restored.d = true;

  // 只提供部分字段
  auto st =
      tinyrefl::reflection_from_json(restored, R"({"a":42,"b":"hello"})");
  CHECK(st.ok);
  CHECK(restored.a == 42);
  CHECK(restored.b == "hello");
  // c 和 d 应保持之前的值（缺失字段不被修改）
  CHECK(restored.c == doctest::Approx(1.23));
  CHECK(restored.d == true);
}

TEST_CASE("反序列化鲁棒性: 多余字段应被忽略") {
  BasicStruct restored{};
  auto st = tinyrefl::reflection_from_json(
      restored, R"({"i":10,"d":2.5,"b":true,"s":"ok","extra":999,"unknown":"x"})");
  CHECK(st.ok);
  CHECK(restored.i == 10);
  CHECK(restored.d == doctest::Approx(2.5));
  CHECK(restored.b == true);
  CHECK(restored.s == "ok");
}

// ======================================================
// 层级 5：属性测试（Property-Based Testing）
// ======================================================

TEST_CASE("属性测试: BasicStruct roundtrip 不变性 (2000 轮)") {
  SplitMix64 rng(12345);
  for (int iter = 0; iter < 2000; ++iter) {
    BasicStruct original;
    original.i = static_cast<int>(rng.next());
    original.d = static_cast<double>(static_cast<int64_t>(rng.next())) /
                 static_cast<double>(1 + (rng.next() % 1000000));
    original.b = (rng.next() % 2) == 0;
    // 生成可打印 ASCII 字符串
    size_t len = rng.next() % 33;
    original.s.resize(len);
    for (size_t j = 0; j < len; ++j) {
      original.s[j] = static_cast<char>(32 + (rng.next() % 95));
    }

    std::string json;
    tinyrefl::reflection_to_json(original, json);

    // 验证 JSON 合法性
    CAPTURE(iter);
    CAPTURE(json);
    CHECK(is_valid_json(json));

    BasicStruct restored{};
    auto st = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(st.ok);
    CHECK(original.i == restored.i);
    CHECK(original.d == doctest::Approx(restored.d));
    CHECK(original.b == restored.b);
    CHECK(original.s == restored.s);
  }
}

TEST_CASE("属性测试: VectorStruct roundtrip 不变性 (500 轮)") {
  SplitMix64 rng(67890);
  for (int iter = 0; iter < 500; ++iter) {
    VectorStruct original;
    size_t vi_len = rng.next() % 10;
    for (size_t j = 0; j < vi_len; ++j) {
      original.vi.push_back(static_cast<int>(rng.next()));
    }
    size_t vs_len = rng.next() % 5;
    for (size_t j = 0; j < vs_len; ++j) {
      size_t slen = rng.next() % 16;
      std::string s(slen, ' ');
      for (size_t k = 0; k < slen; ++k) {
        s[k] = static_cast<char>(32 + (rng.next() % 95));
      }
      original.vs.push_back(std::move(s));
    }

    std::string json;
    tinyrefl::reflection_to_json(original, json);

    CAPTURE(iter);
    CAPTURE(json);
    CHECK(is_valid_json(json));

    VectorStruct restored{};
    auto st = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(st.ok);
    CHECK(original.vi == restored.vi);
    CHECK(original.vs == restored.vs);
  }
}
