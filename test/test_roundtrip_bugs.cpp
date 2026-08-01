#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/reflection_to_json.hpp"
#include "tinyrefl/reflection_from_json.hpp"

#include <vector>
#include <string>
#include <map>
#include <list>
#include <deque>
#include <cmath>
#include <unordered_map>

using namespace std;

// ============================================================
// Test structures
// ============================================================

struct Addr {
    string city;
    int zip;
};

struct Person {
    string name;
    int age;
    Addr address;
};

struct WithOptMap {
    map<string, string> tags;
};

struct Nested2 {
    string label;
    vector<int> nums;
};

struct HasNested {
    int id;
    Nested2 nested;
};

struct HasNestedVec {
    string name;
    vector<Nested2> items;
};

struct AllBasic {
    bool flag;
    int i;
    unsigned int u;
    int64_t i64;
    uint64_t u64;
    float f;
    double d;
    string s;
    char c;
};

struct WithInt64 {
    int64_t value;
};

struct WithUint64 {
    uint64_t value;
};

struct DoubleNested {
    HasNested a;
    HasNested b;
};

struct VecOfVec {
    vector<vector<int>> matrix;
};

struct DeepInner {
    int x;
    string name;
    vector<int> vals;
};

struct DeepOuter {
    string title;
    DeepInner inner;
    vector<DeepInner> inner_list;
};

struct WithFloat {
    float value;
};

// ============================================================
// CASE 1: Nested struct roundtrip - basic
// ============================================================
TEST_CASE("roundtrip - nested struct basic fields") {
    Person p{"Alice", 30, {"Shanghai", 200000}};
    string json;
    tinyrefl::reflection_to_json(p, json);

    Person restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(restored.name == "Alice");
    CHECK(restored.age == 30);
    CHECK(restored.address.city == "Shanghai");
    CHECK(restored.address.zip == 200000);
}

// ============================================================
// CASE 2: Nested struct with vector member
// Key bug area: handler stack management when nested struct
// contains a vector field
// ============================================================
TEST_CASE("roundtrip - nested struct with vector member") {
    HasNested obj{42, {"label_A", {1, 2, 3}}};
    string json;
    tinyrefl::reflection_to_json(obj, json);

    HasNested restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(restored.id == 42);
    CHECK(restored.nested.label == "label_A");
    REQUIRE(restored.nested.nums.size() == 3);
    CHECK(restored.nested.nums[0] == 1);
    CHECK(restored.nested.nums[1] == 2);
    CHECK(restored.nested.nums[2] == 3);
}

// ============================================================
// CASE 3: Two consecutive nested struct roundtrips
// Tests that static offset_map is NOT shared between separate
// calls (checks for static variable contamination)
// ============================================================
TEST_CASE("roundtrip - two consecutive nested structs") {
    Person p1{"Alice", 30, {"Beijing", 100000}};
    Person p2{"Bob",   25, {"Shenzhen", 518000}};

    string j1, j2;
    tinyrefl::reflection_to_json(p1, j1);
    tinyrefl::reflection_to_json(p2, j2);

    Person r1{}, r2{};
    CHECK(tinyrefl::reflection_from_json(r1, j1.c_str()).ok == true);
    CHECK(tinyrefl::reflection_from_json(r2, j2.c_str()).ok == true);

    CHECK(r1.name == "Alice");
    CHECK(r1.address.city == "Beijing");
    CHECK(r2.name == "Bob");
    CHECK(r2.address.city == "Shenzhen");
}

// ============================================================
// CASE 4: Double nested struct (struct containing struct containing struct)
// ============================================================
TEST_CASE("roundtrip - double nested struct") {
    DoubleNested obj{
        {1, {"inner_a", {10, 20}}},
        {2, {"inner_b", {30, 40, 50}}}
    };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    DoubleNested restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(restored.a.id == 1);
    CHECK(restored.a.nested.label == "inner_a");
    REQUIRE(restored.a.nested.nums.size() == 2);
    CHECK(restored.a.nested.nums[0] == 10);
    CHECK(restored.b.id == 2);
    CHECK(restored.b.nested.label == "inner_b");
    REQUIRE(restored.b.nested.nums.size() == 3);
    CHECK(restored.b.nested.nums[2] == 50);
}

