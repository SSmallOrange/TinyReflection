#pragma once

#include <tuple>
#include <array>
#include <map>
#include <deque>
#include <list>
#include <queue>
#include <string>
#include <vector>
#include <type_traits>
#include <string_view>
#include <unordered_map>

namespace tinyrefl {  
// remove const volitale and reference
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

// is template instant 
template <template <typename...> typename U, typename T>
struct is_template_instant_of : std::false_type {};

template <template <typename...> typename U, typename... args>
struct is_template_instant_of<U, U<args...>> : std::true_type {};

// Check is sequecnce container
template <typename T>
inline constexpr bool is_sequence_container_v =
    is_template_instant_of<std::deque, remove_cvref_t<T>>::value  ||
    is_template_instant_of<std::list, remove_cvref_t<T>>::value   ||
    is_template_instant_of<std::vector, remove_cvref_t<T>>::value;

// Check is associative container
template <typename T>
inline constexpr bool is_associative_container_v = 
    is_template_instant_of<std::map, remove_cvref_t<T>>::value || 
    is_template_instant_of<std::unordered_map, remove_cvref_t<T>>::value;

// Check is string 
template <typename T>
inline constexpr bool is_string_v = is_template_instant_of<std::basic_string, remove_cvref_t<T>>::value;

// Check is array
template <typename T>
inline constexpr bool is_array_v = std::is_array_v<remove_cvref_t<T>>;

// Check is char
template <typename T>
inline constexpr bool is_char_v = std::is_same_v<remove_cvref_t<T>, char> || 
                        std::is_same_v<remove_cvref_t<T>, signed char> || 
                        std::is_same_v<remove_cvref_t<T>, unsigned char>;

// Check is char*
template <typename T>

inline constexpr bool is_char_pointer_v = is_char_v<std::remove_pointer_t<std::remove_cvref_t<T>>> &&
                        std::is_pointer_v<std::remove_cvref_t<T>>;

// Check is char array
template <typename T>
inline constexpr bool is_char_array_v = std::is_array_v<remove_cvref_t<T>> && is_char_v<std::remove_extent_t<T>>;

// Check is bool
template <typename T>
inline constexpr bool is_bool_v = std::is_same_v<remove_cvref_t<T>, bool> && std::is_same_v<bool, remove_cvref_t<T>>;

// Check is int
template <typename T>
inline constexpr bool is_int_v = std::is_integral_v<remove_cvref_t<T>> && 
                        !is_char_v<T> &&
                        !is_char_pointer_v<T> &&
                        !is_char_array_v<T> &&
                        !is_bool_v<T>;

// Check is int64_t
template <typename T>
inline constexpr bool is_int64_v = std::is_same_v<remove_cvref_t<T>, int64_t> && std::is_same_v<uint64_t, remove_cvref_t<T>>;

// Check is float
template <typename T>
inline constexpr bool is_float_v = std::is_floating_point_v<remove_cvref_t<T>>;

// Check is double
template <typename T>
inline constexpr bool is_double_v = std::is_same_v<double, remove_cvref_t<T>> && std::is_same_v<remove_cvref_t<T>, double>;

// Other
template <typename T>
inline constexpr bool is_custom_type_v = !is_sequence_container_v<remove_cvref_t<T>> &&
                                     !is_associative_container_v<remove_cvref_t<T>> &&
                                     !is_string_v<remove_cvref_t<T>> &&
                                     !is_char_v<remove_cvref_t<T>> &&
                                     !is_char_pointer_v<remove_cvref_t<T>> &&
                                     !is_array_v<remove_cvref_t<T>> &&
                                     !is_int_v<remove_cvref_t<T>> &&
                                     !is_int64_v<remove_cvref_t<T>> &&
                                     !is_float_v<remove_cvref_t<T>> &&
                                     !is_bool_v<remove_cvref_t<T>>;


// get members name strings
template <auto val>
inline constexpr std::string_view get_member_name() {
	std::string_view funcName = __FUNCSIG__;
	size_t begin = funcName.rfind("->") + 2;
	size_t end = funcName.rfind(">(");
	return funcName.substr(begin, end - begin);
}

template <auto index, auto tuple>
inline constexpr std::string_view get_member_name_v = get_member_name<&std::get<index>(tuple)>();

// get members type strings
template <typename T>
inline consteval std::string_view get_member_type_name() {
	std::string_view funcName = __FUNCSIG__;
	size_t start = funcName.find("get_member_type_name<");
	size_t end = funcName.find("(void)");
	return funcName.substr(start + 21, end - start - 22);
}

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

struct Any {
    constexpr Any(int) {}

    template <typename T>
    requires(std::is_copy_constructible_v<T>)
    operator T&();

    template <typename T>
    requires(std::is_move_constructible_v<T> && !std::is_copy_constructible_v<T>)
    operator T&&();

    struct Empty{};

    template <typename T>
    requires(!std::is_copy_constructible_v<T> && !std::is_move_constructible_v<T> && !std::is_constructible_v<T, Empty>)
    operator T();
}; 

template<typename T, std::size_t N>
constexpr auto test() {
    return []<std::size_t... I>(std::index_sequence<I...>) {
        return requires{ T{ Any(I)... }; };
    }(std::make_index_sequence<N>{});
}

template<typename T, int N = 0>
constexpr auto member_count() {
    if constexpr (test<T, N>() && !test<T, N + 1>()) {
        return N;
    } else {
        return member_count<T, N + 1>();
    }
}

template<typename T, std::size_t N1, std::size_t N2, std::size_t N3>
constexpr bool test_three_parts() {
    return []<std::size_t... I1, std::size_t... I2, std::size_t... I3>(std::index_sequence<I1...>,
                                                                       std::index_sequence<I2...>,
                                                                       std::index_sequence<I3...>) {
        return requires{ T{ Any(I1)..., { Any(I2)... }, Any(I3)... }; };
    }(std::make_index_sequence<N1>{}, std::make_index_sequence<N2>{}, std::make_index_sequence<N3>{});
}

// try insert N Ant{} to pos
template<typename T, std::size_t position, std::size_t N>
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
template <typename T, std::size_t pos, std::size_t N = 0, std::size_t Max = 10>
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

template<typename T, std::size_t pos, std::size_t N = 0>
constexpr auto search_max_in_pos() {
    constexpr auto Total = member_count<T>();
    if constexpr (!has_extra_elements<T, pos>()) {
        return 1;
    } else {
        std::size_t result = 0;
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((try_place_n_in_pos<T, pos, Is>() ? result = Is : 0), ...);
        }(std::make_index_sequence<Total + 1>());
        return result;
    }
}

template<typename T, std::size_t N = 0>
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
        std::array<std::size_t, Total> indices = { 1 };
        search_all_extra_index<T>(indices);
        std::size_t result = Total;
        std::size_t index = 0;
        while (index < Total) {
            auto n = indices[index];
            result -= n - 1;
            index += n;
        }
        return result;
    }
}

template <typename T>
inline constexpr std::size_t members_count_v = true_member_count<remove_cvref_t<T>>();

// hash member struct
using member_hash_t = std::size_t;

// hash func
inline size_t const_hash(std::string_view s) {
    constexpr size_t basis = 14695981039346656037ull;
    constexpr size_t prime = 1099511628211ull;

    size_t hash = basis;
    for (char c : s) {
        hash ^= static_cast<size_t>(c);
        hash *= prime;
    }
    return hash;
}

}
