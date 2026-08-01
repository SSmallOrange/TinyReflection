#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/utils/reflection.hpp"

TEST_CASE("get_member_count - Person struct") {
    static_assert(tinyrefl::detail::members_count_v<Person> == 3);
    CHECK(tinyrefl::detail::members_count_v<Person> == 3);
}
