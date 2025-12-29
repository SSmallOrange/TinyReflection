#pragma once

#include <tuple>
#include <array>
#include <map>
#include <deque>
#include <cstdint>
#include <list>
#include <queue>
#include <string>
#include <vector>
#include <type_traits>
#include <string_view>
#include <unordered_map>
#include <memory>

namespace tinyrefl::detail {
	// remove const volitale and reference
	template <typename T>
	using remove_cvref_t = ::std::remove_cv_t<::std::remove_reference_t<T>>;

	// is template instant 
	template <template <typename...> typename U, typename T>
	struct is_template_instant_of : ::std::false_type {};

	template <template <typename...> typename U, typename... args>
	struct is_template_instant_of<U, U<args...>> : ::std::true_type {};

	// Check is sequecnce container
	template <typename T>
	inline constexpr bool is_sequence_container_v =
		is_template_instant_of<::std::deque, remove_cvref_t<T>>::value ||
		is_template_instant_of<::std::list, remove_cvref_t<T>>::value ||
		is_template_instant_of<::std::vector, remove_cvref_t<T>>::value;

	// Check is associative container
	template <typename T>
	inline constexpr bool is_associative_container_v =
		is_template_instant_of<::std::map, remove_cvref_t<T>>::value ||
		is_template_instant_of<::std::unordered_map, remove_cvref_t<T>>::value;

	// Check is string 
	template <typename T>
	inline constexpr bool is_string_v = is_template_instant_of<::std::basic_string, remove_cvref_t<T>>::value;

	// Check is array
	template <typename T>
	inline constexpr bool is_array_v = ::std::is_array_v<remove_cvref_t<T>>;

	// Check is char
	template <typename T>
	inline constexpr bool is_char_v = ::std::is_same_v<remove_cvref_t<T>, char> ||
		::std::is_same_v<remove_cvref_t<T>, signed char> ||
		::std::is_same_v<remove_cvref_t<T>, unsigned char>;

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

	// Check is int64_t
	template <typename T>
	inline constexpr bool is_int64_v = ::std::is_same_v<remove_cvref_t<T>, int64_t> || ::std::is_same_v<uint64_t, remove_cvref_t<T>>;

	// Check is float
	template <typename T>
	inline constexpr bool is_floating_v = ::std::is_floating_point_v<remove_cvref_t<T>>;

	// Check is smart pointer (shared_ptr, unique_ptr, weak_ptr)
	template <typename T>
	inline constexpr bool is_smart_pointer_v =
		is_template_instant_of<::std::shared_ptr, remove_cvref_t<T>>::value ||
		is_template_instant_of<::std::unique_ptr, remove_cvref_t<T>>::value ||
		is_template_instant_of<::std::weak_ptr, remove_cvref_t<T>>::value;

} // end namespace tinyrefl::detail

namespace tinyrefl {
	// 标记忽略成员
	template <typename T>
	struct ignore {
		T value{};

		ignore() = default;
		ignore(const T& v) : value(v) {}
		ignore(T&& v) : value(::std::move(v)) {}

		ignore(const ignore&) = default;
		ignore(ignore&&) = default;
		ignore& operator=(const ignore&) = default;
		ignore& operator=(ignore&&) = default;

		ignore& operator=(const T& v) { value = v; return *this; }
		ignore& operator=(T&& v) { value = std::move(v); return *this; }

		ignore(std::nullptr_t) : value(nullptr) {}						// nullptr
		bool operator!() const { return !static_cast<bool>(*this); }	// !
		explicit operator bool() const {								// bool
			if constexpr (requires { static_cast<bool>(value); }) {
				return static_cast<bool>(value);
			}
			else {
				return true;
			}
		}

		// compare operators
		bool operator==(const ignore& other) const { return value == other.value; }
		bool operator!=(const ignore& other) const { return value != other.value; }
		bool operator==(const T& other) const { return value == other; }
		bool operator!=(const T& other) const { return value != other; }
		bool operator==(std::nullptr_t) const {
			if constexpr (requires { value == nullptr; }) {
				return value == nullptr;
			}
			else {
				return false;
			}
		}
		bool operator!=(std::nullptr_t) const { return !(*this == nullptr); }


