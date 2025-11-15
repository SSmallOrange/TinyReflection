## 第三章：基于反射的 Json 序列化
> 代码实现方案参考：https://github.com/qicosmos/iguana

---


通过前面的努力，我们目前已经有了以下能力：

- 通过编译器内置宏拿到**成员名**  
- 通过结构化绑定和打表拿到**成员引用 tuple**  
- 通过递归构造拿到**成员数量**

简单说，现在我们已经能在编译期拿到一个结构体的“成员列表”，并在运行时获得对应的引用。

有了这些能力，我们就能实现：一行代码序列化结构体对象，摒弃繁琐的Json拼装过程：


```c++
struct Complex obj;
std::string strJson;
tinyrefl::reflection_to_json(obj, strJson);     // 序列化
```

最终支持的能力包括：
```c++
// 普通字段
int / double / bool / std::string / char* 
// 嵌套 struct
std::vector / std::list / std::deque
// 容器和多层嵌套容器
std::map<std::string, T> / std::unordered_map<std::string, T>
vector<vector<int>> / vector<vector<T>> 

```

### 实现方案

目前传统的Json支持各种类型，如：对象、数组、字符串、数字、Bool等。

类型不同，Json的输出方式也不同，比如字符串类型可能需要双引号进行包裹，数组类型或对象类型则需要括号来标识，想要将结构体序列化成一个Json格式的字符串，就需要对结构体中的各种类型分别进行处理。

做编译期类型区分，模板显然是再适合不过了。

前两章已经把“怎么拿到成员”这件事解决掉了：我们能拿到 `members_count_v<T>`，能拿到成员名数组，也能遍历到每个成员的引用。序列化阶段真正要解决的问题，其实只有一个：

> 对于任意一个 `member_reference`，它到底应该被当成什么类型写进 Json。

典型几种情况：

- 内置算术类型，按数字写
- 布尔类型，写成 `true` / `false`
- 字符串、字符指针，加引号并做转义
- 容器，写成数组或对象
- 自定义聚合类型，展开成对象并递归处理成员

在类型分发之前，首先我们需要能够遍历结构体成员。

#### 结构体成员循环遍历

对外只暴露一行接口：
```c++
template <detail::AggregateType T, detail::OutputStream Stream>
inline void reflection_to_json(T&& object, Stream &stream);
```

其中`AggregateType`是目前序列化所支持的成员类型，`OutputStream`是我们能够支持的输出类型，可以只限定为`std::string`。
```c++
// 检查是否为聚合类型
template <typename T>
concept AggregateType = ::std::is_aggregate_v<remove_cvref_t<T>>;
// 检查字符串类型
template<typename Stream>
concept OutputStream = requires(Stream& s) {{ s.append("abc") };};
// 或
template<typename Stream>
concept OutputStream = ::std::is_same_v<std::remove_cvref_t<Stream>, std::string>;
```

在第一层类型检查通过后，我们就需要对具体类型进行递归分发了，首先我们要能够循环的遍历结构体成员：

```c++

template <typename T, typename Function>
inline void for_each_member(T&& object, Function&& function);
```
这里留出两个参数，`object`是结构体成员，`function`是对不同结构体成员的处理回调

先拿到我们需要的基本信息：成员名称和成员数量
```c++
using object_type = remove_cvref_t<T>;
constexpr member_array<object_type> object_name_array = struct_members_to_array<object_type>();
constexpr size_t object_member_count = members_count_v<object_type>;
```

作为一个`反射的结构体成员遍历`，我们需要检查回调函数是否能够接收：成员引用、成员名称、当前成员索引，否则抛出错误。

```c++
    if constexpr (::std::is_invocable_v<Function, decltype(struct_member_reference<0>(object)), ::std::string_view, size_t>) {
        ...
    } else {
        static_assert(::std::is_invocable_v<Function, ::std::string_view, size_t>, "invalid function args,  \
            param is: [::std::string_view, size_t]");
    }
```

检查后调用回调：
```c++
[&]<size_t... Is>(::std::index_sequence<Is...>) {
    (function(struct_member_reference<Is>(object), object_name_array[Is], Is), ...);
}(::std::make_index_sequence<object_member_count>{});
```
这是一个模板Lambda表达式，在C++20的加持下，很多代码都可以写的非常简单，这里展开其实是写了很多的回调函数，依次传入不同的索引：
```c++
(function(struct_member_reference<0>(object), object_name_array[0], 0), ...);
(function(struct_member_reference<1>(object), object_name_array[1], 1), ...);
(function(struct_member_reference<2>(object), object_name_array[2], 2), ...);
...
```

因为成员类型是在回调函数被调用时才能确定的，所以这里需要一个模板函数，我们可以用auto来代指模板：
```c++
[&](auto&& member_reference, auto&& member_name, auto&& member_index) {
    ...
}
```
效果和：
```c++
 template<typename MemberRef, typename Name, typename Index>
    void CallbackFunc(MemberRef&& member_reference, Name&& member_name, Index&& member_index) const;
```
是一样的，不过模板函数在未实例化之前是没有确切地址的，所以不能作为参数传入，所以需要一层封装：

