#pragma once

#include "reflection_utils.hpp"

namespace tinyrefl {  
// get members tuple
template <typename T>
concept AggregateType = std::is_aggregate_v<remove_cvref_t<T>>;

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

	inline static auto get_reference_value(T&& t) {
		if constexpr (N <= 0) {
			static_assert(NULL <= 0, "Too few structural member parameters (size <= 0)");
		} else {
			static_assert(N > 5, "Too many structural member parameters (size >= 5)");
		}
	}
};

#define GET_MEMBER_TUPLE_HELPER(n, ...) 						\
template <AggregateType T>   									\
struct get_member_references_tuple<T, n> { 						\
	inline static constexpr auto get_tuple() {  	   			\
		auto& [__VA_ARGS__] = get_global_value<T>();			\
		return std::tie(__VA_ARGS__);							\
	}   														\
	inline static decltype(auto) get_reference_value(T&& t) {   \
		auto&& [__VA_ARGS__] = std::forward<T>(t);				\
		return std::tie(__VA_ARGS__);							\
	}															\
};															

#include "reflection_get_member_tuple_helper.hpp"

template <AggregateType T>
using member_array = std::array<std::string_view, members_count_v<remove_cvref_t<T>>>;

// get static members reference tuple
template  <AggregateType T>
inline constexpr auto static_struct_members_to_tuple() {
	return get_member_references_tuple<T, members_count_v<T>>::get_tuple();
}

// get members array
template <AggregateType T>
inline consteval member_array<T> struct_members_to_array() {
	using type = remove_cvref_t<T>;
	constexpr auto tuple = static_struct_members_to_tuple<type>();
	return [&] <size_t... Is>(std::index_sequence<Is...>) {
        return member_array<T>{get_member_name<&std::get<Is>(tuple)>()...};
		// return member_array{get_member_name_v<Is, tuple>()...};
    }(std::make_index_sequence<members_count_v<type>>());
}

// get member reference by Index
template <size_t Index, typename T>
inline decltype(auto) struct_member_reference_by_index(T&& t) {
	using U = remove_cvref_t<T>;
    constexpr size_t count = members_count_v<U>;
	static_assert(Index < count, "Index out of range");
	return std::get<Index>(get_member_references_tuple<T, count>::get_reference_value(std::forward<T>(t)));
}

// get member hash map reference
template <AggregateType T>
inline std::unordered_map<std::string, size_t> struct_member_reference_to_map() {
    constexpr member_array<T> member_name_array = struct_members_to_array<T>();

    std::unordered_map<std::string, size_t> result;
    [&]<size_t... Is>(std::index_sequence<Is...>) -> void {
        (result.emplace(member_name_array[Is], Is), ...);  // fold expression，unordered_map not support Initialize list because pair template
    }(std::make_index_sequence<members_count_v<T>>{});
    
    return result;
}
}
