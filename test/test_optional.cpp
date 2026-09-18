#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"
#include "tinyrefl/utils/reflection_utils.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Test types
// ---------------------------------------------------------------------------

enum class Color { Red, Green, Blue };

struct Inner {
  int id;
  std::string label;
};

struct OptScalar {
  std::optional<int> i;
  std::optional<bool> b;
  std::optional<double> d;
};

struct OptInt64 {
  std::optional<int64_t> i;
  std::optional<uint64_t> u;
};

struct OptString {
  std::optional<std::string> s;
};

struct OptChar {
  std::optional<char> c;
};

struct OptEnum {
  std::optional<Color> c;
};

struct OptInner {
  std::optional<Inner> inner;
};

struct OptVec {
  std::optional<std::vector<int>> v;
};

struct OptMap {
  std::optional<std::map<std::string, int>> m;
};

struct Mixed {
  int plain;
  std::optional<int> opt;
};

struct VecOfOptInt {
  std::vector<std::optional<int>> items;
};

struct VecOfOptStr {
  std::vector<std::optional<std::string>> items;
};

struct VecOfOptEnum {
  std::vector<std::optional<Color>> colors;
};

struct VecOfOptInner {
  std::vector<std::optional<Inner>> items;
};

struct MapOfOptInt {
  std::map<std::string, std::optional<int>> table;
};

struct MapOfOptEnum {
  std::map<std::string, std::optional<Color>> colors;
};

struct MapOfOptInner {
  std::map<std::string, std::optional<Inner>> items;
};

struct InnerWithOptional {
  int id;
  std::optional<std::string> note;
};

struct Middle {
  std::optional<InnerWithOptional> deep;
};

struct Top {
  std::optional<Middle> mid;
};

struct Config {
  std::optional<int> retry_count;
  std::optional<std::string> comment;
  std::optional<Color> color;
  std::optional<Inner> inner;
  std::optional<std::vector<int>> values;
  std::vector<std::optional<int>> slots;
  std::map<std::string, std::optional<int>> table;
};

// ---------------------------------------------------------------------------
// Type traits
// ---------------------------------------------------------------------------

TEST_CASE("type traits - is_optional_v") {
  static_assert(tinyrefl::detail::is_optional_v<std::optional<int>>);
  static_assert(
      tinyrefl::detail::is_optional_v<const std::optional<int>&>);
  static_assert(!tinyrefl::detail::is_optional_v<int>);
  static_assert(!tinyrefl::detail::is_optional_v<std::vector<int>>);
  static_assert(!tinyrefl::detail::is_optional_v<std::string>);
  CHECK(true);
}

TEST_CASE("type traits - optional_inner_type_t") {
  static_assert(std::is_same_v<
                tinyrefl::detail::optional_inner_type_t<std::optional<int>>,
                int>);
  static_assert(
      std::is_same_v<
          tinyrefl::detail::optional_inner_type_t<
              const std::optional<std::string>&>,
          std::string>);
  CHECK(true);
}

TEST_CASE("type traits - optional is not custom type and is serializable") {
  static_assert(!tinyrefl::detail::is_custom_type_v<std::optional<Inner>>);
  static_assert(
      !tinyrefl::detail::is_custom_type_v<std::optional<std::vector<int>>>);
  static_assert(
      tinyrefl::detail::is_serializable_v<std::optional<Inner>>);
  CHECK(true);
}

// ---------------------------------------------------------------------------
// Scalar optionals
// ---------------------------------------------------------------------------

TEST_CASE("optional scalar: nullopt serializes as null") {
  OptScalar value;
  std::string out;
  tinyrefl::reflection_to_json(value, out);
  CHECK(out == R"({"i":null,"b":null,"d":null})");
}

TEST_CASE("optional scalar: value serializes as inner value") {
  OptScalar value;
  value.i = 42;
  value.b = true;
  value.d = 3.5;
  std::string out;
  tinyrefl::reflection_to_json(value, out);
  CHECK(out == R"({"i":42,"b":true,"d":3.5})");
}

