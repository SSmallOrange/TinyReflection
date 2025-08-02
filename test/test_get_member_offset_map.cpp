#include "tinyrefl/reflection.hpp"

#include <format>

template <typename T, typename Tuple, size_t... Is>
auto func(std::index_sequence<Is...>) {
    using ValueType = decltype(tinyrefl::get_variant_type<T, Tuple, Is...>());
    return ValueType{};
}

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
    
    printf("\n------------\n");

    static auto member_offset_map = tinyrefl::struct_member_offset_map<Person>();
    auto member_name_arr = tinyrefl::struct_members_to_array<Person>();

    for (int i = 0; i < member_offset_map.size(); ++i) {
        auto it = member_offset_map.find(member_name_arr[i]);
        std::cout << std::format("{}: {}\n", i, member_name_arr[i]);
        if (it != member_offset_map.end()) {
            auto offset = it->second;
            std::visit([&](auto&& arg) {
                // 将it->first转为std::string输出，按照string_view输出会出现输出与key不一致的情况
                // 但是frozen::string不支持转换string, 正常string_view可以
                printf("%s offset: %d\n", std::string(it->first.data()).c_str(), arg.value);
            }, offset);
        }
    }

    std::variant<int, float, double> var{std::in_place_index<1>, 0.5};
    std::visit([&](auto&& value) {
        // 此时value类型推导为float
        std::cout << std::format("type name: {}\n", typeid(value).name());
    }, var);
    return 0;
}