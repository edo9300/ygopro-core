/*
 * Copyright (c) 2022-2026, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef FUNCTION_ARRAY_HELPER_H
#define FUNCTION_ARRAY_HELPER_H

#define MAKE_LUA_NAME_IMPL(module, name) c_lua_##module##_##name
#define MAKE_LUA_NAME(module, name) MAKE_LUA_NAME_IMPL(module, name)

// in clang 22, the usage of the __COUNTER__ macro is now diagnosed as c2y extension and a warning is raised
#if (defined(__clang__) && __clang_major__ >= 22)
#pragma GCC diagnostic ignored "-Wc2y-extensions"
#endif
// __COUNTER__ is a nonstandard extension, check if it's present and properly working,
// if that's not the case, fall back to an ISO C++ implementation using the standard
// __LINE__ preprocessor macro to do the lua function magic
#if !defined(__COUNTER__) || (__COUNTER__ + 0 != __COUNTER__ - 1)
#define HAS_COUNTER 0
#else
#define HAS_COUNTER 1
#endif

#if !defined(__INTELLISENSE__) || !HAS_COUNTER
#include <array>
#include <cmath> //std::round
#include <lauxlib.h>
#include <optional>
#include <type_traits> //std::conditional_t, std::enable_if_t, std::false_type, std::is_same_v, std::true_type
#include <tuple>
#include <utility> //std::index_sequence, std::make_index_sequence
#include <variant>
#include "common.h"
#include "scriptlib.h"

// ISO C++ prohibits calling variadic macros with no arguments, but
// major compilers (GCC, MSVC and clang) have extensions always enabled
// to support this behaviour.
// If no such compiler is detected, fall back to a ISO C++ compliant
// implementation using macro overloading
#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#define NEEDS_VARIADIC_OVERLOADING 0
// under pedantic, variadic macros with 0 arguments are treated as warning
// clang allows disabling that warning explicitly, gcc doesn't, so the only
// solution is to mark this header as system header, so that those warnings
// aren't diagnosed
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#elif defined(__GNUC__)
#pragma GCC system_header
#endif
#else
#define NEEDS_VARIADIC_OVERLOADING 1
#endif

// Works around MSVC preprocessor bugs
#define EXPAND( x ) x

// use forceinline only in release builds
#if defined(_DEBUG) || (!defined(_MSC_VER) && !defined(__OPTIMIZE__))
#define LUA_INLINE NoInline
#else
#define LUA_INLINE ForceInline
#endif

#define LUA_NAMESPACE

namespace {
namespace LUA_NAMESPACE {

struct Function {
	int value;
	operator int() {
		return value;
	}
	operator int() const {
		return value;
	}
};

struct Table {
	int value;
	operator int() {
		return value;
	}
	operator int() const {
		return value;
	}
};

using Nil = struct {}*;

namespace Detail {

template<std::size_t N>
struct LuaFunction {
	static constexpr luaL_Reg elem{ nullptr, nullptr };
	static constexpr bool initialized{ false };
};

#if !HAS_COUNTER
#define COUNTER_MACRO __LINE__
[[maybe_unused]] static constexpr auto COUNTER_OFFSET = 0;

template<size_t offset, size_t total, std::size_t... I>
constexpr size_t find_prev_element(std::index_sequence<I...>) {
	constexpr auto index_to_check = offset - (total - sizeof...(I)) - 1;
	if constexpr(LuaFunction<index_to_check>::initialized)
		return index_to_check;
	else if constexpr(sizeof...(I) <= 1)
		return 0;
	else
		return find_prev_element<offset, total>(std::make_index_sequence<sizeof...(I) - 1>());
}

template<typename T>
constexpr auto count() {
	if constexpr(T::initialized)
		return 1 + count<typename T::prev_element>();
	else
		return 0;
}

template <size_t counter, size_t amount = std::min<size_t>(counter, 200) - 1>
using previous_element_t = LuaFunction<find_prev_element<counter, amount>(std::make_index_sequence<amount>())>;

#define TAG_STRUCT(COUNTER,...) \
	static constexpr bool initialized{ true }; \
	using prev_element = previous_element_t<COUNTER>;

template<typename T, typename Arr>
constexpr auto populate_array([[maybe_unused]] Arr& arr, [[maybe_unused]] size_t idx) {
	if constexpr(T::initialized) {
		arr[idx] = T::elem;
		populate_array<typename T::prev_element>(arr, --idx);
	}
}

template<size_t counter>
constexpr auto make_lua_functions_array() {
	using last_elem = previous_element_t<counter>;
	constexpr auto total = count<last_elem>();
	std::array<luaL_Reg, total + 1> arr{};
	populate_array<last_elem>(arr, arr.size() - 2);
	arr[total] = { nullptr, nullptr };
	return arr;
}

#else

#define COUNTER_MACRO __COUNTER__
static constexpr auto COUNTER_OFFSET = __COUNTER__ + 1;
#define TAG_STRUCT(COUNTER,...) \
	using prev_element = std::conditional_t< \
		COUNTER != Detail::COUNTER_OFFSET, \
				/* "COUNTER - Detail::COUNTER_OFFSET - 1" is always evaluated, even if the condition is false, leading \
					to a compilation error when since the value would underflow, work around that */ \
				Detail::LuaFunction<COUNTER - Detail::COUNTER_OFFSET - (1 * COUNTER != Detail::COUNTER_OFFSET)>, \
				void \
		>;

