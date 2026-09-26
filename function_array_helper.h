/*
 * Copyright (c) 2022-2026, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef FUNCTION_ARRAY_HELPER_H
#define FUNCTION_ARRAY_HELPER_H

#define MAKE_LUA_NAME_IMPL(module, name) c_lua_##module##_##name
#define MAKE_LUA_NAME(module, name) MAKE_LUA_NAME_IMPL(module, name)

#if defined(__cpp_impl_reflection) && __has_include(<meta>)
#define USE_REFLECTION 1
#define HAS_COUNTER 0
#else
#define USE_REFLECTION 0
#endif

#if !USE_REFLECTION
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-const-variable"
#if __clang_major__ >= 22
 // in clang 22, the usage of the __COUNTER__ macro is now diagnosed as c2y extension and a warning is raised
#pragma GCC diagnostic ignored "-Wc2y-extensions"
#endif
#endif
// __COUNTER__ is a nonstandard extension, check if it's present and properly working,
// if that's not the case, fall back to an ISO C++ implementation using the standard
// __LINE__ preprocessor macro to do the lua function magic
#if !defined(__COUNTER__) || (__COUNTER__ + 0 != __COUNTER__ - 1)
#define HAS_COUNTER 0
#else
#define HAS_COUNTER 1
#endif
#endif

#if !defined(__INTELLISENSE__) || USE_REFLECTION || !HAS_COUNTER
#if USE_REFLECTION
#include <algorithm>
#include <inplace_vector>
#include <meta>
#include <ranges>
#include <vector>
#else
#include <type_traits> //std::conditional_t
#endif
#include <array>
#include <lauxlib.h>
#include <string_view>
#include <tuple>
#include <utility> //std::index_sequence, std::make_index_sequence, std::tuple_size_v
#include "scriptlib.h"
#include "type_traits_utilities.h"

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

#if USE_REFLECTION
#define LUA_NAMESPACE lua_functions
#else
#define LUA_NAMESPACE
#endif

namespace {
namespace LUA_NAMESPACE {

using scriptlib::Nil;

namespace Detail {

#if USE_REFLECTION

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define COUNTER_MACRO 0

#else

template<std::size_t N>
struct LuaFunction {
	static constexpr luaL_Reg elem{ nullptr, nullptr };
	static constexpr bool initialized{ false };
	static constexpr size_t max_number_of_arguments{ 0 };
	static constexpr const char* lua_name = "";
};

#define TAG_STRUCT(name, COUNTER, ...) \
	static constexpr const char* lua_name = #name; \
	static constexpr size_t max_number_of_arguments = std::tuple_size_v<Detail::get_lua_function_arguments_t<void(*)(__VA_ARGS__)>>; \
	TAG_STRUCT_IMPL(COUNTER)

#define TAG_STRUCT_NO_ARGS(name, COUNTER) \
	static constexpr const char* lua_name = #name; \
	static constexpr size_t max_number_of_arguments = 0; \
	TAG_STRUCT_IMPL(COUNTER)

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

#define TAG_STRUCT_IMPL(COUNTER) \
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
#define TAG_STRUCT_IMPL(COUNTER) \
	using prev_element = std::conditional_t< \
		COUNTER != Detail::COUNTER_OFFSET, \
				/* "COUNTER - Detail::COUNTER_OFFSET - 1" is always evaluated, even if the condition is false, leading \
					to a compilation error when since the value would underflow, work around that */ \
				Detail::LuaFunction<COUNTER - Detail::COUNTER_OFFSET - (1 * COUNTER != Detail::COUNTER_OFFSET)>, \
				/* we instantiate a high specialization so that it's not picked up when iterating but it's still valid to pass around */ \
				Detail::LuaFunction<0xFFFFFF> \
		>;

template<std::size_t... I>
constexpr auto make_lua_functions_array_int(std::index_sequence<I...> seq) {
	return std::array<luaL_Reg, seq.size() + 1>{LuaFunction<I>::elem..., luaL_Reg{ nullptr, nullptr }};
}

template<std::size_t counter>
constexpr auto make_lua_functions_array() {
	return make_lua_functions_array_int(std::make_index_sequence<counter - COUNTER_OFFSET>());
}

#endif // !HAS_COUNTER
#endif // USE_REFLECTION

