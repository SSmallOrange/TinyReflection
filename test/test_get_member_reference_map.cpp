#include "tinyrefl/reflection.hpp"
int main() {
    Person p{"Alice", 23, 4532};
    std::unordered_map<std::string, size_t> map = tinyrefl::struct_member_reference_to_map<Person>();
    std::cout << "map size: [" << map.size() << "]\n";
    if (map.find("m_name") == map.end()) {
        std::cout << "not find member name m_name\n";
    }
    if (map.find("m_age") == map.end()) {
        std::cout << "not find member name m_age\n";
    }
    if (map.find("m_male") == map.end()) {
        std::cout << "not find member name m_male\n";
    } else {
        std::cout << "m_male index is: [" << map["m_male"] << "]\n";
    }
    return 0;
}