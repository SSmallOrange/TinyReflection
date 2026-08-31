#include "utils/reflection_tuple_foreach.hpp"

#include <charconv>
#include <cmath>
#include <cstdio>

namespace tinyrefl {

template <detail::AggregateType T, detail::OutputStream Stream>
inline void reflection_to_json(T&& object, Stream& stream);
}

namespace tinyrefl::detail {

// declear
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_custom_type_v<T>;

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_sequence_container_v<T>;

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_associative_container_v<T>;

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_string_v<T>;

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_char_v<T>;

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_bool_v<T>;

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_char_pointer_v<T>;

template <OutputStream Stream, typename T>
  requires(is_int_v<T> || is_int64_v<T> || is_floating_v<T>)
inline void to_json_value(Stream&& s, T&& object);

// implement
template <typename T>
concept KeyValue = requires(const T& t) {
  { t.data() };
  { t.size() };
};

// JSON single-char escape (shared by key, string, char, char*)
template <OutputStream Stream>
inline void escape_json_char(Stream&& s, char ch) {
  switch (ch) {
    case '"':
      s.append("\\\"", 2);
      break;
    case '\\':
      s.append("\\\\", 2);
      break;
    case '\b':
      s.append("\\b", 2);
      break;
    case '\f':
      s.append("\\f", 2);
      break;
    case '\n':
      s.append("\\n", 2);
      break;
    case '\r':
      s.append("\\r", 2);
      break;
    case '\t':
      s.append("\\t", 2);
      break;
    default:
      if (static_cast<unsigned char>(ch) < 0x20) {
        char buffer[7];
        ::std::snprintf(buffer, sizeof(buffer), "\\u%04X",
                        static_cast<unsigned char>(ch));
        s.append(buffer, 6);
      } else {
        s.append(&ch, 1);
      }
      break;
  }
}

template <OutputStream Stream, KeyValue Value>
inline void to_json_key(Stream&& s, Value&& value) {
  s.append("\"", 1);
  const char* data = value.data();
  const ::std::size_t len = value.size();
  for (::std::size_t i = 0; i < len; ++i) {
    escape_json_char(s, data[i]);
  }
  s.append("\"", 1);
}

// to_json_value main template, recursion reslove custom type
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_custom_type_v<T>
{
  ::tinyrefl::reflection_to_json(object, s);
}
// sequence to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_sequence_container_v<T>
{
  s.append("[");
  for_each_by_iterator(s, object.cbegin(), object.cend(), ",",
                       [&](const auto& member) { to_json_value(s, member); });
  s.append("]");
}

// associative to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_associative_container_v<T>
{
  s.append("{");
  for_each_by_iterator(
      s, object.cbegin(), object.cend(), ",",
      [&](const auto& pair_value) {  // ::std::pair
        if constexpr (is_string_v<decltype(pair_value.first)>) {
          to_json_key(s, pair_value.first);
          s.append(":");
          to_json_value(s, pair_value.second);
        } else {
          static_assert(is_string_v<decltype(pair_value.first)>,
                        "Only string keys are supported in JSON");
        }
      });
  s.append("}");
}

// string to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_string_v<T>
{
  s.append("\"", 1);
  for (::std::size_t i = 0; i < object.size(); ++i) {
    escape_json_char(s, object[i]);
  }
  s.append("\"", 1);
}

// char to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_char_v<T>
{
  s.append("\"", 1);
  escape_json_char(s, static_cast<char>(object));
  s.append("\"", 1);
}

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_bool_v<T>
{
  s.append(object ? "true" : "false");
}

// char* to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object)
  requires is_char_pointer_v<T>
{
  const char* str = object;

  if (str == nullptr) {
    s.append("null", 4);
    return;
  }
  s.append("\"", 1);
  while (*str) {
    escape_json_char(s, *str);
    ++str;
  }
  s.append("\"", 1);
}

// number to json
template <OutputStream Stream, typename T>
  requires(is_int_v<T> || is_int64_v<T> || is_floating_v<T>)
inline void to_json_value(Stream&& s, T&& object) {
  if constexpr (is_floating_v<T>) {
    if (::std::isinf(object) || ::std::isnan(object)) {
      s.append("null", 4);
    } else {
      auto normalized = (object == 0) ? static_cast<remove_cvref_t<T>>(0) : object;
      char buffer[32];
      auto [ptr, ec] =
          ::std::to_chars(buffer, buffer + sizeof(buffer), normalized);
      s.append(buffer, static_cast<::std::size_t>(ptr - buffer));
    }
  } else {
    s.append(::std::to_string(object));
  }
}

}  // end namespace tinyrefl::detail

namespace tinyrefl {
template <detail::AggregateType T, detail::OutputStream Stream>
inline void reflection_to_json(T&& object, Stream& stream) {
  constexpr size_t serializable_count =
      detail::serializable_members_count_v<detail::remove_cvref_t<T>>;

  stream.append("{");
  detail::for_each_serializable_member(
      ::std::forward<T>(object),
      [&](auto&& member_reference, auto&& member_name, auto&& member_index) {
        detail::to_json_key(stream, member_name);
        stream.append(":");
        detail::to_json_value(stream, member_reference);
        if (member_index < serializable_count - 1) {
          stream.append(",");
        }
      });
  stream.append("}");
}

}  // end namespace tinyrefl
