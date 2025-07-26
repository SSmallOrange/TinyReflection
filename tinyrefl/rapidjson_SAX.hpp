#include "reflection_get_tuple.hpp"

#include "rapidjson/reader.h"

#include <iostream>

namespace tinyrefl {
    template <typename T, size_t... Is>
    struct ReaderHandlerImp : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, ReaderHandlerImp<T, Is...>> {
        using Tuple = decltype(struct_members_to_tuple<T>());
        using ValueType = decltype(get_variant_type<T, Tuple, Is...>());
        using MapType = frozen::unordered_map<frozen::string, ValueType, members_count_v<T>>;
    public:
        ReaderHandlerImp(const MapType& map_value, const T& value) : _struct_member_offset_map(map_value), _value(value) {}

    public:
        bool Null() { 
            if (_iterator != _struct_member_offset_map.end()) {
                // TODO
            }
            return true;
        }
        bool Bool(bool b) {
            if (_iterator != _struct_member_offset_map.end()) {
                auto offset = _iterator->second;
                std::visit([&](auto arg) {
                    using Value_Type = typename decltype(arg)::type;
                    // static_assert(remove_cvref_t<Value_Type> == bool, "type mismatch");
                    Value_Type& member_value = *reinterpret_cast<Value_Type*>(
                         reinterpret_cast<char*>(const_cast<T*>(&_value)) + arg.value
                    );
                    member_value = b;
                    printf("type name: [%s]\n", typeid(Value_Type).name());
                }, offset);
            }
            return true;
        }
        bool Int(int i) { return true; }
        bool Uint(unsigned u) { return true; }
        bool Int64(int64_t i) { return true; }
        bool Uint64(uint64_t u) { return true; }
        bool Double(double d) { return true; }
        bool String(const char* str, rapidjson::SizeType length, bool copy) { return true; }
        bool StartObject() { std::cout << "StartObject" << std::endl; return true; }
        bool Key(const char* str, rapidjson::SizeType length, bool copy) {
            _iterator = _struct_member_offset_map.find(frozen::string(str, length));
            std::cout << "Key: " << str << std::endl;
            return true;
        }
        bool EndObject(rapidjson::SizeType memberCount) { std::cout << "EndObject" << std::endl; return true; }
        bool StartArray() { return true; }
        bool EndArray(rapidjson::SizeType elementCount) { return true; }

    private:
        const MapType& _struct_member_offset_map;
        MapType::const_iterator _iterator;
        const T& _value;
    };

    template <typename T, typename IndexSeq>
    struct ReaderHandlerHelper;

    template <typename T, size_t... Is>
    struct ReaderHandlerHelper<T, std::index_sequence<Is...>> {
        using Type = ReaderHandlerImp<T, Is...>;
    };

    template <typename T>
    using ReaderHandleImpType = ReaderHandlerHelper<
            remove_cvref_t<T>,
            std::make_index_sequence<members_count_v<remove_cvref_t<T>>>
        >::Type;

    template <typename T>
    struct ReaderHandler : public ReaderHandleImpType<T> {
        using U = remove_cvref_t<T>;
        using Base = typename ReaderHandlerHelper<U, std::make_index_sequence<members_count_v<U>>>::Type;

        using MapType = typename Base::MapType;

        ReaderHandler(const MapType& map_value, const U& value)
            : Base(map_value, value) {}
    };

} // end ttinyrefl namespace
