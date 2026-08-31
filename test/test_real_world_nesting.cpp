#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"

#include <string>
#include <vector>

using namespace std;

// ============================================================
// 模拟真实 API 响应的复杂嵌套结构
// ============================================================

struct GeoLocation {
  double latitude;
  double longitude;
};

struct Address {
  string street;
  string city;
  string state;
  string zip;
  GeoLocation geo;
};

struct ContactInfo {
  string email;
  string phone;
  Address address;
};

struct Skill {
  string name;
  int level;
};

struct Employee {
  int id;
  string first_name;
  string last_name;
  int age;
  ContactInfo contact;
  vector<Skill> skills;
  string department;
};

struct Department {
  string name;
  int floor;
  Employee manager;
  vector<Employee> employees;
};

struct Company {
  string company_name;
  int founded_year;
  Address headquarters;
  vector<Department> departments;
  int total_employees;
};

// ============================================================
// 测试 1: 完整的 Company JSON roundtrip（6层嵌套）
// Company -> Department -> Employee -> ContactInfo -> Address -> GeoLocation
// ============================================================
TEST_CASE("真实场景 - Company 6 层嵌套完整 roundtrip") {
  Company original;
  original.company_name = "TechCorp";
  original.founded_year = 2010;
  original.headquarters = {
      "123 Main St", "San Francisco", "CA", "94105", {37.7749, -122.4194}};
  original.total_employees = 5;

  Department eng;
  eng.name = "Engineering";
  eng.floor = 3;
  eng.manager = {
      1,
      "Alice",
      "Smith",
      35,
      {"alice@tech.com",
       "555-0001",
       {"456 Oak Ave", "San Francisco", "CA", "94110", {37.7599, -122.4148}}},
      {{"C++", 9}, {"Python", 8}, {"Leadership", 7}},
      "Engineering"};

  Employee emp1 = {
      2,
      "Bob",
      "Jones",
      28,
      {"bob@tech.com",
       "555-0002",
       {"789 Pine Rd", "Oakland", "CA", "94612", {37.8044, -122.2712}}},
      {{"C++", 7}, {"Rust", 6}},
      "Engineering"};

  Employee emp2 = {
      3,
      "Carol",
      "Lee",
      31,
      {"carol@tech.com",
       "555-0003",
       {"321 Elm St", "Berkeley", "CA", "94704", {37.8716, -122.2727}}},
      {{"Java", 8}, {"Go", 5}, {"Docker", 7}},
      "Engineering"};

  eng.employees = {emp1, emp2};

  Department hr;
  hr.name = "HR";
  hr.floor = 2;
  hr.manager = {4,
                "Dave",
                "Wilson",
                40,
                {"dave@tech.com",
                 "555-0004",
                 {"100 First St", "San Jose", "CA", "95113", {37.3382, -121.8863}}},
                {{"Recruiting", 9}, {"Communication", 8}},
                "HR"};
  hr.employees = {
      {5,
       "Eve",
       "Brown",
       26,
       {"eve@tech.com",
        "555-0005",
        {"200 Second St", "Palo Alto", "CA", "94301", {37.4419, -122.1430}}},
       {{"Recruiting", 6}},
       "HR"},
  };

  original.departments = {eng, hr};

  // 序列化
  string json;
  tinyrefl::reflection_to_json(original, json);

  // 反序列化
  Company restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  REQUIRE(status.ok == true);

  // 验证根层
  CHECK(restored.company_name == "TechCorp");
  CHECK(restored.founded_year == 2010);
  CHECK(restored.total_employees == 5);

  // 验证总部地址（3层: Company -> Address -> GeoLocation）
  CHECK(restored.headquarters.street == "123 Main St");
  CHECK(restored.headquarters.city == "San Francisco");
  CHECK(restored.headquarters.state == "CA");
  CHECK(restored.headquarters.zip == "94105");
  CHECK(restored.headquarters.geo.latitude == doctest::Approx(37.7749));
  CHECK(restored.headquarters.geo.longitude == doctest::Approx(-122.4194));

  // 验证部门
  REQUIRE(restored.departments.size() == 2);

  // === Engineering 部门 ===
  auto& eng_r = restored.departments[0];
  CHECK(eng_r.name == "Engineering");
  CHECK(eng_r.floor == 3);

  // Manager（6层: Company -> Department -> Employee -> ContactInfo -> Address
  // -> Geo）
  CHECK(eng_r.manager.id == 1);
  CHECK(eng_r.manager.first_name == "Alice");
  CHECK(eng_r.manager.last_name == "Smith");
  CHECK(eng_r.manager.age == 35);
  CHECK(eng_r.manager.contact.email == "alice@tech.com");
  CHECK(eng_r.manager.contact.phone == "555-0001");
  CHECK(eng_r.manager.contact.address.street == "456 Oak Ave");
  CHECK(eng_r.manager.contact.address.city == "San Francisco");
  CHECK(eng_r.manager.contact.address.geo.latitude ==
        doctest::Approx(37.7599));
  CHECK(eng_r.manager.contact.address.geo.longitude ==
        doctest::Approx(-122.4148));
  REQUIRE(eng_r.manager.skills.size() == 3);
  CHECK(eng_r.manager.skills[0].name == "C++");
  CHECK(eng_r.manager.skills[0].level == 9);
  CHECK(eng_r.manager.skills[1].name == "Python");
  CHECK(eng_r.manager.skills[1].level == 8);
  CHECK(eng_r.manager.skills[2].name == "Leadership");
  CHECK(eng_r.manager.skills[2].level == 7);
  CHECK(eng_r.manager.department == "Engineering");

  // Employees
  REQUIRE(eng_r.employees.size() == 2);

  CHECK(eng_r.employees[0].id == 2);
  CHECK(eng_r.employees[0].first_name == "Bob");
  CHECK(eng_r.employees[0].contact.email == "bob@tech.com");
  CHECK(eng_r.employees[0].contact.address.city == "Oakland");
  CHECK(eng_r.employees[0].contact.address.geo.latitude ==
        doctest::Approx(37.8044));
  REQUIRE(eng_r.employees[0].skills.size() == 2);
  CHECK(eng_r.employees[0].skills[0].name == "C++");
  CHECK(eng_r.employees[0].skills[1].name == "Rust");

  CHECK(eng_r.employees[1].id == 3);
  CHECK(eng_r.employees[1].first_name == "Carol");
  CHECK(eng_r.employees[1].contact.address.city == "Berkeley");
  REQUIRE(eng_r.employees[1].skills.size() == 3);
  CHECK(eng_r.employees[1].skills[2].name == "Docker");
  CHECK(eng_r.employees[1].skills[2].level == 7);

  // === HR 部门 ===
  auto& hr_r = restored.departments[1];
  CHECK(hr_r.name == "HR");
  CHECK(hr_r.floor == 2);
  CHECK(hr_r.manager.id == 4);
  CHECK(hr_r.manager.first_name == "Dave");
  CHECK(hr_r.manager.contact.address.city == "San Jose");
  CHECK(hr_r.manager.contact.address.geo.latitude ==
        doctest::Approx(37.3382));
  REQUIRE(hr_r.manager.skills.size() == 2);
  CHECK(hr_r.manager.skills[0].name == "Recruiting");

  REQUIRE(hr_r.employees.size() == 1);
  CHECK(hr_r.employees[0].id == 5);
  CHECK(hr_r.employees[0].first_name == "Eve");
  CHECK(hr_r.employees[0].contact.address.city == "Palo Alto");
  CHECK(hr_r.employees[0].contact.address.geo.longitude ==
        doctest::Approx(-122.1430));
}

