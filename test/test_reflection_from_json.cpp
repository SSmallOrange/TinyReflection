#include "rapidjson/reader.h"
#include <variant>
#include <iostream>
#include <vector>

#include "tinyrefl/rapidjson_SAX.hpp"

using namespace rapidjson;
using namespace std;

struct Person {
    bool m_bool;
};

int main() {
    const char json[] = " { \"m_bool\" : false }";

    Person person;
    person.m_bool = true;
    static auto member_offset_map = tinyrefl::struct_member_offset_map<Person>();
    
    tinyrefl::ReaderHandler<Person> handle(member_offset_map, person);

    Reader reader;
    StringStream ss(json);
    reader.Parse(ss, handle);
    std::cout << (person.m_bool ? "true" : "false") << endl;
    return 0;
}
