#include "tinyrefl/reflection_to_json.hpp"
#include "tinyrefl/reflection.hpp"

struct Male {
    int m_male;
};

struct People {
    std::string m_name;
    int m_age;
    Male m_male;
}; 

int main() {

    std::string strJson;
    People p{"Mail", 22, {2}};
    tinyrefl::reflection_to_json(p, strJson);
    std::cout << strJson << std::endl;
    return 0;
}