template<std::size_t... I>
constexpr auto make_lua_functions_array_int(std::index_sequence<I...> seq) {
	return std::array<luaL_Reg, seq.size() + 1>{LuaFunction<I>::elem..., luaL_Reg{ nullptr, nullptr }};
}

template<std::size_t counter>
constexpr auto make_lua_functions_array() {
	return make_lua_functions_array_int(std::make_index_sequence<counter - COUNTER_OFFSET>());
}

#endif

template <typename T, typename Enable = void>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template<typename T>
[[maybe_unused]] inline constexpr bool is_optional_v = is_optional<T>::value;

template <typename T, typename Enable = void>
struct is_variant : std::false_type {};

template <typename... Args>
struct is_variant<std::variant<Args...>> : std::true_type {};

template<typename T>
[[maybe_unused]] inline constexpr bool is_variant_v = is_variant<T>::value;

template<typename VARIANT_T, typename T>
struct is_variant_member : public std::false_type {};

template<typename T, typename... Args>
struct is_variant_member<std::variant<Args...>, T>
	: public std::disjunction<std::is_same<T, Args>...> {};

template<typename variant, typename T>
[[maybe_unused]] inline constexpr bool is_variant_member_v = is_variant_member<variant, T>::value;

template <typename Arg, typename... Args>
constexpr auto count_trailing_optionals(std::tuple<Arg, Args...>) {
	if constexpr(sizeof...(Args) == 0) {
		return static_cast<int>(is_optional_v<Arg> || (is_variant_v<Arg> && is_variant_member_v<Arg, Nil>));
	} else {
		constexpr auto result = count_trailing_optionals(std::tuple<Args...>{});
		if constexpr(result != sizeof...(Args))
			return result;
		else
			return result + static_cast<int>(is_optional_v<Arg> || (is_variant_v<Arg> && is_variant_member_v<Arg, Nil>));
	}
}

