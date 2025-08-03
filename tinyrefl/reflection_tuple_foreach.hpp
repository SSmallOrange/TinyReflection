#include "tinyrefl/reflection_get_tuple.hpp"

namespace tinyrefl {
template<typename Stream>
concept OutputStream = requires(Stream& s) {
    { s.append("abc") };
};

template <typename T, typename Function>
inline void for_each_member(T&& object, Function&& function) {
    using object_type = remove_cvref_t<T>;
    constexpr member_array<object_type> object_name_array = struct_members_to_array<object_type>();
    constexpr size_t object_member_count = members_count_v<object_type>;

    if constexpr (object_member_count <= 0) {
        static_assert(object_member_count > 0, "The number of member variables cannot be empty");
        return;
    }

    if constexpr (std::is_invocable_v<Function, decltype(struct_member_reference<0>(object)), std::string_view, size_t>) {
        [&]<size_t... Is>(std::index_sequence<Is...>) {
            (function(struct_member_reference<Is>(object), object_name_array[Is], Is), ...);
        }(std::make_index_sequence<object_member_count>{});
    } else {
        static_assert(std::is_invocable_v<Function, std::string_view, size_t>, "invalid function args,  \
            param is: [std::string_view, size_t]");
    }
}

template <OutputStream Stream, typename Iter, typename Delimiter, typename Function>
requires requires(Iter&& iter) { *iter; ++iter;}
inline void for_each_by_iterator(Stream&& s, Iter first, Iter end, const Delimiter& delimiter, Function&& function) {
    if (first == end) {
        return;
    }
    for (; first != end; ++first) {
        if constexpr (std::is_invocable_v<Function, decltype(*first)>) {
            function(*first);
        } else {
            static_assert(std::is_invocable_v<Function, decltype(*first)>, "invalid function args");
        }
        if (std::next(first) != end) {
            s.append(delimiter);
        }
    }
}

}  // end namespace tinyrefl