template <typename Tuple, size_t idx = std::tuple_size_v<Tuple>>
constexpr auto count_trailing_optionals() {
	if constexpr(idx == 0) {
		return 0;
	} else {
		using Arg = std::tuple_element_t<idx - 1, Tuple>;
		if constexpr(!(is_optional_v<Arg> || (is_variant_v<Arg> && is_variant_member_v<Arg, Nil>))) {
			return 0;
		} else {
			return 1 + count_trailing_optionals<Tuple, idx - 1>();
		}
	}
}

template<typename Sig>
struct get_lua_function_arguments;

template<>
struct get_lua_function_arguments<int(*)(lua_State*)> {
	using type = std::tuple<>;
};
template<typename Ret, typename Arg1, typename Arg2, typename... Args>
struct get_lua_function_arguments<Ret(*)(Arg1, Arg2, Args...)> {
	using type = std::tuple<Args...>;
};
template<typename Sig>
using get_lua_function_arguments_t = typename get_lua_function_arguments<Sig>::type;

template<typename tuple, size_t... indices>
static inline decltype(auto) parse_helper([[maybe_unused]] lua_State* L, std::index_sequence<indices...>) {
	using namespace scriptlib;
	// Visual Studio 2017 crashes when using make_tuple with a RangedInteger as last parameter, work around that
#ifdef _MSC_VER
	tuple t;
	((std::get<indices>(t) = get_lua<std::tuple_element_t<indices, tuple>>(L, indices + 1)), ...);
	return t;
#else
	return std::make_tuple(get_lua<std::tuple_element_t<indices, tuple>>(L, indices + 1)...);
#endif
}

template<typename tuple>
static inline decltype(auto) parse_arguments_tuple(lua_State* L) {
	return parse_helper<tuple>(L, std::make_index_sequence<std::tuple_size_v<tuple>>{});
}

} // namespace Detail
} // namespace LUA_NAMESPACE
} // namespace

#if NEEDS_VARIADIC_OVERLOADING

#define __NARG__(...)  __NARG_I_(__VA_ARGS__, __RSEQ_N())
#define __NARG_I_(...) EXPAND(__ARG_N(__VA_ARGS__))
#define __ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, N, ...) N
#define __RSEQ_N() VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, VAR, 1, 0

#define _VFUNC_(name, n) name##n
#define _VFUNC(name, n)  _VFUNC_(name, n)
#define DISPATCH(name, ...) EXPAND(_VFUNC(name, __NARG__(__VA_ARGS__))( __VA_ARGS__ ))

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

#define LUA_FUNCTION_EXISTING(name,...) LUA_FUNCTION_EXISTING_INT(name, COUNTER_MACRO, __VA_ARGS__)

#define LUA_FUNCTION_ALIAS(name) LUA_FUNCTION_ALIAS_INT(name, COUNTER_MACRO)

#if USE_REFLECTION

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define MAKE_LUA_MODULE_IMPL(module) c_lua_##module##_
#define MAKE_LUA_MODULE(module) MAKE_LUA_MODULE_IMPL(module)

#define LUA_PREFIX STR(MAKE_LUA_MODULE(LUA_MODULE))


#define LUA_STATIC_FUNCTION_INT(name, COUNTER, ...) \
[[maybe_unused]] static LUA_INLINE int32_t MAKE_LUA_NAME(LUA_MODULE,name)(__VA_ARGS__)

#define LUA_FUNCTION_ALIAS_INT(name, COUNTER) \
[[maybe_unused]] static lua_alias MAKE_LUA_NAME(LUA_MODULE,name)

#define LUA_FUNCTION_EXISTING_INT(name, COUNTER, ...) \
[[maybe_unused]] [[=existing_lua_function(__VA_ARGS__)]] static int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L)

struct existing_lua_function { lua_CFunction field; };

struct lua_alias { };

struct LuaBaseFunction {
	std::meta::info obj;
	std::meta::info lua_arguments;
	size_t max_number_of_arguments{ 0 };
};

struct LuaFunctionSymbol {
	const char* lua_name = "";
	LuaBaseFunction f;
	int total_overloads;
	bool alias;
};


