#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"

#include <string>
#include <vector>

using namespace std;

// ============================================================
// 深层嵌套结构体定义 — 纯 struct 嵌套 4 层
// ============================================================
struct Level4 {
  int val4;
  string name4;
};

struct Level3 {
  Level4 l4;
  int val3;
  string name3;
};

struct Level2 {
  Level3 l3;
  int val2;
  string name2;
};

struct Level1 {
  Level2 l2;
  int val1;
  string name1;
};

// ============================================================
// 测试 1: 4 层纯 struct 嵌套 roundtrip
// ============================================================
TEST_CASE("深层嵌套 - 4 层纯 struct 嵌套 roundtrip") {
  Level1 original;
  original.l2.l3.l4.val4 = 400;
  original.l2.l3.l4.name4 = "level4";
  original.l2.l3.val3 = 300;
  original.l2.l3.name3 = "level3";
  original.l2.val2 = 200;
  original.l2.name2 = "level2";
  original.val1 = 100;
  original.name1 = "level1";

  // 序列化
  string json;
  tinyrefl::reflection_to_json(original, json);

  // 反序列化
  Level1 restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);

  // 逐字段验证 — 最深层先检查
  CHECK(restored.l2.l3.l4.val4 == 400);
  CHECK(restored.l2.l3.l4.name4 == "level4");
  CHECK(restored.l2.l3.val3 == 300);
  CHECK(restored.l2.l3.name3 == "level3");
  CHECK(restored.l2.val2 == 200);
  CHECK(restored.l2.name2 == "level2");
  CHECK(restored.val1 == 100);
  CHECK(restored.name1 == "level1");
}

// ============================================================
// 测试 2: 嵌套字段在前，基本字段在后（多层）
// 验证 EndObject pop 后父 handler 仍然能正确接收后续字段
// ============================================================
TEST_CASE("深层嵌套 - 嵌套字段在前基本字段在后") {
  // JSON 中嵌套字段先出现，基本字段后出现
  const char* json = R"({
    "l2": {
      "l3": {
        "l4": {
          "val4": 999,
          "name4": "deep"
        },
        "val3": 333,
        "name3": "mid"
      },
      "val2": 222,
      "name2": "shallow"
    },
    "val1": 111,
    "name1": "root"
  })";

  Level1 restored{};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);

  // 验证最深层数据不丢失
  CHECK(restored.l2.l3.l4.val4 == 999);
  CHECK(restored.l2.l3.l4.name4 == "deep");
  // 验证中间层数据不丢失
  CHECK(restored.l2.l3.val3 == 333);
  CHECK(restored.l2.l3.name3 == "mid");
  // 验证浅层数据不丢失
  CHECK(restored.l2.val2 == 222);
  CHECK(restored.l2.name2 == "shallow");
  // 验证根层数据不丢失
  CHECK(restored.val1 == 111);
  CHECK(restored.name1 == "root");
}

// ============================================================
// 同一层有多个嵌套 struct 字段
// ============================================================
struct SubA {
  int a;
  string sa;
};
struct SubB {
  int b;
  string sb;
};
struct SubC {
  int c;
  string sc;
};
struct MultiNested {
  SubA sub_a;
  SubB sub_b;
  SubC sub_c;
  int top_val;
};

TEST_CASE("深层嵌套 - 同层 3 个嵌套 struct 字段") {
  MultiNested original{{10, "aa"}, {20, "bb"}, {30, "cc"}, 42};
  string json;
  tinyrefl::reflection_to_json(original, json);

  MultiNested restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);

  CHECK(restored.sub_a.a == 10);
  CHECK(restored.sub_a.sa == "aa");
  CHECK(restored.sub_b.b == 20);
  CHECK(restored.sub_b.sb == "bb");
  CHECK(restored.sub_c.c == 30);
  CHECK(restored.sub_c.sc == "cc");
  CHECK(restored.top_val == 42);
}

// ============================================================
// 深层嵌套 + vector 混合
// ============================================================
struct Leaf {
  int id;
  string text;
};

struct Branch {
  string branch_name;
  vector<Leaf> leaves;
  Leaf single_leaf;
};

