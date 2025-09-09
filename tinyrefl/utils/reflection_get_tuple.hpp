#pragma once

#include <variant>

#include "tinyrefl/thirdparty/frozen/string.h"
#include "reflection_utils.hpp"
#include "tinyrefl/thirdparty/frozen/unordered_map.h"

namespace tinyrefl::detail {

template <typename T>
struct Wrapper {
	inline static remove_cvref_t<T> value;
};

struct Any {
    constexpr Any(int) {}

    template <typename T>
    requires(::std::is_copy_constructible_v<T>)
    operator T&();

    template <typename T>
    requires(::std::is_move_constructible_v<T> && !::std::is_copy_constructible_v<T>)
    operator T&&();

    struct Empty{};

    template <typename T>
    requires(!::std::is_copy_constructible_v<T> && !::std::is_move_constructible_v<T> && !::std::is_constructible_v<T, Empty>)
    operator T();
}; 

template<typename T, ::std::size_t N>
constexpr auto test() {
    return []<::std::size_t... I>(::std::index_sequence<I...>) {
        return requires{ T{ Any(I)... }; };
    }(::std::make_index_sequence<N>{});
}

template<typename T, int N = 0>
constexpr auto member_count() {
    if constexpr (test<T, N>() && !test<T, N + 1>()) {
        return N;
    } else {
        return member_count<T, N + 1>();
    }
}

template<typename T, ::std::size_t N1, ::std::size_t N2, ::std::size_t N3>
constexpr bool test_three_parts() {
    return []<::std::size_t... I1, ::std::size_t... I2, ::std::size_t... I3>(::std::index_sequence<I1...>,
                                                                       ::std::index_sequence<I2...>,
                                                                       ::std::index_sequence<I3...>) {
        return requires{ T{ Any(I1)..., { Any(I2)... }, Any(I3)... }; };
    }(::std::make_index_sequence<N1>{}, ::std::make_index_sequence<N2>{}, ::std::make_index_sequence<N3>{});
}

// try insert N Ant{} to pos
template<typename T, ::std::size_t position, ::std::size_t N>
constexpr bool try_place_n_in_pos() {
    constexpr auto Total = member_count<T>();
    if constexpr (N == 0) {
        return true;
    } else if constexpr (position + N <= Total) {
        return test_three_parts<T, position, N, Total - position - N>();
    } else {
        return false;
    }
}

// try pos max count
template <typename T, ::std::size_t pos, ::std::size_t N = 0, ::std::size_t Max = 10>
constexpr bool has_extra_elements() {
    constexpr auto Total = member_count<T>();
    if constexpr (test_three_parts<T, pos, N, Total - pos - 1>()) {
        return false;
    } else if constexpr (N + 1 <= Max) {
        return has_extra_elements<T, pos, N + 1>();
    } else {
        return true;
    }
}

template<typename T, ::std::size_t pos, ::std::size_t N = 0>
constexpr auto search_max_in_pos() {
    constexpr auto Total = member_count<T>();
    if constexpr (!has_extra_elements<T, pos>()) {
        return 1;
    } else {
        ::std::size_t result = 0;
        [&]<::std::size_t... Is>(::std::index_sequence<Is...>) {
            ((try_place_n_in_pos<T, pos, Is>() ? result = Is : 0), ...);
        }(::std::make_index_sequence<Total + 1>());
        return result;
    }
}

template<typename T, ::std::size_t N = 0>
constexpr auto search_all_extra_index(auto&& array) {
    constexpr auto total = member_count<T>();
    constexpr auto num = search_max_in_pos<T, N>();
    constexpr auto value = num > 1 ? num : 1;
    array[N] = value;
    if constexpr (N + value < total) {
        search_all_extra_index<T, N + value>(array);
    }
}

template<typename T>
constexpr auto true_member_count() {
    constexpr auto Total = member_count<T>();
    if constexpr (Total == 0) {
        return 0;
    } else {
        ::std::array<::std::size_t, Total> indices = { 1 };
        search_all_extra_index<T>(indices);
        ::std::size_t result = Total;
        ::std::size_t index = 0;
        while (index < Total) {
            auto n = indices[index];
            result -= n - 1;
            index += n;
        }
        return result;
    }
}

template <typename T>
inline constexpr ::std::size_t members_count_v = true_member_count<remove_cvref_t<T>>();

// get members tuple
template <typename T>
concept AggregateType = ::std::is_aggregate_v<remove_cvref_t<T>>;

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
		auto& [__VA_ARGS__] = Wrapper<T>::value;			    \
		return ::std::tie(__VA_ARGS__);							\
	}   														\
	inline static decltype(auto) get_reference_value(T&& t) {   \
		auto&& [__VA_ARGS__] = ::std::forward<T>(t);				\
		return ::std::tie(__VA_ARGS__);							\
	}															\
};															

