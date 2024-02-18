/*
 * Copyright (c) 2022-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef FUNCTION_ARRAY_HELPER_H
#define FUNCTION_ARRAY_HELPER_H

#define MAKE_LUA_NAME_IMPL(module, name) c_lua_##module##_##name
#define MAKE_LUA_NAME(module, name) MAKE_LUA_NAME_IMPL(module, name)

#ifndef __INTELLISENSE__
#include <array>
#include <lauxlib.h>
#include <type_traits>
#include "common.h"

//use forceinline only in release builds
#if defined(_DEBUG) || (!defined(_MSC_VER) && !defined(__OPTIMIZE__))
#define LUA_INLINE NoInline
#else
#define LUA_INLINE ForceInline
#endif

namespace {
namespace Detail {
static constexpr auto COUNTER_OFFSET = __COUNTER__ + 1;
template<std::size_t N>
struct LuaFunction;

template<std::size_t... I>
constexpr auto make_lua_functions_array(std::index_sequence<I...> seq) {
	return std::array<luaL_Reg, seq.size() + 1>{Detail::LuaFunction<I>::elem..., luaL_Reg{ nullptr, nullptr }};
}

} // namespace Detail
} // namespace

#define LUA_STATIC_FUNCTION(name) \
static LUA_INLINE int32_t MAKE_LUA_NAME(LUA_MODULE,name)(lua_State* const, duel* const); \
template<> \
struct Detail::LuaFunction<__COUNTER__ - Detail::COUNTER_OFFSET> { \
	static int32_t call(lua_State* L) { \
		return MAKE_LUA_NAME(LUA_MODULE,name)(L, lua_get<duel*>(L)); \
	} \
	static constexpr luaL_Reg elem{#name, call}; \
}; \
static LUA_INLINE int32_t MAKE_LUA_NAME(LUA_MODULE,name)([[maybe_unused]] lua_State* const L, [[maybe_unused]] duel* const pduel)

#define LUA_FUNCTION(name) \
static LUA_INLINE int32_t MAKE_LUA_NAME(LUA_MODULE,name)(lua_State* const, LUA_CLASS* const, duel* const); \
template<> \
struct Detail::LuaFunction<__COUNTER__ - Detail::COUNTER_OFFSET> { \
	static int32_t call(lua_State* L) { \
		return MAKE_LUA_NAME(LUA_MODULE,name)(L, lua_get<LUA_CLASS*, true>(L, 1), lua_get<duel*>(L)); \
	} \
	static constexpr luaL_Reg elem{#name, call}; \
}; \
static LUA_INLINE int32_t MAKE_LUA_NAME(LUA_MODULE,name)([[maybe_unused]] lua_State* const L, \
	[[maybe_unused]] LUA_CLASS* const self, [[maybe_unused]] duel* const pduel)

#define GET_LUA_FUNCTIONS_ARRAY() \
	Detail::make_lua_functions_array(std::make_index_sequence<__COUNTER__ - Detail::COUNTER_OFFSET>())


#define LUA_FUNCTION_EXISTING(name,...) \
template<> \
struct Detail::LuaFunction<__COUNTER__ - Detail::COUNTER_OFFSET> { \
    static constexpr luaL_Reg elem{#name,__VA_ARGS__}; \
}

#define LUA_FUNCTION_ALIAS(name) LUA_DECLARE_ALIAS_INT(name, __COUNTER__)
#define LUA_DECLARE_ALIAS_INT(name, COUNTER) \
template<> \
struct Detail::LuaFunction<COUNTER - Detail::COUNTER_OFFSET> { \
    static constexpr luaL_Reg elem{#name,Detail::LuaFunction<COUNTER - Detail::COUNTER_OFFSET-1>::elem.func}; \
}
#else
#define LUA_FUNCTION(name) static int32_t MAKE_LUA_NAME(LUA_MODULE,name)([[maybe_unused]] lua_State* const L, \
	[[maybe_unused]] LUA_CLASS* const self, [[maybe_unused]] duel* const pduel)
#define LUA_STATIC_FUNCTION(name) static int32_t MAKE_LUA_NAME(LUA_MODULE,name)([[maybe_unused]] lua_State* const L, [[maybe_unused]] duel* const pduel)
#define LUA_FUNCTION_EXISTING(name,...) struct MAKE_LUA_NAME(LUA_MODULE,name) {}
#define LUA_FUNCTION_ALIAS(name) struct MAKE_LUA_NAME(LUA_MODULE,name) {}
#define GET_LUA_FUNCTIONS_ARRAY() std::array{luaL_Reg{nullptr,nullptr}}
#endif // __INTELLISENSE__
#endif // FUNCTION_ARRAY_HELPER_H
