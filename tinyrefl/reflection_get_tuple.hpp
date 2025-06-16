#pragma once

#include "reflection_utils.hpp"

namespace tinyrefl {  
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
inline constexpr std::size_t members_count_v = members_count_impl<remove_cvref_t<T>>();

// get members tuple
template <typename T>
concept AggregateType = std::is_aggregate_v<T>;

template <AggregateType T, size_t N>
struct get_member_references_tuple {
	// get tuple by type
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
		auto& [__VA_ARGS__] = get_global_value<T>();		\
		return std::tie(__VA_ARGS__);						\
	}   													\
};															

GET_MEMBER_TUPLE_HELPER(1, m1)
GET_MEMBER_TUPLE_HELPER(2, m1, m2)
GET_MEMBER_TUPLE_HELPER(3, m1, m2, m3)
GET_MEMBER_TUPLE_HELPER(4, m1, m2, m3, m4)
GET_MEMBER_TUPLE_HELPER(5, m1, m2, m3, m4, m5)

// get members tuple
template  <AggregateType T>
inline constexpr auto struct_members_to_tuple() {
	return get_member_references_tuple<T, members_count_v<T>>::get_tuple();
}

template <AggregateType T>
using member_array = std::array<std::string_view, members_count_v<remove_cvref_t<T>>>;

// get members array
template <AggregateType T>
inline consteval member_array<T> struct_members_to_array() {
	using type = remove_cvref_t<T>;
	constexpr auto tuple = struct_members_to_tuple<type>();
	return [&] <size_t... Is>(std::index_sequence<Is...>) {
        return member_array{get_member_name<&std::get<Is>(tuple)>()...};
		// return member_array{get_member_name_v<Is, tuple>()...};
    }(std::make_index_sequence<members_count_v<type>>());
}

// get members reference
template <size_t Index, size_t N, typename T>
struct get_member_reference_value {
	inline static auto get_reference_value(T&& t) {
		if constexpr (N <= 0) {
			static_assert(NULL <= 0, "Too few structural member parameters (size <= 0)");
		} else {
			static_assert(N > 5, "Too many structural member parameters (size >= 5)");
		}
	}
};

#define GET_MEMBER_REFERENCE_HELPER(n, ...) 					\
template <size_t Index, typename T>   							\
struct get_member_reference_value<Index, n, T> { 				\
	inline static decltype(auto) get_reference_value(T&& t) {   \
		auto&& [__VA_ARGS__] = std::forward<T>(t);				\
		return std::get<Index>(std::tie(__VA_ARGS__));			\
	}															\
};

GET_MEMBER_REFERENCE_HELPER(1, m1)
GET_MEMBER_REFERENCE_HELPER(2, m1, m2)
GET_MEMBER_REFERENCE_HELPER(3, m1, m2, m3)
GET_MEMBER_REFERENCE_HELPER(4, m1, m2, m3, m4)
GET_MEMBER_REFERENCE_HELPER(5, m1, m2, m3, m4, m5)

template <size_t Index, typename T>
inline decltype(auto) struct_member_reference(T&& t) {
	using U = remove_cvref_t<T>;
    constexpr size_t count = members_count_v<U>;
	static_assert(Index < count, "Index out of range");
	return get_member_reference_value<Index, count, T>::get_reference_value(std::forward<T>(t));
}
}