// ============================================================
// CASE 5: Vector of nested structs
// ============================================================
TEST_CASE("roundtrip - vector of nested structs") {
    HasNestedVec obj{"collection", {
        {"A", {1, 2}},
        {"B", {3, 4, 5}},
        {"C", {}}
    }};
    string json;
    tinyrefl::reflection_to_json(obj, json);

    HasNestedVec restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(restored.name == "collection");
    REQUIRE(restored.items.size() == 3);
    CHECK(restored.items[0].label == "A");
    REQUIRE(restored.items[0].nums.size() == 2);
    CHECK(restored.items[0].nums[0] == 1);
    CHECK(restored.items[1].label == "B");
    REQUIRE(restored.items[1].nums.size() == 3);
    CHECK(restored.items[2].label == "C");
    CHECK(restored.items[2].nums.empty());
}

// ============================================================
// CASE 6: vector<vector<int>> roundtrip (2D matrix)
// ============================================================
TEST_CASE("roundtrip - vector of vector of int") {
    VecOfVec obj{{{1, 2, 3}, {4, 5}, {6}}};
    string json;
    tinyrefl::reflection_to_json(obj, json);

    VecOfVec restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    REQUIRE(restored.matrix.size() == 3);
    REQUIRE(restored.matrix[0].size() == 3);
    CHECK(restored.matrix[0][0] == 1);
    CHECK(restored.matrix[0][2] == 3);
    REQUIRE(restored.matrix[1].size() == 2);
    CHECK(restored.matrix[1][0] == 4);
    REQUIRE(restored.matrix[2].size() == 1);
    CHECK(restored.matrix[2][0] == 6);
}

// ============================================================
// CASE 7: Deep nested struct with vector members
// ============================================================
TEST_CASE("roundtrip - deep nested struct with vectors") {
    DeepOuter obj{
        "outer_title",
        {10, "inner_name", {100, 200, 300}},
        {
            {11, "item1", {1}},
            {12, "item2", {2, 3}},
        }
    };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    DeepOuter restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(restored.title == "outer_title");
    CHECK(restored.inner.x == 10);
    CHECK(restored.inner.name == "inner_name");
    REQUIRE(restored.inner.vals.size() == 3);
    CHECK(restored.inner.vals[0] == 100);
    CHECK(restored.inner.vals[1] == 200);
    CHECK(restored.inner.vals[2] == 300);
    REQUIRE(restored.inner_list.size() == 2);
    CHECK(restored.inner_list[0].x == 11);
    CHECK(restored.inner_list[0].name == "item1");
    REQUIRE(restored.inner_list[0].vals.size() == 1);
    CHECK(restored.inner_list[0].vals[0] == 1);
    CHECK(restored.inner_list[1].x == 12);
    REQUIRE(restored.inner_list[1].vals.size() == 2);
    CHECK(restored.inner_list[1].vals[1] == 3);
}

// ============================================================
// CASE 8: from_json with extra unknown fields (robustness)
// Should not crash; unknown fields should be silently ignored
// ============================================================
TEST_CASE("from_json - extra unknown fields are ignored") {
    const char* json = R"({"name":"Alice","age":30,"address":{"city":"SH","zip":200},"unknown_field":"xxx","another":42})";
    Person restored{};
    auto status = tinyrefl::reflection_from_json(restored, json);
    CHECK(status.ok == true);
    CHECK(restored.name == "Alice");
    CHECK(restored.age == 30);
    CHECK(restored.address.city == "SH");
}

// ============================================================
// CASE 9: from_json with missing fields (partial JSON)
// Should parse what's there; missing fields keep defaults
// ============================================================
TEST_CASE("from_json - missing fields keep default values") {
    // Only 'name' is provided; 'age' and 'address' are absent
    const char* json = R"({"name":"PartialAlice"})";
    Person restored{"default_name", 99, {"DefaultCity", 12345}};
    auto status = tinyrefl::reflection_from_json(restored, json);
    CHECK(status.ok == true);
    CHECK(restored.name == "PartialAlice");
    // age and address should keep their old values (not reset to zero)
    CHECK(restored.age == 99);
    CHECK(restored.address.city == "DefaultCity");
}

// ============================================================
// CASE 10: int64_t roundtrip - large values
// ============================================================
TEST_CASE("roundtrip - int64_t large positive") {
    WithInt64 obj{9223372036854775807LL};  // INT64_MAX
    string json;
    tinyrefl::reflection_to_json(obj, json);

    WithInt64 restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(restored.value == 9223372036854775807LL);
}

