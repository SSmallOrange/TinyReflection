## 第四章：基于反射的 Json 反序列化

前一章我们已经用编译期反射实现了 Json 序列化，把一个聚合类型一次性吐成一段 Json 字符串，不需要手写拼接。

这章做反方向的事情：

> 给定一段 Json 文本，把它自动填充进一个结构体对象里，支持嵌套结构体、顺序容器、关联容器等常见组合。

最终对外只保留一个接口：

```c++
tinyrefl::Complex obj{};
const char* json = R"({
    "id": 42,
    "name": "tinyrefl",
    "values": [1, 2, 3]
})";

auto st = tinyrefl::reflection_from_json(obj, json);

```

Json 正确就把 obj 填好；Json 有问题就返回一份结构化的错误信息，包含错误种类、偏移、行列号和可读的错误描述。

### 实现方案

这里我采用不构建 DOM，直接基于`RapidJSON`的流式 SAX 接口，把事件派发到对应的结构体成员。
主要思路：
- 由于`RapidJson`的SAX接口只提供回调，所以对于递归反序列化，还需要我们自己维护一个解析栈来处理嵌套情况
- 同时，`RapidJson`对于数组类型和普通类型的回调方式不同，所以这里需要分别进行处理

剩下的就是使用`RapidJson`的接口了，`RapidJson`相关文档：https://rapidjson.org/zh-cn/

#### 结构体成员KV化

到反序列化这里，前面的铺垫已经很充分了：

- `members_count_v<T>` 能在编译期算出成员个数  
- `struct_members_to_array<T>()` 能拿到成员名数组  
- `struct_members_to_tuple<T>()` 能拿到成员引用 tuple（在有对象实例的前提下）  

序列化的时候，我们只需要在“有对象”的场景下遍历一遍成员，把 `(member_name, member_reference)`这一对在运行时交给 `to_json_value` 就行，所以直接用 **tuple + 泛型 lambda** 遍历就够了。
反序列化就不一样了，真正要解决的是下面这个问题：

> 当 RapidJSON 回调告诉我「现在读到了 key = 'id'」，如何在 O(1) 时间里找到这个成员在结构体里的“位置”和“类型”，然后把后续的值事件填到正确的成员上？

也就是说，我们需要把“结构体的成员元信息”预先组织成某种 Key → Value 的形式：

```
member_name       → 成员的元信息（类型 + 在对象里的位置）
"id"              → { type = int,  offset = 0 }
"name"            → { type = std::string, offset = 8 }
"values"          → { type = std::vector<int>, offset = 40 }
```
这里 `member_name` 很好办，前面我们已经能够构建出 `std::array<string_view, N>`。真正难的是「成员引用」这一侧。

##### 为什么不能直接存「成员引用」
直觉上的想法可能是：能不能搞一个类似这样的东西？

```c++
struct MemberRefBase { virtual ~MemberRefBase() = default; };

template <typename T>
struct MemberRef : MemberRefBase {
    using type = T;
    T* ptr;
};
```
然后弄一个 `std::unordered_map<string, std::unique_ptr<MemberRefBase>>`，把每个成员都塞进去，反序列化时查到 key 就取出对应指针，强转回真实类型再赋值。

想法很美好，但是这个思路有一个关键问题：**不同成员类型没法丢进同一个“简单容器”**，因为成员类型是异构的：`int、std::string、std::vector<double>...`，我们没法把他放在同一个`map`里。

聪明的同学可能会举一反三，那我们使用类型擦除，把他们全都擦成`void*`，然后自己维护一套类型。

是的，实现方案是这样的，但是光有类型还不够，我们仍然没有成员对象的引用，这里可以考虑用**成员偏移量**来实现。

那么思路清晰了，我们需要构建一个这样的结构体：
```c++
template <typename T>
struct MemberMeta {
    using type = T;
    std::size_t offset;   // 成员在对象内的偏移量
};
```
并且将其存在一个`std::variant`中：
```c++
using ValueType = std::variant<
    MemberMeta<int>,
    MemberMeta<std::string>,
    MemberMeta<std::vector<int>>,
    ...
>;
```
最后将这些整理存在一个`map`里：
```c++
using MapType = ::frozen::unordered_map<
    ::frozen::string,  // 成员名
    ValueType,         // 成员元信息（类型 + 偏移）
    members_count_v<T>
>;
```
`std::variant` 配合 `std::visit`使用可以高效的解决当前成员的类型问题，就不用自己维护脆弱的`void* + type_id`了。

##### 实现方案

