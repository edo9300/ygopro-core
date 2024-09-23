/*
 * Copyright (c) 2016-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef SCRIPTLIB_H_
#define SCRIPTLIB_H_

#include <cmath> //std::round
#include <cstdint>
#include <cstring> //std::memcpy
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <type_traits> //std::is_same_v, std::enable_if_t, std::invoke_result_t, std::result_of_t, std::conditional_t
#include <utility> //std::pair
#include "common.h"
#include "lua_obj.h"

class card;
class effect;
class group;
class duel;

static_assert(LUA_VERSION_NUM == 503 || LUA_VERSION_NUM == 504, "Lua 5.3 or 5.4 is required, the core won't work with other lua versions");
static_assert(LUA_MAXINTEGER >= INT64_MAX, "Lua has to support 64 bit integers");
static_assert(LUA_EXTRASPACE >= sizeof(duel*), "LUA_EXTRASPACE needs to be big enough to hold a pointer to the duel object");

namespace scriptlib {
	void push_card_lib(lua_State* L);
	void push_effect_lib(lua_State* L);
	void push_group_lib(lua_State* L);
	void push_duel_lib(lua_State* L);
	void push_debug_lib(lua_State* L);
	void check_action_permission(lua_State* L);
	int32_t push_return_cards(lua_State* L, int32_t status, lua_KContext ctx);
	inline int32_t push_return_cards(lua_State* L, bool cancelable) {
		return lua_yieldk(L, 0, (lua_KContext)cancelable, push_return_cards);
	}
	int32_t is_deleted_object(lua_State* L);

#define lua_error(...) do { luaL_error(__VA_ARGS__); unreachable(); } while(0)

	inline void check_param_count(lua_State* L, int32_t count) {
		if(lua_gettop(L) < count)
			lua_error(L, "%d Parameters are needed.", count);
	}

	using function = struct {}*;

	template<typename T, typename type>
	using EnableIfTemplate = std::enable_if_t<std::is_same_v<T, type>, int>;

	template<typename T>
	inline constexpr bool IsBool = std::is_same_v<T, bool>;

	template<typename T>
	inline constexpr bool IsInteger = !IsBool<T> && (std::is_integral_v<T> || std::is_enum_v<T>);

	template<typename T>
	inline constexpr bool IsCard = std::is_same_v<T, card*>;

	template<typename T>
	inline constexpr bool IsGroup = std::is_same_v<T, group*>;

	template<typename T>
	inline constexpr bool IsEffect = std::is_same_v<T, effect*>;

	template<typename T>
	inline constexpr bool IsFunction = std::is_same_v<T, function>;

	template<typename T>
	using EnableIfIntegral = std::enable_if_t<IsInteger<T> || IsBool<T>, T>;

	template<LuaParam param_type, bool retfalse = false, typename ReturnType = std::conditional_t<retfalse, bool, void>>
	inline ReturnType check_param(lua_State* L, int32_t index, [[maybe_unused]] lua_obj** retobj = nullptr);

	const char* get_lua_type_name(lua_State* L, int32_t index);

	template<typename T>
	inline constexpr auto get_lua_param_type() {
		if constexpr(IsCard<T>)
			return LuaParam::CARD;
		else if constexpr(IsGroup<T>)
			return LuaParam::GROUP;
		else if constexpr(IsEffect<T>)
			return LuaParam::EFFECT;
		else if constexpr(IsFunction<T>)
			return LuaParam::FUNCTION;
		else if constexpr(IsBool<T>)
			return LuaParam::BOOLEAN;
		else if constexpr(IsInteger<T>)
			return LuaParam::INT;
	}

	template<LuaParam param>
	static constexpr const char* get_lua_param_name() {
		if constexpr(param == LuaParam::INT)
			return "Int";
		else if constexpr(param == LuaParam::STRING)
			return "String";
		else if constexpr(param == LuaParam::CARD)
			return "Card";
		else if constexpr(param == LuaParam::GROUP)
			return "Group";
		else if constexpr(param == LuaParam::EFFECT)
			return "Effect";
		else if constexpr(param == LuaParam::FUNCTION)
			return "Function";
		else if constexpr(param == LuaParam::BOOLEAN)
			return "boolean";
		else if constexpr(param == LuaParam::DELETED)
			return "Deleted";
	}

	template<typename T, EnableIfTemplate<T, duel*> = 0>
	inline duel* lua_get(lua_State* L) {
		duel* pduel = nullptr;
		std::memcpy(&pduel, lua_getextraspace(L), sizeof(duel*));
		return pduel;
	}

	template<typename T, bool forced = false, EnableIfTemplate<T, function> = 0>
	inline int32_t lua_get(lua_State* L, int idx) {
		if constexpr(!forced) {
			if(lua_isnoneornil(L, idx))
				return 0;
		}
		check_param<LuaParam::FUNCTION>(L, idx);
		return idx;
	}

	template<typename T, EnableIfTemplate<T, lua_obj*> = 0>
	inline lua_obj* lua_get(lua_State* L, int idx) {
		if(lua_isnoneornil(L, idx))
			return nullptr;
		if(auto obj = lua_touserdata(L, idx)) {
			auto* ret = *static_cast<lua_obj**>(obj);
			if(ret->lua_type == LuaParam::DELETED)
				lua_error(L, "Attempting to access deleted object.");
			return ret;
		}
		return nullptr;
	}

	template<typename T, bool forced = false, std::enable_if_t<IsCard<T> || IsGroup<T> || IsEffect<T>, int> = 0>
	inline T lua_get(lua_State* L, int idx) {
		if constexpr(!forced) {
			if(lua_isnoneornil(L, idx))
				return nullptr;
		}
		lua_obj* ret = nullptr;
		check_param<get_lua_param_type<T>(), !forced>(L, idx, &ret);
		return reinterpret_cast<T>(ret);
	}

	template<typename T>
	inline EnableIfIntegral<T> lua_get(lua_State* L, int idx) {
		constexpr auto lua_type = get_lua_param_type<T>();
		check_param<lua_type>(L, idx);
		if constexpr(lua_type == LuaParam::BOOLEAN) {
			return static_cast<bool>(lua_toboolean(L, idx));
		} else if constexpr(lua_type == LuaParam::INT) {
			if(lua_isinteger(L, idx))
				return static_cast<T>(lua_tointeger(L, idx));
			return static_cast<T>(static_cast<lua_Integer>(std::round(lua_tonumber(L, idx))));
		}
	}

	template<typename T>
	inline EnableIfIntegral<T> lua_get(lua_State* L, int idx, T def) {
		constexpr auto lua_type = get_lua_param_type<T>();
		if(!check_param<lua_type, true>(L, idx))
			return def;
		if constexpr(lua_type == LuaParam::BOOLEAN) {
			return static_cast<bool>(lua_toboolean(L, idx));
		} else if constexpr(lua_type == LuaParam::INT) {
			if(lua_isinteger(L, idx))
				return static_cast<T>(lua_tointeger(L, idx));
			return static_cast<T>(static_cast<lua_Integer>(std::round(lua_tonumber(L, idx))));
		}
	}

	template<typename T, T def>
	inline EnableIfIntegral<T> lua_get(lua_State* L, int idx) {
		return lua_get<T>(L, idx, def);
	}

	template<bool nil_allowed = false>
	inline std::pair<card*, group*> lua_get_card_or_group(lua_State* L, int idx) {
		if constexpr(nil_allowed) {
			if(lua_isnoneornil(L, idx))
				return { nullptr, nullptr };
		}
		auto obj = lua_get<lua_obj*>(L, idx);
		if(obj) {
			switch(obj->lua_type) {
			case LuaParam::CARD:
				return { reinterpret_cast<card*>(obj), nullptr };
			case LuaParam::GROUP:
				return{ nullptr, reinterpret_cast<group*>(obj) };
			default: break;
			}
		}
		lua_error(L, R"(Parameter %d should be "Card" or "Group" but is "%s".)", idx, get_lua_type_name(L, idx));
	}
	//always return a string, whereas lua might return nullptr
	inline const char* lua_get_string_or_empty(lua_State* L, int idx) {
		size_t retlen = 0;
		auto str = lua_tolstring(L, idx, &retlen);
		return (!str || retlen == 0) ? "" : str;
	}

	template<typename T>
	inline void lua_table_iterate(lua_State* L, int idx, T&& func) {
		lua_pushnil(L);
		while(lua_next(L, idx) != 0) {
			func();
			lua_pop(L, 1);
		}
	}

#if (!defined(_LIBCPP_VERSION) || _LIBCPP_VERSION >= 7000) && \
	((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
	template<typename T, typename... Arg>
	using FunctionResult = std::invoke_result_t<T, Arg...>;
#else
	template<typename T, typename... Arg>
	using FunctionResult = std::result_of_t<T(Arg...)>;
#endif

	template<typename T, typename T2>
	using EnableOnReturn = std::enable_if_t<std::is_same<FunctionResult<T>, T2>::value, int>;

	template<typename T, EnableOnReturn<T, void> = 0>
	inline void lua_iterate_table_or_stack(lua_State* L, int idx, int max, T&& func) {
		if(lua_istable(L, idx))
			return lua_table_iterate(L, idx, func);
		for(; idx <= max; ++idx) {
			lua_pushvalue(L, idx);
			func();
			lua_pop(L, 1);
		}
	}

	template<typename T, EnableOnReturn<T, int> = 0>
	inline void lua_iterate_table_or_stack(lua_State* L, int idx, int max, T&& func) {
		if(lua_istable(L, idx)) {
			lua_pushnil(L);
			while(lua_next(L, idx) != 0) {
				const auto pushed_amount = func();
				if(pushed_amount != 0) {
					//if values are pushed, we need to fix the stack so that the values pushed are
					//put below the key/pair used to iterate the current table
					lua_remove(L, -(pushed_amount + 1));
					lua_rotate(L, -(pushed_amount + 1), pushed_amount);
				}
				else
					lua_pop(L, 1);
			}
			return;
		}
		for(; idx <= max; ++idx) {
			lua_pushvalue(L, idx);
			const auto pushed_amount = func();
			if(pushed_amount != 0)
				lua_remove(L, -(pushed_amount + 1));
			else
				lua_pop(L, 1);
		}
	}

	template<typename T>
	inline bool lua_find_in_table_or_in_stack(lua_State* L, int idx, int max, T&& func) {
		static_assert(std::is_same_v<FunctionResult<T>, bool>, "Callback function must return bool");
		if(lua_istable(L, idx)) {
			lua_pushnil(L);
			while(lua_next(L, idx) != 0) {
				const auto element_found = func();
				lua_pop(L, 1);
				if(element_found) {
					//pops table key from the stack
					lua_pop(L, 1);
					return true;
				}
			}
			return false;
		}
		for(; idx <= max; ++idx) {
			lua_pushvalue(L, idx);
			const auto element_found = func();
			lua_pop(L, 1);
			if(element_found)
				return true;
		}
		return false;
	}
	template<typename T>
	static int32_t get_lua_ref(lua_State* L) {
		lua_pushinteger(L, lua_get<T*>(L, 1)->ref_handle);
		return 1;
	}
	template<typename T>
	static int32_t from_lua_ref(lua_State* L) {
		static_assert(IsCard<T*> || IsGroup<T*> || IsEffect<T*>);
		auto ref = lua_get<int32_t>(L, 1);
		lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
		auto obj = lua_get<T*>(L, -1);
		if(!obj) {
			if constexpr(IsCard<T*>)
				lua_error(L, "Parameter 1 should be a lua reference to a Card.");
			else if constexpr(IsGroup<T*>)
				lua_error(L, "Parameter 1 should be a lua reference to a Group.");
			else if constexpr(IsEffect<T*>)
				lua_error(L, "Parameter 1 should be a lua reference to an Effect.");
		}
		return 1;
	}

	template<LuaParam param_type, bool retfalse, typename ReturnType>
	inline ReturnType check_param(lua_State* L, int32_t index, [[maybe_unused]] lua_obj** retobj) {
		if constexpr(param_type == LuaParam::CARD || param_type == LuaParam::GROUP || param_type == LuaParam::EFFECT) {
			if(auto* obj = lua_get<lua_obj*>(L, index); obj != nullptr && obj->lua_type == param_type) {
				if(retobj)
					*retobj = obj;
				return static_cast<ReturnType>(true);
			}
		} else if constexpr(param_type == LuaParam::FUNCTION) {
			if(lua_type(L, index) == LUA_TFUNCTION)
				return static_cast<ReturnType>(true);
		} else if constexpr(param_type == LuaParam::STRING) {
			if(lua_type(L, index) == LUA_TSTRING)
				return static_cast<ReturnType>(true);
		} else if constexpr(param_type == LuaParam::INT) {
			if(lua_type(L, index) == LUA_TNUMBER)
				return static_cast<ReturnType>(true);
		} else if constexpr(param_type == LuaParam::BOOLEAN) {
			//we're really fine with anything as long as it's something
			if(lua_type(L, index) != LUA_TNONE)
				return static_cast<ReturnType>(true);
		}
		if constexpr(retfalse)
			return false;
		else
			lua_error(L, R"(Parameter %d should be "%s" but is "%s".)", index, get_lua_param_name<param_type>(), get_lua_type_name(L, index));
	}
}

#define yieldk(...) lua_yieldk(L, 0, 0, [](lua_State* L, int32_t status, lua_KContext ctx) -> int {\
	(void)status; \
	(void)ctx; \
	auto pduel = lua_get<duel*>(L); \
	(void)pduel; \
	do __VA_ARGS__ while(0); \
	unreachable(); \
})

#define yield() lua_yield(L, 0)

#define ensure_luaL_stack(func,L,...) [&](){ luaL_checkstack(L, 5, nullptr); return func(L, __VA_ARGS__); }()

#endif /* SCRIPTLIB_H_ */