template<LuaBaseFunction cur_element>
static int32_t call_lua_function(lua_State* L) {
	using namespace scriptlib;
	using namespace LUA_NAMESPACE;
	using lua_function_arguments = typename[:cur_element.lua_arguments:];
	if constexpr(cur_element.max_number_of_arguments == 0) {
		return [:cur_element.obj:](L, lua_get<duel*>(L));
	} else {
		static constexpr int required_args = static_cast<int>(cur_element.max_number_of_arguments) - Detail::count_trailing_optionals<lua_function_arguments>();
		if constexpr(required_args > 0)
			check_param_count(L, required_args);
		return std::apply([:cur_element.obj:],
		  std::tuple_cat(
			  std::make_tuple(L, lua_get<duel*>(L)),
			  Detail::parse_arguments_tuple<lua_function_arguments>(L)
		  )
		);
	}
}

template<std::meta::info Namespace>
consteval auto get_lua_functions() {
	static constexpr auto [lua_symbols, real_functions] = [] {
		std::vector<LuaFunctionSymbol> res;
		// only consider non overloaded functions
		int real_functions = 0;
		{
			template for (constexpr auto symbol : define_static_array(members_of(Namespace, std::meta::access_context::current()))) {
				if constexpr (has_identifier(symbol)) {
					constexpr auto identifier = identifier_of(symbol);
					if constexpr(identifier.starts_with(LUA_PREFIX)) {
						constexpr auto lua_name = define_static_string(identifier.substr(std::string_view{LUA_PREFIX}.size()));
						if constexpr(is_function(symbol)) {
							using lua_args = LUA_NAMESPACE::Detail::get_lua_function_arguments_t<typename[:add_pointer(type_of(symbol)):]>;
							bool is_overload = !res.empty() && res.back().lua_name == lua_name;
							LuaBaseFunction func {
								.obj = symbol,
								.lua_arguments = ^^lua_args,
								.max_number_of_arguments = std::tuple_size_v<lua_args>,
							};
							int total_overloads = is_overload;
							if (is_overload) {
								total_overloads += res.back().total_overloads;
							}
							real_functions += !is_overload;
							res.emplace_back(lua_name, func, total_overloads, false);
						} else if constexpr(is_variable(symbol)) {
							static_assert(type_of(symbol) == ^^lua_alias);
							LuaBaseFunction func {
								.obj = symbol,
							};
							res.emplace_back(lua_name, func, 0, true);
							++real_functions;
						} else {
							static_assert(false);
						}
					}
				}
			}
		}
		return std::make_pair(define_static_array(res), real_functions);
	}();
	std::inplace_vector<luaL_Reg, real_functions + 1> ret_array{};
    template for (constexpr auto pair : std::views::enumerate(lua_symbols)) {
		constexpr auto obj = std::get<1>(pair);
		constexpr auto identifier = obj.lua_name;
		if constexpr(!obj.alias) {
			constexpr auto annotations = define_static_array(annotations_of_with_type(obj.f.obj, ^^existing_lua_function));
			static_assert(annotations.size() <= 1);
			if constexpr(annotations.size() == 1) {
				constexpr auto annotation = extract<existing_lua_function>(annotations[0]);
				static_assert(annotation.field != nullptr);
				ret_array.push_back(luaL_Reg{identifier, annotation.field});
			} else {
				if constexpr(obj.total_overloads) {
					constexpr auto idx = std::get<0>(pair);
					static constexpr auto arr = lua_symbols.subspan(idx - obj.total_overloads, obj.total_overloads);
					static_assert(
						std::ranges::is_sorted(arr, [](const auto& obj1, const auto& obj2) {
							return obj1.f.max_number_of_arguments < obj2.f.max_number_of_arguments;
						})
						&& obj.f.max_number_of_arguments > arr.back().f.max_number_of_arguments,
						"Overloaded functions must be declared in order from the one with less arguments to the one with most"
					);
					auto* func = +[](lua_State* L) -> int32_t {
						size_t argnum = lua_gettop(L);
						lua_CFunction func = call_lua_function<obj.f>;
						template for(constexpr auto elem : std::ranges::views::reverse(arr)) {
							if (argnum <= elem.f.max_number_of_arguments) {
								func = call_lua_function<elem.f>;
							}
						}

						return func(L);
					};
					ret_array.back() = luaL_Reg{identifier, func};
				} else {
					ret_array.push_back(luaL_Reg{identifier, call_lua_function<obj.f>});
				}
			}
		} else {
			ret_array.push_back(luaL_Reg{identifier, ret_array.back().func}); // if this fail, an alias was declared withuout a prior function being declared
		}
    }
	ret_array.push_back(luaL_Reg{nullptr,nullptr});
	return ret_array;
}