// ============================================================
// 测试 2: 从手写 JSON 解析（模拟外部 API 响应）
// 含有额外未知字段的真实场景
// ============================================================
TEST_CASE("真实场景 - 手写 JSON 含未知字段") {
  const char* json = R"({
    "company_name": "StartupInc",
    "founded_year": 2020,
    "headquarters": {
      "street": "1 Startup Way",
      "city": "Austin",
      "state": "TX",
      "zip": "73301",
      "geo": {
        "latitude": 30.2672,
        "longitude": -97.7431
      }
    },
    "departments": [
      {
        "name": "Product",
        "floor": 1,
        "manager": {
          "id": 100,
          "first_name": "Frank",
          "last_name": "Miller",
          "age": 38,
          "contact": {
            "email": "frank@startup.com",
            "phone": "555-1000",
            "address": {
              "street": "50 Startup Blvd",
              "city": "Austin",
              "state": "TX",
              "zip": "73301",
              "geo": {
                "latitude": 30.25,
                "longitude": -97.75
              }
            }
          },
          "skills": [
            {"name": "Product Management", "level": 9},
            {"name": "Analytics", "level": 7}
          ],
          "department": "Product"
        },
        "employees": []
      }
    ],
    "total_employees": 1
  })";

  Company restored{};
  auto status = tinyrefl::reflection_from_json(restored, json);
  REQUIRE(status.ok == true);

  CHECK(restored.company_name == "StartupInc");
  CHECK(restored.founded_year == 2020);
  CHECK(restored.headquarters.city == "Austin");
  CHECK(restored.headquarters.geo.latitude == doctest::Approx(30.2672));
  CHECK(restored.headquarters.geo.longitude == doctest::Approx(-97.7431));

  REQUIRE(restored.departments.size() == 1);
  auto& dept = restored.departments[0];
  CHECK(dept.name == "Product");
  CHECK(dept.floor == 1);
  CHECK(dept.manager.id == 100);
  CHECK(dept.manager.first_name == "Frank");
  CHECK(dept.manager.last_name == "Miller");
  CHECK(dept.manager.contact.email == "frank@startup.com");
  CHECK(dept.manager.contact.address.city == "Austin");
  CHECK(dept.manager.contact.address.geo.latitude == doctest::Approx(30.25));
  CHECK(dept.manager.contact.address.geo.longitude == doctest::Approx(-97.75));
  REQUIRE(dept.manager.skills.size() == 2);
  CHECK(dept.manager.skills[0].name == "Product Management");
  CHECK(dept.manager.skills[0].level == 9);
  CHECK(dept.manager.department == "Product");
  CHECK(dept.employees.empty());
  CHECK(restored.total_employees == 1);
}

