#pragma once
#include "utils/reflection_get_tuple.hpp"

#include "rapidjson/reader.h"

namespace tinyrefl {
    // declear reader
    template <typename T>
    requires is_custom_type_v<T>
    struct ReaderHandler;

    template <typename T>
    requires is_sequence_container_v<T>
    struct SequenceReaderHandler;

    template <typename T, size_t... Is>
    struct ReaderHandlerImp;

    template <typename T>
    class SequenceReaderHandleImp;

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
    requires is_custom_type_v<T>
    struct ReaderHandler : public ReaderHandleImpType<T> {
        using U = remove_cvref_t<T>;
        using Base = typename ReaderHandlerHelper<U, std::make_index_sequence<members_count_v<U>>>::Type;

        using MapType = typename Base::MapType;

        ReaderHandler(const MapType& map_value, U& value)
            : Base(map_value, value) {}
    };

    template <typename T>
    requires is_sequence_container_v<T>
    struct SequenceReaderHandler : public SequenceReaderHandleImp<T> {
        SequenceReaderHandler(T& value) : SequenceReaderHandleImp<T>(value) {}
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

        virtual void set_dispatcher(DispatchHandler* dispatcher) = 0;
        virtual ~IHandler() = default;
    };
    
    class DispatchHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, DispatchHandler> {
    public:
        template <AggregateType T>
        DispatchHandler(T& value) {
            static auto member_offset_map = tinyrefl::struct_member_offset_map<T>();
            this->push_handler(member_offset_map, value);
        }
        ~DispatchHandler() {
            while(!_stack.empty()) {
                delete _stack.back();
                _stack.pop_back();
            }
        }
    public:
        template <typename T>
        requires is_custom_type_v<T>
        void push_handler(const typename ReaderHandler<T>::MapType& map, T& value) {
            auto* h = new ReaderHandler<T>(map, value);
            h->set_dispatcher(this);
            _stack.emplace_back(h);
        }

        template <typename T>
        requires is_sequence_container_v<T>
        void push_handler(T& value) {
            auto* h = new SequenceReaderHandler<T>(value);
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
        bool EndArray(rapidjson::SizeType elementCount) {
/*            std::cout << "EndArray" << std::endl;*/
            bool result = top()->EndArray(elementCount);
            pop_handler();
            return result;
        }
        bool StartObject() {
            if (_is_first_member) {
                _is_first_member = false;
                return true;
            }
            return top()->StartObject();
        }
        bool EndObject(rapidjson::SizeType m) {
            if (_stack.size() == 0) return true;  // End Reflection
            bool result = top()->EndObject(m);
            pop_handler();
            return result;
        }
        
    private:
        IHandler* top() const { return _stack.back(); }

    private:
        std::vector<IHandler*> _stack;
        bool _is_first_member = true;
    };

    // rapidjson::BaseReaderHandler<rapidjson::UTF8<>, ReaderHandlerImp<T, Is...>>
    template <typename T, size_t... Is>
    struct ReaderHandlerImp : public IHandler {
        using Tuple = decltype(struct_members_to_tuple<T>());
        using ValueType = decltype(get_variant_type<T, Tuple, Is...>());
        using MapType = frozen::unordered_map<frozen::string, ValueType, members_count_v<T>>;
    public:
        ReaderHandlerImp(const MapType& map_value, T& value) : _struct_member_offset_map(map_value), _value(value) {}

    public:
        bool Null() { 
            if (_iterator != _struct_member_offset_map.end()) {
                // TODO
            }
            return true;
        }

        bool Bool(bool b) override {
            return assign_if_match<bool>([&](auto& member) { member = b; });
        }

        bool Int(int i) override {
            return assign_if_match<int>([&](auto& member) { member = i; });
        }

        bool Uint(unsigned u) override {
            return assign_if_match<unsigned>([&](auto& member) { member = u; });
        }

        bool Int64(int64_t i) override {
            return assign_if_match<int64_t>([&](auto& member) { member = i; });
        }

