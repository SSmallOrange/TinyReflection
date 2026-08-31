#pragma once
#include "thirdparty/rapidjson/error/en.h"
#include "thirdparty/rapidjson/reader.h"
#include "utils/reflection_get_tuple.hpp"

namespace tinyrefl::detail {
// declear reader
template <typename T>
  requires is_custom_type_v<T>
struct ReaderHandler;

template <typename T>
  requires is_sequence_container_v<T>
struct SequenceReaderHandler;

template <typename T>
  requires is_associative_container_v<T>
struct AssociativeReaderHandler;

template <typename T, typename IndexSeq>
struct ReaderHandlerImp;

template <typename T>
class SequenceReaderHandleImp;

class DispatchHandler;

// Can convert assignment (require both constructible and assignable to avoid
// invalid static_cast, e.g. static_cast<std::string>(bool) is ill-formed even
// though std::string::operator=(char) makes is_assignable_v true)
template <typename Target, typename From>
constexpr bool is_json_compatible_v =
    ::std::is_constructible_v<Target, From> &&
    ::std::is_assignable_v<Target&, From>;

template <typename T, typename IndexSeq>
struct ReaderHandlerHelper;

template <typename T, size_t... Is>
struct ReaderHandlerHelper<T, ::std::index_sequence<Is...>> {
  using Type = ReaderHandlerImp<T, ::std::index_sequence<Is...>>;
};

template <typename T>
using ReaderHandleImpType =
    ReaderHandlerHelper<remove_cvref_t<T>,
                        serializable_indices_t<remove_cvref_t<T>>>::Type;

template <typename T>
  requires is_custom_type_v<T>
struct ReaderHandler : public ReaderHandleImpType<T> {
  using U = remove_cvref_t<T>;
  using Base = ReaderHandleImpType<U>;

  using MapType = typename Base::MapType;

  ReaderHandler(const MapType& map_value, U& value) : Base(map_value, value) {}
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
  virtual bool RawNumber(const char* str, ::rapidjson::SizeType length,
                         bool copy) = 0;
  virtual bool String(const char*, ::rapidjson::SizeType, bool) = 0;
  virtual bool StartObject() = 0;
  virtual bool Key(const char*, ::rapidjson::SizeType, bool) = 0;
  virtual bool EndObject(::rapidjson::SizeType) = 0;
  virtual bool StartArray() = 0;
  virtual bool EndArray(::rapidjson::SizeType) = 0;

  virtual void set_dispatcher(DispatchHandler* dispatcher) = 0;

  // Optional lifecycle hooks for testing/debugging.
  // Set these before parsing to count ctor/dtor calls; leave null in
  // production.
  inline static void (*on_construct)() = nullptr;
  inline static void (*on_destruct)() = nullptr;

