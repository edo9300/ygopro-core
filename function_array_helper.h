/*
 * Copyright (c) 2022, Edoardo Lolletti <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef FUNCTION_ARRAY_HELPER_H
#define FUNCTION_ARRAY_HELPER_H

#define MAKE_LUA_NAME_IMPL(module, name) c_lua_##module##_##name
#define MAKE_LUA_NAME(module, name) MAKE_LUA_NAME_IMPL(module, name)

#ifndef __INTELLISENSE__
#include <type_traits>
#include <array>
#include <lauxlib.h>
namespace {
namespace Detail {
template<std::size_t N>
struct LuaFunction;

template<std::size_t... I>
constexpr auto make_lua_functions_array(std::index_sequence<I...> seq) {
	return std::array<luaL_Reg, seq.size() + 1>{Detail::LuaFunction<I>::elem..., luaL_Reg{ nullptr, nullptr }};
}

} // namespace Detail
} // namespace

#define LUA_FUNCTION(name) \
static int32_t MAKE_LUA_NAME(LUA_MODULE,name)(lua_State*); \
template<> \
struct Detail::LuaFunction<__COUNTER__> { \
	static constexpr luaL_Reg elem{#name, MAKE_LUA_NAME(LUA_MODULE,name)}; \
}; \
static int32_t MAKE_LUA_NAME(LUA_MODULE,name)(lua_State* L)

#define GET_LUA_FUNCTIONS_ARRAY() \
	Detail::make_lua_functions_array(std::make_index_sequence<__COUNTER__>());


#define LUA_FUNCTION_EXISTING(name,...) \
template<> \
struct Detail::LuaFunction<__COUNTER__> { \
    static constexpr luaL_Reg elem{#name,__VA_ARGS__}; \
}

#define LUA_FUNCTION_ALIAS(name) LUA_DECLARE_ALIAS_INT(name, __COUNTER__)
#define LUA_DECLARE_ALIAS_INT(name, __COUNTER__) \
template<> \
struct Detail::LuaFunction<__COUNTER__> { \
    static constexpr luaL_Reg elem{#name,Detail::LuaFunction<__COUNTER__-1>::elem.func}; \
}
#else
#define LUA_FUNCTION(name) static int32_t MAKE_LUA_NAME(LUA_MODULE,name)(lua_State* L)
#define LUA_FUNCTION_EXISTING(name,...) struct MAKE_LUA_NAME(LUA_MODULE,name) {}
#define LUA_FUNCTION_ALIAS(name) struct MAKE_LUA_NAME(LUA_MODULE,name) {}
#define GET_LUA_FUNCTIONS_ARRAY() std::array<luaL_Reg,1>{{{nullptr,nullptr}}};
#endif // __INTELLISENSE__
#endif // FUNCTION_ARRAY_HELPER_H