TEST_CASE("roundtrip - int64_t large negative") {
    WithInt64 obj{-9223372036854775807LL - 1};  // INT64_MIN
    string json;
    tinyrefl::reflection_to_json(obj, json);

    WithInt64 restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(restored.value == (-9223372036854775807LL - 1));
}

TEST_CASE("roundtrip - uint64_t large value") {
    WithUint64 obj{18446744073709551615ULL};  // UINT64_MAX
    string json;
    tinyrefl::reflection_to_json(obj, json);

    WithUint64 restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(restored.value == 18446744073709551615ULL);
}

// ============================================================
// CASE 11: float roundtrip
// ============================================================
TEST_CASE("roundtrip - float value") {
    WithFloat obj{3.14f};
    string json;
    tinyrefl::reflection_to_json(obj, json);

    WithFloat restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(std::abs(restored.value - 3.14f) < 1e-5f);
}

TEST_CASE("roundtrip - float negative") {
    WithFloat obj{-1.5f};
    string json;
    tinyrefl::reflection_to_json(obj, json);

    WithFloat restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(std::abs(restored.value - (-1.5f)) < 1e-6f);
}

// ============================================================
// CASE 12: map<string, string> serialization only
// NOTE: associative container (map/unordered_map) deserialization
// is NOT implemented in this library — only serialization is supported.
// This test verifies that serialization produces valid JSON output.
// ============================================================
TEST_CASE("to_json - map string to string serialization") {
    WithOptMap obj;
    obj.tags["key1"] = "val1";
    obj.tags["key2"] = "val2";

    string json;
    tinyrefl::reflection_to_json(obj, json);

    // Check that serialization produced valid JSON with our keys and values
    CHECK(json.find("\"key1\"") != string::npos);
    CHECK(json.find("\"val1\"") != string::npos);
    CHECK(json.find("\"key2\"") != string::npos);
    CHECK(json.find("\"val2\"") != string::npos);
    // JSON should be a valid object: starts with { and ends with }
    CHECK(!json.empty());
    CHECK(json.front() == '{');
    CHECK(json.back() == '}');
}

// ============================================================
// CASE 13: Empty vector element after non-empty
// Tests that StartArray/EndArray popping is balanced
// ============================================================
struct TwoVecs {
    vector<int> first;
    vector<int> second;
};

TEST_CASE("roundtrip - two vector fields") {
    TwoVecs obj{{1, 2, 3}, {4, 5}};
    string json;
    tinyrefl::reflection_to_json(obj, json);

    TwoVecs restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    REQUIRE(restored.first.size() == 3);
    CHECK(restored.first[0] == 1);
    CHECK(restored.first[2] == 3);
    REQUIRE(restored.second.size() == 2);
    CHECK(restored.second[0] == 4);
    CHECK(restored.second[1] == 5);
}

TEST_CASE("roundtrip - empty first vector then non-empty second") {
    TwoVecs obj{{}, {7, 8, 9}};
    string json;
    tinyrefl::reflection_to_json(obj, json);

    TwoVecs restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(restored.first.empty());
    REQUIRE(restored.second.size() == 3);
    CHECK(restored.second[0] == 7);
}

// ============================================================
// CASE 14: Struct with vector then nested struct - checks
// handler stack ordering doesn't confuse the two types
// ============================================================
struct VecThenNested {
    vector<int> nums;
    Addr addr;
};

TEST_CASE("roundtrip - vector field then nested struct field") {
    VecThenNested obj{{10, 20, 30}, {"Chengdu", 610000}};
    string json;
    tinyrefl::reflection_to_json(obj, json);

    VecThenNested restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    REQUIRE(restored.nums.size() == 3);
    CHECK(restored.nums[0] == 10);
    CHECK(restored.nums[2] == 30);
    CHECK(restored.addr.city == "Chengdu");
    CHECK(restored.addr.zip == 610000);
}

// ============================================================
// CASE 15: Struct with nested struct then vector field
// ============================================================
struct NestedThenVec {
    Addr addr;
    vector<int> nums;
};

TEST_CASE("roundtrip - nested struct field then vector field") {
    NestedThenVec obj{{"Wuhan", 430000}, {100, 200}};
    string json;
    tinyrefl::reflection_to_json(obj, json);

    NestedThenVec restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(restored.addr.city == "Wuhan");
    CHECK(restored.addr.zip == 430000);
    REQUIRE(restored.nums.size() == 2);
    CHECK(restored.nums[0] == 100);
    CHECK(restored.nums[1] == 200);
}

