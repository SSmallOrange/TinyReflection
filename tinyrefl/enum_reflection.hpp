#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>

namespace tinyrefl {

/// Scan range for enum reflection; specialize to widen for a specific enum.
template <typename E>
struct enum_range {
  static constexpr int min = -128;
  static constexpr int max = 128;
};

/// JSON serialization mode for enum values.
enum class enum_serialize_mode {
  as_string,
  as_integer,
};

/// Per-enum serialization mode override; specialize to switch to as_integer.
template <typename E>
struct enum_serialize_policy {
  static constexpr enum_serialize_mode mode = enum_serialize_mode::as_string;
};

}  // namespace tinyrefl

namespace tinyrefl::detail {

// Extract the enumerator name of NTTP E from the compiler's function
// signature. Invalid values produce a cast expression like "(Color)42".
template <auto E>
inline constexpr ::std::string_view get_enum_pretty_name() {
#if defined(__clang__)
  // Clang: "... [E = Color::Red]"
  constexpr ::std::string_view func_name = __PRETTY_FUNCTION__;
  constexpr ::std::size_t marker = func_name.find("E = ");
  if constexpr (marker == ::std::string_view::npos) {
    return {};
  } else {
    constexpr ::std::size_t begin = marker + 4;
    constexpr ::std::size_t end = func_name.rfind(']');
    if constexpr (end == ::std::string_view::npos || end <= begin) {
      return {};
    } else {
      return func_name.substr(begin, end - begin);
    }
  }
#elif defined(__GNUC__)
  // GCC: "... [with auto E = Color::Red; std::string_view = ...]"
  constexpr ::std::string_view func_name = __PRETTY_FUNCTION__;
  constexpr ::std::size_t marker = func_name.find("E = ");
  if constexpr (marker == ::std::string_view::npos) {
    return {};
  } else {
    constexpr ::std::size_t begin = marker + 4;
    constexpr ::std::size_t semi = func_name.find(';', begin);
    constexpr ::std::size_t end =
        (semi == ::std::string_view::npos) ? func_name.rfind(']') : semi;
    if constexpr (end == ::std::string_view::npos || end <= begin) {
      return {};
    } else {
      return func_name.substr(begin, end - begin);
    }
  }
#elif defined(_MSC_VER)
  // MSVC: "... get_enum_pretty_name<Color::Red>(void)"
  constexpr ::std::string_view func_name = __FUNCSIG__;
  constexpr ::std::size_t begin = func_name.find("get_enum_pretty_name<");
  if constexpr (begin == ::std::string_view::npos) {
    return {};
  } else {
    constexpr ::std::size_t start = begin + 21;
    constexpr ::std::size_t end = func_name.rfind(">(");
    if constexpr (end == ::std::string_view::npos || end <= start) {
      return {};
    } else {
      return func_name.substr(start, end - start);
    }
  }
#else
  return {};
#endif
}

/// A valid enumerator name contains no parentheses (rules out "(Color)42").
inline constexpr bool is_pretty_name_valid(::std::string_view name) {
  if (name.empty()) {
    return false;
  }
  for (char c : name) {
    if (c == '(' || c == ')') {
      return false;
    }
  }
  return true;
}

/// Strip the leading scope qualifier from a pretty name (e.g. "Color::Red" -> "Red").
inline constexpr ::std::string_view strip_enum_scope(::std::string_view name) {
  const ::std::size_t pos = name.rfind("::");
  if (pos == ::std::string_view::npos) {
    return name;
  }
  return name.substr(pos + 2);
}

/// Compile-time check: does integer V map to a named enumerator of E?
template <typename E, auto V>
inline constexpr bool is_valid_enum_value() {
  constexpr auto name = get_enum_pretty_name<static_cast<E>(
      static_cast<::std::underlying_type_t<E>>(V))>();
  return is_pretty_name_valid(name);
}

template <typename E>
struct enum_entry {
  E value{};
  ::std::string_view name{};
};

/// enum_range<E> clamped to what underlying_type_t<E> can actually hold.
template <typename E>
struct effective_enum_range {
 private:
  using U = ::std::underlying_type_t<E>;
  static constexpr long long u_min =
      static_cast<long long>(::std::numeric_limits<U>::min());
  static constexpr long long u_max =
      static_cast<long long>(::std::numeric_limits<U>::max());
  static constexpr long long r_min =
      static_cast<long long>(::tinyrefl::enum_range<E>::min);
  static constexpr long long r_max =
      static_cast<long long>(::tinyrefl::enum_range<E>::max);

