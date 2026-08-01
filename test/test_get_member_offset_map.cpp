#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/utils/reflection.hpp"

#include <format>
#include <string>

TEST_CASE("struct_member_offset_array - basic functionality") {
    auto& arr = tinyrefl::detail::struct_member_offset_array<Person>();

    // Person 有 3 个成员，offset 数组大小应为 3
    CHECK(arr.size() == 3);

    // 第一个成员的 offset 应该 >= 0
    CHECK(arr[0] >= 0);

    // offset 应该是递增的（成员按顺序排列）
    for (size_t i = 1; i < arr.size(); ++i) {
        CHECK(arr[i] > arr[i - 1]);
    }
}

TEST_CASE("struct_member_offset_map - basic functionality") {
    static auto member_offset_map = tinyrefl::detail::struct_member_offset_map<Person>();
    auto member_name_arr = tinyrefl::detail::struct_members_to_array<Person>();

    // map 大小应等于成员数
    CHECK(member_offset_map.size() == 3);

    // 每个成员名都应该在 map 中找到
    for (size_t i = 0; i < member_name_arr.size(); ++i) {
        auto it = member_offset_map.find(std::string(member_name_arr[i]));
        CHECK(it != member_offset_map.end());
    }
}

TEST_CASE("Wrapper - value size consistency") {
    // Wrapper 中的 value 大小应等于 Person 的大小
    CHECK(sizeof(tinyrefl::detail::Wrapper<Person>::value) == sizeof(Person));
}
