#pragma once

#include <type_traits>
#include <tuple>

#include <map>
#include <deque>
#include <list>
#include <queue>
#include <string>
#include <vector>
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
inline constexpr bool is_sequence_container =
    is_template_instant_of<std::deque, T>::value  ||
    is_template_instant_of<std::list, T>::value   ||
    is_template_instant_of<std::vector, T>::value ||
    is_template_instant_of<std::queue, T>::value;

// Check is associative container
template <typename T>
inline constexpr bool is_associative_container = 
    is_template_instant_of<std::map, T>::value || 
    is_template_instant_of<std::unordered_map, T>::value;

// Check is string 
template <typename T>
inline constexpr bool is_string = is_template_instant_of<std::basic_string, T>::value;

// Check is char
template <typename T>
inline constexpr bool is_char = std::is_same_v<remove_cvref_t<T>, char> || 
                         std::is_same_v<remove_cvref_t<T>, signed char> || 
                         std::is_same_v<remove_cvref_t<T>, unsigned char>;

// Check is char*
template <typename T>
inline constexpr bool is_char_pointer = is_char<std::remove_pointer_t<T>>;

// Check is char array
template <typename T>
inline constexpr bool is_char_array = std::is_array_v<T> && is_char<std::remove_extent_t<T>>;

// Check is int
template <typename T>
inline constexpr bool is_int = std::is_integral_v<T>;

// Check is int64_t
template <typename T>
inline constexpr bool is_int64 = std::is_same_v<T, int64_t> && std::is_same_v<uint64_t, T>;

// Check is float
template <typename T>
inline constexpr bool is_float = std::is_floating_point_v<T>;

// Check is double
template <typename T>
inline constexpr bool is_double = std::is_same_v<double, T> && std::is_same_v<T, double>;

}