// ============================================================
// 测试 3: 二次 roundtrip 验证 JSON 完整性
// ============================================================
TEST_CASE("真实场景 - Company 二次 roundtrip JSON 一致性") {
  Company original;
  original.company_name = "MegaCorp";
  original.founded_year = 1995;
  original.headquarters = {"100 Corp Dr", "NYC", "NY", "10001", {40.7128, -74.0060}};
  original.departments = {
    {"Sales", 5,
     {10, "Grace", "Hopper", 45,
      {"grace@mega.com", "555-9999", {"1 Park Ave", "NYC", "NY", "10002", {40.7, -74.0}}},
      {{"Sales", 10}}, "Sales"},
     {}},
  };
  original.total_employees = 1;

  // 第一次 roundtrip
  string json1;
  tinyrefl::reflection_to_json(original, json1);
  Company r1{};
  REQUIRE(tinyrefl::reflection_from_json(r1, json1.c_str()).ok);

  // 第二次 roundtrip
  string json2;
  tinyrefl::reflection_to_json(r1, json2);

  // JSON 应完全一致
  CHECK(json1 == json2);
}

// ============================================================
// 测试 4: 含有 ignore<> 字段的嵌套 struct roundtrip
// ============================================================
struct Metadata {
  string key;
  string value;
};

struct Resource {
  int id;
  string name;
  tinyrefl::ignore<string> internal_id;  // 被忽略的字段
  Metadata meta;
  vector<Metadata> tags;
  int version;
};

struct ApiResponse {
  string status_text;
  tinyrefl::ignore<int> http_code;  // 被忽略的字段
  vector<Resource> resources;
  int page;
};

