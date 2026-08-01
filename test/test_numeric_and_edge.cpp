#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/reflection_to_json.hpp"
#include "tinyrefl/reflection_from_json.hpp"

#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <limits>

using namespace std;

// ============================================================
// Structures
// ============================================================

struct I64Val  { int64_t  v; };
struct U64Val  { uint64_t v; };
struct I32Val  { int32_t  v; };
struct U32Val  { uint32_t v; };
struct F32Val  { float    v; };
struct F64Val  { double   v; };

struct Mixed64 {
    int64_t  big_signed;
    uint64_t big_unsigned;
    int      small_int;
};

// Struct to test that a field named the same as a nested field
// in a child struct doesn't bleed into the parent
struct Child  { int x; int y; };
struct Parent { Child child; int x; };   // parent also has 'x'

// Array-of-arrays of different sizes
struct Ragged {
    vector<vector<int>> rows;
};

// Consecutive empty arrays
struct ThreeVecs {
    vector<int> a;
    vector<int> b;
    vector<int> c;
};

// Struct with nested struct whose field name clashes with parent field
struct Inner2 { string name; int val; };
struct Outer2 { string name; Inner2 inner; int count; };

// ============================================================
// CASE 1: int64_t roundtrip with small value (triggers Int event)
// RapidJSON emits Int(42), not Int64(42), for small numbers.
// is_json_compatible_v<int64_t, int> must be true for this to work.
// ============================================================
TEST_CASE("from_json - int64_t field from small JSON integer (Int event)") {
    const char* json = R"({"v": 42})";
    I64Val r{};
    auto st = tinyrefl::reflection_from_json(r, json);
    CHECK(st.ok);
    CHECK(r.v == 42LL);
}

TEST_CASE("from_json - int64_t field from large negative JSON integer") {
    I64Val obj{ -9000000000LL };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    I64Val r{};
    auto st = tinyrefl::reflection_from_json(r, json.c_str());
    CHECK(st.ok);
    CHECK(r.v == -9000000000LL);
}

TEST_CASE("from_json - uint64_t field from small positive JSON integer (Int event)") {
    const char* json = R"({"v": 100})";
    U64Val r{};
    auto st = tinyrefl::reflection_from_json(r, json);
    CHECK(st.ok);
    CHECK(r.v == 100ULL);
}

// ============================================================
// CASE 2: int32_t roundtrip boundary values
// ============================================================
TEST_CASE("roundtrip - int32_t INT_MAX") {
    I32Val obj{ std::numeric_limits<int32_t>::max() };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    I32Val r{};
    CHECK(tinyrefl::reflection_from_json(r, json.c_str()).ok);
    CHECK(r.v == std::numeric_limits<int32_t>::max());
}

TEST_CASE("roundtrip - int32_t INT_MIN") {
    I32Val obj{ std::numeric_limits<int32_t>::min() };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    I32Val r{};
    CHECK(tinyrefl::reflection_from_json(r, json.c_str()).ok);
    CHECK(r.v == std::numeric_limits<int32_t>::min());
}

// ============================================================
// CASE 3: uint32_t roundtrip with UINT_MAX
// UINT_MAX = 4294967295; RapidJSON triggers Uint event
// ============================================================
TEST_CASE("roundtrip - uint32_t UINT_MAX") {
    U32Val obj{ std::numeric_limits<uint32_t>::max() };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    U32Val r{};
    CHECK(tinyrefl::reflection_from_json(r, json.c_str()).ok);
    CHECK(r.v == std::numeric_limits<uint32_t>::max());
}

// ============================================================
// CASE 4: float precision roundtrip
// ============================================================
TEST_CASE("roundtrip - float precision boundary") {
    F32Val obj{ std::numeric_limits<float>::max() };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    F32Val r{};
    CHECK(tinyrefl::reflection_from_json(r, json.c_str()).ok);
    // Allow relative error for large float
    CHECK(std::abs(r.v - obj.v) / obj.v < 1e-6f);
}

TEST_CASE("roundtrip - float smallest positive") {
    F32Val obj{ std::numeric_limits<float>::min() };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    F32Val r{};
    CHECK(tinyrefl::reflection_from_json(r, json.c_str()).ok);
    CHECK(r.v > 0.0f);
}