  IHandler() {
    if (on_construct) on_construct();
  }
  virtual ~IHandler() {
    if (on_destruct) on_destruct();
  }
};

// SkipHandler: 吸收未知字段子对象/数组中的所有 SAX 事件
class SkipHandler : public IHandler {
 public:
  bool Null() override { return true; }
  bool Bool(bool) override { return true; }
  bool Int(int) override { return true; }
  bool Uint(unsigned) override { return true; }
  bool Int64(int64_t) override { return true; }
  bool Uint64(uint64_t) override { return true; }
  bool Double(double) override { return true; }
  bool RawNumber(const char*, ::rapidjson::SizeType, bool) override {
    return true;
  }
  bool String(const char*, ::rapidjson::SizeType, bool) override {
    return true;
  }
  bool StartObject() override { return false; }
  bool Key(const char*, ::rapidjson::SizeType, bool) override { return true; }
  bool EndObject(::rapidjson::SizeType) override { return true; }
  bool StartArray() override { return false; }
  bool EndArray(::rapidjson::SizeType) override { return true; }
  void set_dispatcher(DispatchHandler*) override {}
};

class DispatchHandler
    : public ::rapidjson::BaseReaderHandler<::rapidjson::UTF8<>,
                                            DispatchHandler> {
 public:
  template <AggregateType T>
  DispatchHandler(T& value) {
    static auto member_offset_map = struct_member_offset_map<T>();
    this->push_handler(member_offset_map, value);
  }
  ~DispatchHandler() {
    while (!_stack.empty()) {
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

  template <typename T>
    requires is_associative_container_v<T>
  void push_handler(T& value) {
    auto* h = new AssociativeReaderHandler<T>(value);
    h->set_dispatcher(this);
    _stack.emplace_back(h);
  }

  void pop_handler() {
    delete _stack.back();
    _stack.pop_back();
  }

 public:
  bool Bool(bool b) { return top()->Bool(b); }
  bool Int(int i) { return top()->Int(i); }
  bool Key(const char* s, ::rapidjson::SizeType l, bool c) {
    return top()->Key(s, l, c);
  }
  bool Null() { return top()->Null(); }
  bool Uint(unsigned u) { return top()->Uint(u); }

  bool Int64(int64_t i) { return top()->Int64(i); }

  bool Uint64(uint64_t u) { return top()->Uint64(u); }
  bool Double(double d) { return top()->Double(d); }
  bool RawNumber(const char* str, ::rapidjson::SizeType length, bool copy) {
    return top()->RawNumber(str, length, copy);
  }
  bool String(const char* str, ::rapidjson::SizeType length, bool copy) {
    return top()->String(str, length, copy);
  }
  bool StartArray() {
    bool result = top()->StartArray();
    if (!result) {
      _stack.emplace_back(new SkipHandler());
    }
    _array_depth.push_back(true);
    return true;
  }
  bool EndArray(::rapidjson::SizeType elementCount) {
    if (_array_depth.empty()) {
      return true;
    }

    bool should_pop = _array_depth.back();
    _array_depth.pop_back();

    if (_stack.empty()) {
      return true;
    }

    bool result = top()->EndArray(elementCount);
    if (should_pop) {
      pop_handler();
    }
    return result;
  }
  bool StartObject() {
    if (_is_first_member) {
      _is_first_member = false;
      _nested_depth.push_back(true);
      return true;
    }
    bool result = top()->StartObject();
    if (!result) {
      _stack.emplace_back(new SkipHandler());
    }
    _nested_depth.push_back(true);
    return true;
  }

  bool EndObject(::rapidjson::SizeType m) {
    if (_nested_depth.empty()) return true;

    bool should_pop = _nested_depth.back();
    _nested_depth.pop_back();

    if (_stack.empty()) return true;

    bool result = top()->EndObject(m);
    if (should_pop) {
      pop_handler();
    }
    return result;
  }

 private:
  IHandler* top() const { return _stack.back(); }

 private:
  ::std::vector<IHandler*> _stack;
  ::std::vector<bool> _nested_depth;
  ::std::vector<bool> _array_depth;

  bool _is_first_member = true;
};

// ::rapidjson::BaseReaderHandler<::rapidjson::UTF8<>, ReaderHandlerImp<T,
// Is...>>
template <typename T, size_t... Is>
struct ReaderHandlerImp<T, ::std::index_sequence<Is...>> : public IHandler {
  using Tuple = decltype(struct_members_to_tuple<T>());
  using ValueType = decltype(get_variant_type<T, Tuple, Is...>());
  using MapType =
      ::frozen::unordered_map<::frozen::string, ValueType,
                              serializable_members_count_v<T>>;

 public:
  ReaderHandlerImp(const MapType& map_value, T& value)
      : _struct_member_offset_map(map_value),
        _iterator(_struct_member_offset_map.end()),
        _value(value) {}

 public:
  bool Null() override {
    if (_iterator != _struct_member_offset_map.end()) {
      // TODO
    }
    return true;
  }

  bool Bool(bool b) override {
    return assign_if_match<bool>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(b);
    });
  }

  bool Int(int i) override {
    return assign_if_match<int>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(i);
    });
  }

  bool Uint(unsigned u) override {
    return assign_if_match<unsigned>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(u);
    });
  }

  bool Int64(int64_t i) override {
    return assign_if_match<int64_t>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(i);
    });
  }

  bool Uint64(uint64_t u) override {
    return assign_if_match<uint64_t>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(u);
    });
  }
  bool Double(double d) override {
    return assign_if_match<double>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(d);
    });
  }
  bool RawNumber(const char* str, ::rapidjson::SizeType /*length*/,
                 bool /*copy*/) override {
    return assign_if_match<const char*>([&](auto& member) { member = str; });
  }
  bool String(const char* str, ::rapidjson::SizeType length,
              bool /*copy*/) override {
    // Handle char type: assign first character of the string
    bool char_handled = false;
    if (_iterator != _struct_member_offset_map.end()) {
      auto offset = _iterator->second;
      ::std::visit(
          [&](auto arg) {
            using Value_Type = typename decltype(arg)::type;
            if constexpr (is_char_v<Value_Type>) {
              Value_Type& member_value = *reinterpret_cast<Value_Type*>(
                  reinterpret_cast<char*>(static_cast<T*>(&_value)) +
                  arg.value);
              member_value =
                  (length > 0) ? static_cast<Value_Type>(str[0]) : Value_Type{};
              char_handled = true;
            } else if constexpr (is_string_v<Value_Type>) {
              // Use assign(str, length) to preserve embedded null bytes.
              // Assigning via operator=(const char*) would truncate at the
              // first '\0'.
              Value_Type& member_value = *reinterpret_cast<Value_Type*>(
                  reinterpret_cast<char*>(static_cast<T*>(&_value)) +
                  arg.value);
              member_value.assign(str, length);
              char_handled = true;
            }
          },
          offset);
    }
    if (char_handled) return true;
    return assign_if_match<const char*>([&](auto& member) { member = str; });
  }
  bool StartObject() override {
    bool found = (_iterator != _struct_member_offset_map.end());
    if (found) {
      auto offset = _iterator->second;
      bool pushed = false;
      ::std::visit(
          [&](auto arg) {
            using Value_Type = typename decltype(arg)::type;
            if constexpr (is_custom_type_v<Value_Type>) {
              static auto member_offset_map =
                  struct_member_offset_map<Value_Type>();
              Value_Type& member_value = *reinterpret_cast<Value_Type*>(
                  reinterpret_cast<char*>(static_cast<T*>(&_value)) +
                  arg.value);
              _dispatch_handler->push_handler<Value_Type>(member_offset_map,
                                                          member_value);
              pushed = true;
            } else if constexpr (is_associative_container_v<Value_Type>) {
              Value_Type& member_value = *reinterpret_cast<Value_Type*>(
                  reinterpret_cast<char*>(static_cast<T*>(&_value)) +
                  arg.value);
              _dispatch_handler->push_handler<Value_Type>(member_value);
              pushed = true;
            }
          },
          offset);
      return pushed;
    }
    return false;
  }
  bool Key(const char* str, ::rapidjson::SizeType length, bool /*copy*/) override {
    _iterator = _struct_member_offset_map.find(::frozen::string(str, length));
    return true;
  }
  bool EndObject(::rapidjson::SizeType /*memberCount*/) override { return true; }
  bool StartArray() override {
    bool found = (_iterator != _struct_member_offset_map.end());
    if (found) {
      auto offset = _iterator->second;
      bool pushed = false;
      ::std::visit(
          [&](auto arg) {
            using Value_Type = typename decltype(arg)::type;
            if constexpr (is_sequence_container_v<Value_Type>) {
              Value_Type& member_value = *reinterpret_cast<Value_Type*>(
                  reinterpret_cast<char*>(static_cast<T*>(&_value)) +
                  arg.value);
              _dispatch_handler->push_handler<Value_Type>(member_value);
              pushed = true;
            }
          },
          offset);
      return pushed;
    }
    return false;
  }
  bool EndArray(::rapidjson::SizeType /*elementCount*/) override { return true; }

 private:
  template <typename TargetType, typename F>
  bool assign_if_match(F&& assign_func) {
    if (_iterator != _struct_member_offset_map.end()) {
      auto offset = _iterator->second;
      ::std::visit(
          [&](auto arg) {
            using Value_Type = typename decltype(arg)::type;
            if constexpr (is_json_compatible_v<remove_cvref_t<Value_Type>,
                                               TargetType>) {
              auto* member_ptr = reinterpret_cast<Value_Type*>(
                  reinterpret_cast<char*>(static_cast<T*>(&_value)) +
                  arg.value);
              assign_func(*member_ptr);
            }
          },
          offset);
    }
    return true;
  }

 public:
  void set_dispatcher(DispatchHandler* dispatcher) override {
    _dispatch_handler = dispatcher;
  }

 private:
  const MapType& _struct_member_offset_map;
  typename MapType::const_iterator _iterator;
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
  bool Null() override { return true; }

  bool Bool(bool b) override {
    return assign_if_match<bool>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(b);
    });
  }

  bool Int(int i) override {
    return assign_if_match<int>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(i);
    });
  }

  bool Uint(unsigned u) override {
    return assign_if_match<unsigned>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(u);
    });
  }

  bool Int64(int64_t i) override {
    return assign_if_match<int64_t>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(i);
    });
  }

  bool Uint64(uint64_t u) override {
    return assign_if_match<uint64_t>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(u);
    });
  }
  bool Double(double d) override {
    return assign_if_match<double>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(d);
    });
  }
  bool RawNumber(const char* str, ::rapidjson::SizeType /*length*/,
                 bool /*copy*/) override {
    return assign_if_match<const char*>([&](auto& member) { member = str; });
  }
  bool String(const char* str, ::rapidjson::SizeType length,
              bool /*copy*/) override {
    if constexpr (is_char_v<ElementType>) {
      // Handle vector<char>: each JSON string element contributes its first
      // char
      _value.emplace_back((length > 0) ? static_cast<ElementType>(str[0])
                                       : ElementType{});
      return true;
    } else if constexpr (is_string_v<ElementType>) {
      // Use assign with length to preserve embedded null bytes.
      // emplace_back(str) would truncate at the first '\0'.
      _value.emplace_back(str, length);
      return true;
    }
    return assign_if_match<const char*>([&](auto& member) { member = str; });
  }
  bool StartObject() override {
    if constexpr (is_custom_type_v<ElementType>) {
      static auto member_offset_map = struct_member_offset_map<ElementType>();
      _dispatch_handler->push_handler<ElementType>(member_offset_map,
                                                   _value.emplace_back());
      return true;
    } else if constexpr (is_associative_container_v<ElementType>) {
      _dispatch_handler->push_handler<ElementType>(_value.emplace_back());
      return true;
    }
    return false;
  }
  bool Key(const char* /*str*/, ::rapidjson::SizeType /*length*/, bool /*copy*/) override {
    return true;
  }
  bool EndObject(::rapidjson::SizeType /*memberCount*/) override { return true; }
  bool StartArray() override {
    if constexpr (is_sequence_container_v<ElementType>) {
      _dispatch_handler->push_handler<ElementType>(_value.emplace_back());
      return true;
    }
    return false;
  }
  bool EndArray(::rapidjson::SizeType /*elementCount*/) override { return true; }

 private:
  template <typename TargetType, typename F>
  bool assign_if_match(F&& assign_func) {
    if constexpr (is_json_compatible_v<remove_cvref_t<ElementType>,
                                       TargetType>) {
      assign_func(_value.emplace_back());
    }
    return true;
  }

 public:
  void set_dispatcher(DispatchHandler* dispatcher) override {
    _dispatch_handler = dispatcher;
  }

 private:
  T& _value;
  DispatchHandler* _dispatch_handler = nullptr;
};