TEST_CASE("真实场景 - 含 ignore<> 字段的嵌套 roundtrip") {
  ApiResponse original;
  original.status_text = "OK";
  original.http_code = tinyrefl::ignore<int>(200);
  original.resources = {
      {1, "res1", tinyrefl::ignore<string>("internal_1"), {"type", "doc"},
       {{"tag1", "v1"}, {"tag2", "v2"}}, 3},
      {2, "res2", tinyrefl::ignore<string>("internal_2"), {"type", "img"},
       {{"tag3", "v3"}}, 1},
  };
  original.page = 1;

  string json;
  tinyrefl::reflection_to_json(original, json);

  ApiResponse restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  REQUIRE(status.ok == true);

  CHECK(restored.status_text == "OK");
  CHECK(restored.page == 1);

  REQUIRE(restored.resources.size() == 2);
  CHECK(restored.resources[0].id == 1);
  CHECK(restored.resources[0].name == "res1");
  CHECK(restored.resources[0].meta.key == "type");
  CHECK(restored.resources[0].meta.value == "doc");
  REQUIRE(restored.resources[0].tags.size() == 2);
  CHECK(restored.resources[0].tags[0].key == "tag1");
  CHECK(restored.resources[0].tags[0].value == "v1");
  CHECK(restored.resources[0].tags[1].key == "tag2");
  CHECK(restored.resources[0].tags[1].value == "v2");
  CHECK(restored.resources[0].version == 3);

  CHECK(restored.resources[1].id == 2);
  CHECK(restored.resources[1].name == "res2");
  CHECK(restored.resources[1].meta.key == "type");
  CHECK(restored.resources[1].meta.value == "img");
  REQUIRE(restored.resources[1].tags.size() == 1);
  CHECK(restored.resources[1].tags[0].key == "tag3");
  CHECK(restored.resources[1].version == 1);
}

// ============================================================
// 测试 5: 未知字段中含有与已知字段同名的嵌套对象
// 这是一个潜在的数据污染 bug
// ============================================================
struct SimpleInner {
  int value;
};

struct SimpleOuter {
  SimpleInner inner;
  int after;
};

TEST_CASE("潜在 bug - 未知字段含同名嵌套对象时是否污染数据") {
  // JSON 中 "unknown" 是未知字段，但其子对象含 "inner" 键
  // 且这个 "inner" 里有 "value" 键
  // 问题: inner.value 是否被未知对象中的值覆盖？
  const char* json = R"({
    "inner": {"value": 42},
    "unknown_obj": {"inner": {"value": 999}},
    "after": 10
  })";

  SimpleOuter restored{};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);

  // inner.value 应该是 42（来自正确的 "inner" 字段）
  // 而不是 999（来自 unknown_obj 中的子对象）
  // 但如果已知 "inner" 在前，unknown 在后，其内部 "inner" 可能覆盖
  INFO("inner.value = " << restored.inner.value
                        << " (expected 42, but unknown_obj may contaminate)");

  // after 应该是 10
  CHECK(restored.after == 10);

  // 这个 CHECK 可能揭示污染 bug:
  // 如果 unknown_obj 的子对象 "inner":{"value":999} 被当前 handler 处理，
  // 且 "inner" 匹配到了 SimpleOuter 的 inner 字段，那么会 push 一个新的 InnerHandler，
  // 导致 inner.value 被覆盖为 999
  CHECK(restored.inner.value == 42);
}

TEST_CASE("潜在 bug - 未知字段在已知字段之前时的数据完整性") {
  // 未知字段在前，已知字段在后
  const char* json = R"({
    "unknown_obj": {"inner": {"value": 999}, "after": 888},
    "inner": {"value": 42},
    "after": 10
  })";

  SimpleOuter restored{};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);

  // 如果 unknown_obj 的处理没有 push handler，
  // 其内部的 "inner":{"value":999} 会被当前 (SimpleOuter) handler 处理
  // "inner" 匹配到了 SimpleOuter.inner，会 push InnerHandler
  // value=999 会被写入 inner.value
  // 然后 "after":888 会被写入 SimpleOuter.after
  // 最后正确的 "inner":{"value":42} 会覆盖 inner.value=42
  // "after":10 会覆盖 after=10

  CHECK(restored.inner.value == 42);
  CHECK(restored.after == 10);
}

// ============================================================
// 测试 6: 更复杂的未知字段污染场景
// 未知字段在最后，且含有与已知字段同名的键
// ============================================================
TEST_CASE("污染 bug - 已知字段在前 未知字段在后且含同名键") {
  // 已知字段先出现，但 unknown 子对象含同名键，可能覆盖
  const char* json = R"({
    "inner": {"value": 42},
    "after": 10,
    "unknown_trailing": {"inner": {"value": 0}, "after": 0}
  })";

  SimpleOuter restored{};
  auto status = tinyrefl::reflection_from_json(restored, json);
  CHECK(status.ok == true);

  INFO("inner.value = " << restored.inner.value);
  INFO("after = " << restored.after);

  // 如果存在污染 bug，inner.value 会被覆盖为 0
  // after 也可能被覆盖为 0
  CHECK(restored.inner.value == 42);
  CHECK(restored.after == 10);
}
