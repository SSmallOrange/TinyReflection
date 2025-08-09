#include "tinyrefl/reflection_to_json.hpp"
#include "tinyrefl/utils/reflection.hpp"

struct BasicTypes {
    int m_int;
    float m_float;
    double m_double;
    char m_char;
    const char* m_cstr = nullptr;
    std::string m_str;
    bool m_bool;
};

struct SequenceContainers {
    std::vector<int> m_vec;
    std::list<std::string> m_list;
    std::deque<double> m_deque;
    
    std::vector<char> m_queue_like;
};

struct MapContainers {
    std::map<std::string, int> m_map_str_int;
    std::unordered_map<std::string, std::string> m_umap_str_str;
};

struct NestedStruct {
    BasicTypes m_basic;
    SequenceContainers m_seq;
    MapContainers m_maps;
};

struct DeepNest {
    NestedStruct m_nested1;
    NestedStruct m_nested2;
};

struct PointerStruct {
    int* m_ptr;
    const char* m_cptr;
};

struct QueueTest {
    std::queue<double> m_queue;
};


int main() {
    std::string output;

    BasicTypes baseObj {123, 4.5f, 6.78, 'Z', "hello", "world", false};    

    NestedStruct obj {
        {123, 4.5f, 6.78, 'Z', "hello", "world", true},
        {{1, 2, 3}, {"one", "two"}, {1.11, 2.22}, {'a', 'b'}},
        {{{"apple", 5}, {"banana", 10}}, {{"k1", "v1"}, {"k2", "v2"}}}
    };

    DeepNest deep {
        obj,
        {
            {999, 3.14f, 2.72, 'Y', "ptr", "deep", true},
            {{}, {}, {}, {}},
            {{}, {}}
        }
    };
    std::cout << "--- BaseStruct ---" << std::endl;
    tinyrefl::reflection_to_json(baseObj, output);
    std::cout << output << "\n\n";

    output.clear();
    std::cout << "--- NestedStruct ---" << std::endl;
    tinyrefl::reflection_to_json(obj, output);
    std::cout << output << "\n\n";

    output.clear();
    std::cout << "--- DeepNest ---" << std::endl;
    tinyrefl::reflection_to_json(deep, output);
    std::cout << output << "\n\n";

    output.clear();
    std::cout << "--- Empty Maps and Containers ---" << std::endl;
    NestedStruct empty;
    empty.m_basic.m_cstr = nullptr;
    tinyrefl::reflection_to_json(empty, output);
    std::cout << output << std::endl;
    
    return 0;
}
