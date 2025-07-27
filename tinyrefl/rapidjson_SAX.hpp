#include "reflection_get_tuple.hpp"

#include "rapidjson/reader.h"

#include <iostream>

namespace tinyrefl {
    // declear reader
    template <typename T>
    struct ReaderHandler;

    template <typename T, size_t... Is>
    struct ReaderHandlerImp;

    class DispatchHandler;
    // Can convert assignment
    template <typename Target, typename From>
    constexpr bool is_json_compatible_v = std::is_assignable_v<Target&, From>;

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

    struct IHandler {
        virtual bool Null() = 0;
        virtual bool Bool(bool) = 0;
        virtual bool Int(int) = 0;
        virtual bool Uint(unsigned) = 0;
        virtual bool Int64(int64_t) = 0;
        virtual bool Uint64(uint64_t) = 0;
        virtual bool Double(double) = 0;
        virtual bool RawNumber(const char* str, rapidjson::SizeType length, bool copy) = 0;
        virtual bool String(const char*, rapidjson::SizeType, bool) = 0;
        virtual bool StartObject() = 0;
        virtual bool Key(const char*, rapidjson::SizeType, bool) = 0;
        virtual bool EndObject(rapidjson::SizeType) = 0;
        virtual bool StartArray() = 0;
        virtual bool EndArray(rapidjson::SizeType) = 0;

        virtual ~IHandler() = default;
    };

    class DispatchHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, DispatchHandler> {
    public:
        template <typename T>
        void push_handler(const typename ReaderHandler<T>::MapType& map, const T& value) {
            auto* h = new ReaderHandler<T>(map, value);
            h->set_dispatcher(this);
            _stack.emplace_back(h);
        }

        void pop_handler() {
            delete _stack.back();
            _stack.pop_back();
        }
    public:
        bool Bool(bool b)        { return top()->Bool(b); }
        bool Int(int i)          { return top()->Int(i); }
        bool Key(const char* s, rapidjson::SizeType l, bool c) { return top()->Key(s, l, c); }
        bool Null() { return top()->Null(); }
        bool Uint(unsigned u) { return top()->Uint(u); }

        bool Int64(int64_t i) { return top()->Int64(i); }

        bool Uint64(uint64_t u) { return top()->Uint64(u); }
        bool Double(double d) { return top()->Double(d); }
        bool RawNumber(const char* str, rapidjson::SizeType length, bool copy) { return top()->RawNumber(str, length, copy); }
        bool String(const char* str, rapidjson::SizeType length, bool copy) { return top()->String(str, length, copy); }
        bool StartArray() { return top()->StartArray(); }
        bool EndArray(rapidjson::SizeType elementCount) { return top()->EndArray(elementCount); }
        bool StartObject() { return top()->StartObject(); }
        bool EndObject(rapidjson::SizeType m) {
            bool result = top()->EndObject(m);
            pop_handler();
            return result;
        }
        
    private:
        IHandler* top() const { return _stack.back(); }

    private:
        std::vector<IHandler*> _stack;
    };

    // rapidjson::BaseReaderHandler<rapidjson::UTF8<>, ReaderHandlerImp<T, Is...>>
    template <typename T, size_t... Is>
    struct ReaderHandlerImp : public IHandler {
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

        bool Bool(bool b) override {
            assign_if_match<bool>([&](auto& member) {
                member = b;
            });
            return true;
        }

        bool Int(int i) override {
            assign_if_match<int>([&](auto& member) {
                member = i;
            });
            return true;
        }

        bool Uint(unsigned u) override {
            assign_if_match<unsigned>([&](auto& member) {
                member = u;
            });
            return true;
        }

        bool Int64(int64_t i) override {
            assign_if_match<int64_t>([&](auto& member) {
                member = i;
            });
            return true;
        }

        bool Uint64(uint64_t u) override {
            assign_if_match<uint64_t>([&](auto& member) {
                member = u;
            });
            return true;
        }
        bool Double(double d) override {
            assign_if_match<double>([&](auto& member) {
                member = d;
            });
            return true;
        }
        bool RawNumber(const char* str, rapidjson::SizeType length, bool copy) override {
            assign_if_match<const char*>([&](auto& member) {
                member = str;
            });
            return true;
        }
        bool String(const char* str, rapidjson::SizeType length, bool copy) override {
            assign_if_match<const char*>([&](auto& member) {
                member = str;
            });
            return true;
        }
        bool StartObject() override {
            if (!_is_first_member) {
                if (_iterator != _struct_member_offset_map.end()) {
                    auto offset = _iterator->second;
                    std::visit([&](auto arg) {
                        using Value_Type = typename decltype(arg)::type;
                        if constexpr (is_custom_type_v<Value_Type>) {
                            static auto member_offset_map = struct_member_offset_map<Value_Type>();
                            Value_Type& member_value = *reinterpret_cast<Value_Type*>(
                                reinterpret_cast<char*>(const_cast<T*>(&_value)) + arg.value
                            );
                            _dispatch_handler->push_handler<Value_Type>(member_offset_map, member_value);
                        }
                    }, offset);
                }
            }
            _is_first_member = false;
            return true;
        }
        bool Key(const char* str, rapidjson::SizeType length, bool copy) override {
            _iterator = _struct_member_offset_map.find(frozen::string(str, length));
            std::cout << "Key: " << str << std::endl;
            return true;
        }
        bool EndObject(rapidjson::SizeType memberCount) override { std::cout << "EndObject" << std::endl; return true; }
        bool StartArray() override { return true; }
        bool EndArray(rapidjson::SizeType elementCount) override { return true; }

    private:
        template <typename TargetType, typename F>
        void assign_if_match(F&& assign_func) {
            if (_iterator != _struct_member_offset_map.end()) {
                auto offset = _iterator->second;
                std::visit([&](auto arg) {
                    using Value_Type = typename decltype(arg)::type;
                    if constexpr (is_json_compatible_v<remove_cvref_t<Value_Type>, TargetType>) {
                        Value_Type& member_value = *reinterpret_cast<Value_Type*>(
                            reinterpret_cast<char*>(const_cast<T*>(&_value)) + arg.value
                        );
                        assign_func(member_value);
                    }
                }, offset);
            }
        }
    
    public:
        void set_dispatcher(class DispatchHandler* dispatcher) { _dispatch_handler = dispatcher; }

    private:
        const MapType& _struct_member_offset_map;
        MapType::const_iterator _iterator = nullptr;
        const T& _value;
        bool _is_first_member = true;
        DispatchHandler* _dispatch_handler = nullptr;
    };

} // end ttinyrefl namespace 
