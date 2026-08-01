#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/utils/reflection_utils.hpp"

using namespace tinyrefl;

TEST_CASE("type traits - is_string") {
    static_assert(tinyrefl::detail::is_string_v<std::string>);
    static_assert(!tinyrefl::detail::is_string_v<const char*>);
    static_assert(!tinyrefl::detail::is_string_v<char>);
    CHECK(true); // static_assert 通过即测试通过
}

TEST_CASE("type traits - is_char") {
    static_assert(!tinyrefl::detail::is_char_v<std::string>);
    static_assert(tinyrefl::detail::is_char_v<unsigned char>);
    static_assert(!tinyrefl::detail::is_char_v<const char*>);
    CHECK(true);
}

TEST_CASE("type traits - is_char_pointer") {
    static_assert(tinyrefl::detail::is_char_pointer_v<const char*>);
    static_assert(tinyrefl::detail::is_char_pointer_v<char*>);
    static_assert(!tinyrefl::detail::is_char_pointer_v<char>);
    CHECK(true);
}

TEST_CASE("type traits - is_char_array") {
    static_assert(tinyrefl::detail::is_char_array_v<char[10]>);
    static_assert(!tinyrefl::detail::is_char_array_v<char>);
    CHECK(true);
}

TEST_CASE("type traits - is_custom_type") {
    static_assert(!tinyrefl::detail::is_custom_type_v<const char*>);
    CHECK(true);
}

TEST_CASE("type traits - is_bool") {
    static_assert(tinyrefl::detail::is_bool_v<bool>);
    static_assert(!tinyrefl::detail::is_bool_v<int>);
    static_assert(!tinyrefl::detail::is_bool_v<int64_t>);
    static_assert(!tinyrefl::detail::is_bool_v<int&>);
    static_assert(!tinyrefl::detail::is_bool_v<int&&>);
    static_assert(!tinyrefl::detail::is_bool_v<double>);
    static_assert(!tinyrefl::detail::is_bool_v<char>);
    CHECK(true);
}

TEST_CASE("type traits - is_int") {
    static_assert(tinyrefl::detail::is_int_v<int>, "int should be integral");
    static_assert(tinyrefl::detail::is_int_v<int32_t>, "int32_t should be integral");
    static_assert(!tinyrefl::detail::is_int_v<char>, "char should not be integral");
    static_assert(!tinyrefl::detail::is_int_v<const char*>, "const char* should not be integral");
    static_assert(!tinyrefl::detail::is_int_v<bool>, "bool should not be integral");
    static_assert(tinyrefl::detail::is_int_v<unsigned long long>, "unsigned long long should be integral");
    static_assert(!tinyrefl::detail::is_int_v<float>, "float should not be integral");
    static_assert(!tinyrefl::detail::is_int_v<double>, "double should not be integral");
    static_assert(!tinyrefl::detail::is_int_v<void>, "void should not be integral");
    CHECK(true);
}

TEST_CASE("type traits - is_int64") {
    static_assert(!tinyrefl::detail::is_int64_v<int32_t>, "int32_t should not match");
    static_assert(!tinyrefl::detail::is_int64_v<long>, "long might not match on all platforms");
    static_assert(!tinyrefl::detail::is_int64_v<float>, "float should not match");
    static_assert(!tinyrefl::detail::is_int64_v<void*>, "pointer should not match");
    CHECK(true);
}

TEST_CASE("type traits - is_floating") {
    static_assert(tinyrefl::detail::is_floating_v<float>, "float should match");
    static_assert(tinyrefl::detail::is_floating_v<const float>, "const float should match");
    static_assert(tinyrefl::detail::is_floating_v<volatile float>, "volatile float should match");
    static_assert(!tinyrefl::detail::is_floating_v<int>, "int should not match");
    static_assert(tinyrefl::detail::is_floating_v<float&>, "float reference should match");
    static_assert(!tinyrefl::detail::is_floating_v<float*>, "float pointer should not match");
    CHECK(true);
}

TEST_CASE("type traits - platform types") {
    static_assert(tinyrefl::detail::is_int_v<size_t>, "size_t should be integral");
    static_assert(tinyrefl::detail::is_int_v<std::ptrdiff_t>, "ptrdiff_t should be integral");
    static_assert(!tinyrefl::detail::is_floating_v<size_t>, "size_t should not be float");
    CHECK(true);
}

TEST_CASE("type traits - cv qualifiers and references") {
    static_assert(tinyrefl::detail::is_int_v<const volatile int&>, "cv int& should match");
    static_assert(tinyrefl::detail::is_floating_v<float&>, "float& should match");
    CHECK(true);
}

TEST_CASE("type traits - custom types should not match primitives") {
    struct Empty {};
    static_assert(!tinyrefl::detail::is_int_v<Empty>, "custom type should not match int");
    static_assert(!tinyrefl::detail::is_floating_v<Empty>, "custom type should not match float");
    static_assert(!tinyrefl::detail::is_int64_v<int[10]>, "array should not match int64");
    CHECK(true);
}
