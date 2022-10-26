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
	void check_action_permission(lua_State* L);
	int32_t push_return_cards(lua_State* L, int32_t status, lua_KContext ctx);
	int32_t is_deleted_object(lua_State* L);

	template<typename... Args>
	[[noreturn]] __forceinline void lua_error(Args... args) {
		// std::forward is not used as visual studio 17+ isn't able to optimize
		// it out even with the highest optimization level, the passed parameters are always
		// integers/pointers so there's no loss in passing them by value.
		// __forceinline is used as otherwise clang wouldn't be able to inline this function
		// but we do want it to be inlined
		luaL_error(args...);
		unreachable();
	}

	inline void check_param_count(lua_State* L, int32_t count) {
		if(lua_gettop(L) < count)
			lua_error(L, "%d Parameters are needed.", count);
	}

//Visual Studio raises a warning on const conditional expressions.
//In these templated functions those warnings will be wrongly raised
//so they can be safely disabled
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif

	template<typename T, typename type>
	using EnableIfTemplate = std::enable_if_t<std::is_same<T, type>::value, int>;

	template<typename T>
	using IsBool = std::is_same<T, bool>;

	template<typename T>
	using EnableIfBool = std::enable_if_t<IsBool<T>::value, bool>;

	template<typename T>
	using EnableIfIntegerNotBool = std::enable_if_t<std::integral_constant<bool, std::is_integral<T>::value && !IsBool<T>::value>::value, T>;

	struct function{};

	template<typename T, EnableIfTemplate<T, duel*> = 0>
	inline duel* lua_get(lua_State* L) {
		duel* pduel = nullptr;
		memcpy(&pduel, lua_getextraspace(L), LUA_EXTRASPACE);
		return pduel;
	}

	template<typename T, bool forced = false, EnableIfTemplate<T, function> = 0>
	inline int32_t lua_get(lua_State* L, int idx) {
		if(lua_isnoneornil(L, idx) && !forced)
			return 0;
		check_param(L, PARAM_TYPE_FUNCTION, idx);
		return idx;
	}

	template<typename T, EnableIfTemplate<T, lua_obj*> = 0>
	inline lua_obj* lua_get(lua_State* L, int idx) {
		if(lua_isnone(L, idx))
			return nullptr;
		if(auto obj = lua_touserdata(L, idx)) {
			auto* ret = *static_cast<lua_obj**>(obj);
			if(ret->lua_type == PARAM_TYPE_DELETED)
				lua_error(L, "Attempting to access deleted object.");
			return ret;
		}
		return nullptr;
	}

	template<typename T, bool check = false, EnableIfTemplate<T, card*> = 0>
	inline card* lua_get(lua_State* L, int idx) {
		if(!check && lua_isnone(L, idx))
			return nullptr;
		card* ret = nullptr;
		check_param(L, PARAM_TYPE_CARD, idx, !check, &ret);
		return ret;
	}

	template<typename T, bool check = false, EnableIfTemplate<T, group*> = 0>
	inline group* lua_get(lua_State* L, int idx) {
		if(!check && lua_isnone(L, idx))
			return nullptr;
		group* ret = nullptr;
		check_param(L, PARAM_TYPE_GROUP, idx, !check, &ret);
		return ret;
	}

	template<typename T, bool check = false, EnableIfTemplate<T, effect*> = 0>
	inline effect* lua_get(lua_State* L, int idx) {
		if(!check && lua_isnone(L, idx))
			return nullptr;
		effect* ret = nullptr;
		check_param(L, PARAM_TYPE_EFFECT, idx, !check, &ret);
		return ret;
	}

	template<typename T>
	inline EnableIfBool<T> lua_get(lua_State* L, int idx) {
		check_param(L, PARAM_TYPE_BOOLEAN, idx);
		return lua_toboolean(L, idx);
	}

	template<typename T, bool def>
	inline EnableIfBool<T> lua_get(lua_State* L, int idx) {
		if(!check_param(L, PARAM_TYPE_BOOLEAN, idx, true))
			return def;
		return lua_toboolean(L, idx);
	}

	template<typename T>
	inline EnableIfIntegerNotBool<T> lua_get(lua_State* L, int idx) {
		check_param(L, PARAM_TYPE_INT, idx);
		if(lua_isinteger(L, idx))
			return static_cast<T>(lua_tointeger(L, idx));
		return static_cast<T>(std::round(lua_tonumber(L, idx)));
	}

	template<typename T>
	inline EnableIfIntegerNotBool<T> lua_get(lua_State* L, int idx, T chk) {
		if(lua_isnone(L, idx) || !check_param(L, PARAM_TYPE_INT, idx, true))
			return chk;
		if(lua_isinteger(L, idx))
			return static_cast<T>(lua_tointeger(L, idx));
		return static_cast<T>(std::round(lua_tonumber(L, idx)));
	}

	template<typename T, T def>
	inline EnableIfIntegerNotBool<T> lua_get(lua_State* L, int idx) {
		if(lua_isnone(L, idx) || !check_param(L, PARAM_TYPE_INT, idx, true))
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
			default: break;
			}
		}
		lua_error(L, "Parameter %d should be \"Card\" or \"Group\".", idx);
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

	template<typename T, typename T2>
	using EnableOnReturn = std::enable_if_t<std::is_same<std::result_of_t<T()>, T2>::value, int>;

	template<typename T, EnableOnReturn<T, void> = 0>
	inline void lua_iterate_table_or_stack(lua_State* L, int idx, int max, T&& func) {
		if(lua_istable(L, idx)) {
			lua_table_iterate(L, idx, func);
			return;
		}
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
		static_assert(std::is_same<std::result_of_t<T()>, bool>::value, "Callback function must return bool");
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
		auto ref = lua_get<int32_t>(L, 1);
		lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
		auto obj = lua_get<T*>(L, -1);
		if(!obj) {
			if(std::is_same<T, card>::value)
				lua_error(L, "Parameter 1 should be a lua reference to a Card.");
			else if(std::is_same<T, group>::value)
				lua_error(L, "Parameter 1 should be a lua reference to a Group.");
			else if(std::is_same<T, effect>::value)
				lua_error(L, "Parameter 1 should be a lua reference to an Effect.");
		}
		return 1;
	}
}

#endif /* SCRIPTLIB_H_ */