struct TreeRoot {
  string tree_name;
  vector<Branch> branches;
  Branch main_branch;
  int count;
};

TEST_CASE("深层嵌套 - struct 内含 vector<struct> 和 struct 混合") {
  TreeRoot original;
  original.tree_name = "oak";
  original.branches = {
      {"b1", {{1, "leaf1"}, {2, "leaf2"}}, {10, "single1"}},
      {"b2", {{3, "leaf3"}}, {20, "single2"}},
  };
  original.main_branch = {"main", {{100, "main_leaf"}}, {200, "main_single"}};
  original.count = 5;

  string json;
  tinyrefl::reflection_to_json(original, json);

  TreeRoot restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);

  CHECK(restored.tree_name == "oak");
  CHECK(restored.count == 5);

  // 验证 branches vector
  REQUIRE(restored.branches.size() == 2);
  CHECK(restored.branches[0].branch_name == "b1");
  REQUIRE(restored.branches[0].leaves.size() == 2);
  CHECK(restored.branches[0].leaves[0].id == 1);
  CHECK(restored.branches[0].leaves[0].text == "leaf1");
  CHECK(restored.branches[0].leaves[1].id == 2);
  CHECK(restored.branches[0].leaves[1].text == "leaf2");
  CHECK(restored.branches[0].single_leaf.id == 10);
  CHECK(restored.branches[0].single_leaf.text == "single1");

  CHECK(restored.branches[1].branch_name == "b2");
  REQUIRE(restored.branches[1].leaves.size() == 1);
  CHECK(restored.branches[1].leaves[0].id == 3);
  CHECK(restored.branches[1].leaves[0].text == "leaf3");
  CHECK(restored.branches[1].single_leaf.id == 20);
  CHECK(restored.branches[1].single_leaf.text == "single2");

  // 验证 main_branch struct
  CHECK(restored.main_branch.branch_name == "main");
  REQUIRE(restored.main_branch.leaves.size() == 1);
  CHECK(restored.main_branch.leaves[0].id == 100);
  CHECK(restored.main_branch.leaves[0].text == "main_leaf");
  CHECK(restored.main_branch.single_leaf.id == 200);
  CHECK(restored.main_branch.single_leaf.text == "main_single");
}

// ============================================================
// 5 层嵌套: struct 内含 struct 内含 struct 内含 vector<struct>
// ============================================================
struct Deep5Item {
  int x;
};

struct Deep4 {
  vector<Deep5Item> items;
  int d4_val;
};

struct Deep3 {
  Deep4 d4;
  string d3_name;
};

struct Deep2 {
  Deep3 d3;
  int d2_val;
};

struct Deep1 {
  Deep2 d2;
  string d1_name;
  int d1_val;
};

TEST_CASE("深层嵌套 - 5 层 struct+vector 混合 roundtrip") {
  Deep1 original;
  original.d2.d3.d4.items = {{10}, {20}, {30}};
  original.d2.d3.d4.d4_val = 444;
  original.d2.d3.d3_name = "deep3";
  original.d2.d2_val = 222;
  original.d1_name = "deep1";
  original.d1_val = 111;

  string json;
  tinyrefl::reflection_to_json(original, json);

  Deep1 restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);

  // 验证最深层 vector
  REQUIRE(restored.d2.d3.d4.items.size() == 3);
  CHECK(restored.d2.d3.d4.items[0].x == 10);
  CHECK(restored.d2.d3.d4.items[1].x == 20);
  CHECK(restored.d2.d3.d4.items[2].x == 30);
  CHECK(restored.d2.d3.d4.d4_val == 444);

  // 验证中间层
  CHECK(restored.d2.d3.d3_name == "deep3");
  CHECK(restored.d2.d2_val == 222);

  // 验证根层
  CHECK(restored.d1_name == "deep1");
  CHECK(restored.d1_val == 111);
}

// ============================================================
// 测试: vector<struct> 中的每个 struct 包含嵌套 struct
// ============================================================
struct InnerData {
  int score;
  string label;
};

struct OuterItem {
  string name;
  InnerData data;
  int rank;
};