TEST_CASE("roundtrip - double precision boundary") {
    F64Val obj{ std::numeric_limits<double>::max() };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    F64Val r{};
    CHECK(tinyrefl::reflection_from_json(r, json.c_str()).ok);
    CHECK(std::abs(r.v - obj.v) / obj.v < 1e-10);
}

// ============================================================
// CASE 5: mixed int64 / uint64 / int in one struct
// ============================================================
TEST_CASE("roundtrip - mixed 64-bit and 32-bit fields") {
    Mixed64 obj{ -9000000000LL, 18000000000ULL, -1 };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    Mixed64 r{};
    CHECK(tinyrefl::reflection_from_json(r, json.c_str()).ok);
    CHECK(r.big_signed   == -9000000000LL);
    CHECK(r.big_unsigned == 18000000000ULL);
    CHECK(r.small_int    == -1);
}

// ============================================================
// CASE 6: field name collision between parent and child struct
// Parent has field 'x'; Child also has field 'x'.
// When parsing child's 'x', parent's 'x' must NOT be modified.
// When parsing parent's 'x' after EndObject of child, child's 'x'
// must NOT be modified.
// ============================================================
TEST_CASE("from_json - parent and child have same field name 'x'") {
    const char* json = R"({"child":{"x":10,"y":20},"x":99})";
    Parent r{};
    auto st = tinyrefl::reflection_from_json(r, json);
    CHECK(st.ok);
    CHECK(r.child.x == 10);
    CHECK(r.child.y == 20);
    CHECK(r.x == 99);          // parent's x must be 99, not 10
}

TEST_CASE("roundtrip - parent and child have same field name 'x'") {
    Parent obj{ {5, 6}, 77 };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    Parent r{};
    auto st = tinyrefl::reflection_from_json(r, json.c_str());
    CHECK(st.ok);
    CHECK(r.child.x == 5);
    CHECK(r.child.y == 6);
    CHECK(r.x == 77);
}

// ============================================================
// CASE 7: ragged vector<vector<int>> (rows of different lengths)
// ============================================================
TEST_CASE("roundtrip - ragged 2D vector") {
    Ragged obj{ {{1}, {2,3}, {4,5,6}, {}, {7,8,9,10}} };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    Ragged r{};
    CHECK(tinyrefl::reflection_from_json(r, json.c_str()).ok);
    REQUIRE(r.rows.size() == 5);
    REQUIRE(r.rows[0].size() == 1); CHECK(r.rows[0][0] == 1);
    REQUIRE(r.rows[1].size() == 2); CHECK(r.rows[1][1] == 3);
    REQUIRE(r.rows[2].size() == 3); CHECK(r.rows[2][2] == 6);
    CHECK(r.rows[3].empty());
    REQUIRE(r.rows[4].size() == 4); CHECK(r.rows[4][3] == 10);
}

// ============================================================
// CASE 8: three consecutive vectors (all empty, all non-empty, mixed)
// Tests that EndArray properly pops handler for each vector
// ============================================================
TEST_CASE("roundtrip - three consecutive vectors all non-empty") {
    ThreeVecs obj{ {1,2}, {3,4,5}, {6} };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    ThreeVecs r{};
    CHECK(tinyrefl::reflection_from_json(r, json.c_str()).ok);
    REQUIRE(r.a.size() == 2); CHECK(r.a[0]==1); CHECK(r.a[1]==2);
    REQUIRE(r.b.size() == 3); CHECK(r.b[1]==4);
    REQUIRE(r.c.size() == 1); CHECK(r.c[0]==6);
}

TEST_CASE("roundtrip - three consecutive vectors all empty") {
    ThreeVecs obj{ {}, {}, {} };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    ThreeVecs r{};
    CHECK(tinyrefl::reflection_from_json(r, json.c_str()).ok);
    CHECK(r.a.empty());
    CHECK(r.b.empty());
    CHECK(r.c.empty());
}