template<typename T, std::enable_if_t<!is_variant_v<T>, int> = 0>
inline constexpr T get_lua([[maybe_unused]] lua_State* L, [[maybe_unused]] int idx) {
	using namespace scriptlib;
	// we need to not have type::value_type be evaluated if the type isn't an optional
	auto _ = [](auto a) {
		using type = decltype(a);
		if constexpr(is_optional_v<type>) {
			return typename type::value_type{};
		} else {
			return type{};
		}
	};
	using actual_type = FunctionResult<decltype(_), T>;
	if constexpr(is_optional_v<T>) {
		if(lua_isnoneornil(L, idx))
			return std::nullopt;
	}
	auto return_value = [](auto val) {
		if constexpr(is_optional_v<T>) {
			return std::make_optional<actual_type>(val);
		} else {
			return static_cast<actual_type>(val);
		}
	};
	if constexpr(std::is_same_v<actual_type, lua_obj*>) {
		auto* ret = lua_get<lua_obj*>(L, idx);
		if(!ret) {
			lua_error(L, R"(Parameter %d should be one of "Card", "Group", "Effect" but is "%s".)", idx, get_lua_type_name(L, idx));
		}
		return return_value(ret);
	} else {
		constexpr auto lua_type = [] {
			if constexpr(std::is_same_v<Function, actual_type>)
				return LuaParam::FUNCTION;
			else if constexpr(std::is_same_v<Table, actual_type>)
				return LuaParam::TABLE;
			else
				return get_lua_param_type<actual_type>();
		}();
		if constexpr(lua_type == LuaParam::CARD || lua_type == LuaParam::GROUP || lua_type == LuaParam::EFFECT) {
			lua_obj* ret = nullptr;
			check_param<lua_type>(L, idx, &ret);
			return return_value(reinterpret_cast<actual_type>(ret));
		} else {
			check_param<lua_type>(L, idx);
			if constexpr(lua_type == LuaParam::BOOLEAN) {
				return return_value(static_cast<bool>(lua_toboolean(L, idx)));
			} else if constexpr(lua_type == LuaParam::INT) {
				if(lua_isinteger(L, idx))
					return return_value(static_cast<actual_type>(lua_tointeger(L, idx)));
				return return_value(static_cast<actual_type>(static_cast<lua_Integer>(std::round(lua_tonumber(L, idx)))));
			} else if constexpr(lua_type == LuaParam::FUNCTION) {
				return return_value(Function{ idx });
			} else if constexpr(lua_type == LuaParam::TABLE) {
				return return_value(Table{ idx });
			}
		}
	}
}

template<typename variant_t>
struct check_variant_types_functor;

template<typename... Args>
struct check_variant_types_functor<std::variant<Args...>> {
	constexpr bool operator()(LuaParam lua_type) {
		using namespace scriptlib;
		if constexpr((IsCard<Args> || ...) || (IsLuaObj<Args> || ...)) {
			if(lua_type == LuaParam::CARD)
				return true;
		}
		if constexpr((IsGroup<Args> || ...) || (IsLuaObj<Args> || ...)) {
			if(lua_type == LuaParam::GROUP)
				return true;
		}
		if constexpr((IsEffect<Args> || ...) || (IsLuaObj<Args> || ...)) {
			if(lua_type == LuaParam::EFFECT)
				return true;
		}
		if constexpr((std::is_same_v<Function, Args> || ...)) {
			if(lua_type == LuaParam::FUNCTION)
				return true;
		}
		if constexpr((std::is_same_v<Table, Args> || ...)) {
			if(lua_type == LuaParam::TABLE)
				return true;
		}
		if constexpr((IsBool<Args> || ...)) {
			if(lua_type == LuaParam::BOOLEAN)
				return true;
		}
		if constexpr((IsInteger<Args> || ...)) {
			if(lua_type == LuaParam::INT)
				return true;
		}
		if constexpr((std::is_same_v<Nil, Args> || ...)) {
			if(lua_type == LuaParam::NIL || lua_type == LuaParam::NONE)
				return true;
		}
		return false;		
	}
};

template<typename variant_t>
struct get_variant_type_functor;