struct Collection {
  vector<OuterItem> items;
  int total;
};

TEST_CASE("深层嵌套 - vector 中每个元素含嵌套 struct") {
  Collection original;
  original.items = {
      {"item1", {95, "excellent"}, 1},
      {"item2", {80, "good"}, 2},
      {"item3", {60, "pass"}, 3},
  };
  original.total = 3;

  string json;
  tinyrefl::reflection_to_json(original, json);

  Collection restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);

  CHECK(restored.total == 3);
  REQUIRE(restored.items.size() == 3);

  CHECK(restored.items[0].name == "item1");
  CHECK(restored.items[0].data.score == 95);
  CHECK(restored.items[0].data.label == "excellent");
  CHECK(restored.items[0].rank == 1);

  CHECK(restored.items[1].name == "item2");
  CHECK(restored.items[1].data.score == 80);
  CHECK(restored.items[1].data.label == "good");
  CHECK(restored.items[1].rank == 2);

  CHECK(restored.items[2].name == "item3");
  CHECK(restored.items[2].data.score == 60);
  CHECK(restored.items[2].data.label == "pass");
  CHECK(restored.items[2].rank == 3);
}

// ============================================================
// 测试: 深层嵌套 JSON 字符串直接解析（非 roundtrip）
// 模拟用户描述的场景 — 手写深层 JSON 反序列化
// ============================================================
TEST_CASE("深层嵌套 - 从手写 JSON 反序列化 4 层") {
  const char* json = R"({
    "l2": {
      "l3": {
        "l4": {
          "val4": 42,
          "name4": "innermost"
        },
        "val3": 30,
        "name3": "third"
      },
      "val2": 20,
      "name2": "second"
    },
    "val1": 10,
    "name1": "first"
  })";

  Level1 restored{};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);

  // 逐层验证数据完整性
  CHECK(restored.l2.l3.l4.val4 == 42);
  CHECK(restored.l2.l3.l4.name4 == "innermost");
  CHECK(restored.l2.l3.val3 == 30);
  CHECK(restored.l2.l3.name3 == "third");
  CHECK(restored.l2.val2 == 20);
  CHECK(restored.l2.name2 == "second");
  CHECK(restored.val1 == 10);
  CHECK(restored.name1 == "first");
}

// ============================================================
// 测试: 嵌套 struct 后面跟 vector 字段
// 验证 handler pop 后 vector 仍然能正确解析
// ============================================================
struct NestedThenVector {
  SubA nested;
  vector<int> nums;
  string suffix;
};

TEST_CASE("深层嵌套 - 嵌套 struct 后跟 vector 字段") {
  NestedThenVector original{{42, "nested_a"}, {1, 2, 3, 4, 5}, "end"};
  string json;
  tinyrefl::reflection_to_json(original, json);

  NestedThenVector restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);

  CHECK(restored.nested.a == 42);
  CHECK(restored.nested.sa == "nested_a");
  REQUIRE(restored.nums.size() == 5);
  CHECK(restored.nums[0] == 1);
  CHECK(restored.nums[4] == 5);
  CHECK(restored.suffix == "end");
}

// ============================================================
// 测试: vector 字段后面跟嵌套 struct
// ============================================================
struct VectorThenNested {
  vector<int> nums;
  SubA nested;
  string suffix;
};

TEST_CASE("深层嵌套 - vector 字段后跟嵌套 struct") {
  VectorThenNested original{{10, 20, 30}, {99, "after_vec"}, "tail"};
  string json;
  tinyrefl::reflection_to_json(original, json);

  VectorThenNested restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);

  REQUIRE(restored.nums.size() == 3);
  CHECK(restored.nums[0] == 10);
  CHECK(restored.nums[2] == 30);
  CHECK(restored.nested.a == 99);
  CHECK(restored.nested.sa == "after_vec");
  CHECK(restored.suffix == "tail");
}

