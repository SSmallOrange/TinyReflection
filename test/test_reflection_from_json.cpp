#include "rapidjson/reader.h"
#include <variant>
#include <iostream>
#include <vector>

#include "tinyrefl/rapidjson_SAX.hpp"

using namespace rapidjson;
using namespace std;

struct Address {
    string m_street;
    string m_city;
};

struct Person {
    bool m_bool;
    int m_int;
    Address m_address;
};

int main() {
    const char json[] = " { \"m_bool\" : false, \"m_int\" : 1, \"m_address\" : { \"m_street\" : \"Street\", \"m_city\" : \"City\" }";

    Person person;
    person.m_bool = true;
    person.m_int = 2;
    std::cout << "before: " << (person.m_bool ? "true" : "false") << endl;
    std::cout << "before: " << person.m_int << endl;
    static auto member_offset_map = tinyrefl::struct_member_offset_map<Person>();
    
    // tinyrefl::ReaderHandler<Person> handle(member_offset_map, person);
    tinyrefl::DispatchHandler handle;
    handle.push_handler(member_offset_map, person);

    Reader reader;
    StringStream ss(json);
    reader.Parse(ss, handle);
    std::cout << "parse after: " << (person.m_bool ? "true" : "false") << endl;
    std::cout << "parse after: " << person.m_int << endl;
    std::cout << "parse after: " << person.m_address.m_street << endl;
    std::cout << "parse after: " << person.m_address.m_city << endl;
    return 0;
}