 public:
  static constexpr int min =
      static_cast<int>(r_min < u_min ? u_min : r_min);
  static constexpr int max =
      static_cast<int>((r_max - 1) > u_max ? (u_max + 1) : r_max);
};

template <typename E, int Min, int Max>
inline constexpr ::std::size_t count_valid_enum_values() {
  ::std::size_t n = 0;
  [&]<::std::size_t... Is>(::std::index_sequence<Is...>) {
    ((is_valid_enum_value<E, static_cast<int>(Is) + Min>() ? ++n : n), ...);
  }(::std::make_index_sequence<static_cast<::std::size_t>(Max - Min)>{});
  return n;
}

/// Scan the effective range and collect all named enumerators of E.
template <typename E>
inline constexpr auto collect_enum_entries() {
  constexpr int min_v = effective_enum_range<E>::min;
  constexpr int max_v = effective_enum_range<E>::max;
  static_assert(max_v > min_v, "enum_range: max must be greater than min");

  constexpr ::std::size_t count = count_valid_enum_values<E, min_v, max_v>();
  ::std::array<enum_entry<E>, count> entries{};

  ::std::size_t idx = 0;
  [&]<::std::size_t... Is>(::std::index_sequence<Is...>) {
    (([&] {
       constexpr int raw = static_cast<int>(Is) + min_v;
       if constexpr (is_valid_enum_value<E, raw>()) {
         constexpr auto full_name = get_enum_pretty_name<static_cast<E>(
             static_cast<::std::underlying_type_t<E>>(raw))>();
         constexpr auto short_name = strip_enum_scope(full_name);
         entries[idx++] = enum_entry<E>{
             static_cast<E>(static_cast<::std::underlying_type_t<E>>(raw)),
             short_name};
       }
     }()),
     ...);
  }(::std::make_index_sequence<static_cast<::std::size_t>(max_v - min_v)>{});

  return entries;
}

/// Cached entries per enum type.
template <typename E>
inline constexpr auto enum_entries_v = collect_enum_entries<E>();

}  // namespace tinyrefl::detail

namespace tinyrefl {

/// Number of named enumerators discovered for E.
template <typename E>
  requires ::std::is_enum_v<E>
inline constexpr ::std::size_t enum_count() {
  return detail::enum_entries_v<E>.size();
}

/// All discovered {value, name} pairs of E.
template <typename E>
  requires ::std::is_enum_v<E>
inline constexpr const auto& enum_entries() {
  return detail::enum_entries_v<E>;
}

/// Enum value -> name; empty string_view if unnamed (e.g. bit-flag combinations).
template <typename E>
  requires ::std::is_enum_v<E>
inline constexpr ::std::string_view enum_to_string(E value) {
  for (const auto& entry : detail::enum_entries_v<E>) {
    if (entry.value == value) {
      return entry.name;
    }
  }
  return {};
}

/// Name -> enum value (case-sensitive); nullopt if the name is unknown.
template <typename E>
  requires ::std::is_enum_v<E>
inline constexpr ::std::optional<E> enum_from_string(::std::string_view name) {
  for (const auto& entry : detail::enum_entries_v<E>) {
    if (entry.name == name) {
      return entry.value;
    }
  }
  return ::std::nullopt;
}

/// Enum value -> underlying integer.
template <typename E>
  requires ::std::is_enum_v<E>
inline constexpr auto enum_to_underlying(E value) {
  return static_cast<::std::underlying_type_t<E>>(value);
}

/// Integer -> enum value with validation; nullopt if no matching enumerator.
template <typename E, typename Int>
  requires ::std::is_enum_v<E> && ::std::is_integral_v<Int>
inline constexpr ::std::optional<E> enum_cast(Int value) {
  using U = ::std::underlying_type_t<E>;
  for (const auto& entry : detail::enum_entries_v<E>) {
    if (static_cast<U>(entry.value) == static_cast<U>(value)) {
      return entry.value;
    }
  }
  return ::std::nullopt;
}

}  // namespace tinyrefl