这里看下整体的实现方案：
```c++
template <typename T, size_t... Is>
inline auto get_variant_map(::std::index_sequence<Is...>) {
    using U = remove_cvref_t<T>;
    constexpr auto member_name_arr = struct_members_to_array<U>();
    auto& member_offset_arr = struct_member_offset_array<U>();
    using Tuple = decltype(struct_members_to_tuple<U>());
    // using Remove_Tuple_CV_Type = decltype(remove_cvref_t<::std::tuple_element_t<Is, Tuple>>);
    using ValueType = decltype(get_variant_type<U, Tuple, Is...>());
    // runtime plan
    // ::std::unordered_map<::std::string_view, ValueType> map;
    // (map.emplace(member_name_arr[Is],
    //     ValueType{::std::in_place_index<Is>, 
    //         offset_of_member<decltype(remove_tuple_cv_type<Is, Tuple>())>{member_offset_arr[Is]}}),
    // ...);
    // return map;
    return frozen::unordered_map<frozen::string, ValueType, sizeof...(Is)>{
        {member_name_arr[Is],
            ValueType{::std::in_place_index<Is>, 
                offset_of_member<decltype(remove_tuple_cv_type<Is, Tuple>())>{member_offset_arr[Is]}}
        }...
    };
}
```
这里的具体实现方式就不展开了，和之前的模板实现过程大差不差，就是多了`std::variant`，感兴趣的读者可以自行阅读：https://github.com/SSmallOrange/TinyReflection/blob/master/tinyrefl/utils/reflection_get_tuple.hpp


#### RapidJson解析

要使用SAX风格的解析接口，我们需要继承`BaseReaderHandler<UTF8<>, MyHandler>`，这是一种叫`CRTP(Curiously Recurring Template Pattern（奇异递归模板模式）)`的静态多态方式，相比传统多态省去了虚函数表的调用开销，原理是在其内部进行静态类型转换并调用函数，类似于：
```c++
static_cast<T&>(*this).Func();
```

继承后需要实现如下接口：
```c++
    struct IHandler
    {
        virtual bool Null() = 0;
        virtual bool Bool(bool) = 0;
        virtual bool Int(int) = 0;
        virtual bool Uint(unsigned) = 0;
        virtual bool Int64(int64_t) = 0;
        virtual bool Uint64(uint64_t) = 0;
        virtual bool Double(double) = 0;
        virtual bool RawNumber(const char *str, ::rapidjson::SizeType length, bool copy) = 0;
        virtual bool String(const char *, ::rapidjson::SizeType, bool) = 0;
        virtual bool StartObject() = 0;
        virtual bool Key(const char *, ::rapidjson::SizeType, bool) = 0;
        virtual bool EndObject(::rapidjson::SizeType) = 0;
        virtual bool StartArray() = 0;
        virtual bool EndArray(::rapidjson::SizeType) = 0;

        virtual void set_dispatcher(DispatchHandler *dispatcher) = 0;
        virtual ~IHandler() = default;
    };

```

这里我们自己抽象出一个`Handler`，整体实现分为三层：
- `DispatchHandler`
继承自 rapidjson::BaseReaderHandler，是唯一交给 RapidJSON 的 handler
内部维护一个 std::vector<IHandler*> 栈，负责把回调转发给栈顶 handler
- `ReaderHandlerImp<T, Is...>`
面向自定义聚合类型，负责解析一个 Json 对象 { ... } 并写进结构体 T
- `SequenceReaderHandleImp<T>`
面向顺序容器，负责解析一个 Json 数组 [ ... ] 并往容器里 emplace_back

##### DispatchHandler：事件分发

`DispatchHandler`的职责很简单：

- 构建初始的 handler 栈
- 把 RapidJSON 的所有回调，转发给栈顶的 IHandler
- 在遇到新的对象或数组时，按需要压栈或出栈

构造函数里会根据根类型是 struct 还是容器，压入第一个 handler：
```c++
template <AggregateType T>
DispatchHandler(T& value) {
    static auto member_offset_map = struct_member_offset_map<T>();
    this->push_handler(member_offset_map, value);
}
```
入栈的实现方式如下：
```c++
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
```

在入栈时，需要区分首次入栈和解析时入栈，两种情况的处理方式不同：
- 第一次 `StartObject()` 是根对象，不转发，只是把 _is_first_member 标记成 false
- 后续 `StartObject()` 全部转发给当前 handler，由它决定是否要继续创建子 handler 并压栈
- `EndObject()` 如果栈空，就说明反序列化结束，直接返回 true；否则转发给栈顶并 pop_handler


所有 RapidJSON 回调函数都由分发器转发给栈顶 `Handler` ：
```c++
bool Bool(bool b)                 { return top()->Bool(b); }
bool Int(int i)                   { return top()->Int(i); }
bool String(const char* s, ::rapidjson::SizeType len, bool copy) { return top()->String(s, len, copy); }
bool StartArray()                 { return top()->StartArray(); }
bool EndArray(::rapidjson::SizeType n) {
    bool result = top()->EndArray(n);
    pop_handler();  // 解析结束时出栈
    return result;
}
...
```

##### ReaderHandlerImp<T, Is...>：结构体对象解析

对于自定义聚合类型，Json 里对应的是一个对象 { ... }。反序列化时需要完成两件事：
- 找到当前 Key 对应的是哪个成员
- 把后续的值事件写入这个成员里，或者把子对象 / 子数组继续交给新的 handler 解析

