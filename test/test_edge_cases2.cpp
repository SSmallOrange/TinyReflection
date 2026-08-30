#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"

#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace std;

// ============================================================
// Structures with ignore fields at various positions
// ============================================================

// ignore field at the END
struct IgnoreAtEnd {
  int id;
  string name;
  tinyrefl::ignore<shared_ptr<int>> _cache;
};

// ignore field at the BEGINNING
struct IgnoreAtBegin {
  tinyrefl::ignore<shared_ptr<int>> _cache;
  int id;
  string name;
};

// ignore field in the MIDDLE
struct IgnoreInMiddle {
  int id;
  tinyrefl::ignore<shared_ptr<int>> _cache;
  string name;
};

// multiple ignore fields
struct MultiIgnore {
  tinyrefl::ignore<int*> _p1;
  int value;
  tinyrefl::ignore<int*> _p2;
  string label;
  tinyrefl::ignore<int*> _p3;
};

// only ignore fields
// After fix to for_each_serializable_member, this now compiles correctly.
struct AllIgnore {
  tinyrefl::ignore<shared_ptr<int>> a;
  tinyrefl::ignore<shared_ptr<int>> b;
};

// ============================================================
// CASE 1: ignore at end - JSON should NOT have trailing comma
// ============================================================
TEST_CASE("to_json - ignore at end: no trailing comma") {
  IgnoreAtEnd obj{42, "hello", {}};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  // Should produce {"id":42,"name":"hello"} - no trailing comma, valid JSON
  INFO("json = " << json);
  CHECK(!json.empty());
  // Must not end with comma before closing brace
  CHECK(json.find(",}") == string::npos);
  // Roundtrip
  IgnoreAtEnd restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.id == 42);
  CHECK(restored.name == "hello");
}

// ============================================================
// CASE 2: ignore at beginning - no leading comma
// ============================================================
TEST_CASE("to_json - ignore at beginning: no leading comma") {
  IgnoreAtBegin obj{{}, 99, "world"};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  INFO("json = " << json);
  CHECK(!json.empty());
  // Must not start with comma after opening brace
  CHECK(json.find("{,") == string::npos);
  // Roundtrip
  IgnoreAtBegin restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.id == 99);
  CHECK(restored.name == "world");
}

// ============================================================
// CASE 3: ignore in middle - no double comma
// ============================================================
TEST_CASE("to_json - ignore in middle: no double comma") {
  IgnoreInMiddle obj{7, {}, "mid"};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  INFO("json = " << json);
  CHECK(!json.empty());
  CHECK(json.find(",,") == string::npos);
  CHECK(json.find(",}") == string::npos);
  CHECK(json.find("{,") == string::npos);
  // Roundtrip
  IgnoreInMiddle restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.id == 7);
  CHECK(restored.name == "mid");
}

// ============================================================
// CASE 4: multiple ignore fields - commas only between real fields
// ============================================================
TEST_CASE("to_json - multiple ignore fields: correct comma count") {
  MultiIgnore obj{{}, 100, {}, "label_x", {}};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  INFO("json = " << json);
  CHECK(!json.empty());
  CHECK(json.find(",,") == string::npos);
  CHECK(json.find(",}") == string::npos);
  CHECK(json.find("{,") == string::npos);

  // Should contain exactly one comma (between "value" and "label")
  int comma_count = 0;
  for (char ch : json) {
    if (ch == ',') ++comma_count;
  }
  CHECK(comma_count == 1);

  // Roundtrip
  MultiIgnore restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.value == 100);
  CHECK(restored.label == "label_x");
}

// ============================================================
// CASE 5: all-ignore struct - should produce empty JSON object {}
// ============================================================
TEST_CASE("to_json - all ignore fields: produces empty object") {
  AllIgnore obj{{}, {}};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  INFO("json = " << json);
  // Should be "{}" - empty object
  CHECK(json == "{}");
}

// ============================================================
// CASE 6: roundtrip with ignore field - ignore field keeps default
// after deserialization (even if JSON has extra keys matching the name)
// ============================================================
TEST_CASE("from_json - ignore field stays at default after parse") {
  // Serialize first to get clean JSON (won't contain _cache)
  IgnoreAtEnd obj{55, "test", {}};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  IgnoreAtEnd restored{11, "old", {}};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.id == 55);
  CHECK(restored.name == "test");
  // ignore field should remain nullptr (default)
  CHECK(restored._cache.get() == nullptr);
}

// ============================================================
// CASE 7: Repeated deserialization of same type into same object
// Tests that static offset_map inside DispatchHandler constructor
// does not corrupt across calls
// ============================================================
struct SimpleVal {
  int x;
  int y;
};

