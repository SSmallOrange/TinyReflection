#include "tinyrefl/utils/reflection.hpp"

int main() {
	// test get members count
	static_assert(tinyrefl::detail::members_count_v<Person> == 3);
    return 0;
}