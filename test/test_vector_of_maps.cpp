#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"
#include <deque>
#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

// ==================== 测试结构体 ====================

// 场景1：vector<map<string,int>> — 序列容器内嵌关联容器
struct VecOfMap {
  std::vector<std::map<std::string, int>> items;
};

// 场景2：list<map<string,string>> — list 同样是序列容器
struct ListOfMap {
  std::list<std::map<std::string, std::string>> entries;
};

// 场景3：deque<map<string,double>> — deque 同理
struct DequeOfMap {
  std::deque<std::map<std::string, double>> records;
};

// 场景4：vector<unordered_map<string,int>> — unordered_map 也是关联容器
struct VecOfUnorderedMap {
  std::vector<std::unordered_map<std::string, int>> data;
};

// 场景5：嵌套更深 — struct 包含 vector<map>，同时 map value 也是 struct
struct Inner {
  int x;
  std::string y;
};

struct VecOfMapOfStruct {
  std::vector<std::map<std::string, Inner>> groups;
};

// 场景6：map 的 value 是 vector（对比测试，应该已经正常工作）
struct MapOfVec {
  std::map<std::string, std::vector<int>> lookup;
};

// ==================== 测试用例 ====================

TEST_CASE("vector<map<string,int>> roundtrip") {
  VecOfMap original;
  original.items.push_back({{"a", 1}, {"b", 2}});
  original.items.push_back({{"c", 3}});
  original.items.push_back({});  // 空 map

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  // 序列化应生成合法 JSON
  INFO("序列化结果: " << json);
  CHECK(!json.empty());

  // 反序列化
  VecOfMap restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);

  // 验证 roundtrip 正确性
  REQUIRE(restored.items.size() == 3);
  CHECK(restored.items[0].size() == 2);
  CHECK(restored.items[0].at("a") == 1);
  CHECK(restored.items[0].at("b") == 2);
  CHECK(restored.items[1].size() == 1);
  CHECK(restored.items[1].at("c") == 3);
  CHECK(restored.items[2].empty());
}

TEST_CASE("list<map<string,string>> roundtrip") {
  ListOfMap original;
  original.entries.push_back({{"key1", "val1"}, {"key2", "val2"}});
  original.entries.push_back({{"hello", "world"}});

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  ListOfMap restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);

  REQUIRE(restored.entries.size() == 2);
  auto it = restored.entries.begin();
  CHECK(it->at("key1") == "val1");
  CHECK(it->at("key2") == "val2");
  ++it;
  CHECK(it->at("hello") == "world");
}

TEST_CASE("deque<map<string,double>> roundtrip") {
  DequeOfMap original;
  original.records.push_back({{"pi", 3.14}, {"e", 2.718}});

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  DequeOfMap restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);

  REQUIRE(restored.records.size() == 1);
  // 浮点比较使用 doctest::Approx
  CHECK(restored.records[0].at("pi") == doctest::Approx(3.14));
  CHECK(restored.records[0].at("e") == doctest::Approx(2.718));
}

TEST_CASE("vector<unordered_map<string,int>> roundtrip") {
  VecOfUnorderedMap original;
  original.data.push_back({{"x", 10}, {"y", 20}});

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  VecOfUnorderedMap restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);

  REQUIRE(restored.data.size() == 1);
  CHECK(restored.data[0].at("x") == 10);
  CHECK(restored.data[0].at("y") == 20);
}

TEST_CASE("vector<map<string, struct>> roundtrip") {
  VecOfMapOfStruct original;
  original.groups.push_back({{"first", {1, "one"}}, {"second", {2, "two"}}});
  original.groups.push_back({{"third", {3, "three"}}});

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  VecOfMapOfStruct restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);

  REQUIRE(restored.groups.size() == 2);
  CHECK(restored.groups[0].at("first").x == 1);
  CHECK(restored.groups[0].at("first").y == "one");
  CHECK(restored.groups[0].at("second").x == 2);
  CHECK(restored.groups[0].at("second").y == "two");
  CHECK(restored.groups[1].at("third").x == 3);
  CHECK(restored.groups[1].at("third").y == "three");
}

TEST_CASE("map<string, vector<int>> roundtrip（对比测试）") {
  MapOfVec original;
  original.lookup["nums"] = {1, 2, 3};
  original.lookup["empty"] = {};

  std::string json;
  tinyrefl::reflection_to_json(original, json);

  MapOfVec restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok);

  REQUIRE(restored.lookup.size() == 2);
  CHECK(restored.lookup["nums"] == std::vector<int>{1, 2, 3});
  CHECK(restored.lookup["empty"].empty());
}
