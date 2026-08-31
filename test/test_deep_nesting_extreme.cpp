#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"

#include <cstdint>
#include <string>
#include <vector>

using namespace std;

// ============================================================
// 极端情况：同一类型在多层嵌套中复用
// ============================================================
struct Node {
  int id;
  string name;
};

struct NodeWrapper {
  Node node;
  int extra;
};

struct NodeContainer {
  Node direct_node;
  NodeWrapper wrapped_node;
  vector<Node> node_list;
  int count;
};

TEST_CASE("极端 - 同一类型 Node 在多个位置出现") {
  NodeContainer original;
  original.direct_node = {1, "direct"};
  original.wrapped_node = {{2, "wrapped"}, 99};
  original.node_list = {{3, "list0"}, {4, "list1"}, {5, "list2"}};
  original.count = 3;

  string json;
  tinyrefl::reflection_to_json(original, json);

  NodeContainer restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);

  CHECK(restored.direct_node.id == 1);
  CHECK(restored.direct_node.name == "direct");
  CHECK(restored.wrapped_node.node.id == 2);
  CHECK(restored.wrapped_node.node.name == "wrapped");
  CHECK(restored.wrapped_node.extra == 99);
  REQUIRE(restored.node_list.size() == 3);
  CHECK(restored.node_list[0].id == 3);
  CHECK(restored.node_list[0].name == "list0");
  CHECK(restored.node_list[2].id == 5);
  CHECK(restored.node_list[2].name == "list2");
  CHECK(restored.count == 3);
}

// ============================================================
// JSON 字段顺序与 struct 声明顺序不同
// ============================================================
struct Inner {
  int x;
  string y;
};

struct Outer {
  int a;
  Inner inner;
  string b;
};

TEST_CASE("极端 - JSON 字段顺序与 struct 不同") {
  // struct 声明顺序: a, inner, b
  // JSON 字段顺序: b, a, inner (倒过来)
  const char* json = R"({
    "b": "beta",
    "a": 42,
    "inner": {
      "y": "hello",
      "x": 7
    }
  })";

  Outer restored{};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);
  CHECK(restored.a == 42);
  CHECK(restored.b == "beta");
  CHECK(restored.inner.x == 7);
  CHECK(restored.inner.y == "hello");
}

// ============================================================
// 深层嵌套 + 字段倒序
// ============================================================
struct D {
  int d_val;
  string d_str;
};
struct C {
  D d;
  int c_val;
};
struct B {
  C c;
  string b_str;
};
struct A {
  B b;
  int a_val;
};

TEST_CASE("极端 - 4 层嵌套 JSON 字段全部倒序") {
  // 每一层的字段在 JSON 中都倒序排列
  const char* json = R"({
    "a_val": 100,
    "b": {
      "b_str": "bbb",
      "c": {
        "c_val": 300,
        "d": {
          "d_str": "ddd",
          "d_val": 400
        }
      }
    }
  })";

  A restored{};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);
  CHECK(restored.a_val == 100);
  CHECK(restored.b.b_str == "bbb");
  CHECK(restored.b.c.c_val == 300);
  CHECK(restored.b.c.d.d_val == 400);
  CHECK(restored.b.c.d.d_str == "ddd");
}

// ============================================================
// vector<struct> 中每个 struct 含有多个嵌套 struct
// ============================================================
struct Coord {
  double lat;
  double lng;
};

struct Address {
  string city;
  Coord coord;
};

struct Person {
  string name;
  int age;
  Address home;
  Address work;
};

struct Team {
  string team_name;
  vector<Person> members;
  Person leader;
  int size;
};

