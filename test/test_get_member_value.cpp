#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/utils/reflection.hpp"

#include <string>
#include <type_traits>

struct Test1 {
  int a;
};
struct Test2 {
  double x;
  double y;
};
struct Test3 {
  std::string name;
  int age;
  bool active;
};
struct Test4 {
  char c;
  short s;
  int i;
  long l;
};
struct Test5 {
  float f1;
  float f2;
  float f3;
  float f4;
  float f5;
};

TEST_CASE("member_count - compile time") {
  static_assert(tinyrefl::detail::members_count_v<Test1> == 1);
  static_assert(tinyrefl::detail::members_count_v<Test2> == 2);
  static_assert(tinyrefl::detail::members_count_v<Test3> == 3);
  static_assert(tinyrefl::detail::members_count_v<Test4> == 4);
  static_assert(tinyrefl::detail::members_count_v<Test5> == 5);
  CHECK(true);
}

TEST_CASE("struct_member_reference - single member struct") {
  Test1 t1{42};
  auto& ref = tinyrefl::detail::struct_member_reference<0>(t1);
  static_assert(std::is_same_v<decltype(ref), int&>);

  CHECK(ref == 42);
  ref = 100;
  CHECK(t1.a == 100);
}

TEST_CASE("struct_member_reference - two members struct") {
  Test2 t2{3.14, 2.71};
  auto& ref_x = tinyrefl::detail::struct_member_reference<0>(t2);
  auto& ref_y = tinyrefl::detail::struct_member_reference<1>(t2);
  static_assert(std::is_same_v<decltype(ref_x), double&>);
  static_assert(std::is_same_v<decltype(ref_y), double&>);

  ref_x = 1.0;
  ref_y = 2.0;
  CHECK(t2.x == 1.0);
  CHECK(t2.y == 2.0);
}

TEST_CASE("struct_member_reference - three members struct") {
  Test3 t3{"Alice", 30, true};
  auto& name_ref = tinyrefl::detail::struct_member_reference<0>(t3);
  auto& age_ref = tinyrefl::detail::struct_member_reference<1>(t3);
  auto& active_ref = tinyrefl::detail::struct_member_reference<2>(t3);
  static_assert(std::is_same_v<decltype(name_ref), std::string&>);
  static_assert(std::is_same_v<decltype(age_ref), int&>);
  static_assert(std::is_same_v<decltype(active_ref), bool&>);

  name_ref = "Bob";
  age_ref = 25;
  active_ref = false;

  CHECK(t3.name == "Bob");
  CHECK(t3.age == 25);
  CHECK(t3.active == false);
}

TEST_CASE("struct_member_reference - four members struct") {
  Test4 t4{'A', 10, 20, 30L};
  auto& c_ref = tinyrefl::detail::struct_member_reference<0>(t4);
  auto& s_ref = tinyrefl::detail::struct_member_reference<1>(t4);
  auto& i_ref = tinyrefl::detail::struct_member_reference<2>(t4);
  auto& l_ref = tinyrefl::detail::struct_member_reference<3>(t4);

  c_ref = 'B';
  s_ref = 100;
  i_ref = 200;
  l_ref = 300L;

  CHECK(t4.c == 'B');
  CHECK(t4.s == 100);
  CHECK(t4.i == 200);
  CHECK(t4.l == 300L);
}

TEST_CASE("struct_member_reference - five members struct") {
  Test5 t5{1.1f, 2.2f, 3.3f, 4.4f, 5.5f};
  auto& f1_ref = tinyrefl::detail::struct_member_reference<0>(t5);
  auto& f5_ref = tinyrefl::detail::struct_member_reference<4>(t5);

  f1_ref = 10.0f;
  f5_ref = 50.0f;

  CHECK(t5.f1 == 10.0f);
  CHECK(t5.f5 == 50.0f);
}

TEST_CASE("struct_member_reference - const object") {
  const Test3 const_t3{"Charlie", 40, true};
  const auto& name_ref = tinyrefl::detail::struct_member_reference<0>(const_t3);
  const auto& age_ref = tinyrefl::detail::struct_member_reference<1>(const_t3);
  static_assert(std::is_same_v<decltype(name_ref), const std::string&>);
  static_assert(std::is_same_v<decltype(age_ref), const int&>);

  CHECK(name_ref == "Charlie");
  CHECK(age_ref == 40);
}

TEST_CASE("struct_member_reference - rvalue") {
  Test3 temp{"Dave", 50, false};
  auto&& name_ref =
      tinyrefl::detail::struct_member_reference<0>(std::move(temp));
  CHECK(name_ref == "Dave");
}

TEST_CASE("struct_member_reference - type verification") {
  Test3 t3{};
  static_assert(
      std::is_same_v<decltype(tinyrefl::detail::struct_member_reference<0>(t3)),
                     std::string&>);
  static_assert(
      std::is_same_v<decltype(tinyrefl::detail::struct_member_reference<2>(t3)),
                     bool&>);
  CHECK(true);
}

TEST_CASE("struct_member_reference - value categories") {
  Test2 t2{1.0, 2.0};

  // lvalue reference
  auto& lv_ref = tinyrefl::detail::struct_member_reference<0>(t2);
  lv_ref = 10.0;
  CHECK(t2.x == 10.0);

  // rvalue reference
  auto&& rv_ref = tinyrefl::detail::struct_member_reference<1>(std::move(t2));
  rv_ref = 20.0;
  CHECK(t2.y == 20.0);
}
