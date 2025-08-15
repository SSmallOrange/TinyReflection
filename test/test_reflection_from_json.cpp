#include "tinyrefl/reflection_to_json.hpp"
#include "tinyrefl/reflection_from_json.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

using namespace rapidjson;
using namespace std;

struct Inner {
    int id;
    string label;
};

struct Config {
    bool flag;
    double ratio;
    vector<int> values;
    Inner inner;
    vector<Inner> inner_list;
};

struct Complex {
    string name;
    Config config;
    vector<vector<int>> matrix;
    vector<vector<Inner>> inner_matrix;
};

struct Object_Vector {
    int m_int;
    Inner config;
    vector<int> values;
    vector<Inner> inner_list;
    std::vector<int> vecInt;
    std::vector<std::vector<int>> vecInner;
};

void test() {
    Complex obj;

    const char* json = R"(
        {
            "name": "TestComplex",
            "config": {
                "flag": true,
                "ratio": 3.1415,
                "values": [10, 20, 30],
                "inner": {
                "id": 42,
                "label": "InnerLabel"
                },
                "inner_list": [
                { "id": 1, "label": "A" },
                { "id": 2, "label": "B" },
                { "id": 3, "label": "C" }
                ]
            },
            "matrix": [
                [1, 2, 3],
                [4, 5, 6]
            ],
            "inner_matrix": [
                [
                { "id": 101, "label": "X" },
                { "id": 102, "label": "Y" }
                ],
                [
                { "id": 201, "label": "Z" },
                { "id": 202, "label": "W" }
                ]
            ]
        }
    )";

    if (tinyrefl::Status status = tinyrefl::reflection_from_json(obj, json); !status) {
        std:: cout << status.error.message << std::endl;
        std:: cout << status.error.line << std::endl;
        std:: cout << status.error.column << std::endl;
        std:: cout << status.error.offset << std::endl;
    } else {
        std::cout << "Parse Success!\n" << std::endl;
    }
    
    std::string out;
    tinyrefl::reflection_to_json(obj, out);
    std::cout << "after:\n" << out << std::endl;
}

int main() {

    test();
    return 0;
}