TEST_CASE("optional scalar: roundtrip") {
  OptScalar original;
  original.i = -7;
  original.b = false;
  original.d = 1.25;
  std::string out;
  tinyrefl::reflection_to_json(original, out);

  OptScalar restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  CHECK(restored.i == original.i);
  CHECK(restored.b == original.b);
  REQUIRE(restored.d.has_value());
  CHECK(*restored.d == doctest::Approx(*original.d));
}

TEST_CASE("optional scalar: null deserializes to nullopt") {
  const char* json = R"({"i":null,"b":null,"d":null})";
  OptScalar restored;
  restored.i = 1;
  restored.b = true;
  restored.d = 2.0;
  auto st = tinyrefl::reflection_from_json(restored, json);
  REQUIRE(st.ok);
  CHECK_FALSE(restored.i.has_value());
  CHECK_FALSE(restored.b.has_value());
  CHECK_FALSE(restored.d.has_value());
}

TEST_CASE("optional<int64_t>/optional<uint64_t>: roundtrip") {
  OptInt64 original;
  original.i = -9000000000LL;
  original.u = 18000000000ULL;
  std::string out;
  tinyrefl::reflection_to_json(original, out);
  CHECK(out == R"({"i":-9000000000,"u":18000000000})");

  OptInt64 restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  CHECK(restored.i == original.i);
  CHECK(restored.u == original.u);
}

TEST_CASE("mixed plain and optional members roundtrip") {
  Mixed original{5, 6};
  std::string out;
  tinyrefl::reflection_to_json(original, out);
  CHECK(out == R"({"plain":5,"opt":6})");

  Mixed restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  CHECK(restored.plain == 5);
  REQUIRE(restored.opt.has_value());
  CHECK(*restored.opt == 6);
}

// ---------------------------------------------------------------------------
// String / char optionals
// ---------------------------------------------------------------------------

TEST_CASE("optional<string>: serialize both states") {
  OptString value;
  std::string out;
  tinyrefl::reflection_to_json(value, out);
  CHECK(out == R"({"s":null})");

  value.s = "hello";
  out.clear();
  tinyrefl::reflection_to_json(value, out);
  CHECK(out == R"({"s":"hello"})");
}

TEST_CASE("optional<string>: deserialize value and null") {
  OptString restored;
  auto st = tinyrefl::reflection_from_json(restored, R"({"s":"world"})");
  REQUIRE(st.ok);
  REQUIRE(restored.s.has_value());
  CHECK(restored.s.value() == "world");

  restored.s = "pre";
  st = tinyrefl::reflection_from_json(restored, R"({"s":null})");
  REQUIRE(st.ok);
  CHECK_FALSE(restored.s.has_value());
}

TEST_CASE("optional<string>: embedded \\u0000 is preserved") {
  const char* json = R"({"s":"hello\u0000world"})";
  OptString restored;
  auto st = tinyrefl::reflection_from_json(restored, json);
  REQUIRE(st.ok);
  REQUIRE(restored.s.has_value());
  CHECK(restored.s->size() == 11);
  CHECK((*restored.s)[5] == '\0');
  CHECK((*restored.s)[6] == 'w');
}

TEST_CASE("optional<char>: roundtrip") {
  OptChar original;
  original.c = 'Z';
  std::string out;
  tinyrefl::reflection_to_json(original, out);
  CHECK(out == R"({"c":"Z"})");

  OptChar restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  REQUIRE(restored.c.has_value());
  CHECK(*restored.c == 'Z');
}

TEST_CASE("optional<char>: null deserializes to nullopt") {
  OptChar restored;
  restored.c = 'A';
  auto st = tinyrefl::reflection_from_json(restored, R"({"c":null})");
  REQUIRE(st.ok);
  CHECK_FALSE(restored.c.has_value());
}

// ---------------------------------------------------------------------------
// Enum optionals
// ---------------------------------------------------------------------------

TEST_CASE("optional<enum>: serialize as name string") {
  OptEnum original;
  original.c = Color::Green;
  std::string out;
  tinyrefl::reflection_to_json(original, out);
  CHECK(out == R"({"c":"Green"})");

  OptEnum nulled;
  out.clear();
  tinyrefl::reflection_to_json(nulled, out);
  CHECK(out == R"({"c":null})");
}