#undef LUA_PREFIX
#undef STR
#undef STR_HELPER
#undef MAKE_LUA_MODULE_IMPL
#undef MAKE_LUA_MODULE

#define GET_LUA_FUNCTIONS_ARRAY() get_lua_functions<^^LUA_NAMESPACE>()

#else

#define GET_LUA_FUNCTIONS_ARRAY() \
	LUA_NAMESPACE::Detail::make_lua_functions_array<COUNTER_MACRO>()

template<auto* function_ptr, bool is_overload, typename previous_element, auto* prev_function_ptr>
static int32_t call_lua_function(lua_State* L) {
	using namespace scriptlib;
	using lua_function_arguments = Detail::get_lua_function_arguments_t<decltype(function_ptr)>;
	static constexpr auto max_number_of_arguments = std::tuple_size_v<lua_function_arguments>;
	if constexpr(is_overload) {
		static_assert(max_number_of_arguments != previous_element::max_number_of_arguments,
					  "Cannot have overloaded functions take the same number of arguments");
		static_assert(max_number_of_arguments > previous_element::max_number_of_arguments,
					  "Overloaded functions must be declared in order from the one with less arguments to the one with most");
		auto argnum = lua_gettop(L);
		if(argnum <= static_cast<int>(previous_element::max_number_of_arguments))
			return prev_function_ptr(L);
	}
	if constexpr(max_number_of_arguments == 0) {
		return function_ptr(L, lua_get<duel*>(L));
	} else {
		static constexpr int required_args = static_cast<int>(max_number_of_arguments) - Detail::count_trailing_optionals<lua_function_arguments>();
		if constexpr(required_args > 0)
			check_param_count(L, required_args);
		return std::apply(function_ptr,
		  std::tuple_cat(
			  std::make_tuple(L, lua_get<duel*>(L)),
			  Detail::parse_arguments_tuple<lua_function_arguments>(L)
		  )
		);
	}
}

#define LUA_STATIC_FUNCTION_INT(name, COUNTER, ...) \
static LUA_INLINE int32_t MAKE_LUA_NAME(LUA_MODULE,name)(__VA_ARGS__); \
template<> \
struct Detail::LuaFunction<COUNTER - Detail::COUNTER_OFFSET> { \
	TAG_STRUCT(name, COUNTER, __VA_ARGS__) \
	using lua_function_typedef = int32_t(*)(__VA_ARGS__); \
	static constexpr luaL_Reg elem{lua_name, call_lua_function< \
					/* pick the right overload */ \
					static_cast<lua_function_typedef>(&MAKE_LUA_NAME(LUA_MODULE, name)), \
					std::string_view{lua_name} == std::string_view{prev_element::lua_name}, \
					prev_element, prev_element::elem.func>}; \
}; \
static LUA_INLINE int32_t MAKE_LUA_NAME(LUA_MODULE,name)(__VA_ARGS__)

#define LUA_FUNCTION_EXISTING_INT(name, COUNTER, ...) \
template<> \
struct Detail::LuaFunction<COUNTER - Detail::COUNTER_OFFSET> { \
	TAG_STRUCT_NO_ARGS(name, COUNTER) \
	static constexpr luaL_Reg elem{#name,__VA_ARGS__}; \
}

#define LUA_FUNCTION_ALIAS_INT(name, COUNTER) \
template<> \
struct Detail::LuaFunction<COUNTER - Detail::COUNTER_OFFSET> { \
	TAG_STRUCT_NO_ARGS(name, COUNTER) \
	static constexpr luaL_Reg elem{lua_name,prev_element::elem.func}; \
}
#endif

#else
#include <string_view>

#define LUA_FUNCTION(name, ...) static int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L, [[maybe_unused]] duel* const pduel, [[maybe_unused]] LUA_CLASS* const self, ##__VA_ARGS__)
#define LUA_STATIC_FUNCTION(name, ...) static int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L, [[maybe_unused]] duel* const pduel, ##__VA_ARGS__)
#define LUA_FUNCTION_EXISTING(name,...) struct MAKE_LUA_NAME(LUA_MODULE,name) {}
#define LUA_FUNCTION_ALIAS(name) struct MAKE_LUA_NAME(LUA_MODULE,name) {}
#define GET_LUA_FUNCTIONS_ARRAY() std::array{luaL_Reg{nullptr,nullptr}}
#endif // __INTELLISENSE__
#endif // FUNCTION_ARRAY_HELPER_H
