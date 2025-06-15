#include <iostream>
#include <string>
#include <type_traits>
#include <tuple>

struct Person {
	std::string m_name;	
	int m_age;
	bool m_male;
};

// utils
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T>
struct Wrapper {
	inline static remove_cvref_t<T> _value;
};

template <typename T>
inline constexpr T& get_global_value() {
	return Wrapper<remove_cvref_t<T>>::_value;
}

// get members count
template <typename T>
struct AnyType {
	template <typename T>
    operator T();
};

template <typename T, typename construct_param_t, typename = void, typename... Args> 
struct is_constructable  : std::false_type {};
template <typename T, typename construct_param_t, typename... Args> 
struct is_constructable<T, construct_param_t, std::void_t<decltype(T{std::declval<Args>()..., construct_param_t{}})>, Args...> : std::true_type {};

template <typename T, typename construct_param_t, typename... Args>
constexpr bool is_constructable_v = is_constructable<T, construct_param_t, void, Args...>::value;

template <typename T, typename... Args>
inline constexpr std::size_t members_count_impl() {
	if constexpr (is_constructable_v<T, AnyType<T>, Args...>) {
		return members_count_impl<T, Args..., AnyType<T>>();
	} else {
		return sizeof...(Args);
	}
}

template <typename T>
inline constexpr std::size_t members_count_v = members_count_impl<T>();

// get members tuple
template <typename T>
concept AggregateType = std::is_aggregate_v<T>;

template <AggregateType T, size_t N>
struct get_member_references_tuple {
	inline static constexpr auto get_tuple() {
		if constexpr (N <= 0) {
			static_assert(N <= 0, "Too few structural member parameters (size <= 0)");
		} else {
			static_assert(N > 5, "Too many structural member parameters (size >= 5)");
		}
	}
};

#define GET_MEMBER_TUPLE_HELPER(n, ...) 					\
template <AggregateType T>   								\
struct get_member_references_tuple<T, n> { 					\
	inline static constexpr auto get_tuple() {  	   		\
			auto& [__VA_ARGS__] = get_global_value<T>();    \
			return std::tie(__VA_ARGS__);					\
		}   												\
};															

GET_MEMBER_TUPLE_HELPER(1, m1)
GET_MEMBER_TUPLE_HELPER(2, m1, m2)
GET_MEMBER_TUPLE_HELPER(3, m1, m2, m3)
GET_MEMBER_TUPLE_HELPER(4, m1, m2, m3, m4)
GET_MEMBER_TUPLE_HELPER(5, m1, m2, m3, m4, m5)

template  <AggregateType T>
inline constexpr auto struct_members_to_tuple() {
	return get_member_references_tuple<T, members_count_v<T>>::get_tuple();
}

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

int main() {
	// test get members count
	static_assert(members_count_v<Person> == 3);

	// test get members name
	constexpr auto tp_name = struct_members_to_tuple<Person>();
	[&]<size_t... Is>(std::index_sequence<Is...> seq) {
		((std::cout << get_member_name<&std::get<Is>(tp_name)>() << "\n"), ...);
	}(std::make_index_sequence<std::tuple_size_v<decltype(tp_name)>>{});

	std::cout << "\n\n\n";

	// test get members tgype
	constexpr auto tp_tuple = struct_members_to_tuple<Person>();
	[&]<size_t... Is>(std::index_sequence<Is...> seq) {
		((std::cout << get_member_type_name<std::remove_const_t<std::remove_reference_t<decltype(std::get<Is>(tp_tuple))>>>() << "\n"), ...);
	}(std::make_index_sequence<std::tuple_size_v<decltype(tp_tuple)>>{});


	return 0;
}

