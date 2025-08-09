#include "utils/reflection_tuple_foreach.hpp"

namespace tinyrefl {

// declear
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_custom_type_v<T>;

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_sequence_container_v<T>;

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_associative_container_v<T>;

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_string_v<T>;

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_char_v<T>;

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_bool_v<T>;

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_char_pointer_v<T>;

template <OutputStream Stream, typename T>
requires (is_int_v<T> || is_int64_v<T> || is_float_v<T> || is_double_v<T>)
inline void to_json_value(Stream&& s, T&& object);

template <AggregateType T, OutputStream Stream>
inline void reflection_to_json(T&& object, Stream &stream);

// implement
template <typename T>
concept KeyValue = requires(const T& t) {
    { t.data() };
    { t.size() };
};

template <OutputStream Stream, KeyValue Value>
inline void to_json_key(Stream&& s, Value&& value) {
    s.append("\"");
    s.append(value.data(), value.size() );
    s.append("\"");
}

// to_json_value main template, recursion reslove custom type
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_custom_type_v<T> {
    reflection_to_json(object, s);
}
// sequence to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_sequence_container_v<T> {
    s.append("[");
    for_each_by_iterator(s, object.cbegin(), object.cend(), ",", [&](const auto& member) {
        to_json_value(s, member);
    });
    s.append("]");
}

// associative to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_associative_container_v<T> {
    s.append("{");
    for_each_by_iterator(s, object.cbegin(), object.cend(), ",", [&](const auto& pair_value) {  // std::pair
        if constexpr (is_string_v<decltype(pair_value.first)>) {
            to_json_key(s, pair_value.first);
            s.append(":");
            to_json_value(s, pair_value.second);
        } else {
            static_assert(is_string_v<decltype(pair_value.first)>, "Only string keys are supported in JSON");
        }
    });
    s.append("}");
}

// string to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_string_v<T> {
    s.append("\"");
    s.append(object.data(), object.size());
    s.append("\"");
}

// char to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_char_v<T> {
    s.append("\"");
    s.push_back(object);
    s.append("\"");
}

template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_bool_v<T> {
    s.append(object ? "true" : "false");
}

// char* to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_char_pointer_v<T> 
{
    const char* str = object;
    
    if (str == nullptr) {
        s.append("null", 4);
        return;
    }
    s.append("\"", 1);
    while (*str) {
        switch (*str) {
            case '"':  s.append("\\\"", 2); break;
            case '\\': s.append("\\\\", 2); break;
            case '\b': s.append("\\b", 2);  break;
            case '\f': s.append("\\f", 2);  break;
            case '\n': s.append("\\n", 2);  break;
            case '\r': s.append("\\r", 2);  break;
            case '\t': s.append("\\t", 2);  break;
            default:
                if (static_cast<unsigned char>(*str) < 0x20) {
                    char buffer[7];
                    std::snprintf(buffer, sizeof(buffer), "\\u%04X", static_cast<unsigned char>(*str));
                    s.append(buffer, 6);
                } else {
                    s.append(str, 1);
                }
                break;
        }
        ++str;
    }
    s.append("\"", 1);
}

// int to json
template <OutputStream Stream, typename T>
requires (is_int_v<T> || is_int64_v<T> || is_float_v<T> || is_double_v<T>)
inline void to_json_value(Stream&& s, T&& object) {
    s.append(std::to_string(object));
}

template <AggregateType T, OutputStream Stream>
inline void reflection_to_json(T&& object, Stream &stream) {
    constexpr size_t member_count = members_count_v<remove_cvref_t<T>>;

    stream.append("{");
    for_each_member(std::forward<T>(object), [&](auto&& member_reference, 
        auto&& member_name, auto&& member_index) {
        to_json_key(stream, member_name);
        stream.append(":");
        to_json_value(stream, member_reference);
        if (member_index < member_count - 1) {
            stream.append(",");
        }
    });
    stream.append("}");
}
    
}  // end namespace tinyrefl