TEST_CASE("极端 - vector<struct> 每个元素含多个嵌套 struct") {
  Team original;
  original.team_name = "alpha";
  original.members = {
      {"Alice", 30, {"Beijing", {39.9, 116.4}}, {"Shanghai", {31.2, 121.5}}},
      {"Bob", 25, {"Shenzhen", {22.5, 114.1}}, {"Guangzhou", {23.1, 113.3}}},
  };
  original.leader = {"Charlie", 35, {"Hangzhou", {30.3, 120.2}},
                     {"Nanjing", {32.1, 118.8}}};
  original.size = 3;

  string json;
  tinyrefl::reflection_to_json(original, json);

  Team restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);

  CHECK(restored.team_name == "alpha");
  CHECK(restored.size == 3);

  REQUIRE(restored.members.size() == 2);
  CHECK(restored.members[0].name == "Alice");
  CHECK(restored.members[0].age == 30);
  CHECK(restored.members[0].home.city == "Beijing");
  CHECK(restored.members[0].home.coord.lat == doctest::Approx(39.9));
  CHECK(restored.members[0].home.coord.lng == doctest::Approx(116.4));
  CHECK(restored.members[0].work.city == "Shanghai");
  CHECK(restored.members[0].work.coord.lat == doctest::Approx(31.2));
  CHECK(restored.members[0].work.coord.lng == doctest::Approx(121.5));

  CHECK(restored.members[1].name == "Bob");
  CHECK(restored.members[1].home.city == "Shenzhen");
  CHECK(restored.members[1].work.city == "Guangzhou");
  CHECK(restored.members[1].work.coord.lng == doctest::Approx(113.3));

  CHECK(restored.leader.name == "Charlie");
  CHECK(restored.leader.home.city == "Hangzhou");
  CHECK(restored.leader.work.city == "Nanjing");
  CHECK(restored.leader.work.coord.lng == doctest::Approx(118.8));
}

// ============================================================
// 嵌套 struct 后紧跟另一个嵌套 struct + vector
// 重点验证第二个嵌套 struct 的数据不丢失
// ============================================================
struct Config1 {
  int timeout;
  string url;
};
struct Config2 {
  int retries;
  string endpoint;
};

struct Service {
  string service_name;
  Config1 primary;
  Config2 fallback;
  vector<string> tags;
  int priority;
};

TEST_CASE("极端 - 连续两个嵌套 struct 后跟 vector") {
  Service original{
      "api-gateway",
      {3000, "http://primary.example.com"},
      {5, "http://fallback.example.com"},
      {"production", "v2", "critical"},
      1,
  };

  string json;
  tinyrefl::reflection_to_json(original, json);

  Service restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);

  CHECK(restored.service_name == "api-gateway");
  CHECK(restored.primary.timeout == 3000);
  CHECK(restored.primary.url == "http://primary.example.com");
  CHECK(restored.fallback.retries == 5);
  CHECK(restored.fallback.endpoint == "http://fallback.example.com");
  REQUIRE(restored.tags.size() == 3);
  CHECK(restored.tags[0] == "production");
  CHECK(restored.tags[1] == "v2");
  CHECK(restored.tags[2] == "critical");
  CHECK(restored.priority == 1);
}

// ============================================================
// 6 层嵌套结构
// ============================================================
struct L6 { int v6; };
struct L5 { L6 l6; int v5; };
struct L4 { L5 l5; int v4; };
struct L3 { L4 l4; int v3; };
struct L2 { L3 l3; int v2; };
struct L1 { L2 l2; int v1; };

TEST_CASE("极端 - 6 层纯 struct 嵌套") {
  L1 original;
  original.l2.l3.l4.l5.l6.v6 = 6;
  original.l2.l3.l4.l5.v5 = 5;
  original.l2.l3.l4.v4 = 4;
  original.l2.l3.v3 = 3;
  original.l2.v2 = 2;
  original.v1 = 1;

  string json;
  tinyrefl::reflection_to_json(original, json);

  L1 restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);

  CHECK(restored.l2.l3.l4.l5.l6.v6 == 6);
  CHECK(restored.l2.l3.l4.l5.v5 == 5);
  CHECK(restored.l2.l3.l4.v4 == 4);
  CHECK(restored.l2.l3.v3 == 3);
  CHECK(restored.l2.v2 == 2);
  CHECK(restored.v1 == 1);
}

// ============================================================
// vector<struct> 中的 struct 含有 vector<struct>（递归式嵌套）
// ============================================================
struct Comment {
  int id;
  string text;
};

struct Post {
  int post_id;
  string title;
  vector<Comment> comments;
};

struct Blog {
  string blog_name;
  vector<Post> posts;
  int total_posts;
};