TEST_CASE("roundtrip - three consecutive vectors mixed empty/non-empty") {
    ThreeVecs obj{ {}, {10,20}, {} };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    ThreeVecs r{};
    CHECK(tinyrefl::reflection_from_json(r, json.c_str()).ok);
    CHECK(r.a.empty());
    REQUIRE(r.b.size() == 2); CHECK(r.b[0]==10); CHECK(r.b[1]==20);
    CHECK(r.c.empty());
}

// ============================================================
// CASE 9: name field collision between parent and nested struct
// Outer2 { string name; Inner2 inner; int count; }
// Inner2  { string name; int val; }
// ============================================================
TEST_CASE("from_json - nested struct with same field name as parent") {
    const char* json = R"({
        "name": "outer_name",
        "inner": {"name": "inner_name", "val": 42},
        "count": 7
    })";
    Outer2 r{};
    auto st = tinyrefl::reflection_from_json(r, json);
    CHECK(st.ok);
    CHECK(r.name == "outer_name");
    CHECK(r.inner.name == "inner_name");
    CHECK(r.inner.val == 42);
    CHECK(r.count == 7);
}

TEST_CASE("roundtrip - nested struct with same field name as parent") {
    Outer2 obj{ "out", {"in", 99}, 3 };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    Outer2 r{};
    auto st = tinyrefl::reflection_from_json(r, json.c_str());
    CHECK(st.ok);
    CHECK(r.name == "out");
    CHECK(r.inner.name == "in");
    CHECK(r.inner.val == 99);
    CHECK(r.count == 3);
}

// ============================================================
// CASE 10: unicode content in strings (UTF-8 passthrough)
// ============================================================
struct StrVal { string s; };

TEST_CASE("roundtrip - UTF-8 string content") {
    // Chinese and ASCII mixed, stored as plain UTF-8 bytes in std::string
    StrVal obj{ "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc\x8c\xe4\xb8\x96\xe7\x95\x8c\xef\xbc\x81Hello, World!" };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    StrVal r{};
    CHECK(tinyrefl::reflection_from_json(r, json.c_str()).ok);
    CHECK(r.s == obj.s);
}

// ============================================================
// CASE 11: very long string (stress buffer handling)
// ============================================================
TEST_CASE("roundtrip - very long string") {
    string long_str(10000, 'x');
    long_str[5000] = '"';    // inject a special char in the middle
    long_str[7500] = '\\';
    StrVal obj{ long_str };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    StrVal r{};
    CHECK(tinyrefl::reflection_from_json(r, json.c_str()).ok);
    CHECK(r.s == long_str);
}

// ============================================================
// CASE 12: int field receives a large unsigned value that fits
// ============================================================
TEST_CASE("from_json - int field with value 2147483647 (INT_MAX from JSON)") {
    const char* json = R"({"v": 2147483647})";
    I32Val r{};
    CHECK(tinyrefl::reflection_from_json(r, json).ok);
    CHECK(r.v == 2147483647);
}

// ============================================================
// CASE 13: double field from integer JSON literal
// RapidJSON triggers Int event; assign_if_match<int> must set double
// ============================================================
TEST_CASE("from_json - double field from JSON integer literal") {
    const char* json = R"({"v": 7})";
    F64Val r{};
    auto st = tinyrefl::reflection_from_json(r, json);
    CHECK(st.ok);
    CHECK(r.v == 7.0);
}

// ============================================================
// CASE 14: serialization of NaN and infinity (edge values)
// std::to_chars for inf/nan outputs "inf"/"nan" which is not
// valid JSON - this test checks what actually happens
// ============================================================
TEST_CASE("to_json - double NaN produces some output without crash") {
    F64Val obj{ std::numeric_limits<double>::quiet_NaN() };
    string json;
    // Must not crash, even if output is not valid JSON
    CHECK_NOTHROW(tinyrefl::reflection_to_json(obj, json));
    CHECK(!json.empty());
}

TEST_CASE("to_json - double infinity produces some output without crash") {
    F64Val obj{ std::numeric_limits<double>::infinity() };
    string json;
    CHECK_NOTHROW(tinyrefl::reflection_to_json(obj, json));
    CHECK(!json.empty());
}