// AssociativeReaderHandleImp: 处理 map/unordered_map 的反序列化
template <typename T>
class AssociativeReaderHandleImp : public IHandler {
  using KeyType = typename remove_cvref_t<T>::key_type;
  using MappedType = typename remove_cvref_t<T>::mapped_type;

 public:
  AssociativeReaderHandleImp(T& value) : _value(value) {}

 public:
  bool Null() override { return true; }

  bool Bool(bool b) override {
    return assign_value<bool>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(b);
    });
  }

  bool Int(int i) override {
    return assign_value<int>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(i);
    });
  }

  bool Uint(unsigned u) override {
    return assign_value<unsigned>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(u);
    });
  }

  bool Int64(int64_t i) override {
    return assign_value<int64_t>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(i);
    });
  }

  bool Uint64(uint64_t u) override {
    return assign_value<uint64_t>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(u);
    });
  }

  bool Double(double d) override {
    return assign_value<double>([&](auto& member) {
      member = static_cast<remove_cvref_t<decltype(member)>>(d);
    });
  }

  bool RawNumber(const char* /*str*/, ::rapidjson::SizeType /*length*/,
                 bool /*copy*/) override {
    return true;
  }

  bool String(const char* str, ::rapidjson::SizeType length,
              bool /*copy*/) override {
    if constexpr (is_char_v<MappedType>) {
      _value[_current_key] =
          (length > 0) ? static_cast<MappedType>(str[0]) : MappedType{};
      return true;
    } else if constexpr (is_string_v<MappedType>) {
      _value[_current_key].assign(str, length);
      return true;
    }
    return true;
  }

  bool StartObject() override {
    if constexpr (is_custom_type_v<MappedType>) {
      static auto member_offset_map = struct_member_offset_map<MappedType>();
      auto& inserted = _value[_current_key];
      _dispatch_handler->push_handler<MappedType>(member_offset_map, inserted);
      return true;
    } else if constexpr (is_associative_container_v<MappedType>) {
      auto& inserted = _value[_current_key];
      _dispatch_handler->push_handler<MappedType>(inserted);
      return true;
    }
    return false;
  }

  bool Key(const char* str, ::rapidjson::SizeType length,
           bool /*copy*/) override {
    _current_key.assign(str, length);
    return true;
  }

  bool EndObject(::rapidjson::SizeType /*memberCount*/) override { return true; }

  bool StartArray() override {
    if constexpr (is_sequence_container_v<MappedType>) {
      auto& inserted = _value[_current_key];
      _dispatch_handler->push_handler<MappedType>(inserted);
      return true;
    }
    return false;
  }

  bool EndArray(::rapidjson::SizeType /*elementCount*/) override { return true; }

 private:
  template <typename TargetType, typename F>
  bool assign_value(F&& assign_func) {
    if constexpr (is_json_compatible_v<remove_cvref_t<MappedType>,
                                       TargetType>) {
      assign_func(_value[_current_key]);
    }
    return true;
  }

 public:
  void set_dispatcher(DispatchHandler* dispatcher) override {
    _dispatch_handler = dispatcher;
  }

 private:
  T& _value;
  ::std::string _current_key;
  DispatchHandler* _dispatch_handler = nullptr;
};