TEST_CASE("极端 - blog/post/comment 三层 vector+struct 嵌套") {
  Blog original;
  original.blog_name = "Tech Blog";
  original.posts = {
      {1, "First Post", {{101, "Great!"}, {102, "Nice work"}, {103, "Thanks"}}},
      {2, "Second Post", {{201, "Interesting"}}},
      {3, "Third Post", {}},  // 空评论
  };
  original.total_posts = 3;

  string json;
  tinyrefl::reflection_to_json(original, json);

  Blog restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);

  CHECK(restored.blog_name == "Tech Blog");
  CHECK(restored.total_posts == 3);
  REQUIRE(restored.posts.size() == 3);

  CHECK(restored.posts[0].post_id == 1);
  CHECK(restored.posts[0].title == "First Post");
  REQUIRE(restored.posts[0].comments.size() == 3);
  CHECK(restored.posts[0].comments[0].id == 101);
  CHECK(restored.posts[0].comments[0].text == "Great!");
  CHECK(restored.posts[0].comments[1].id == 102);
  CHECK(restored.posts[0].comments[1].text == "Nice work");
  CHECK(restored.posts[0].comments[2].id == 103);
  CHECK(restored.posts[0].comments[2].text == "Thanks");

  CHECK(restored.posts[1].post_id == 2);
  CHECK(restored.posts[1].title == "Second Post");
  REQUIRE(restored.posts[1].comments.size() == 1);
  CHECK(restored.posts[1].comments[0].id == 201);
  CHECK(restored.posts[1].comments[0].text == "Interesting");

  CHECK(restored.posts[2].post_id == 3);
  CHECK(restored.posts[2].title == "Third Post");
  CHECK(restored.posts[2].comments.empty());
}

// ============================================================
// 属性测试: SplitMix64 PRNG 生成随机数据做大量 roundtrip
// ============================================================
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
  int next_int(int lo, int hi) {
    return lo + static_cast<int>(next() % static_cast<uint64_t>(hi - lo + 1));
  }
  string next_string(int max_len = 20) {
    int len = next_int(0, max_len);
    string s;
    s.reserve(len);
    for (int i = 0; i < len; ++i) {
      s += static_cast<char>('a' + next_int(0, 25));
    }
    return s;
  }
};

struct PropItem {
  int id;
  string label;
};

struct PropInner {
  PropItem item;
  vector<int> nums;
  int val;
};

struct PropOuter {
  string name;
  PropInner inner;
  vector<PropItem> items;
  int count;
};

TEST_CASE("属性测试 - 随机生成 PropOuter 做 2000 次 roundtrip") {
  SplitMix64 rng(12345);

  for (int trial = 0; trial < 2000; ++trial) {
    PropOuter original;
    original.name = rng.next_string(15);
    original.inner.item.id = rng.next_int(-1000, 1000);
    original.inner.item.label = rng.next_string(10);

    int num_count = rng.next_int(0, 10);
    for (int i = 0; i < num_count; ++i) {
      original.inner.nums.push_back(rng.next_int(-100000, 100000));
    }
    original.inner.val = rng.next_int(-999, 999);

    int item_count = rng.next_int(0, 5);
    for (int i = 0; i < item_count; ++i) {
      original.items.push_back(
          {rng.next_int(0, 99999), rng.next_string(8)});
    }
    original.count = rng.next_int(0, 100);

    // 序列化
    string json;
    tinyrefl::reflection_to_json(original, json);

    // 反序列化
    PropOuter restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());

    INFO("trial=" << trial << " json=" << json);
    REQUIRE(status.ok == true);

    CHECK(restored.name == original.name);
    CHECK(restored.inner.item.id == original.inner.item.id);
    CHECK(restored.inner.item.label == original.inner.item.label);
    REQUIRE(restored.inner.nums.size() == original.inner.nums.size());
    for (size_t i = 0; i < original.inner.nums.size(); ++i) {
      CHECK(restored.inner.nums[i] == original.inner.nums[i]);
    }
    CHECK(restored.inner.val == original.inner.val);
    REQUIRE(restored.items.size() == original.items.size());
    for (size_t i = 0; i < original.items.size(); ++i) {
      CHECK(restored.items[i].id == original.items[i].id);
      CHECK(restored.items[i].label == original.items[i].label);
    }
    CHECK(restored.count == original.count);
  }
}

