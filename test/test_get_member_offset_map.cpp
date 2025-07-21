#include "tinyrefl/reflection.hpp"

#include <format>
int main()
{
    auto& arr = tinyrefl::struct_member_offset_array<Person>();

    for (int i = 0; i < arr.size(); ++i) {
        std::cout << std::format("{}: {}\n", i, arr[i]);
    }

    std::cout << std::format("size: {}\n", sizeof std::string);
    std::cout << std::format("size: {}\n", sizeof Person);

    
    std::cout << std::format("size: {}\n", sizeof tinyrefl::Wrapper<Person>::value);
    printf("address: %p\n", (char*)&tinyrefl::Wrapper<Person>::value);

    printf("m_name address: %p\n", (char*)&tinyrefl::Wrapper<Person>::value.m_name);
    printf("m_age address: %p\n", (char*)&tinyrefl::Wrapper<Person>::value.m_age);
    printf("m_male address: %p\n", (char*)&tinyrefl::Wrapper<Person>::value.m_male);

    return 0;
}