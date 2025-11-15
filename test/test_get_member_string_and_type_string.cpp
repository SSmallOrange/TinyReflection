#include "tinyrefl/utils/reflection.hpp"

#include <array>
int main() {
    	// test get members name
	constexpr auto tp_name = tinyrefl::detail::struct_members_to_tuple<Person>();
	[&]<size_t... Is>(std::index_sequence<Is...> seq) {
		((std::cout << tinyrefl::detail::get_member_name<&std::get<Is>(tp_name)>() << "\n"), ...);
	}(std::make_index_sequence<std::tuple_size_v<decltype(tp_name)>>{});

	std::cout << "\n\n\n";

	// test get members type
	constexpr auto tp_tuple = tinyrefl::detail::struct_members_to_tuple<Person>();
	[&]<size_t... Is>(std::index_sequence<Is...> seq) {
		((std::cout << tinyrefl::detail::get_member_type_name<std::remove_const_t<std::remove_reference_t<decltype(std::get<Is>(tp_tuple))>>>() << "\n"), ...);
	}(std::make_index_sequence<std::tuple_size_v<decltype(tp_tuple)>>{});

	// test get member array
	constexpr auto array = tinyrefl::detail::struct_members_to_array<Person>();
	for (int i = 0; i < array.size(); i++) {
		std::cout << array[i] << "\n";
	}

    return 0;
}