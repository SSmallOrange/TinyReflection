#include "tinyrefl/reflection_tuple_foreach.hpp"
#include <iostream>

namespace tinyrefl {

template<typename Stream>
concept OutputStream = requires(Stream& s) {
    { s.append("abc") };
};

// declear
template <typename T, OutputStream Stream>
void reflection_to_json(T&& object, Stream &stream);

template <typename T>
concept KeyValue = requires(const T& t) {
    { t.data() };
    { t.size() };
};

template <OutputStream Stream, KeyValue Value>
void to_json_key(Stream&& s, Value&& value) {
    s.append("\"");
    s.append(value.data(), value.size() );
    s.append("\"");
}

template <OutputStream Stream, typename T>
void to_json_key(Stream&& s, T&& object) {
    to_json_key(s, reflection_get_name(object));
}

// to_json_value main template, recursion reslove custom type
template <OutputStream Stream, typename T>
void to_json_value(Stream&& s, T&& object) requires is_custom_type<T> {
    // std::cout << typeid(object).name() << std::endl;
    reflection_to_json(object, s);
}
// sequence to json
template <OutputStream Stream, typename T>
void to_json_value(Stream&& s, T&& object) requires is_sequence_container<T> {
     
}

// associative to json
template <OutputStream Stream, typename T>
void to_json_value(Stream&& s, T&& object) requires is_associative_container<T> {
     
}

// string to json
template <OutputStream Stream, typename T>
void to_json_value(Stream&& s, T&& object) requires is_string<T> {
    s.append("\"");
    s.append(object.data(), object.size() );
    s.append("\"");
}

// char to json
template <OutputStream Stream, typename T>
void to_json_value(Stream&& s, T&& object) requires is_char<T> {
     
}

// char* to json
template <OutputStream Stream, typename T>
void to_json_value(Stream&& s, T&& object) requires is_char_pointer<T> {
     
}

// char array to json
template <OutputStream Stream, typename T>
void to_json_value(Stream&& s, T&& object) requires is_char_array<T> {
     
}

// char int to json
template <OutputStream Stream, typename T>
void to_json_value(Stream&& s, T&& object) requires is_int<T> || is_int64<T> {
    s.append("\"");
    s.append(std::to_string(object));
    s.append("\"");
}

// char float to json
template <OutputStream Stream, typename T>
void to_json_value(Stream&& s, T&& object) requires is_float<T> {
     
}

// char double to json
template <OutputStream Stream, typename T>
void to_json_value(Stream&& s, T&& object) requires is_double<T> {
     
}

template <typename T, OutputStream Stream>
void reflection_to_json(T&& object, Stream &stream) {
    constexpr size_t member_count = members_count_v<remove_cvref_t<T>>;

    stream.append("{");
    for_each_member(std::forward<T>(object), [&](auto&& member_reference, 
        auto&& member_name, auto&& member_index) {
        to_json_key(stream, member_name);
        stream.append(":");
        // std::cout << typeid(member_reference).name() << std::endl;
        to_json_value(stream, member_reference);
        if (member_index < member_count - 1) {
            stream.append(",");
        }
    });
    stream.append("}");
}
    
}  // end namespace tinyrefl