		T& operator*()& { return value; }
		const T& operator*() const& { return value; }
		T&& operator*()&& { return ::std::move(value); }

		auto operator->() {
			if constexpr (requires { value.operator->(); }) {
				return value.operator->();
			}
			else {
				return &value;
			}
		}

		auto operator->() const {
			if constexpr (requires { value.operator->(); }) {
				return value.operator->();
			}
			else {
				return &value;
			}
		}

		// 隐式转换
		operator T& ()& { return value; }
		operator const T& () const& { return value; }

		// 获取内部值
		T& get()& { return value; }
		const T& get() const& { return value; }
		T&& get()&& { return ::std::move(value); }
	};

	template <typename T>
	using skip = ignore<T>;

} // end namespace tinyrefl

namespace tinyrefl::detail {

	// Check is ignored type
	template <typename T>
	inline constexpr bool is_ignored_v = is_template_instant_of<::tinyrefl::ignore, remove_cvref_t<T>>::value;

	// Check is serializable
	template <typename T>
	inline constexpr bool is_serializable_v = !is_ignored_v<T> && !is_smart_pointer_v<T>;

	template <typename T>
	inline constexpr bool is_custom_type_v = !is_sequence_container_v<remove_cvref_t<T>> &&
		!is_associative_container_v<remove_cvref_t<T>> &&
		!is_string_v<remove_cvref_t<T>> &&
		!is_char_v<remove_cvref_t<T>> &&
		!is_char_pointer_v<remove_cvref_t<T>> &&
		!is_array_v<remove_cvref_t<T>> &&
		!is_int_v<remove_cvref_t<T>> &&
		!is_int64_v<remove_cvref_t<T>> &&
		!is_floating_v<remove_cvref_t<T>> &&
		!is_bool_v<remove_cvref_t<T>> &&
		!is_ignored_v<remove_cvref_t<T>> &&
		!is_smart_pointer_v<remove_cvref_t<T>>;

	template <typename T>
	struct sequence_element_type_impl {
		static_assert(false, "sequence_element_type just be use in sequence container type!");
	};

	template <template <typename, typename...> class Container, typename T, typename... Args>
	struct sequence_element_type_impl<Container<T, Args...>> {
		using type = T;
	};

	// Get Sequence Element Type
	template <typename T>
	using sequence_element_type_t = typename sequence_element_type_impl<T>::type;

	// get members name strings
	template <auto val>
	inline constexpr ::std::string_view get_member_name() {
#if defined(_MSC_VER)
		::std::string_view funcName = __FUNCSIG__;
		size_t begin = funcName.rfind("->") + 2;
		size_t end = funcName.rfind(">(");
		return funcName.substr(begin, end - begin);
#else
		constexpr ::std::string_view funcName = __PRETTY_FUNCTION__;
		constexpr size_t tmpBegin = funcName.find("val = (& ") + 9;
		constexpr size_t tmpEnd = funcName.rfind(");");
		constexpr size_t begin = funcName.substr(tmpBegin, tmpEnd - tmpBegin).rfind("::") + 2;
		return funcName.substr(tmpBegin + begin, tmpEnd - tmpBegin - begin);
#endif
	}


	template <auto index, auto tuple>
	inline constexpr ::std::string_view get_member_name_v = get_member_name<&::std::get<index>(tuple)>();

	// get members type strings
	template <typename T>
	inline consteval ::std::string_view get_member_type_name() {
#if defined(_MSC_VER)
		::std::string_view funcName = __FUNCSIG__;
		size_t start = funcName.find("get_member_type_name<");
		size_t end = funcName.find("(void)");
		return funcName.substr(start + 21, end - start - 22);
#elif defined(__clang__) || defined(__GNUC__)
		::std::string_view funcName = __PRETTY_FUNCTION__;
		// Clang: ::std::string_view get_member_type_name() [T = TypeName]
		size_t start = funcName.find("T = ") + 4;
		size_t end = funcName.find("]", start);
		return funcName.substr(start, end - start);
#elif
		return "Unsupported compiler";
#endif
	}

	template <typename T>
	struct offset_of_member {
		using type = T;
		::std::size_t value;
	};

}