        bool Uint64(uint64_t u) override {
            return assign_if_match<uint64_t>([&](auto& member) { member = u; });
        }
        bool Double(double d) override {
            return assign_if_match<double>([&](auto& member) { member = d; });
        }
        bool RawNumber(const char* str, rapidjson::SizeType length, bool copy) override {
            return assign_if_match<const char*>([&](auto& member) { member = str; });
        }
        bool String(const char* str, rapidjson::SizeType length, bool copy) override {
            return assign_if_match<const char*>([&](auto& member) { 
                member = str;
            });
        }
        bool StartObject() override {
            if (_iterator != _struct_member_offset_map.end()) {
                auto offset = _iterator->second;
                std::visit([&](auto arg) {
                    using Value_Type = typename decltype(arg)::type;
                    if constexpr (is_custom_type_v<Value_Type>) {
                        static auto member_offset_map = struct_member_offset_map<Value_Type>();
                        Value_Type& member_value = *reinterpret_cast<Value_Type*>(
                            reinterpret_cast<char*>(static_cast<T*>(&_value)) + arg.value
                        );
                        _dispatch_handler->push_handler<Value_Type>(member_offset_map, member_value);
                    }
                }, offset);
            }
            return true;
        }
        bool Key(const char* str, rapidjson::SizeType length, bool copy) override {
            _iterator = _struct_member_offset_map.find(frozen::string(str, length));
            return true;
        }
        bool EndObject(rapidjson::SizeType memberCount) override { return true; }
        bool StartArray() override {
            if (_iterator != _struct_member_offset_map.end()) {
                auto offset = _iterator->second;
                std::visit([&](auto arg) {
                    using Value_Type = typename decltype(arg)::type;
                    if constexpr (is_sequence_container_v<Value_Type>) {
                        Value_Type& member_value = *reinterpret_cast<Value_Type*>(
                            reinterpret_cast<char*>(static_cast<T*>(&_value)) + arg.value
                        );
                        _dispatch_handler->push_handler<Value_Type>(member_value);
                    }
                }, offset);
            }
            return true;
        }
        bool EndArray(rapidjson::SizeType elementCount) override { return true; }

    private:
        template <typename TargetType, typename F>
        bool assign_if_match(F&& assign_func) {
            if (_iterator != _struct_member_offset_map.end()) {
                auto offset = _iterator->second;
                std::visit([&](auto arg) {
                    using Value_Type = typename decltype(arg)::type;
                    if constexpr (is_json_compatible_v<remove_cvref_t<Value_Type>, TargetType>) {
                        Value_Type& member_value = *reinterpret_cast<Value_Type*>(
                            reinterpret_cast<char*>(static_cast<T*>(&_value)) + arg.value
                        );
                        auto member_ptr = (Value_Type *)((char *)(&_value) + arg.value);
                        assign_func(*member_ptr);
                    }
                }, offset);
            }
            return true;
        }
    
    public:
        void set_dispatcher(DispatchHandler* dispatcher) override { _dispatch_handler = dispatcher; }

    private:
        const MapType& _struct_member_offset_map;
        MapType::const_iterator _iterator = nullptr;
        T& _value;
        DispatchHandler* _dispatch_handler = nullptr;
    };


    // SequenceReaderHandle
    template <typename T>
    class SequenceReaderHandleImp : public IHandler {
        using ElementType = sequence_element_type_t<remove_cvref_t<T>>;
    public:
        SequenceReaderHandleImp(T& value) : _value(value) {}
    public:
        bool Null() {
            return true;
        }

        bool Bool(bool b) override {
            return assign_if_match<bool>([&](auto& member) { member = b; });
        }

        bool Int(int i) override {
            return assign_if_match<int>([&](auto& member) { member = i; });
        }

        bool Uint(unsigned u) override {
            return assign_if_match<unsigned>([&](auto& member) { member = u; });
        }

        bool Int64(int64_t i) override {
            return assign_if_match<int64_t>([&](auto& member) { member = i; });
        }

        bool Uint64(uint64_t u) override {
            return assign_if_match<uint64_t>([&](auto& member) { member = u; });
        }
        bool Double(double d) override {
            return assign_if_match<double>([&](auto& member) { member = d; });
        }
        bool RawNumber(const char* str, rapidjson::SizeType length, bool copy) override {
            return assign_if_match<const char*>([&](auto& member) { member = str; });
        }
        bool String(const char* str, rapidjson::SizeType length, bool copy) override {
            return assign_if_match<const char*>([&](auto& member) { member = str; });
        }
        bool StartObject() override {
            if constexpr (is_custom_type_v<ElementType>) {
                static auto member_offset_map = tinyrefl::struct_member_offset_map<ElementType>();
                _dispatch_handler->push_handler<ElementType>(member_offset_map, _value.emplace_back());
            }
            return true;
        }
        bool Key(const char* str, rapidjson::SizeType length, bool copy) override {
            return true;
        }
        bool EndObject(rapidjson::SizeType memberCount) override { return true; }
        bool StartArray() override {
            if constexpr (is_sequence_container_v<ElementType>) {
                _dispatch_handler->push_handler<ElementType>(_value.emplace_back());
            }
            return true;
        }
        bool EndArray(rapidjson::SizeType elementCount) override { return true; }

    private:
        template <typename TargetType, typename F>
        bool assign_if_match(F&& assign_func) {
            if constexpr (is_json_compatible_v<remove_cvref_t<ElementType>, TargetType>) {
                assign_func(_value.emplace_back());
            }
            return true;
        }
    
    public:
        void set_dispatcher(DispatchHandler* dispatcher) override { _dispatch_handler = dispatcher; }

    private:
        T& _value;
        DispatchHandler* _dispatch_handler = nullptr;
    };
    
    // Deserialization Interface
    template <AggregateType T>
    inline void reflection_from_json(T&& object, const char* str) {
        DispatchHandler handler(object);
        rapidjson::StringStream ss(str);
        rapidjson::Reader reader;
        reader.Parse(ss, handler);
    }

} // end tinyrefl namespace
