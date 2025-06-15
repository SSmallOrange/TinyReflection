#include "tinyrefl/reflection.hpp"

int main() {
	// test get members count
	static_assert(tinyrefl::members_count_v<Person> == 3);
    return 0;
}