当我们在 `Key`回调拿到成员名时，就可以：
```c++
bool Key(const char* str, ::rapidjson::SizeType length, bool) override {
    _iterator = _struct_member_offset_map.find(::frozen::string(str, length));
    return true;
}
```
后面的所有操作都会围绕这个迭代器展开。
针对每一种类型的 Json 回调，`ReaderHandlerImp`都会将其分发给 `assign_if_match`:
```c++
bool Bool(bool b) override {
    return assign_if_match<bool>([&](auto& member) { member = b; });
}
bool Int(int i) override {
    return assign_if_match<int>([&](auto& member) { member = i; });
}
bool String(const char* s, ::rapidjson::SizeType len, bool copy) override {
    return assign_if_match<const char*>([&](auto& member) { member = s; });
}
...
```
 `assign_if_match` 实现如下：
```c++
template <typename TargetType, typename F>
bool assign_if_match(F&& assign_func) {
    if (_iterator != _struct_member_offset_map.end()) {
        auto offset = _iterator->second;
        std::visit([&](auto arg) {
            using Value_Type = typename decltype(arg)::type;
            if constexpr (is_json_compatible_v<remove_cvref_t<Value_Type>, TargetType>) {
                auto* member_ptr = (Value_Type*)(
                    (char*)&_value + arg.value
                );
                assign_func(*member_ptr);  // 调用回调进行赋值
            }
        }, offset);
    }
    return true;  // 检查不到Key时忽略该字段
}
```
其中 `is_json_compatible_v`是通过：
```c++
template <typename Target, typename From>
constexpr bool is_json_compatible_v = ::std::is_assignable_v<Target &, From>;
```
实现的，`std::is_assignable_v`能够检查`Target`能不能被`From`赋值。

当一个对象开始时，RapidJson 会从`StartObject()` 开始：
```c++
bool StartObject() override {
    if (_iterator != _struct_member_offset_map.end()) {
        auto offset = _iterator->second;
        std::visit([&](auto arg) {
            using Value_Type = typename decltype(arg)::type;
            if constexpr (is_custom_type_v<Value_Type>) {
                static auto member_offset_map = struct_member_offset_map<Value_Type>();
                auto* member_value = reinterpret_cast<Value_Type*>(
                    reinterpret_cast<char*>(&_value) + arg.value
                );
                _dispatch_handler->push_handler<Value_Type>(member_offset_map, *member_value);
            }
        }, offset);
    }
    return true;
}
```
先通过前面提到的 `iterator`拿到对应的成员类型信息，通过`std::visit`匹配当前`std::variant`存储的类型，并通过偏移量拿到当前成员的引用。随后将其入栈，嵌套处理。

##### SequenceReaderHandleImp<T>：顺序容器解析
顺序容器对应 Json 里的数组：
```json
{
  "values": [1, 2, 3]
}
```
对于容器本身，直接视为一串“连续的值事件”。这里通过一个模板别名拿到元素类型：
```c++
using ElementType = sequence_element_type_t<remove_cvref_t<T>>;
```
核心思路：
- 对于标量元素类型：每次数值事件来一发，就 emplace_back 一个元素，然后写入
- 对于元素是 struct：在 StartObject() 时为每个元素创建一个新的 ReaderHandler
- 对于元素还是容器：在 StartArray() 时继续压入下一层 SequenceReaderHandler

对于第一种情况，直接处理：
```c++
template <typename TargetType, typename F>
bool assign_if_match(F&& assign_func) {
    if constexpr (is_json_compatible_v<remove_cvref_t<ElementType>, TargetType>) {
        assign_func(_value.emplace_back());
    }
    return true;
}
```
第二种情况，元素是自定义 struct：
```c++
bool StartObject() override {
    if constexpr (is_custom_type_v<ElementType>) {
        static auto member_offset_map = struct_member_offset_map<ElementType>();
        _dispatch_handler->push_handler<ElementType>(  // 因为这里传引用，所以需要静态变量，对不同类型都只会生成一次
            member_offset_map,
            _value.emplace_back()
        );
    }
    return true;
}
```
第三种情况，元素本身是容器：
```c++
bool StartArray() override {
    if constexpr (is_sequence_container_v<ElementType>) {
        _dispatch_handler->push_handler<ElementType>(_value.emplace_back());
    }
    return true;
}
```
小知识：序列化容器的`emplace_back`函数会返回插入元素的引用。

#### 外层接口

至此，Json的反序列工作就做完了，整理接口后得到：
```c++
template <detail::AggregateType T>
inline bool reflection_from_json(T &&object, const char *str) {
    detail::DispatchHandler handler(object);
    ::rapidjson::StringStream ss(str);
    ::rapidjson::Reader reader;
    auto result = reader.Parse<::rapidjson::kParseDefaultFlags>(ss, handler);

    return !result.IsError();
}
```

