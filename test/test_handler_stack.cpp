#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/reflection_to_json.hpp"
#include "tinyrefl/reflection_from_json.hpp"

#include <vector>
#include <string>
#include <list>
#include <cmath>

using namespace std;

// ============================================================
// Structures
// ============================================================

struct Point { int x; int y; };
struct Segment { Point a; Point b; };
struct Polygon { vector<Point> points; string label; };

// nested struct in the middle of a vector, then more fields
struct Frame {
    int id;
    Point origin;
    vector<int> data;
    string tag;
};

// struct with vector<struct> where the struct itself has a vector
struct Row {
    string name;
    vector<int> values;
};
struct Table {
    string title;
    vector<Row> rows;
    int version;
};

// 3-level nesting with vectors at each level
struct Cell   { int val; };
struct MatRow { vector<Cell> cells; };
struct Matrix { vector<MatRow> rows; string name; };

// test struct whose fields come in a specific order that stresses stack ordering
struct Stress {
    string s1;
    Point p1;
    vector<int> v1;
    Point p2;
    vector<string> v2;
    string s2;
};

// struct with same-type nested structs at multiple levels
struct Node {
    int id;
    string label;
};
struct Graph {
    Node source;
    Node target;
    vector<Node> waypoints;
    int weight;
};

// ============================================================
// CASE 1: Two adjacent nested structs (Segment)
// ============================================================
TEST_CASE("roundtrip - two adjacent nested structs") {
    Segment seg{ {1,2}, {3,4} };
    string json;
    tinyrefl::reflection_to_json(seg, json);

    Segment r{};
    auto st = tinyrefl::reflection_from_json(r, json.c_str());
    CHECK(st.ok);
    CHECK(r.a.x == 1); CHECK(r.a.y == 2);
    CHECK(r.b.x == 3); CHECK(r.b.y == 4);
}

// ============================================================
// CASE 2: Vector of structs with label field after the vector
// ============================================================
TEST_CASE("roundtrip - vector of structs then scalar field") {
    Polygon poly{ {{0,0},{1,0},{1,1},{0,1}}, "square" };
    string json;
    tinyrefl::reflection_to_json(poly, json);

    Polygon r{};
    auto st = tinyrefl::reflection_from_json(r, json.c_str());
    CHECK(st.ok);
    REQUIRE(r.points.size() == 4);
    CHECK(r.points[0].x == 0); CHECK(r.points[0].y == 0);
    CHECK(r.points[1].x == 1); CHECK(r.points[1].y == 0);
    CHECK(r.points[2].x == 1); CHECK(r.points[2].y == 1);
    CHECK(r.points[3].x == 0); CHECK(r.points[3].y == 1);
    CHECK(r.label == "square");   // field after vector<struct>
}

// ============================================================
// CASE 3: Frame: int, nested struct, vector<int>, string
// All four different kinds in sequence
// ============================================================
TEST_CASE("roundtrip - int nested_struct vector_int string in sequence") {
    Frame f{ 7, {10,20}, {1,2,3,4,5}, "frame_tag" };
    string json;
    tinyrefl::reflection_to_json(f, json);

    Frame r{};
    auto st = tinyrefl::reflection_from_json(r, json.c_str());
    CHECK(st.ok);
    CHECK(r.id == 7);
    CHECK(r.origin.x == 10); CHECK(r.origin.y == 20);
    REQUIRE(r.data.size() == 5);
    CHECK(r.data[4] == 5);
    CHECK(r.tag == "frame_tag");
}

// ============================================================
// CASE 4: Table: string, vector<Row(string+vector<int>)>, int
// This tests handler stack with rows containing nested vectors
// ============================================================
TEST_CASE("roundtrip - table with rows each having a vector") {
    Table tbl{
        "my_table",
        {
            {"row0", {10, 20, 30}},
            {"row1", {40, 50}},
            {"row2", {}}
        },
        42
    };
    string json;
    tinyrefl::reflection_to_json(tbl, json);

    Table r{};
    auto st = tinyrefl::reflection_from_json(r, json.c_str());
    CHECK(st.ok);
    CHECK(r.title == "my_table");
    REQUIRE(r.rows.size() == 3);
    CHECK(r.rows[0].name == "row0");
    REQUIRE(r.rows[0].values.size() == 3);
    CHECK(r.rows[0].values[0] == 10);
    CHECK(r.rows[0].values[2] == 30);
    CHECK(r.rows[1].name == "row1");
    REQUIRE(r.rows[1].values.size() == 2);
    CHECK(r.rows[1].values[1] == 50);
    CHECK(r.rows[2].name == "row2");
    CHECK(r.rows[2].values.empty());
    CHECK(r.version == 42);   // scalar field after vector<struct-with-vector>
}