template<typename... Args>
struct get_variant_type_functor<std::variant<Args...>> {
	using variant_t = std::variant<Args...>;
	template<typename T>
	static inline constexpr bool is_handled_variant_type = scriptlib::IsCard<T> || scriptlib::IsGroup<T> ||
		scriptlib::IsEffect<T> || scriptlib::IsLuaObj<T> || std::is_same_v<Function, T> ||
		std::is_same_v<Table, T> || scriptlib::IsBool<T> || scriptlib::IsInteger<T> || std::is_same_v<Nil, T>;
	constexpr variant_t operator()(lua_State* L, int idx, LuaParam lua_type) {
		static_assert(((is_handled_variant_type<Args> * 1) +...) == std::variant_size_v<variant_t>, "Unhandled variant type passed");
		using namespace scriptlib;
		if constexpr((IsCard<Args> || ...)) {
			if(lua_type == LuaParam::CARD)
				return reinterpret_cast<card*>(lua_get<lua_obj*>(L, idx));
		}
		if constexpr((IsGroup<Args> || ...)) {
			if(lua_type == LuaParam::GROUP)
				return reinterpret_cast<group*>(lua_get<lua_obj*>(L, idx));
		}
		if constexpr((IsEffect<Args> || ...)) {
			if(lua_type == LuaParam::EFFECT)
				return reinterpret_cast<effect*>(lua_get<lua_obj*>(L, idx));
		}
		if constexpr((IsLuaObj<Args> || ...)) {
			if(lua_type == LuaParam::CARD || lua_type == LuaParam::EFFECT || lua_type == LuaParam::GROUP)
				return lua_get<lua_obj*>(L, idx);
		}
		if constexpr((std::is_same_v<Function, Args> || ...)) {
			if(lua_type == LuaParam::FUNCTION)
				return Function{ idx };
		}
		if constexpr((std::is_same_v<Table, Args> || ...)) {
			if(lua_type == LuaParam::TABLE)
				return Table{ idx };
		}
		if constexpr((IsBool<Args> || ...)) {
			if(lua_type == LuaParam::BOOLEAN)
				return static_cast<bool>(lua_toboolean(L, idx));
		}
		if constexpr((IsInteger<Args> || ...)) {
			static_assert((IsInteger<Args> + ...) <= 1, "Variant must have at most 1 integer type");
			if(lua_type == LuaParam::INT) {
				constexpr auto int_index = [] {
					int i = 0;
					return ((IsInteger<Args> * i++) + ...);
				}();
				using integer_type = std::tuple_element_t<int_index, std::tuple<Args...>>;
				auto val = lua_isinteger(L, idx) ? lua_tointeger(L, idx) : static_cast<lua_Integer>(std::round(lua_tonumber(L, idx)));
				return static_cast<integer_type>(val);
			}
		}
		if constexpr((std::is_same_v<Nil, Args> || ...)) {
			if(lua_type == LuaParam::NIL || lua_type == LuaParam::NONE)
				return Nil{};
		}
		unreachable();
	}
};

template<typename variant_t>
struct get_variant_names_functor;

template<typename... Args>
struct get_variant_names_functor<std::variant<Args...>> {
	constexpr std::array<char, 128> operator()() {
		using namespace scriptlib;
		std::array<char, 128> ret{};
		auto it = ret.begin();
		bool is_first = true;
		auto copy_string = [&](const char* string) {
			if(!is_first) {
				*it++ = ',';
				*it++ = ' ';
			}
			is_first = false;
			*it++ = '"';
			while(*string) {
				*it++ = *string++;
			}
			*it++ = '"';
		};
		if constexpr((IsCard<Args> || ...) || (IsLuaObj<Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::CARD>());
		}
		if constexpr((IsGroup<Args> || ...) || (IsLuaObj<Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::GROUP>());
		}
		if constexpr((IsEffect<Args> || ...) || (IsLuaObj<Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::EFFECT>());
		}
		if constexpr((std::is_same_v<Function, Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::FUNCTION>());
		}
		if constexpr((std::is_same_v<Table, Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::TABLE>());
		}
		if constexpr((IsBool<Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::BOOLEAN>());
		}
		if constexpr((IsInteger<Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::INT>());
		}
		if constexpr((std::is_same_v<Nil, Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::NIL>());
		}
		return ret;
	}
};

template<typename T, std::enable_if_t<is_variant_v<T>, int> = 0>
inline constexpr T get_lua([[maybe_unused]] lua_State* L, [[maybe_unused]] int idx) {
	using namespace scriptlib;
	auto type = get_lua_type(L, idx);
	if(!check_variant_types_functor<T>()(type)) {
		constexpr auto types_string = get_variant_names_functor<T>()();
		static_assert(types_string.back() == '\0');
		lua_error(L, R"(Parameter %d should be one of %s but is "%s".)", idx, types_string.data(), get_lua_type_name(L, idx));
	}
	return get_variant_type_functor<T>()(L, idx, type);
}

