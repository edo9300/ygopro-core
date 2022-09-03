/*
 * interpreter.h
 *
 *  Created on: 2010-4-28
 *      Author: Argon
 */

#ifndef INTERPRETER_H_
#define INTERPRETER_H_

// Due to longjmp behaviour, we must build Lua as C++ to avoid UB
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "common.h"
#include <unordered_map>
#include <list>
#include <vector>
#include <utility> //std::forward
#include <cstdio>
#include <cstring>
#include <cmath>
#include "lua_obj.h"
#include "ocgapi_types.h"

class card;
class effect;
class group;
class duel;

using lua_invalid = lua_obj_helper<PARAM_TYPE_DELETED>;

class interpreter {
	char msgbuf[128];
public:
	using coroutine_map = std::unordered_map<int32_t, std::pair<lua_State*, int32_t>>;
	union lua_param {
		void* ptr;
		lua_Integer integer;
	};
private:
	void add_param(lua_param param, LuaParamType type, bool front) {
		if(front)
			params.emplace_front(param, type);
		else
			params.emplace_back(param, type);
	}
public:
	using param_list = std::list<std::pair<lua_param, LuaParamType>>;
	
	duel* pduel;
	lua_State* lua_state;
	lua_State* current_state;
	param_list params;
	coroutine_map coroutines;
	int32_t no_action;
	int32_t call_depth;
	lua_invalid deleted;

	explicit interpreter(duel* pd, const OCG_DuelOptions& options);
	~interpreter();

	void register_card(card* pcard);
	void register_effect(effect* peffect);
	void unregister_effect(effect* peffect);
	void register_group(group* pgroup);
	void unregister_group(group* pgroup);
	void register_obj(lua_obj* obj, const char* tablename);

	bool load_script(const char* buffer, int len = 0, const char* script_name = nullptr);
	bool load_card_script(uint32_t code);
	template<LuaParamType type, typename T>
	void add_param(T* param, bool front = false) {
		static_assert(type == PARAM_TYPE_STRING || type == PARAM_TYPE_CARD || type == PARAM_TYPE_GROUP || type == PARAM_TYPE_EFFECT,
					  "Passed parameter type doesn't match provided LuaParamType");
		lua_param p;
		p.ptr = param;
		add_param(p, type, front);
	}
	template<LuaParamType type, typename T>
	void add_param(T param, bool front = false) {
		static_assert(type == PARAM_TYPE_INT || type == PARAM_TYPE_FUNCTION || type == PARAM_TYPE_BOOLEAN || type == PARAM_TYPE_INDEX,
					  "Passed parameter type doesn't match provided LuaParamType");
		lua_param p;
		p.integer = param;
		add_param(p, type, front);
	}
	void push_param(lua_State* L, bool is_coroutine = false);
	bool call_function(int32_t f, uint32_t param_count, int32_t ret_count);
	bool call_card_function(card* pcard, const char* f, uint32_t param_count, int32_t ret_count, bool forced = true);
	bool call_code_function(uint32_t code, const char* f, uint32_t param_count, int32_t ret_count);
	bool check_condition(int32_t f, uint32_t param_count);
	bool check_matching(card* pcard, int32_t findex, int32_t extraargs);
	bool check_matching_table(card* pcard, int32_t findex, int32_t table_index);
	lua_Integer get_operation_value(card* pcard, int32_t findex, int32_t extraargs);
	bool get_operation_value(card* pcard, int32_t findex, int32_t extraargs, std::vector<lua_Integer>& result);
	lua_Integer get_function_value(int32_t f, uint32_t param_count);
	bool get_function_value(int32_t f, uint32_t param_count, std::vector<lua_Integer>& result);
	int32_t call_coroutine(int32_t f, uint32_t param_count, lua_Integer* yield_value, uint16_t step);
	int32_t clone_lua_ref(int32_t lua_ref);
	void* get_ref_object(int32_t ref_handler);
	bool call_function(int param_count, int ret_count);
	inline bool ret_fail(const char* message);
	inline bool ret_fail(const char* message, bool error);
	inline void deepen();
	inline void flatten();

	static void pushobject(lua_State* L, lua_obj* obj);
	static void pushobject(lua_State* L, int32_t lua_ptr);
	static int pushExpandedTable(lua_State* L, int32_t table_index);
	static int32_t get_function_handle(lua_State* L, int32_t index);
	static inline duel* get_duel_info(lua_State* L) {
		duel* pduel;
		memcpy(&pduel, lua_getextraspace(L), LUA_EXTRASPACE);
		return pduel;
	}
	static void print_stacktrace(lua_State* L);

	template <size_t N, typename... TR>
	static inline const char* format_to(char (&out)[N], const char* format, TR&&... args) {
		if(std::snprintf(out, sizeof(out), format, std::forward<TR>(args)...) >= 0)
			return out;
		return "";
	}

	template <typename... TR>
	inline const char* format(const char* format, TR&&... args) {
		return format_to(msgbuf, format, std::forward<TR>(args)...);
	}
};

#define COROUTINE_FINISH	1
#define COROUTINE_YIELD		2
#define COROUTINE_ERROR		3

static_assert(LUA_VERSION_NUM == 503 || LUA_VERSION_NUM == 504, "Lua 5.3 or 5.4 is required, the core won't work with other lua versions");
static_assert(LUA_MAXINTEGER >= INT64_MAX, "Lua has to support 64 bit integers");

#endif /* INTERPRETER_H_ */
