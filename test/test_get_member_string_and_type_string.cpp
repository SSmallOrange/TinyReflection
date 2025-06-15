#include "tinyrefl/reflection.hpp"

namespace tinyrefl {
// get members name strings
template <auto val>
inline consteval std::string_view get_member_name() {
	std::string_view funcName = __FUNCSIG__;
	size_t begin = funcName.rfind("->") + 2;
	size_t end = funcName.rfind(">(");
	return funcName.substr(begin, end - begin);
}

// get members type strings
template <typename T>
inline consteval std::string_view get_member_type_name() {
	std::string_view funcName = __FUNCSIG__;
	size_t start = funcName.find("get_member_type_name<");
	size_t end = funcName.find("(void)");
	return funcName.substr(start + 21, end - start - 22);
}
}

int main() {
    	// test get members name
	constexpr auto tp_name = tinyrefl::struct_members_to_tuple<Person>();
	[&]<size_t... Is>(std::index_sequence<Is...> seq) {
		((std::cout << tinyrefl::get_member_name<&std::get<Is>(tp_name)>() << "\n"), ...);
	}(std::make_index_sequence<std::tuple_size_v<decltype(tp_name)>>{});

	std::cout << "\n\n\n";

	// test get members tgype
	constexpr auto tp_tuple = tinyrefl::struct_members_to_tuple<Person>();
	[&]<size_t... Is>(std::index_sequence<Is...> seq) {
		((std::cout << tinyrefl::get_member_type_name<std::remove_const_t<std::remove_reference_t<decltype(std::get<Is>(tp_tuple))>>>() << "\n"), ...);
	}(std::make_index_sequence<std::tuple_size_v<decltype(tp_tuple)>>{});

    return 0;
}