// ============================================================
// 属性测试: 随机 Blog (三层嵌套) 做 500 次 roundtrip
// ============================================================
TEST_CASE("属性测试 - 随机 Blog 三层嵌套 500 次 roundtrip") {
  SplitMix64 rng(67890);

  for (int trial = 0; trial < 500; ++trial) {
    Blog original;
    original.blog_name = rng.next_string(12);
    int post_count = rng.next_int(0, 5);
    for (int p = 0; p < post_count; ++p) {
      Post post;
      post.post_id = rng.next_int(1, 10000);
      post.title = rng.next_string(15);
      int comment_count = rng.next_int(0, 4);
      for (int c = 0; c < comment_count; ++c) {
        post.comments.push_back(
            {rng.next_int(1, 99999), rng.next_string(20)});
      }
      original.posts.push_back(post);
    }
    original.total_posts = post_count;

    string json;
    tinyrefl::reflection_to_json(original, json);

    Blog restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());

    INFO("trial=" << trial << " json=" << json);
    REQUIRE(status.ok == true);

    CHECK(restored.blog_name == original.blog_name);
    CHECK(restored.total_posts == original.total_posts);
    REQUIRE(restored.posts.size() == original.posts.size());

    for (size_t p = 0; p < original.posts.size(); ++p) {
      CHECK(restored.posts[p].post_id == original.posts[p].post_id);
      CHECK(restored.posts[p].title == original.posts[p].title);
      REQUIRE(restored.posts[p].comments.size() ==
              original.posts[p].comments.size());
      for (size_t c = 0; c < original.posts[p].comments.size(); ++c) {
        CHECK(restored.posts[p].comments[c].id ==
              original.posts[p].comments[c].id);
        CHECK(restored.posts[p].comments[c].text ==
              original.posts[p].comments[c].text);
      }
    }
  }
}

// ============================================================
// 属性测试: 随机 Team (四层嵌套 struct) 做 500 次 roundtrip
// ============================================================
TEST_CASE("属性测试 - 随机 Team 四层嵌套 500 次 roundtrip") {
  SplitMix64 rng(11111);

  for (int trial = 0; trial < 500; ++trial) {
    Team original;
    original.team_name = rng.next_string(10);
    int member_count = rng.next_int(0, 4);
    for (int m = 0; m < member_count; ++m) {
      Person person;
      person.name = rng.next_string(8);
      person.age = rng.next_int(18, 65);
      person.home.city = rng.next_string(6);
      person.home.coord.lat = static_cast<double>(rng.next_int(-90, 90));
      person.home.coord.lng = static_cast<double>(rng.next_int(-180, 180));
      person.work.city = rng.next_string(6);
      person.work.coord.lat = static_cast<double>(rng.next_int(-90, 90));
      person.work.coord.lng = static_cast<double>(rng.next_int(-180, 180));
      original.members.push_back(person);
    }
    original.leader.name = rng.next_string(8);
    original.leader.age = rng.next_int(25, 60);
    original.leader.home = {rng.next_string(6),
                            {static_cast<double>(rng.next_int(-90, 90)),
                             static_cast<double>(rng.next_int(-180, 180))}};
    original.leader.work = {rng.next_string(6),
                            {static_cast<double>(rng.next_int(-90, 90)),
                             static_cast<double>(rng.next_int(-180, 180))}};
    original.size = member_count + 1;

    string json;
    tinyrefl::reflection_to_json(original, json);

    Team restored{};
    auto status = tinyrefl::reflection_from_json(restored, json.c_str());

    INFO("trial=" << trial << " json=" << json);
    REQUIRE(status.ok == true);

    CHECK(restored.team_name == original.team_name);
    CHECK(restored.size == original.size);
    REQUIRE(restored.members.size() == original.members.size());
    for (size_t m = 0; m < original.members.size(); ++m) {
      CHECK(restored.members[m].name == original.members[m].name);
      CHECK(restored.members[m].age == original.members[m].age);
      CHECK(restored.members[m].home.city == original.members[m].home.city);
      CHECK(restored.members[m].home.coord.lat ==
            original.members[m].home.coord.lat);
      CHECK(restored.members[m].home.coord.lng ==
            original.members[m].home.coord.lng);
      CHECK(restored.members[m].work.city == original.members[m].work.city);
      CHECK(restored.members[m].work.coord.lat ==
            original.members[m].work.coord.lat);
      CHECK(restored.members[m].work.coord.lng ==
            original.members[m].work.coord.lng);
    }
    CHECK(restored.leader.name == original.leader.name);
    CHECK(restored.leader.age == original.leader.age);
    CHECK(restored.leader.home.city == original.leader.home.city);
    CHECK(restored.leader.work.city == original.leader.work.city);
  }
}