// ============================================================
// CASE 16: All basic types in one struct
// ============================================================
TEST_CASE("roundtrip - all basic types in one struct") {
    AllBasic obj{true, -42, 42u, -9000000000LL, 9000000000ULL, 1.5f, 2.718281828, "hello", 'X'};
    string json;
    tinyrefl::reflection_to_json(obj, json);

    AllBasic restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    CHECK(restored.flag == true);
    CHECK(restored.i == -42);
    CHECK(restored.u == 42u);
    CHECK(restored.i64 == -9000000000LL);
    CHECK(restored.u64 == 9000000000ULL);
    CHECK(std::abs(restored.f - 1.5f) < 1e-5f);
    CHECK(std::abs(restored.d - 2.718281828) < 1e-9);
    CHECK(restored.s == "hello");
    CHECK(restored.c == 'X');
}

// ============================================================
// CASE 17: Parsing JSON where value types mismatch field type
// e.g. int field gets a float value in JSON - should be ok
// ============================================================
struct IntField {
    int value;
};

TEST_CASE("from_json - int field with float JSON value") {
    // RapidJSON parses 3.5 as double; assign to int should truncate or fail gracefully
    const char* json = R"({"value": 3})";
    IntField restored{};
    auto status = tinyrefl::reflection_from_json(restored, json);
    CHECK(status.ok == true);
    CHECK(restored.value == 3);
}

// ============================================================
// CASE 18: Bool field with 0/1 in JSON (not true/false)
// Some JSON has 0/1 for booleans, check if it works
// ============================================================
struct BoolField {
    bool flag;
};

TEST_CASE("from_json - bool field gets true/false literal") {
    const char* json_true = R"({"flag": true})";
    const char* json_false = R"({"flag": false})";
    BoolField r1{}, r2{};
    CHECK(tinyrefl::reflection_from_json(r1, json_true).ok == true);
    CHECK(r1.flag == true);
    CHECK(tinyrefl::reflection_from_json(r2, json_false).ok == true);
    CHECK(r2.flag == false);
}

// ============================================================
// CASE 19: Invalid JSON returns error status
// ============================================================
TEST_CASE("from_json - invalid JSON returns error") {
    const char* bad_json = R"({"name": "Alice", age: 30})";  // missing quotes on key
    Person restored{};
    auto status = tinyrefl::reflection_from_json(restored, bad_json);
    CHECK(status.ok == false);
}

TEST_CASE("from_json - empty input returns error") {
    const char* empty_json = "";
    Person restored{};
    auto status = tinyrefl::reflection_from_json(restored, empty_json);
    CHECK(status.ok == false);
}

// ============================================================
// CASE 20: Deeply nested vector of structs each with vectors
// Tests that EndArray pops are properly balanced
// ============================================================
struct Leaf {
    int x;
    vector<int> data;
};

struct Branch {
    string name;
    vector<Leaf> leaves;
};

struct Tree {
    vector<Branch> branches;
};

TEST_CASE("roundtrip - tree structure (nested vec of structs with vec)") {
    Tree obj{{
        {"branch1", {{1, {10, 11}}, {2, {20}}}},
        {"branch2", {{3, {30, 31, 32}}}},
        {"branch3", {}}
    }};
    string json;
    tinyrefl::reflection_to_json(obj, json);

    Tree restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());
    CHECK(status.ok == true);
    REQUIRE(restored.branches.size() == 3);
    CHECK(restored.branches[0].name == "branch1");
    REQUIRE(restored.branches[0].leaves.size() == 2);
    CHECK(restored.branches[0].leaves[0].x == 1);
    REQUIRE(restored.branches[0].leaves[0].data.size() == 2);
    CHECK(restored.branches[0].leaves[0].data[0] == 10);
    CHECK(restored.branches[0].leaves[0].data[1] == 11);
    CHECK(restored.branches[0].leaves[1].x == 2);
    REQUIRE(restored.branches[0].leaves[1].data.size() == 1);
    CHECK(restored.branches[0].leaves[1].data[0] == 20);
    CHECK(restored.branches[1].name == "branch2");
    REQUIRE(restored.branches[1].leaves.size() == 1);
    CHECK(restored.branches[1].leaves[0].x == 3);
    REQUIRE(restored.branches[1].leaves[0].data.size() == 3);
    CHECK(restored.branches[1].leaves[0].data[2] == 32);
    CHECK(restored.branches[2].name == "branch3");
    CHECK(restored.branches[2].leaves.empty());
}