TEST_CASE("optional<enum>: deserialize from name / integer / null") {
  OptEnum restored;
  auto st = tinyrefl::reflection_from_json(restored, R"({"c":"Blue"})");
  REQUIRE(st.ok);
  REQUIRE(restored.c.has_value());
  CHECK(*restored.c == Color::Blue);

  st = tinyrefl::reflection_from_json(restored, R"({"c":1})");
  REQUIRE(st.ok);
  REQUIRE(restored.c.has_value());
  CHECK(*restored.c == Color::Green);

  // invalid integer -> stays nullopt
  OptEnum invalid;
  st = tinyrefl::reflection_from_json(invalid, R"({"c":999})");
  REQUIRE(st.ok);
  CHECK_FALSE(invalid.c.has_value());

  // invalid name -> stays nullopt
  OptEnum bad_name;
  st = tinyrefl::reflection_from_json(bad_name, R"({"c":"Purple"})");
  REQUIRE(st.ok);
  CHECK_FALSE(bad_name.c.has_value());

  // null -> nullopt
  OptEnum nulled;
  nulled.c = Color::Red;
  st = tinyrefl::reflection_from_json(nulled, R"({"c":null})");
  REQUIRE(st.ok);
  CHECK_FALSE(nulled.c.has_value());
}

// ---------------------------------------------------------------------------
// Struct / container optionals
// ---------------------------------------------------------------------------

TEST_CASE("optional<Inner>: roundtrip") {
  OptInner original;
  original.inner = Inner{7, "seven"};
  std::string out;
  tinyrefl::reflection_to_json(original, out);
  CHECK(out == R"({"inner":{"id":7,"label":"seven"}})");

  OptInner restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  REQUIRE(restored.inner.has_value());
  CHECK(restored.inner->id == 7);
  CHECK(restored.inner->label == "seven");
}

TEST_CASE("optional<Inner>: null in both directions") {
  OptInner nulled;
  std::string out;
  tinyrefl::reflection_to_json(nulled, out);
  CHECK(out == R"({"inner":null})");

  OptInner restored;
  restored.inner = Inner{1, "x"};
  auto st = tinyrefl::reflection_from_json(restored, R"({"inner":null})");
  REQUIRE(st.ok);
  CHECK_FALSE(restored.inner.has_value());
}

TEST_CASE("optional<vector<int>>: roundtrip") {
  OptVec original;
  original.v = std::vector<int>{1, 2, 3};
  std::string out;
  tinyrefl::reflection_to_json(original, out);
  CHECK(out == R"({"v":[1,2,3]})");

  OptVec restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  REQUIRE(restored.v.has_value());
  CHECK(*restored.v == std::vector<int>{1, 2, 3});
}

TEST_CASE("optional<vector<int>>: null in both directions") {
  OptVec nulled;
  std::string out;
  tinyrefl::reflection_to_json(nulled, out);
  CHECK(out == R"({"v":null})");

  OptVec restored;
  auto st = tinyrefl::reflection_from_json(restored, R"({"v":null})");
  REQUIRE(st.ok);
  CHECK_FALSE(restored.v.has_value());
}

TEST_CASE("optional<map<string,int>>: roundtrip") {
  OptMap original;
  original.m = std::map<std::string, int>{{"a", 1}, {"b", 2}};
  std::string out;
  tinyrefl::reflection_to_json(original, out);

  OptMap restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  REQUIRE(restored.m.has_value());
  REQUIRE(restored.m->size() == 2);
  CHECK((*restored.m)["a"] == 1);
  CHECK((*restored.m)["b"] == 2);
}

TEST_CASE("optional<map<string,int>>: null deserializes to nullopt") {
  OptMap restored;
  auto st = tinyrefl::reflection_from_json(restored, R"({"m":null})");
  REQUIRE(st.ok);
  CHECK_FALSE(restored.m.has_value());
}

// ---------------------------------------------------------------------------
// Sequence elements
// ---------------------------------------------------------------------------