template <typename T>
  requires is_associative_container_v<T>
struct AssociativeReaderHandler : public AssociativeReaderHandleImp<T> {
  AssociativeReaderHandler(T& value) : AssociativeReaderHandleImp<T>(value) {}
};

}  // namespace tinyrefl::detail

namespace tinyrefl {
enum class ErrorKind {
  None = 0,
  SyntaxError,          // General syntax error
  Incomplete,           // JSON is not complete
  InvalidEncoding,      // Invalid or unsupported encoding
  ExtraDataAfterRoot,   // Extra data found after JSON root
  NumberOutOfRange,     // Number too large or invalid format
  StringEscapeInvalid,  // Invalid string escape sequence
  TrailingComma,        // Trailing comma not allowed
  CommentNotAllowed,    // Comments are not allowed
  Unknown
};

struct Error {
  ErrorKind kind = ErrorKind::None;
  ::std::string message;
  ::std::size_t offset = 0;
  ::std::size_t line = 0;
  ::std::size_t column = 0;
};

struct Status {
  bool ok = true;
  Error error;

  operator bool() const { return ok; }
};

inline ::std::pair<::std::size_t, ::std::size_t> offset_to_linecol(
    ::std::string_view s, ::std::size_t offset) {
  ::std::size_t line = 1, col = 1;
  const ::std::size_t n = ::std::min(offset, s.size());
  for (::std::size_t i = 0; i < n; ++i) {
    if (s[i] == '\n') {
      ++line;
      col = 1;
    } else {
      ++col;
    }
  }
  return {line, col};
}

inline ErrorKind map_kind(::rapidjson::ParseErrorCode code) {
  using C = ::rapidjson::ParseErrorCode;
  switch (code) {
    case C::kParseErrorNone:
      return ErrorKind::None;

    case C::kParseErrorValueInvalid:
    case C::kParseErrorObjectMissName:
    case C::kParseErrorObjectMissColon:
    case C::kParseErrorStringMissQuotationMark:
    case C::kParseErrorStringUnicodeEscapeInvalidHex:
    case C::kParseErrorStringUnicodeSurrogateInvalid:
    case C::kParseErrorUnspecificSyntaxError:
    case C::kParseErrorObjectMissCommaOrCurlyBracket:
    case C::kParseErrorArrayMissCommaOrSquareBracket:
      return ErrorKind::SyntaxError;

    case C::kParseErrorNumberTooBig:
    case C::kParseErrorNumberMissFraction:
    case C::kParseErrorNumberMissExponent:
      return ErrorKind::NumberOutOfRange;

    case C::kParseErrorStringEscapeInvalid:
      return ErrorKind::StringEscapeInvalid;

    case C::kParseErrorDocumentRootNotSingular:
      return ErrorKind::ExtraDataAfterRoot;

    case C::kParseErrorDocumentEmpty:
      return ErrorKind::Incomplete;

    case C::kParseErrorStringInvalidEncoding:
      return ErrorKind::InvalidEncoding;

    default:
      return ErrorKind::Unknown;
  }
}

inline ::std::string translate_message(ErrorKind k,
                                      ::rapidjson::ParseErrorCode code) {
  switch (k) {
    case ErrorKind::SyntaxError:
      return "JSON syntax error";
    case ErrorKind::Incomplete:
      return "JSON is incomplete";
    case ErrorKind::InvalidEncoding:
      return "Invalid or unsupported encoding";
    case ErrorKind::ExtraDataAfterRoot:
      return "Extra data after root element";
    case ErrorKind::NumberOutOfRange:
      return "Number out of range or invalid format";
    case ErrorKind::StringEscapeInvalid:
      return "Invalid string escape sequence";
    case ErrorKind::TrailingComma:
      return "Trailing comma not allowed";
    case ErrorKind::CommentNotAllowed:
      return "Comments are not allowed in JSON";
    case ErrorKind::Unknown:
      return ::std::string("Parse failed: ") +
             ::rapidjson::GetParseError_En(code);
    case ErrorKind::None:
      return "";
  }
  return "Parse failed";
}

// Deserialization Interface
template <detail::AggregateType T>
inline Status reflection_from_json(T&& object, const char* str) {
  detail::DispatchHandler handler(object);
  ::rapidjson::StringStream ss(str);
  ::rapidjson::Reader reader;
  auto result = reader.Parse<::rapidjson::kParseDefaultFlags>(ss, handler);

  Status st{};
  st.ok = !result.IsError();

  if (!st.ok) {
    const auto code = result.Code();
    const auto off = result.Offset();

    st.error.kind = map_kind(code);
    st.error.offset = off;
    ::std::tie(st.error.line, st.error.column) = offset_to_linecol(str, off);
    st.error.message = translate_message(st.error.kind, code);
  }
  return st;
}

// Deserialization Interface
template <detail::AggregateType T>
inline std::pair<bool, ::std::remove_cvref_t<T>> reflection_from_json(
    const char* str) {
  T value;
  detail::DispatchHandler handler(value);
  ::rapidjson::StringStream ss(str);
  ::rapidjson::Reader reader;
  auto result = reader.Parse<::rapidjson::kParseDefaultFlags>(ss, handler);

  return {!result.IsError(), value};
}

}  // namespace tinyrefl
