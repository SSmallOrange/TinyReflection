#include "tinyrefl/reflection_to_json.hpp"
#include "tinyrefl/reflection.hpp"

struct Male {
    int m_male;
    float m_float;
    double m_double;
};

struct People {
    std::string m_name;
    int m_age;
    Male m_male;
    char m_sex;
    const char* m_charrr;
}; 

int main() {

    static_assert(tinyrefl::is_int_v<const int>);
    static_assert(tinyrefl::is_array_v<int[]>);

    std::string strJson;
    People p{"Mail", 22, {23, 1.1f, 3.456}, 'm', "Hello World"};

    tinyrefl::reflection_to_json(p, strJson);
    std::cout << strJson << std::endl;


    return 0;
}