```c++
struct Callback {
    Stream& stream;
    template<typename MemberRef, typename Name, typename Index>
    void operator()(MemberRef&& member_reference,
                    Name&& member_name, Index member_index) const {
        ...
    }
};

Callback cb{ stream };
detail::for_each_member(std::forward<T>(object), cb);
```

#### 分发不同Json类型

##### 类型区分

这里就要用到我们前面介绍的模板约束了，对于不同的类型，在编译期匹配不同的模板：
```c++
// Check is sequecnce container
template <typename T>
inline constexpr bool is_sequence_container_v =
    is_template_instant_of<::std::deque, remove_cvref_t<T>>::value  ||
    is_template_instant_of<::std::list, remove_cvref_t<T>>::value   ||
    is_template_instant_of<::std::vector, remove_cvref_t<T>>::value;

// Check is char*
template <typename T>

inline constexpr bool is_char_pointer_v = is_char_v<::std::remove_pointer_t<::std::remove_cvref_t<T>>> &&
                        ::std::is_pointer_v<::std::remove_cvref_t<T>>;

// Check is char array
template <typename T>
inline constexpr bool is_char_array_v = ::std::is_array_v<remove_cvref_t<T>> && is_char_v<::std::remove_extent_t<T>>;

// Check is bool
template <typename T>
inline constexpr bool is_bool_v = ::std::is_same_v<remove_cvref_t<T>, bool>;

// Check is int
template <typename T>
inline constexpr bool is_int_v = ::std::is_integral_v<remove_cvref_t<T>> && 
                        !is_char_v<T> &&
                        !is_char_pointer_v<T> &&
                        !is_char_array_v<T> &&
                        !is_bool_v<T>;
...
```

可以看到我在int类型的检查处做了很多'非'的判断，是因为假设我同时处理`int`类型和`char`类型，对于约束来说：
```c++
is_char_v<char> == true
std::is_integral_v<char> == true
```
也就是说，这两个模板的 requires 对 char 都成立，编译器会认为两个重载都“可行”，从而触发编译错误，有多个匹配的函数，同理，对于`int`和`bool`也是同理。

对于用户自定义的结构体类型，只需要把目前支持的类型全都`false`掉就好。

##### 函数实现

对于普通类型：
```c++
// int
template <OutputStream Stream, typename T>
requires (is_int_v<T> || is_int64_v<T> || is_float_v<T> || is_double_v<T>)
inline void to_json_value(Stream&& s, T&& object) {
    s.append(::std::to_string(object));
}
// string to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_string_v<T> {
    s.append("\"");
    s.append(object.data(), object.size());
    s.append("\"");
}
// char to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_char_v<T> {
    s.append("\"");
    s.push_back(object);
    s.append("\"");
}
// bool
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_bool_v<T> {
    s.append(object ? "true" : "false");
}
...
```
对于顺序容器和关联容器，需要遍历其容器内数据，这时候迭代器的好处就体现出来了，我们可以统一的用迭代器处理各种容器，只需要写一套代码：
```c++
// sequence to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_sequence_container_v<T> {
    s.append("[");
    for_each_by_iterator(s, object.cbegin(), object.cend(), ",", [&](const auto& member) {
        to_json_value(s, member);
    });
    s.append("]");
}
// associative to json
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_associative_container_v<T> {
    s.append("{");
    for_each_by_iterator(s, object.cbegin(), object.cend(), ",", [&](const auto& pair_value) {  // ::std::pair
        if constexpr (is_string_v<decltype(pair_value.first)>) {
            to_json_key(s, pair_value.first);
            s.append(":");
            to_json_value(s, pair_value.second);
        } else {
            static_assert(is_string_v<decltype(pair_value.first)>, "Only string keys are supported in JSON");
        }
    });
    s.append("}");
}
```

其中`for_each_by_iterator`和前面介绍的结构体遍历实现类似，这里就不过多介绍了。
特别的，对于关联容器，需要在回调检查`Key`类型为`std::string`类型，这才符合`Json`的KV格式。

对于自定义`struct`，只需要调用最开始实现的序列化函数就能处理：
```c++
template <OutputStream Stream, typename T>
inline void to_json_value(Stream&& s, T&& object) requires is_custom_type_v<T> {
    ::tinyrefl::reflection_to_json(object, s);
}
```

至此，整个序列化功能实现闭环。

---

## 本章总结

本章的代码见：https://github.com/SSmallOrange/TinyReflection/blob/master/tinyrefl/reflectiopn_to_json.hpp

本章相关测试代码：

https://github.com/SSmallOrange/TinyReflection/blob/master/test/test_reflection_to_json.cpp


有了序列化能力自然也需要一个反序列化能力，下一章实现。

