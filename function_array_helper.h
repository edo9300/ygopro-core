/*
 * Copyright (c) 2022-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef FUNCTION_ARRAY_HELPER_H
#define FUNCTION_ARRAY_HELPER_H

#define MAKE_LUA_NAME_IMPL(module, name) c_lua_##module##_##name
#define MAKE_LUA_NAME(module, name) MAKE_LUA_NAME_IMPL(module, name)

// if subsequent calls to __COUNTER__ don't produce consecutive integers, that macro is broken
#if !defined(__COUNTER__) || (__COUNTER__ + 0 != __COUNTER__ - 1)
#define HAS_COUNTER 0
#else
#define HAS_COUNTER 1
#endif

#if !defined(__INTELLISENSE__) || !HAS_COUNTER
#include <array>
#include <lauxlib.h>
#if HAS_COUNTER
#include <type_traits> //std::conditional_t
#endif
#include <utility> //std::index_sequence, std::make_index_sequence
#include "common.h"

// use forceinline only in release builds
#if defined(_DEBUG) || (!defined(_MSC_VER) && !defined(__OPTIMIZE__))
#define LUA_INLINE NoInline
#else
#define LUA_INLINE ForceInline
#endif

#define LUA_NAMESPACE

namespace {
namespace LUA_NAMESPACE {
namespace Detail {

template<std::size_t N>
struct LuaFunction {
	static constexpr luaL_Reg elem{ nullptr, nullptr };
	static constexpr bool initialized{ false };
};

#if !HAS_COUNTER
#define COUNTER_MACRO __LINE__
static constexpr auto COUNTER_OFFSET = 0;

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

#define TAG_STRUCT(COUNTER) \
	static constexpr bool initialized{ true }; \
	using prev_element = previous_element_t<COUNTER>; \

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
#define TAG_STRUCT(COUNTER) \
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

} // namespace Detail
} // namespace LUA_NAMESPACE
} // namespace

#define GET_LUA_FUNCTIONS_ARRAY() \
	Detail::make_lua_functions_array<COUNTER_MACRO>()

#define LUA_STATIC_FUNCTION(name) LUA_STATIC_FUNCTION_INT(name, COUNTER_MACRO)

#define LUA_STATIC_FUNCTION_INT(name, COUNTER) \
static LUA_INLINE int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	(lua_State* const, duel* const); \
template<> \
struct Detail::LuaFunction<COUNTER - Detail::COUNTER_OFFSET> { \
	TAG_STRUCT(COUNTER) \
	static int32_t call(lua_State* L) { \
		return MAKE_LUA_NAME(LUA_MODULE,name)(L, lua_get<duel*>(L)); \
	} \
	static constexpr luaL_Reg elem{#name, call}; \
}; \
static LUA_INLINE int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L, [[maybe_unused]] duel* const pduel)

#define LUA_FUNCTION(name) LUA_FUNCTION_INT(name, COUNTER_MACRO)
#define LUA_FUNCTION_INT(name, COUNTER) \
static LUA_INLINE int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	(lua_State* const, LUA_CLASS* const, duel* const); \
template<> \
struct Detail::LuaFunction<COUNTER - Detail::COUNTER_OFFSET> { \
	TAG_STRUCT(COUNTER) \
	static int32_t call(lua_State* L) { \
		return MAKE_LUA_NAME(LUA_MODULE,name)(L, lua_get<LUA_CLASS*, true>(L, 1), lua_get<duel*>(L)); \
	} \
	static constexpr luaL_Reg elem{#name, call}; \
}; \
static LUA_INLINE int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L, \
		[[maybe_unused]] LUA_CLASS* const self, [[maybe_unused]] duel* const pduel)


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
#define LUA_FUNCTION(name) static int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L, [[maybe_unused]] LUA_CLASS* const self, [[maybe_unused]] duel* const pduel)
#define LUA_STATIC_FUNCTION(name) static int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L, [[maybe_unused]] duel* const pduel)
#define LUA_FUNCTION_EXISTING(name,...) struct MAKE_LUA_NAME(LUA_MODULE,name) {}
#define LUA_FUNCTION_ALIAS(name) struct MAKE_LUA_NAME(LUA_MODULE,name) {}
#define GET_LUA_FUNCTIONS_ARRAY() std::array{luaL_Reg{nullptr,nullptr}}
#endif // __INTELLISENSE__
#endif // FUNCTION_ARRAY_HELPER_H