TEST_CASE("from_json - repeated deserialization of same type") {
  SimpleVal v{};

  auto s1 = tinyrefl::reflection_from_json(v, R"({"x":1,"y":2})");
  CHECK(s1.ok == true);
  CHECK(v.x == 1);
  CHECK(v.y == 2);

  auto s2 = tinyrefl::reflection_from_json(v, R"({"x":10,"y":20})");
  CHECK(s2.ok == true);
  CHECK(v.x == 10);
  CHECK(v.y == 20);

  auto s3 = tinyrefl::reflection_from_json(v, R"({"x":-5,"y":0})");
  CHECK(s3.ok == true);
  CHECK(v.x == -5);
  CHECK(v.y == 0);
}

// ============================================================
// CASE 8: Repeated deserialization of different objects of same type
// ============================================================
TEST_CASE("from_json - multiple objects of same type independently") {
  SimpleVal a{}, b{}, c{};

  CHECK(tinyrefl::reflection_from_json(a, R"({"x":1,"y":2})").ok);
  CHECK(tinyrefl::reflection_from_json(b, R"({"x":3,"y":4})").ok);
  CHECK(tinyrefl::reflection_from_json(c, R"({"x":5,"y":6})").ok);

  CHECK(a.x == 1);
  CHECK(a.y == 2);
  CHECK(b.x == 3);
  CHECK(b.y == 4);
  CHECK(c.x == 5);
  CHECK(c.y == 6);
}

// ============================================================
// CASE 9: Nested struct: field after nested struct is correctly parsed
// Checks that after EndObject of nested handler, the parent handler
// correctly resumes and processes subsequent fields.
// ============================================================
struct AfterNested {
  string before;
  SimpleVal nested;
  string after;  // <-- this field must be parsed correctly
  int count;
};

TEST_CASE("from_json - field after nested struct is parsed") {
  const char* json =
      R"({"before":"B","nested":{"x":10,"y":20},"after":"A","count":99})";
  AfterNested obj{};
  auto status = tinyrefl::reflection_from_json(obj, json);
  CHECK(status.ok == true);
  CHECK(obj.before == "B");
  CHECK(obj.nested.x == 10);
  CHECK(obj.nested.y == 20);
  CHECK(obj.after == "A");  // <-- key assertion: field after nested
  CHECK(obj.count == 99);
}

// ============================================================
// CASE 10: Field BEFORE nested struct also parsed correctly
// ============================================================
struct BeforeAndAfter {
  int before;
  SimpleVal mid;
  int after;
};

TEST_CASE("from_json - fields before and after nested struct") {
  const char* json = R"({"before":1,"mid":{"x":2,"y":3},"after":4})";
  BeforeAndAfter obj{};
  auto status = tinyrefl::reflection_from_json(obj, json);
  CHECK(status.ok == true);
  CHECK(obj.before == 1);
  CHECK(obj.mid.x == 2);
  CHECK(obj.mid.y == 3);
  CHECK(obj.after ==
        4);  // <-- critical: does parent resume after nested EndObject?
}

// ============================================================
// CASE 11: Vector followed by a non-vector field
// After EndArray, the parent handler must resume for the next key
// ============================================================
struct VecThenField {
  vector<int> nums;
  string name;
  int count;
};

TEST_CASE("from_json - field after vector is parsed") {
  const char* json = R"({"nums":[1,2,3],"name":"hello","count":7})";
  VecThenField obj{};
  auto status = tinyrefl::reflection_from_json(obj, json);
  CHECK(status.ok == true);
  REQUIRE(obj.nums.size() == 3);
  CHECK(obj.nums[0] == 1);
  CHECK(obj.name == "hello");  // <-- field after array
  CHECK(obj.count == 7);
}

// ============================================================
// CASE 12: Multiple vectors in one struct
// ============================================================
struct MultiVec {
  vector<int> a;
  vector<string> b;
  vector<double> c;
};

TEST_CASE("from_json - multiple vector fields all parsed") {
  const char* json = R"({"a":[1,2],"b":["x","y","z"],"c":[1.1,2.2]})";
  MultiVec obj{};
  auto status = tinyrefl::reflection_from_json(obj, json);
  CHECK(status.ok == true);
  REQUIRE(obj.a.size() == 2);
  CHECK(obj.a[0] == 1);
  CHECK(obj.a[1] == 2);
  REQUIRE(obj.b.size() == 3);
  CHECK(obj.b[0] == "x");
  CHECK(obj.b[2] == "z");
  REQUIRE(obj.c.size() == 2);
  CHECK(std::abs(obj.c[0] - 1.1) < 1e-9);
  CHECK(std::abs(obj.c[1] - 2.2) < 1e-9);
}

// ============================================================
// CASE 13: Nested struct with multiple children of same type
// ============================================================
struct TwoNested {
  SimpleVal first;
  SimpleVal second;
};

