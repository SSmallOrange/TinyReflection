#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"

#include <cmath>
#include <string>
#include <vector>

using namespace std;

// ============================================================
// Structures used in tests
// ============================================================

struct IntStruct {
  int value;
};

struct FloatStruct {
  float value;
};

struct DoubleStruct {
  double value;
};

struct MixedNumeric {
  int i;
  float f;
  double d;
  unsigned int u;
  int64_t i64;
  uint64_t u64;
};

struct StringFields {
  string a;
  string b;
  string c;
};

struct NestedInner {
  int x;
  string name;
};

struct NestedOuter {
  string title;
  NestedInner inner;
  int count;
};

struct VecStrings {
  vector<string> items;
};

struct VecInts {
  vector<int> nums;
};

struct NestedWithVec {
  string label;
  vector<int> data;
  int id;
};

struct OuterWithNestedAndVec {
  int outer_id;
  NestedWithVec nested;
  vector<int> extra;
};

// ============================================================
// BUG TEST 1: int field receives JSON double value (e.g. 3.0)
// RapidJSON fires Double() for 3.0, but Int() for 3.
// assign_if_match<double> should NOT assign to int field,
// but assign_if_match<int> also never fires — value stays 0.
// ============================================================
TEST_CASE("bug - int field with JSON float literal (3.0)") {
  // JSON has "3.0" which RapidJSON will parse as Double
  const char* json = R"({"value": 3.0})";
  IntStruct restored{0};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);
  // The int field should be 3 (or at least not crash)
  // If this fails, it reveals that numeric type mismatch silently loses value
  CHECK(restored.value == 3);
}

// ============================================================
// BUG TEST 2: float field receives JSON integer (no decimal point)
// RapidJSON fires Int() for 1, not Double().
// float can be assigned from int (is_assignable = true), so this should work.
// ============================================================
TEST_CASE("bug - float field with JSON integer literal") {
  const char* json = R"({"value": 1})";
  FloatStruct restored{0.0f};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);
  CHECK(std::abs(restored.value - 1.0f) < 1e-6f);
}

// ============================================================
// BUG TEST 3: double field receives JSON integer literal
// RapidJSON fires Int() not Double() for integers.
// ============================================================
TEST_CASE("bug - double field with JSON integer literal") {
  const char* json = R"({"value": 42})";
  DoubleStruct restored{0.0};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);
  CHECK(std::abs(restored.value - 42.0) < 1e-10);
}

// ============================================================
// BUG TEST 4: int field with large positive integer in JSON
// RapidJSON may fire Uint() instead of Int() for large positives
// ============================================================
TEST_CASE("bug - int field with large positive JSON integer") {
  const char* json = R"({"value": 2147483647})";  // INT_MAX
  IntStruct restored{0};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);
  CHECK(restored.value == 2147483647);
}

// ============================================================
// BUG TEST 5: JSON field order different from struct definition
// Struct has fields: title, inner, count
// JSON provides them in a different order: count, inner, title
// ============================================================
TEST_CASE("bug - JSON field order differs from struct definition") {
  // Fields in JSON are in reverse order compared to struct definition
  const char* json = R"({
        "count": 99,
        "inner": {"x": 7, "name": "inner_name"},
        "title": "hello"
    })";
  NestedOuter restored{};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);
  CHECK(restored.title == "hello");
  CHECK(restored.inner.x == 7);
  CHECK(restored.inner.name == "inner_name");
  CHECK(restored.count == 99);
}

// ============================================================
// BUG TEST 6: Multiple string fields — verify assignment goes to correct field
// ============================================================
TEST_CASE("bug - multiple string fields correct assignment") {
  const char* json = R"({"a": "AAA", "b": "BBB", "c": "CCC"})";
  StringFields restored{};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);
  CHECK(restored.a == "AAA");
  CHECK(restored.b == "BBB");
  CHECK(restored.c == "CCC");
}

// ============================================================
// BUG TEST 7: String field followed by int field — iterator stays valid
// ============================================================
TEST_CASE("bug - string then int fields") {
  NestedInner obj{42, "test_name"};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  NestedInner restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.x == 42);
  CHECK(restored.name == "test_name");
}