// ============================================================
// CASE 5: Matrix - 3-level nesting: vector<MatRow(vector<Cell>)>
// Each level has a vector containing structs which themselves
// contain vectors
// ============================================================
TEST_CASE("roundtrip - 3-level vector-struct nesting (Matrix)") {
    Matrix m{
        {
            { { {1},{2},{3} } },
            { { {4},{5} } },
            { {} }
        },
        "mat"
    };
    string json;
    tinyrefl::reflection_to_json(m, json);

    Matrix r{};
    auto st = tinyrefl::reflection_from_json(r, json.c_str());
    CHECK(st.ok);
    CHECK(r.name == "mat");
    REQUIRE(r.rows.size() == 3);
    REQUIRE(r.rows[0].cells.size() == 3);
    CHECK(r.rows[0].cells[0].val == 1);
    CHECK(r.rows[0].cells[2].val == 3);
    REQUIRE(r.rows[1].cells.size() == 2);
    CHECK(r.rows[1].cells[1].val == 5);
    CHECK(r.rows[2].cells.empty());
}

// ============================================================
// CASE 6: Stress test - all types interleaved
// string, nested_struct, vector<int>, nested_struct, vector<string>, string
// ============================================================
TEST_CASE("roundtrip - stress: alternating nested and vector fields") {
    Stress s{"hello", {1,2}, {10,20,30}, {3,4}, {"a","b","c"}, "world"};
    string json;
    tinyrefl::reflection_to_json(s, json);

    Stress r{};
    auto st = tinyrefl::reflection_from_json(r, json.c_str());
    CHECK(st.ok);
    CHECK(r.s1 == "hello");
    CHECK(r.p1.x == 1); CHECK(r.p1.y == 2);
    REQUIRE(r.v1.size() == 3);
    CHECK(r.v1[0] == 10); CHECK(r.v1[2] == 30);
    CHECK(r.p2.x == 3); CHECK(r.p2.y == 4);
    REQUIRE(r.v2.size() == 3);
    CHECK(r.v2[0] == "a"); CHECK(r.v2[2] == "c");
    CHECK(r.s2 == "world");
}

// ============================================================
// CASE 7: Graph - two nested structs + vector<struct> + scalar
// Checks that multiple same-type nested handlers don't interfere
// ============================================================
TEST_CASE("roundtrip - graph with source, target, waypoints, weight") {
    Graph g{
        {1, "src"},
        {2, "dst"},
        {{3,"w1"},{4,"w2"},{5,"w3"}},
        100
    };
    string json;
    tinyrefl::reflection_to_json(g, json);

    Graph r{};
    auto st = tinyrefl::reflection_from_json(r, json.c_str());
    CHECK(st.ok);
    CHECK(r.source.id == 1);   CHECK(r.source.label == "src");
    CHECK(r.target.id == 2);   CHECK(r.target.label == "dst");
    REQUIRE(r.waypoints.size() == 3);
    CHECK(r.waypoints[0].id == 3); CHECK(r.waypoints[0].label == "w1");
    CHECK(r.waypoints[1].id == 4); CHECK(r.waypoints[1].label == "w2");
    CHECK(r.waypoints[2].id == 5); CHECK(r.waypoints[2].label == "w3");
    CHECK(r.weight == 100);
}

// ============================================================
// CASE 8: Empty vector<struct> inside a struct, with fields after
// ============================================================
TEST_CASE("roundtrip - empty vector<struct> then field after") {
    Table tbl{ "empty_table", {}, 99 };
    string json;
    tinyrefl::reflection_to_json(tbl, json);

    Table r{};
    auto st = tinyrefl::reflection_from_json(r, json.c_str());
    CHECK(st.ok);
    CHECK(r.title == "empty_table");
    CHECK(r.rows.empty());
    CHECK(r.version == 99);   // must be parsed correctly
}

// ============================================================
// CASE 9: Parse same type multiple times into different objects
// ensuring no static-state contamination between calls
// ============================================================
TEST_CASE("from_json - no static state contamination across calls") {
    Table t1{ "t1", {{"r1",{1,2}}}, 10 };
    Table t2{ "t2", {{"r2",{3,4,5}},{"r3",{}}}, 20 };

    string j1, j2;
    tinyrefl::reflection_to_json(t1, j1);
    tinyrefl::reflection_to_json(t2, j2);

    Table r1{}, r2{};
    CHECK(tinyrefl::reflection_from_json(r1, j1.c_str()).ok);
    CHECK(tinyrefl::reflection_from_json(r2, j2.c_str()).ok);

    CHECK(r1.title == "t1");
    REQUIRE(r1.rows.size() == 1);
    CHECK(r1.rows[0].name == "r1");
    REQUIRE(r1.rows[0].values.size() == 2);
    CHECK(r1.version == 10);

    CHECK(r2.title == "t2");
    REQUIRE(r2.rows.size() == 2);
    CHECK(r2.rows[0].name == "r2");
    REQUIRE(r2.rows[0].values.size() == 3);
    CHECK(r2.rows[1].values.empty());
    CHECK(r2.version == 20);
}