template<typename Sig>
struct get_function_arguments;

template<typename Ret, typename... Args>
struct get_function_arguments<Ret(*)(Args...)> {
	using type = std::tuple<Args...>;
};
template<typename Sig>
using get_function_arguments_t = typename get_function_arguments<Sig>::type;

template<typename tuple, size_t tuple_offset, size_t lua_offset, size_t... indices>
constexpr decltype(auto) parse_helper([[maybe_unused]] lua_State* L, std::index_sequence<indices...>) {
	return std::make_tuple(get_lua<std::tuple_element_t<indices + tuple_offset, tuple>>(L, indices + lua_offset + 1)...);
}

template<typename tuple, size_t tuple_offset, size_t lua_offset = 0>
decltype(auto) parse_arguments_tuple(lua_State* L) {
	return parse_helper<tuple, tuple_offset, lua_offset>(L, std::make_index_sequence<std::tuple_size_v<tuple> - tuple_offset>{});
}

} // namespace Detail
} // namespace LUA_NAMESPACE
} // namespace

#define GET_LUA_FUNCTIONS_ARRAY() \
	LUA_NAMESPACE::Detail::make_lua_functions_array<COUNTER_MACRO>()

#if NEEDS_VARIADIC_OVERLOADING

#define __NARG__(...)  __NARG_I_(__VA_ARGS__, __RSEQ_N())
#define __NARG_I_(...) EXPAND(__ARG_N(__VA_ARGS__))
#define __ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, N, ...) N
#define __RSEQ_N() VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, 1, 0

#define _VFUNC_(name, n) name##n
#define _VFUNC(name, n)  _VFUNC_(name, n)
#define DISPATCH(name, ...) EXPAND(_VFUNC(name, __NARG__(__VA_ARGS__))( __VA_ARGS__ ))

#endif

#if NEEDS_VARIADIC_OVERLOADING

#define LUA_STATIC_FUNCTION(...) EXPAND(DISPATCH(LUA_STATIC_FUNCTION_INT, __VA_ARGS__))

#define LUA_STATIC_FUNCTION_INT1(name) LUA_STATIC_FUNCTION_INT(name, COUNTER_MACRO, \
		[[maybe_unused]] lua_State* const L, \
		[[maybe_unused]] duel* const pduel)

#define LUA_STATIC_FUNCTION_INTVAR(name, ...) LUA_STATIC_FUNCTION_INT(name, COUNTER_MACRO, \
		[[maybe_unused]] lua_State* const L, \
		[[maybe_unused]] duel* const pduel, \
		__VA_ARGS__)

#define LUA_FUNCTION(...) EXPAND(DISPATCH(LUA_FUNCTION_INT, __VA_ARGS__))

#define LUA_FUNCTION_INT1(name) LUA_STATIC_FUNCTION_INT(name, COUNTER_MACRO, \
		[[maybe_unused]] lua_State* const L, \
		[[maybe_unused]] duel* const pduel, \
		[[maybe_unused]] LUA_CLASS* const self)

#define LUA_FUNCTION_INTVAR(name, ...) LUA_STATIC_FUNCTION_INT(name, COUNTER_MACRO, \
		[[maybe_unused]] lua_State* const L, \
		[[maybe_unused]] duel* const pduel, \
		[[maybe_unused]] LUA_CLASS* const self, \
		__VA_ARGS__)
#else

#define LUA_STATIC_FUNCTION(...) EXPAND(LUA_STATIC_FUNCTION_INT1(__VA_ARGS__))

#define LUA_STATIC_FUNCTION_INT1(name, ...) LUA_STATIC_FUNCTION_INT(name, COUNTER_MACRO, \
		[[maybe_unused]] lua_State* const L, \
		[[maybe_unused]] duel* const pduel, \
		## __VA_ARGS__)