TEST_CASE("from_json - two nested structs of same type") {
  const char* json = R"({"first":{"x":10,"y":20},"second":{"x":30,"y":40}})";
  TwoNested obj{};
  auto status = tinyrefl::reflection_from_json(obj, json);
  CHECK(status.ok == true);
  CHECK(obj.first.x == 10);
  CHECK(obj.first.y == 20);
  CHECK(obj.second.x == 30);
  CHECK(obj.second.y == 40);
}

// ============================================================
// CASE 14: Very deeply nested struct (4 levels)
// ============================================================
struct L1 {
  int v;
};
struct L2 {
  L1 a;
  L1 b;
};
struct L3 {
  L2 x;
  string label;
};
struct L4 {
  L3 inner;
  int count;
};

TEST_CASE("roundtrip - 4-level deep nested struct") {
  L4 obj{{{{1}, {2}}, "lbl"}, 99};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  L4 restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.inner.x.a.v == 1);
  CHECK(restored.inner.x.b.v == 2);
  CHECK(restored.inner.label == "lbl");
  CHECK(restored.count == 99);
}

// ============================================================
// CASE 15: list<int> and deque<int> deserialization
// ============================================================
struct WithList {
  list<int> data;
};
struct WithDeque {
  deque<string> items;
};

TEST_CASE("roundtrip - list<int>") {
  WithList obj{{10, 20, 30}};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  WithList restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  REQUIRE(restored.data.size() == 3);
  auto it = restored.data.begin();
  CHECK(*it++ == 10);
  CHECK(*it++ == 20);
  CHECK(*it == 30);
}

TEST_CASE("roundtrip - deque<string>") {
  WithDeque obj{{"alpha", "beta", "gamma"}};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  WithDeque restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  REQUIRE(restored.items.size() == 3);
  CHECK(restored.items[0] == "alpha");
  CHECK(restored.items[1] == "beta");
  CHECK(restored.items[2] == "gamma");
}

// ============================================================
// CASE 16: JSON keys in different order than struct field order
// ============================================================
struct OrderTest {
  int a;
  int b;
  int c;
};

TEST_CASE("from_json - JSON keys in reversed order") {
  const char* json = R"({"c":3,"b":2,"a":1})";
  OrderTest obj{};
  auto status = tinyrefl::reflection_from_json(obj, json);
  CHECK(status.ok == true);
  CHECK(obj.a == 1);
  CHECK(obj.b == 2);
  CHECK(obj.c == 3);
}

// ============================================================
// CASE 17: Struct member that is a float - from_json with int-like JSON value
// RapidJSON parses "1" as Int, but struct member is float/double.
// Check that Int->float assignment works (is_assignable_v<float&, int> = true)
// ============================================================
struct FloatStruct {
  float f;
  double d;
};

TEST_CASE("from_json - float/double from integer JSON literal") {
  const char* json = R"({"f":3,"d":5})";
  FloatStruct obj{};
  auto status = tinyrefl::reflection_from_json(obj, json);
  CHECK(status.ok == true);
  CHECK(std::abs(obj.f - 3.0f) < 1e-5f);
  CHECK(std::abs(obj.d - 5.0) < 1e-9);
}

// ============================================================
// CASE 18: bool field from JSON — both true/false literals
// ============================================================
struct BoolStruct {
  bool a;
  bool b;
};

TEST_CASE("roundtrip - bool fields") {
  BoolStruct obj{true, false};
  string json;
  tinyrefl::reflection_to_json(obj, json);

  BoolStruct restored{false, true};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.a == true);
  CHECK(restored.b == false);
}

// ============================================================
// CASE 19: (Skipped - vector<bool> is NOT supported: see note above)
// ============================================================

// ============================================================
// CASE 20: Struct with nested struct + vector field after nested
// This is the most complex stack management test:
// struct { nested_struct; vector<int>; string }
// After EndObject of nested, should correctly parse vector and string
// ============================================================
struct Complex3 {
  SimpleVal nested;
  vector<int> nums;
  string tag;
};

TEST_CASE("from_json - nested struct then vector then string field") {
  const char* json = R"({"nested":{"x":5,"y":6},"nums":[7,8,9],"tag":"end"})";
  Complex3 obj{};
  auto status = tinyrefl::reflection_from_json(obj, json);
  CHECK(status.ok == true);
  CHECK(obj.nested.x == 5);
  CHECK(obj.nested.y == 6);
  REQUIRE(obj.nums.size() == 3);
  CHECK(obj.nums[0] == 7);
  CHECK(obj.nums[1] == 8);
  CHECK(obj.nums[2] == 9);
  CHECK(obj.tag ==
        "end");  // <-- must be parsed after both nested obj and array
}
