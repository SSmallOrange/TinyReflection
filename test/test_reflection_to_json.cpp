#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/reflection_to_json.hpp"

#include <iostream>
#include <string>

struct BasicTypes {
  int m_int;
  float m_float;
  double m_double;
  char m_char;
  const char* m_cstr = nullptr;
  std::string m_str;
  bool m_bool;
};

struct SequenceContainers {
  std::vector<int> m_vec;
  std::list<std::string> m_list;
  std::deque<double> m_deque;
  std::vector<char> m_queue_like;
};

struct MapContainers {
  std::map<std::string, int> m_map_str_int;
  std::unordered_map<std::string, std::string> m_umap_str_str;
};

struct NestedStruct {
  BasicTypes m_basic;
  SequenceContainers m_seq;
  MapContainers m_maps;
};

struct DeepNest {
  NestedStruct m_nested1;
  NestedStruct m_nested2;
};

TEST_CASE("reflection_to_json - basic types") {
  BasicTypes obj{123, 4.5f, 6.78, 'Z', "hello", "world", false};
  std::string output;
  tinyrefl::reflection_to_json(obj, output);

  // 输出不应为空
  CHECK(!output.empty());
  // 应该包含字段名
  CHECK(output.find("m_int") != std::string::npos);
  CHECK(output.find("m_float") != std::string::npos);
  CHECK(output.find("m_str") != std::string::npos);
  CHECK(output.find("m_bool") != std::string::npos);
  // 应该包含值
  CHECK(output.find("123") != std::string::npos);
  CHECK(output.find("world") != std::string::npos);
  CHECK(output.find("hello") != std::string::npos);
}

TEST_CASE("reflection_to_json - sequence containers") {
  SequenceContainers obj{{1, 2, 3}, {"one", "two"}, {1.11, 2.22}, {'a', 'b'}};
  std::string output;
  tinyrefl::reflection_to_json(obj, output);

  CHECK(!output.empty());
  CHECK(output.find("m_vec") != std::string::npos);
  CHECK(output.find("m_list") != std::string::npos);
  // 应该包含数组中的值
  CHECK(output.find("1") != std::string::npos);
  CHECK(output.find("one") != std::string::npos);
}

TEST_CASE("reflection_to_json - map containers") {
  MapContainers obj{{{"apple", 5}, {"banana", 10}},
                    {{"k1", "v1"}, {"k2", "v2"}}};
  std::string output;
  tinyrefl::reflection_to_json(obj, output);

  CHECK(!output.empty());
  CHECK(output.find("apple") != std::string::npos);
  CHECK(output.find("banana") != std::string::npos);
  CHECK(output.find("k1") != std::string::npos);
  CHECK(output.find("v1") != std::string::npos);
}

TEST_CASE("reflection_to_json - nested struct") {
  NestedStruct obj{
      {123, 4.5f, 6.78, 'Z', "hello", "world", true},
      {{1, 2, 3}, {"one", "two"}, {1.11, 2.22}, {'a', 'b'}},
      {{{"apple", 5}, {"banana", 10}}, {{"k1", "v1"}, {"k2", "v2"}}}};
  std::string output;
  tinyrefl::reflection_to_json(obj, output);

  CHECK(!output.empty());
  CHECK(output.find("m_basic") != std::string::npos);
  CHECK(output.find("m_seq") != std::string::npos);
  CHECK(output.find("m_maps") != std::string::npos);
}

TEST_CASE("reflection_to_json - deep nested struct") {
  NestedStruct inner{
      {123, 4.5f, 6.78, 'Z', "hello", "world", true},
      {{1, 2, 3}, {"one", "two"}, {1.11, 2.22}, {'a', 'b'}},
      {{{"apple", 5}, {"banana", 10}}, {{"k1", "v1"}, {"k2", "v2"}}}};
  DeepNest deep{inner,
                {{999, 3.14f, 2.72, 'Y', "ptr", "deep", true},
                 {{}, {}, {}, {}},
                 {{}, {}}}};
  std::string output;
  tinyrefl::reflection_to_json(deep, output);

  CHECK(!output.empty());
  CHECK(output.find("m_nested1") != std::string::npos);
  CHECK(output.find("m_nested2") != std::string::npos);
  CHECK(output.find("999") != std::string::npos);
  CHECK(output.find("deep") != std::string::npos);
}

TEST_CASE("reflection_to_json - empty containers") {
  NestedStruct empty{};
  empty.m_basic.m_cstr = nullptr;
  std::string output;
  tinyrefl::reflection_to_json(empty, output);

  // 即使容器为空，也应该能正常序列化
  CHECK(!output.empty());
  CHECK(output.find("m_basic") != std::string::npos);
}