#include "reflection_get_member_tuple_helper.hpp"

template <AggregateType T>
using member_array = ::std::array<::std::string_view, members_count_v<remove_cvref_t<T>>>;

// get members tuple
template  <AggregateType T>
inline constexpr auto struct_members_to_tuple() {
	return get_member_references_tuple<T, members_count_v<T>>::get_tuple();
}

// get members array
template <AggregateType T>
inline consteval member_array<T> struct_members_to_array() {
	using U = remove_cvref_t<T>;
	constexpr auto tuple = struct_members_to_tuple<U>();
	return [&] <size_t... Is>(::std::index_sequence<Is...>) {
        return member_array<T>{get_member_name<&::std::get<Is>(tuple)>()...};
		// return member_array{get_member_name_v<Is, tuple>()...};
    }(::std::make_index_sequence<members_count_v<U>>());
}

// get members reference
template <size_t Index, typename T>
inline decltype(auto) struct_member_reference(T&& t) {
	using U = remove_cvref_t<T>;
    constexpr size_t count = members_count_v<U>;
	static_assert(Index < count, "Index out of range");
	return ::std::get<Index>(get_member_references_tuple<T, count>::get_reference_value(::std::forward<T>(t)));
}

// get members offset array
template <AggregateType T>
inline const auto& struct_member_offset_array() {
    using offset_v = ::std::size_t;
    using member_offset_array_t = ::std::array<offset_v, members_count_v<T>>;

    constexpr ::std::size_t members_count = members_count_v<T>;
    constexpr auto members_tuple = struct_members_to_tuple<T>();
    static member_offset_array_t offset_array = {[&]<size_t... Is>(::std::index_sequence<Is...>) mutable -> member_offset_array_t {
        member_offset_array_t arr;
        T dummy{};
        auto tie = get_member_references_tuple<T, members_count>::get_reference_value(::std::move(dummy));
        ((arr[Is] = offset_v(reinterpret_cast<uint8_t*>(&::std::get<Is>(tie)) - reinterpret_cast<uint8_t*>(&dummy))), ...);
        return arr;
    }(::std::make_index_sequence<members_count>{})};

    return offset_array;
}

// get variant type
template <typename T, typename Tuple, size_t... Is>
inline auto get_variant_type() {
    return ::std::variant<offset_of_member<
        remove_cvref_t<::std::tuple_element_t<Is, Tuple>>
    >...>{};
}

template <size_t Is, typename Tuple>
inline auto remove_tuple_cv_type() {
    return remove_cvref_t<::std::tuple_element_t<Is, Tuple>>{};
}

// get unordered_map of [ member_name, [member_type, member_offset] ]
template <typename T, size_t... Is>
inline auto get_variant_map(::std::index_sequence<Is...>) {
    using U = remove_cvref_t<T>;
    constexpr auto member_name_arr = struct_members_to_array<U>();
    auto& member_offset_arr = struct_member_offset_array<U>();
    using Tuple = decltype(struct_members_to_tuple<U>());
    // using Remove_Tuple_CV_Type = decltype(remove_cvref_t<::std::tuple_element_t<Is, Tuple>>);
    using ValueType = decltype(get_variant_type<U, Tuple, Is...>());
    // runtime plan
    // ::std::unordered_map<::std::string_view, ValueType> map;
    // (map.emplace(member_name_arr[Is],
    //     ValueType{::std::in_place_index<Is>, 
    //         offset_of_member<decltype(remove_tuple_cv_type<Is, Tuple>())>{member_offset_arr[Is]}}),
    // ...);
    // return map;
    return frozen::unordered_map<frozen::string, ValueType, sizeof...(Is)>{
        {member_name_arr[Is],
            ValueType{::std::in_place_index<Is>, 
                offset_of_member<decltype(remove_tuple_cv_type<Is, Tuple>())>{member_offset_arr[Is]}}
        }...
    };
}

template <typename T>
inline auto struct_member_offset_map() {
    return get_variant_map<T>(::std::make_index_sequence<members_count_v<T>>{});
}

}