TEST_CASE("vector<optional<int>>: elements roundtrip") {
  VecOfOptInt original;
  original.items = {1, std::nullopt, 3};
  std::string out;
  tinyrefl::reflection_to_json(original, out);
  CHECK(out == R"({"items":[1,null,3]})");

  VecOfOptInt restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  REQUIRE(restored.items.size() == 3);
  REQUIRE(restored.items[0].has_value());
  CHECK(*restored.items[0] == 1);
  CHECK_FALSE(restored.items[1].has_value());
  REQUIRE(restored.items[2].has_value());
  CHECK(*restored.items[2] == 3);
}

TEST_CASE("vector<optional<string>>: null and embedded null byte") {
  const char* json = R"({"items":["ab\u0000cd",null,"ok"]})";
  VecOfOptStr restored;
  auto st = tinyrefl::reflection_from_json(restored, json);
  REQUIRE(st.ok);
  REQUIRE(restored.items.size() == 3);
  REQUIRE(restored.items[0].has_value());
  CHECK(restored.items[0]->size() == 5);
  CHECK((*restored.items[0])[2] == '\0');
  CHECK_FALSE(restored.items[1].has_value());
  REQUIRE(restored.items[2].has_value());
  CHECK(*restored.items[2] == "ok");
}

TEST_CASE("vector<optional<enum>>: names, integers and null") {
  VecOfOptEnum original;
  original.colors = {Color::Red, std::nullopt, Color::Blue};
  std::string out;
  tinyrefl::reflection_to_json(original, out);
  CHECK(out == R"({"colors":["Red",null,"Blue"]})");

  VecOfOptEnum restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  REQUIRE(restored.colors.size() == 3);
  REQUIRE(restored.colors[0].has_value());
  CHECK(*restored.colors[0] == Color::Red);
  CHECK_FALSE(restored.colors[1].has_value());
  REQUIRE(restored.colors[2].has_value());
  CHECK(*restored.colors[2] == Color::Blue);

  // integers: invalid values are dropped, valid ones kept
  const char* json = R"({"colors":[0,999,2]})";
  VecOfOptEnum restored2;
  st = tinyrefl::reflection_from_json(restored2, json);
  REQUIRE(st.ok);
  REQUIRE(restored2.colors.size() == 2);
  REQUIRE(restored2.colors[0].has_value());
  CHECK(*restored2.colors[0] == Color::Red);
  REQUIRE(restored2.colors[1].has_value());
  CHECK(*restored2.colors[1] == Color::Blue);
}

TEST_CASE("vector<optional<Inner>>: struct elements roundtrip") {
  VecOfOptInner original;
  original.items.push_back(Inner{1, "one"});
  original.items.push_back(std::nullopt);
  original.items.push_back(Inner{3, "three"});
  std::string out;
  tinyrefl::reflection_to_json(original, out);
  CHECK(out ==
        R"({"items":[{"id":1,"label":"one"},null,{"id":3,"label":"three"}]})");

  VecOfOptInner restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  REQUIRE(restored.items.size() == 3);
  REQUIRE(restored.items[0].has_value());
  CHECK(restored.items[0]->id == 1);
  CHECK_FALSE(restored.items[1].has_value());
  REQUIRE(restored.items[2].has_value());
  CHECK(restored.items[2]->label == "three");
}

// ---------------------------------------------------------------------------
// Map values
// ---------------------------------------------------------------------------

TEST_CASE("map<string, optional<int>>: null and values roundtrip") {
  MapOfOptInt original;
  original.table["x"] = 1;
  original.table["y"] = std::nullopt;
  std::string out;
  tinyrefl::reflection_to_json(original, out);

  MapOfOptInt restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  REQUIRE(restored.table.size() == 2);
  REQUIRE(restored.table["x"].has_value());
  CHECK(*restored.table["x"] == 1);
  CHECK_FALSE(restored.table["y"].has_value());
}

TEST_CASE("map<string, optional<enum>>: integer values and null") {
  MapOfOptEnum restored;
  auto st = tinyrefl::reflection_from_json(
      restored, R"({"colors":{"a":0,"b":null,"c":2}})");
  REQUIRE(st.ok);
  REQUIRE(restored.colors.at("a").has_value());
  CHECK(*restored.colors.at("a") == Color::Red);
  CHECK_FALSE(restored.colors.at("b").has_value());
  REQUIRE(restored.colors.at("c").has_value());
  CHECK(*restored.colors.at("c") == Color::Blue);
}