// ============================================================
// BUG TEST 8: Nested struct with fields AFTER the nested struct field
// The field "count" comes after "inner" — does the handler pop correctly?
// ============================================================
TEST_CASE("bug - field after nested struct is correctly deserialized") {
  NestedOuter obj{"title_value", {10, "inner_name"}, 55};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  NestedOuter restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.title == "title_value");
  CHECK(restored.inner.x == 10);
  CHECK(restored.inner.name == "inner_name");
  // This is the KEY check: field after nested struct
  CHECK(restored.count == 55);
}

// ============================================================
// BUG TEST 9: Vector field followed by plain field after the array
// After EndArray, do we correctly continue parsing the outer struct?
// ============================================================
TEST_CASE("bug - field after vector field is correctly deserialized") {
  NestedWithVec obj{"my_label", {1, 2, 3}, 99};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  NestedWithVec restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.label == "my_label");
  REQUIRE(restored.data.size() == 3);
  CHECK(restored.data[0] == 1);
  CHECK(restored.data[2] == 3);
  // KEY CHECK: field after the vector
  CHECK(restored.id == 99);
}

// ============================================================
// BUG TEST 10: Outer struct with nested struct + vector + extra field
// Complex scenario: tests full handler stack lifecycle
// ============================================================
TEST_CASE("bug - outer struct with nested+vec+extra field roundtrip") {
  OuterWithNestedAndVec obj{100, {"nested_label", {10, 20, 30}, 42}, {7, 8, 9}};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  OuterWithNestedAndVec restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.outer_id == 100);
  CHECK(restored.nested.label == "nested_label");
  REQUIRE(restored.nested.data.size() == 3);
  CHECK(restored.nested.data[1] == 20);
  CHECK(restored.nested.id == 42);
  REQUIRE(restored.extra.size() == 3);
  CHECK(restored.extra[0] == 7);
  CHECK(restored.extra[2] == 9);
}

// ============================================================
// BUG TEST 11: JSON with null value for a field
// Null handler currently is a TODO - verify it doesn't crash
// ============================================================
struct WithNullable {
  int id;
  string name;
};

TEST_CASE("bug - JSON null value for string field doesn't crash") {
  const char* json = R"({"id": 1, "name": null})";
  WithNullable restored{};
  auto status = tinyrefl::reflection_from_json(restored, json);
  // Should at least not crash; status.ok may be true or false
  CHECK(status.ok == true);
  CHECK(restored.id == 1);
  // name keeps default value (empty) since null is unhandled
  CHECK(restored.name == "");
}

// ============================================================
// BUG TEST 12: Repeated deserialization of same struct type
// Tests for static variable contamination
// ============================================================
TEST_CASE("bug - repeated deserialization doesn't contaminate static state") {
  for (int iter = 0; iter < 5; ++iter) {
    NestedOuter obj{"iter_title_" + to_string(iter),
                    {iter, "name_" + to_string(iter)},
                    iter * 10};
    string json;
    tinyrefl::reflection_to_json(obj, json);

    NestedOuter restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(restored.title == "iter_title_" + to_string(iter));
    CHECK(restored.inner.x == iter);
    CHECK(restored.inner.name == "name_" + to_string(iter));
    CHECK(restored.count == iter * 10);
  }
}

// ============================================================
// BUG TEST 13: Vector of strings roundtrip (basic)
// ============================================================
TEST_CASE("bug - vector of strings roundtrip") {
  VecStrings obj{{"alpha", "beta", "gamma"}};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  VecStrings restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  REQUIRE(restored.items.size() == 3);
  CHECK(restored.items[0] == "alpha");
  CHECK(restored.items[1] == "beta");
  CHECK(restored.items[2] == "gamma");
}

// ============================================================
// BUG TEST 14: from_json to value (2-arg version) + status checks
// ============================================================
TEST_CASE("bug - reflection_from_json status has correct line/col on error") {
  // Line 2, missing colon
  const char* bad = "{\n\"name\" \"value\"\n}";
  NestedInner restored{};
  auto status = tinyrefl::reflection_from_json(restored, bad);
  CHECK(status.ok == false);
  CHECK(status.error.line >= 1);
}

// ============================================================
// BUG TEST 15: JSON where int field gets a Uint() event
// When JSON value is positive and fits in uint but not cast as int,
// RapidJSON fires Uint() not Int(). Verify int field gets assigned.
// ============================================================
TEST_CASE("bug - int field assigned from positive JSON int (Uint event path)") {
  // 100 is small enough to be fired as Int() but test the path
  const char* json = R"({"value": 100})";
  IntStruct restored{0};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);
  CHECK(restored.value == 100);
}