// ============================================================
// CASE 10: JSON with array of objects where each object has
// a different number of fields than the struct (extra JSON keys)
// ============================================================
TEST_CASE("from_json - vector of structs with extra keys in each element") {
    // JSON has extra "extra" key in each Point element - should be ignored
    const char* json = R"({
        "points": [
            {"x":1,"y":2,"extra":99},
            {"x":3,"y":4,"z":0}
        ],
        "label": "robust"
    })";

    Polygon r{};
    auto st = tinyrefl::reflection_from_json(r, json);
    CHECK(st.ok);
    REQUIRE(r.points.size() == 2);
    CHECK(r.points[0].x == 1); CHECK(r.points[0].y == 2);
    CHECK(r.points[1].x == 3); CHECK(r.points[1].y == 4);
    CHECK(r.label == "robust");
}

// ============================================================
// CASE 11: Repeated roundtrip of same object - value must reset
// ============================================================
TEST_CASE("from_json - overwrite existing non-zero values") {
    Frame f{};
    // Parse once
    CHECK(tinyrefl::reflection_from_json(f,
        R"({"id":1,"origin":{"x":10,"y":20},"data":[1,2,3],"tag":"a"})").ok);
    CHECK(f.id == 1);
    CHECK(f.origin.x == 10);
    REQUIRE(f.data.size() == 3);
    CHECK(f.tag == "a");

    // Parse again with different values - existing values should be overwritten
    // Note: vectors will ACCUMULATE (emplace_back), this tests the behavior
    CHECK(tinyrefl::reflection_from_json(f,
        R"({"id":2,"origin":{"x":30,"y":40},"data":[9],"tag":"b"})").ok);
    CHECK(f.id == 2);
    CHECK(f.origin.x == 30);
    CHECK(f.tag == "b");
    // data will have old + new elements because deserialization appends
    // The important check: the scalar fields are correctly overwritten
}

// ============================================================
// CASE 12: Deeply nested JSON - 4 levels with mixed field types
// ============================================================
struct D1 { int v; string s; };
struct D2 { D1 inner; vector<int> nums; };
struct D3 { D2 child; string label; };
struct D4 { D3 top; int count; };

TEST_CASE("roundtrip - 4-level mixed nesting with vectors") {
    D4 obj{ { { {42, "hello"}, {1,2,3} }, "lvl3" }, 99 };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    D4 r{};
    auto st = tinyrefl::reflection_from_json(r, json.c_str());
    CHECK(st.ok);
    CHECK(r.count == 99);
    CHECK(r.top.label == "lvl3");
    REQUIRE(r.top.child.nums.size() == 3);
    CHECK(r.top.child.nums[0] == 1);
    CHECK(r.top.child.nums[2] == 3);
    CHECK(r.top.child.inner.v == 42);
    CHECK(r.top.child.inner.s == "hello");
}

// ============================================================
// CASE 13: vector<list<int>> - nested sequence containers
// ============================================================
struct VecOfList {
    vector<list<int>> data;
};

TEST_CASE("roundtrip - vector<list<int>>") {
    VecOfList obj{ { {1,2,3}, {4,5}, {} } };
    string json;
    tinyrefl::reflection_to_json(obj, json);

    VecOfList r{};
    auto st = tinyrefl::reflection_from_json(r, json.c_str());
    CHECK(st.ok);
    REQUIRE(r.data.size() == 3);
    REQUIRE(r.data[0].size() == 3);
    auto it = r.data[0].begin();
    CHECK(*it++ == 1); CHECK(*it++ == 2); CHECK(*it == 3);
    REQUIRE(r.data[1].size() == 2);
    CHECK(r.data[2].empty());
}

// ============================================================
// CASE 14: Parsing JSON where nested object keys appear
// in reverse field order
// ============================================================
TEST_CASE("from_json - nested struct keys in reverse order") {
    // JSON has nested Point with y before x
    const char* json = R"({"id":5,"origin":{"y":20,"x":10},"data":[7],"tag":"rev"})";
    Frame r{};
    auto st = tinyrefl::reflection_from_json(r, json);
    CHECK(st.ok);
    CHECK(r.id == 5);
    CHECK(r.origin.x == 10);  // must still be 10
    CHECK(r.origin.y == 20);  // must still be 20
    CHECK(r.tag == "rev");
}

// ============================================================
// CASE 15: Serialization correctness - count of braces and brackets
// ============================================================
TEST_CASE("to_json - brace/bracket balance") {
    Table tbl{ "t", { {"r1",{1,2}}, {"r2",{3}} }, 1 };
    string json;
    tinyrefl::reflection_to_json(tbl, json);

    int open_brace = 0, close_brace = 0;
    int open_bracket = 0, close_bracket = 0;
    bool in_string = false;
    for (char c : json) {
        if (c == '"') { in_string = !in_string; continue; }
        if (in_string) continue;
        if (c == '{') ++open_brace;
        if (c == '}') ++close_brace;
        if (c == '[') ++open_bracket;
        if (c == ']') ++close_bracket;
    }
    CHECK(open_brace == close_brace);
    CHECK(open_bracket == close_bracket);
}
