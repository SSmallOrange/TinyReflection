#include "tinyrefl/reflection_utils.hpp"

using namespace tinyrefl;

int main()
{
    static_assert(tinyrefl::is_string<std::string>);
    static_assert(!tinyrefl::is_string<const char*>);

    static_assert(!tinyrefl::is_char<std::string>);
    static_assert(tinyrefl::is_char<unsigned char>);
    static_assert(tinyrefl::is_char_pointer<const char*>);
    static_assert(tinyrefl::is_char_array<char[10]>);

    // 1. 测试 tinyrefl::is_int
    static_assert(tinyrefl::is_int<int>, "int should be integral");
    static_assert(tinyrefl::is_int<int32_t>, "int32_t should be integral");
    static_assert(tinyrefl::is_int<char>, "char should be integral");
    static_assert(tinyrefl::is_int<bool>, "bool should be integral");
    static_assert(tinyrefl::is_int<unsigned long long>, "unsigned long long should be integral");
    static_assert(!tinyrefl::is_int<float>, "float should not be integral");
    static_assert(!tinyrefl::is_int<double>, "double should not be integral");
    static_assert(!tinyrefl::is_int<void>, "void should not be integral");

    // 2. 测试 tinyrefl::is_int64
    // static_assert(tinyrefl::is_int64<int64_t>, "int64_t should match");
    // static_assert(tinyrefl::is_int64<uint64_t>, "uint64_t should match");
    static_assert(!tinyrefl::is_int64<int32_t>, "int32_t should not match");
    static_assert(!tinyrefl::is_int64<long>, "long might not match on all platforms");
    static_assert(!tinyrefl::is_int64<float>, "float should not match");
    static_assert(!tinyrefl::is_int64<void*>, "pointer should not match");

    // 3. 测试 tinyrefl::is_float
    static_assert(tinyrefl::is_float<float>, "float should match");
    static_assert(tinyrefl::is_float<const float>, "const float should match");
    static_assert(tinyrefl::is_float<volatile float>, "volatile float should match");
    // static_assert(!tinyrefl::is_float<double>, "double should not match");
    // static_assert(!tinyrefl::is_float<long double>, "long double should not match");
    static_assert(!tinyrefl::is_float<int>, "int should not match");
    static_assert(!tinyrefl::is_float<float&>, "float reference should not match");
    static_assert(!tinyrefl::is_float<float*>, "float pointer should not match");

    // 4. 测试 tinyrefl::is_double
    static_assert(tinyrefl::is_double<double>, "double should match");
    // static_assert(tinyrefl::is_double<const double>, "const double should match");
    // static_assert(tinyrefl::is_double<volatile double>, "volatile double should match");
    static_assert(!tinyrefl::is_double<float>, "float should not match");
    static_assert(!tinyrefl::is_double<long double>, "long double should not match");
    static_assert(!tinyrefl::is_double<int>, "int should not match");
    static_assert(!tinyrefl::is_double<double&>, "double reference should not match");
    static_assert(!tinyrefl::is_double<double*>, "double pointer should not match");

    // 5. 测试平台相关类型
    static_assert(tinyrefl::is_int<size_t>, "size_t should be integral");
    static_assert(tinyrefl::is_int<ptrdiff_t>, "ptrdiff_t should be integral");
    static_assert(!tinyrefl::is_float<size_t>, "size_t should not be float");

    // 6. 测试cv限定和引用
    static_assert(!tinyrefl::is_int<const volatile int&>, "reference should not match");
    static_assert(!tinyrefl::is_float<float&>, "float reference should not match");
    static_assert(!tinyrefl::is_double<const double&&>, "double rvalue reference should not match");

    struct Empty {};
    static_assert(!tinyrefl::is_int<Empty>, "custom type should not match");
    static_assert(!tinyrefl::is_float<Empty>, "custom type should not match");
    static_assert(!tinyrefl::is_double<void()>, "function type should not match");
    static_assert(!tinyrefl::is_int64<int[10]>, "array should not match");

    return 0;
}