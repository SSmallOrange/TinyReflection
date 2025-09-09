#include "tinyrefl/utils/reflection_utils.hpp"

using namespace tinyrefl;

int main()
{
    static_assert(tinyrefl::detail::is_string_v<std::string>);
    static_assert(!tinyrefl::detail::is_string_v<const char*>);

    static_assert(!tinyrefl::detail::is_char_v<std::string>);
    static_assert(tinyrefl::detail::is_char_v<unsigned char>);
    static_assert(!tinyrefl::detail::is_char_v<const char*>);
    static_assert(tinyrefl::detail::is_char_pointer_v<const char*>);
    static_assert(tinyrefl::detail::is_char_pointer_v<char*>);
    static_assert(!tinyrefl::detail::is_char_pointer_v<char>);
    static_assert(tinyrefl::detail::is_char_array_v<char[10]>);
    static_assert(!tinyrefl::detail::is_char_array_v<char>);

    static_assert(!tinyrefl::detail::is_custom_type_v<const char*>);

    // Test Is Bool
    static_assert(tinyrefl::detail::is_bool_v<bool>);
    static_assert(!tinyrefl::detail::is_bool_v<int>);
    static_assert(!tinyrefl::detail::is_bool_v<int64_t>);
    static_assert(!tinyrefl::detail::is_bool_v<int&>);
    static_assert(!tinyrefl::detail::is_bool_v<int&&>);
    static_assert(!tinyrefl::detail::is_bool_v<double>);
    static_assert(!tinyrefl::detail::is_bool_v<char>);
    
    // 测试 tinyrefl::detail::is_string
    static_assert(tinyrefl::detail::is_string_v<std::string>);
    static_assert(!tinyrefl::detail::is_string_v<const char*>);
    static_assert(!tinyrefl::detail::is_string_v<char>);

    // 1. 测试 tinyrefl::detail::is_int
    static_assert(tinyrefl::detail::is_int_v<int>, "int should be integral");
    static_assert(tinyrefl::detail::is_int_v<int32_t>, "int32_t should be integral");
    static_assert(!tinyrefl::detail::is_int_v<char>, "char should be integral");
    static_assert(!tinyrefl::detail::is_int_v<const char*>, "char should be integral");
    static_assert(!tinyrefl::detail::is_int_v<bool>, "bool should be integral");
    static_assert(tinyrefl::detail::is_int_v<unsigned long long>, "unsigned long long should be integral");
    static_assert(!tinyrefl::detail::is_int_v<float>, "float should not be integral");
    static_assert(!tinyrefl::detail::is_int_v<double>, "double should not be integral");
    static_assert(!tinyrefl::detail::is_int_v<void>, "void should not be integral");

    // 2. 测试 tinyrefl::detail::is_int64
    // static_assert(tinyrefl::detail::is_int64<int64_t>, "int64_t should match");
    // static_assert(tinyrefl::detail::is_int64<uint64_t>, "uint64_t should match");
    static_assert(!tinyrefl::detail::is_int64_v<int32_t>, "int32_t should not match");
    static_assert(!tinyrefl::detail::is_int64_v<long>, "long might not match on all platforms");
    static_assert(!tinyrefl::detail::is_int64_v<float>, "float should not match");
    static_assert(!tinyrefl::detail::is_int64_v<void*>, "pointer should not match");

    // 3. 测试 tinyrefl::detail::is_float
    static_assert(tinyrefl::detail::is_float_v<float>, "float should match");
    static_assert(tinyrefl::detail::is_float_v<const float>, "const float should match");
    static_assert(tinyrefl::detail::is_float_v<volatile float>, "volatile float should match");
    // static_assert(!tinyrefl::detail::is_float<double>, "double should not match");
    // static_assert(!tinyrefl::detail::is_float<long double>, "long double should not match");
    static_assert(!tinyrefl::detail::is_float_v<int>, "int should not match");
    static_assert(tinyrefl::detail::is_float_v<float&>, "float reference should not match");
    static_assert(!tinyrefl::detail::is_float_v<float*>, "float pointer should not match");

    // 4. 测试 tinyrefl::detail::is_double
    static_assert(tinyrefl::detail::is_double_v<double>, "double should match");
    // static_assert(tinyrefl::detail::is_double<const double>, "const double should match");
    // static_assert(tinyrefl::detail::is_double<volatile double>, "volatile double should match");
    static_assert(!tinyrefl::detail::is_double_v<float>, "float should not match");
    static_assert(!tinyrefl::detail::is_double_v<long double>, "long double should not match");
    static_assert(!tinyrefl::detail::is_double_v<int>, "int should not match");
    static_assert(tinyrefl::detail::is_double_v<double&>, "double reference should not match");
    static_assert(!tinyrefl::detail::is_double_v<double*>, "double pointer should not match");

    // 5. 测试平台相关类型
    static_assert(tinyrefl::detail::is_int_v<size_t>, "size_t should be integral");
    static_assert(tinyrefl::detail::is_int_v<std::ptrdiff_t>, "ptrdiff_t should be integral");
    static_assert(!tinyrefl::detail::is_float_v<size_t>, "size_t should not be float");

    // 6. 测试cv限定和引用
    static_assert(tinyrefl::detail::is_int_v<const volatile int&>, "reference should not match");
    static_assert(tinyrefl::detail::is_float_v<float&>, "float reference should not match");
    static_assert(tinyrefl::detail::is_double_v<const double&&>, "double rvalue reference should not match");

    struct Empty {};
    static_assert(!tinyrefl::detail::is_int_v<Empty>, "custom type should not match");
    static_assert(!tinyrefl::detail::is_float_v<Empty>, "custom type should not match");
    static_assert(!tinyrefl::detail::is_double_v<void()>, "function type should not match");
    static_assert(!tinyrefl::detail::is_int64_v<int[10]>, "array should not match");

    return 0;
}