TEST_CASE("map<string, optional<Inner>>: struct values roundtrip") {
  MapOfOptInner original;
  original.items["a"] = Inner{1, "A"};
  original.items["b"] = std::nullopt;
  std::string out;
  tinyrefl::reflection_to_json(original, out);

  MapOfOptInner restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  REQUIRE(restored.items.size() == 2);
  REQUIRE(restored.items["a"].has_value());
  CHECK(restored.items["a"]->id == 1);
  CHECK_FALSE(restored.items["b"].has_value());
}

// ---------------------------------------------------------------------------
// Deep nesting
// ---------------------------------------------------------------------------

TEST_CASE("nested optional<struct> containing optional members") {
  Top original;
  original.mid = Middle{InnerWithOptional{9, "deep note"}};
  std::string out;
  tinyrefl::reflection_to_json(original, out);
  CHECK(out == R"({"mid":{"deep":{"id":9,"note":"deep note"}}})");

  Top restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  REQUIRE(restored.mid.has_value());
  REQUIRE(restored.mid->deep.has_value());
  CHECK(restored.mid->deep->id == 9);
  CHECK(restored.mid->deep->note == "deep note");
}

TEST_CASE("nested optional: inner nullopt roundtrip") {
  Top original;
  original.mid = Middle{InnerWithOptional{9, std::nullopt}};
  std::string out;
  tinyrefl::reflection_to_json(original, out);
  CHECK(out == R"({"mid":{"deep":{"id":9,"note":null}}})");

  Top restored;
  auto st = tinyrefl::reflection_from_json(restored, out.c_str());
  REQUIRE(st.ok);
  REQUIRE(restored.mid.has_value());
  REQUIRE(restored.mid->deep.has_value());
  CHECK(restored.mid->deep->id == 9);
  CHECK_FALSE(restored.mid->deep->note.has_value());
}

TEST_CASE("nested optional: outer null deserializes to nullopt") {
  Top restored;
  auto st = tinyrefl::reflection_from_json(restored, R"({"mid":null})");
  REQUIRE(st.ok);
  CHECK_FALSE(restored.mid.has_value());
}

// ---------------------------------------------------------------------------
// Full example from the design doc
// ---------------------------------------------------------------------------

TEST_CASE("full example: Config roundtrip") {
  Config cfg;
  cfg.retry_count = 3;
  cfg.comment = std::nullopt;
  cfg.color = Color::Green;
  cfg.inner = Inner{1, "A"};
  cfg.values = std::nullopt;
  cfg.slots = {1, std::nullopt, 3};
  cfg.table = {{"x", 1}, {"y", std::nullopt}};

  std::string out;
  tinyrefl::reflection_to_json(cfg, out);
  CHECK(out == R"({"retry_count":3,"comment":null,"color":"Green",)"
              R"("inner":{"id":1,"label":"A"},"values":null,)"
              R"("slots":[1,null,3],"table":{"x":1,"y":null}})");

  Config obj;
  auto st = tinyrefl::reflection_from_json(obj, out.c_str());
  REQUIRE(st.ok);
  REQUIRE(obj.retry_count.has_value());
  CHECK(*obj.retry_count == 3);
  CHECK_FALSE(obj.comment.has_value());
  REQUIRE(obj.color.has_value());
  CHECK(*obj.color == Color::Green);
  REQUIRE(obj.inner.has_value());
  CHECK(obj.inner->id == 1);
  CHECK(obj.inner->label == "A");
  CHECK(obj.values == std::nullopt);
  REQUIRE(obj.slots.size() == 3);
  CHECK(*obj.slots[0] == 1);
  CHECK_FALSE(obj.slots[1].has_value());
  CHECK(*obj.slots[2] == 3);
  REQUIRE(obj.table.at("x").has_value());
  CHECK(*obj.table.at("x") == 1);
  CHECK_FALSE(obj.table.at("y").has_value());
}