// ============================================================
// 测试: 二次 roundtrip（serialize -> deserialize -> serialize -> compare）
// 如果第一次反序列化丢了数据，第二次序列化的 JSON 会不同
// ============================================================
TEST_CASE("深层嵌套 - 二次 roundtrip JSON 一致性") {
  Level1 original;
  original.l2.l3.l4.val4 = 999;
  original.l2.l3.l4.name4 = "deep";
  original.l2.l3.val3 = 888;
  original.l2.l3.name3 = "mid";
  original.l2.val2 = 777;
  original.l2.name2 = "shallow";
  original.val1 = 666;
  original.name1 = "root";

  // 第一次 roundtrip
  string json1;
  tinyrefl::reflection_to_json(original, json1);

  Level1 restored1{};
  auto s1 = tinyrefl::reflection_from_json(restored1, json1.c_str());
  CHECK(s1.ok == true);

  // 第二次 roundtrip
  string json2;
  tinyrefl::reflection_to_json(restored1, json2);

  Level1 restored2{};
  auto s2 = tinyrefl::reflection_from_json(restored2, json2.c_str());
  CHECK(s2.ok == true);

  // 两次序列化的 JSON 应当完全一致
  CHECK(json1 == json2);

  // 两次反序列化的结果应当一致
  CHECK(restored2.l2.l3.l4.val4 == original.l2.l3.l4.val4);
  CHECK(restored2.l2.l3.l4.name4 == original.l2.l3.l4.name4);
  CHECK(restored2.l2.l3.val3 == original.l2.l3.val3);
  CHECK(restored2.l2.l3.name3 == original.l2.l3.name3);
  CHECK(restored2.l2.val2 == original.l2.val2);
  CHECK(restored2.l2.name2 == original.l2.name2);
  CHECK(restored2.val1 == original.val1);
  CHECK(restored2.name1 == original.name1);
}

// ============================================================
// 测试: vector<vector<struct>> 深层嵌套
// ============================================================
struct SimpleItem {
  int id;
  string name;
};

struct MatrixContainer {
  vector<vector<SimpleItem>> matrix;
  int count;
};

TEST_CASE("深层嵌套 - vector<vector<struct>> roundtrip") {
  MatrixContainer original;
  original.matrix = {
      {{1, "a"}, {2, "b"}},
      {{3, "c"}, {4, "d"}, {5, "e"}},
  };
  original.count = 5;

  string json;
  tinyrefl::reflection_to_json(original, json);

  MatrixContainer restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);

  CHECK(restored.count == 5);
  REQUIRE(restored.matrix.size() == 2);
  REQUIRE(restored.matrix[0].size() == 2);
  CHECK(restored.matrix[0][0].id == 1);
  CHECK(restored.matrix[0][0].name == "a");
  CHECK(restored.matrix[0][1].id == 2);
  CHECK(restored.matrix[0][1].name == "b");
  REQUIRE(restored.matrix[1].size() == 3);
  CHECK(restored.matrix[1][0].id == 3);
  CHECK(restored.matrix[1][0].name == "c");
  CHECK(restored.matrix[1][2].id == 5);
  CHECK(restored.matrix[1][2].name == "e");
}

// ============================================================
// 测试: 含有未知字段的深层嵌套 JSON 反序列化
// 验证 unknown sub-object 不影响后续字段
// ============================================================
TEST_CASE("深层嵌套 - 含未知字段的深层 JSON 不丢失已知字段") {
  const char* json = R"({
    "l2": {
      "unknown1": {"nested_unknown": 123},
      "l3": {
        "l4": {
          "val4": 50,
          "name4": "known_deep",
          "extra_field": "ignored"
        },
        "val3": 40,
        "name3": "known_mid"
      },
      "val2": 30,
      "name2": "known_shallow"
    },
    "val1": 20,
    "name1": "known_root"
  })";

  Level1 restored{};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);

  CHECK(restored.l2.l3.l4.val4 == 50);
  CHECK(restored.l2.l3.l4.name4 == "known_deep");
  CHECK(restored.l2.l3.val3 == 40);
  CHECK(restored.l2.l3.name3 == "known_mid");
  CHECK(restored.l2.val2 == 30);
  CHECK(restored.l2.name2 == "known_shallow");
  CHECK(restored.val1 == 20);
  CHECK(restored.name1 == "known_root");
}
