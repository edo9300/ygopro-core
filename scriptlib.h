/*
 * scriptlib.h
 *
 *  Created on: 2009-1-20
 *      Author: Argon.Sun
 */

#ifndef SCRIPTLIB_H_
#define SCRIPTLIB_H_

#include "common.h"
#include "interpreter.h"
#include "lua_obj.h"

namespace scriptlib {
	void push_card_lib(lua_State* L);
	void push_effect_lib(lua_State* L);
	void push_group_lib(lua_State* L);
	void push_duel_lib(lua_State* L);
	void push_debug_lib(lua_State* L);
	bool check_param(lua_State* L, LuaParamType param_type, int32_t index, bool retfalse = false, void* retobj = nullptr);
	void check_param_count(lua_State* L, int32_t count);
	void check_action_permission(lua_State* L);
	int32_t push_return_cards(lua_State* L, int32_t status, lua_KContext ctx);
	int32_t is_deleted_object(lua_State* L);

//Visual Studio raises a warning on const conditional expressions.
//In these templated functions those warnings will be wrongly raised
//so they can be safely disabled
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif

	template<typename T, typename type>
	using EnableIf = typename std::enable_if_t<std::is_same<T, type>::value, T>;

	template<typename T>
	inline duel* lua_get(lua_State* L) {
		duel* pduel = nullptr;
		memcpy(&pduel, lua_getextraspace(L), LUA_EXTRASPACE);
		return pduel;
	}

	template<typename T>
	EnableIf<T, lua_obj*> lua_get(lua_State* L, int idx) {
		if(lua_gettop(L) < idx)
			return nullptr;
		if(auto obj = lua_touserdata(L, idx)) {
			auto* ret = *reinterpret_cast<lua_obj**>(obj);
			if(ret->lua_type == PARAM_TYPE_DELETED) {
				luaL_error(L, "Attempting to access deleted object.");
				unreachable();
			}
			return ret;
		}
		return nullptr;
	}

	template<typename T, bool check = false>
	EnableIf<T, card*> lua_get(lua_State* L, int idx) {
		if(!check && lua_gettop(L) < idx)
			return nullptr;
		card* ret = nullptr;
		check_param(L, PARAM_TYPE_CARD, idx, !check, &ret);
		return ret;
	}

	template<typename T, bool check = false>
	EnableIf<T, group*> lua_get(lua_State* L, int idx) {
		if(!check && lua_gettop(L) < idx)
			return nullptr;
		group* ret = nullptr;
		check_param(L, PARAM_TYPE_GROUP, idx, !check, &ret);
		return ret;
	}

	template<typename T, bool check = false>
	EnableIf<T, effect*> lua_get(lua_State* L, int idx) {
		if(!check && lua_gettop(L) < idx)
			return nullptr;
		effect* ret = nullptr;
		check_param(L, PARAM_TYPE_EFFECT, idx, !check, &ret);
		return ret;
	}

	template<typename T>
	EnableIf<T, bool> lua_get(lua_State* L, int idx) {
		check_param(L, PARAM_TYPE_BOOLEAN, idx);
		return lua_toboolean(L, idx);
	}

	template<typename T, bool def>
	EnableIf<T, bool> lua_get(lua_State* L, int idx) {
		if(!check_param(L, PARAM_TYPE_BOOLEAN, idx, true))
			return def;
		return lua_toboolean(L, idx);
	}

	template<typename T>
	typename std::enable_if_t<std::is_integral<T>::value && !std::is_same<T, bool>::value, T>
	lua_get(lua_State* L, int idx) {
		check_param(L, PARAM_TYPE_INT, idx);
		if(lua_isinteger(L, idx))
			return static_cast<T>(lua_tointeger(L, idx));
		return static_cast<T>(std::round(lua_tonumber(L, idx)));
	}

	template<typename T>
	typename std::enable_if_t<std::is_integral<T>::value && !std::is_same<T, bool>::value, T>
	lua_get(lua_State* L, int idx, T chk) {
		if(lua_gettop(L) < idx || !check_param(L, PARAM_TYPE_INT, idx, true))
			return chk;
		if(lua_isinteger(L, idx))
			return static_cast<T>(lua_tointeger(L, idx));
		return static_cast<T>(std::round(lua_tonumber(L, idx)));
	}

	template<typename T, T def>
	typename std::enable_if_t<std::is_integral<T>::value && !std::is_same<T, bool>::value, T>
	lua_get(lua_State* L, int idx) {
		if(lua_gettop(L) < idx || !check_param(L, PARAM_TYPE_INT, idx, true))
			return def;
		if(lua_isinteger(L, idx))
			return static_cast<T>(lua_tointeger(L, idx));
		return static_cast<T>(std::round(lua_tonumber(L, idx)));
	}

	#ifdef _MSC_VER
	#pragma warning(pop)
	#endif

	inline void get_card_or_group(lua_State* L, int idx, card*& pcard, group*& pgroup) {
		auto obj = lua_get<lua_obj*>(L, idx);
		if(obj) {
			switch(obj->lua_type) {
			case PARAM_TYPE_CARD:
				pcard = (card*)(obj);
				return;
			case PARAM_TYPE_GROUP:
				pgroup = (group*)(obj);
				return;
			}
		}
		luaL_error(L, "Parameter %d should be \"Card\" or \"Group\".", idx);
		unreachable();
	}
	//always return a string, whereas lua might return nullptr
	inline const char* lua_tostring_or_empty(lua_State* L, int idx) {
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
	template<typename T>
	static int32_t get_lua_ref(lua_State* L) {
		lua_pushinteger(L, lua_get<T*>(L, 1)->ref_handle);
		return 1;
	}
	template<typename T>
	static int32_t from_lua_ref(lua_State* L) {
		auto ref = lua_get<int32_t>(L, 1);
		lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
		auto obj = lua_get<T*>(L, -1);
		if(!obj) {
			if(std::is_same<T, card>::value)
				luaL_error(L, "Parameter 1 should be a lua reference to a Card.");
			else if(std::is_same<T, group>::value)
				luaL_error(L, "Parameter 1 should be a lua reference to a Group.");
			else if(std::is_same<T, effect>::value)
				luaL_error(L, "Parameter 1 should be a lua reference to an Effect.");
			unreachable();
		}
		return 1;
	}
}

#endif /* SCRIPTLIB_H_ */
