#pragma once
#include "tinyrefl/rapidjson_SAX.hpp"

namespace tinyrefl {
    template <AggregateType T>
    inline void reflection_from_json(T&& object, const char* str) {
        DispatchHandler handler(object);
        rapidjson::StringStream ss(str);
        rapidjson::Reader reader;
        reader.Parse(ss, handler);
    }

}  // end namespace tinyrefl