#define LUA_FUNCTION(...) EXPAND(LUA_FUNCTION_INT1(__VA_ARGS__))

#define LUA_FUNCTION_INT1(name, ...) LUA_STATIC_FUNCTION_INT(name, COUNTER_MACRO, \
		[[maybe_unused]] lua_State* const L, \
		[[maybe_unused]] duel* const pduel, \
		[[maybe_unused]] LUA_CLASS* const self, \
		## __VA_ARGS__)
#endif

template<auto* function_ptr>
static int32_t call_lua_function(lua_State* L) {
	using namespace scriptlib;
	using function_arguments = Detail::get_function_arguments_t<decltype(function_ptr)>;
	static constexpr auto extra_args = 2;
	if constexpr(std::tuple_size_v<function_arguments> == extra_args) {
		return function_ptr(L, lua_get<duel*>(L));
	} else {
		static constexpr int required_args = (static_cast<int>(std::tuple_size_v<function_arguments>) - Detail::count_trailing_optionals(function_arguments{})) - extra_args;
		if constexpr(required_args > 0)
			check_param_count(L, required_args);
		return std::apply(function_ptr,
		  std::tuple_cat(
			  std::make_tuple(L, lua_get<duel*>(L)),
			  Detail::parse_arguments_tuple<function_arguments, extra_args>(L)
		  )
		);
	}
}

#define LUA_STATIC_FUNCTION_INT(name, COUNTER, ...) \
static LUA_INLINE int32_t MAKE_LUA_NAME(LUA_MODULE,name)(__VA_ARGS__); \
template<> \
struct Detail::LuaFunction<COUNTER - Detail::COUNTER_OFFSET> { \
	TAG_STRUCT(COUNTER, __VA_ARGS__) \
	static constexpr luaL_Reg elem{#name, call_lua_function<&MAKE_LUA_NAME(LUA_MODULE, name)>}; \
}; \
static LUA_INLINE int32_t MAKE_LUA_NAME(LUA_MODULE,name)(__VA_ARGS__)

#define LUA_FUNCTION_EXISTING(name,...) LUA_FUNCTION_EXISTING_INT(name, COUNTER_MACRO, __VA_ARGS__)
#define LUA_FUNCTION_EXISTING_INT(name, COUNTER, ...) \
template<> \
struct Detail::LuaFunction<COUNTER - Detail::COUNTER_OFFSET> { \
	TAG_STRUCT(COUNTER) \
	static constexpr luaL_Reg elem{#name,__VA_ARGS__}; \
}

#define LUA_FUNCTION_ALIAS(name) LUA_FUNCTION_ALIAS_INT(name, COUNTER_MACRO)
#define LUA_FUNCTION_ALIAS_INT(name, COUNTER) \
template<> \
struct Detail::LuaFunction<COUNTER - Detail::COUNTER_OFFSET> { \
	TAG_STRUCT(COUNTER) \
	static constexpr luaL_Reg elem{#name,prev_element::elem.func}; \
}
#else
struct Function {
	int value;
	operator int() {
		return value;
	}
	operator int() const {
		return value;
	}
};
struct Table {
	int value;
	operator int() {
		return value;
	}
	operator int() const {
		return value;
	}
};
using Nil = struct {}*;
#define LUA_FUNCTION(name, ...) static int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L, [[maybe_unused]] LUA_CLASS* const self, [[maybe_unused]] duel* const pduel, ##__VA_ARGS__)
#define LUA_STATIC_FUNCTION(name, ...) static int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L, [[maybe_unused]] duel* const pduel, ##__VA_ARGS__)
#define LUA_FUNCTION_EXISTING(name,...) struct MAKE_LUA_NAME(LUA_MODULE,name) {}
#define LUA_FUNCTION_ALIAS(name) struct MAKE_LUA_NAME(LUA_MODULE,name) {}
#define GET_LUA_FUNCTIONS_ARRAY() std::array{luaL_Reg{nullptr,nullptr}}
#endif // __INTELLISENSE__
#endif // FUNCTION_ARRAY_HELPER_H
