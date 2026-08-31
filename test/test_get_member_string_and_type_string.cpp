#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/utils/reflection.hpp"

#include <array>
#include <string>

TEST_CASE("struct_members_to_tuple - get member names") {
  constexpr auto tp_name = tinyrefl::detail::struct_members_to_tuple<Person>();
  constexpr auto count = std::tuple_size_v<decltype(tp_name)>;

  // Person 应该有 3 个成员
  CHECK(count == 3);

  // 验证可以获取成员名（编译通过即代表功能正常）
  [[maybe_unused]] auto name0 =
      tinyrefl::detail::get_member_name<&std::get<0>(tp_name)>();
  [[maybe_unused]] auto name1 =
      tinyrefl::detail::get_member_name<&std::get<1>(tp_name)>();
  [[maybe_unused]] auto name2 =
      tinyrefl::detail::get_member_name<&std::get<2>(tp_name)>();

  // 成员名不应为空
  CHECK(name0.size() > 0);
  CHECK(name1.size() > 0);
  CHECK(name2.size() > 0);
}

TEST_CASE("struct_members_to_tuple - get member type names") {
  constexpr auto tp_tuple = tinyrefl::detail::struct_members_to_tuple<Person>();
  [[maybe_unused]] constexpr auto count = std::tuple_size_v<decltype(tp_tuple)>;

  // 验证可以获取类型名
  [[maybe_unused]] auto type0 =
      tinyrefl::detail::get_member_type_name<std::remove_const_t<
          std::remove_reference_t<decltype(std::get<0>(tp_tuple))>>>();
  [[maybe_unused]] auto type1 =
      tinyrefl::detail::get_member_type_name<std::remove_const_t<
          std::remove_reference_t<decltype(std::get<1>(tp_tuple))>>>();
  [[maybe_unused]] auto type2 =
      tinyrefl::detail::get_member_type_name<std::remove_const_t<
          std::remove_reference_t<decltype(std::get<2>(tp_tuple))>>>();

  CHECK(type0.size() > 0);
  CHECK(type1.size() > 0);
  CHECK(type2.size() > 0);
}

TEST_CASE("struct_members_to_array - get member name array") {
  constexpr auto array = tinyrefl::detail::struct_members_to_array<Person>();

  // Person 有 3 个成员
  CHECK(array.size() == 3);

  // 每个成员名不应为空
  for (size_t i = 0; i < array.size(); i++) {
    CHECK(std::string_view(array[i]).size() > 0);
  }
}
