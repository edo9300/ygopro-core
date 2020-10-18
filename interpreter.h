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

class lua_obj;
class card;
class effect;
class group;
class duel;

class interpreter {
public:
	typedef std::unordered_map<int32, lua_State*> coroutine_map;
	typedef std::list<std::pair<lua_Integer, uint32>> param_list;
	
	duel* pduel;
	char msgbuf[64];
	lua_State* lua_state;
	lua_State* current_state;
	param_list params;
	param_list resumes;
	coroutine_map coroutines;
	int32 no_action;
	int32 call_depth;

	explicit interpreter(duel* pd);
	~interpreter();

	int32 register_card(card* pcard);
	void register_effect(effect* peffect);
	void unregister_effect(effect* peffect);
	void register_group(group* pgroup);
	void unregister_group(group* pgroup);
	void register_obj(lua_obj* obj, const char* tablename);

	int32 load_script(const char* buffer, int len = 0, const char* script_name = nullptr);
	int32 load_card_script(uint32 code);
	void add_param(void* param, int32 type, bool front = false);
	void add_param(lua_Integer  param, int32 type, bool front = false);
	void push_param(lua_State* L, bool is_coroutine = false);
	int32 call_function(int32 f, uint32 param_count, int32 ret_count);
	int32 call_card_function(card* pcard, const char* f, uint32 param_count, int32 ret_count, bool forced = true);
	int32 call_code_function(uint32 code, const char* f, uint32 param_count, int32 ret_count);
	int32 check_condition(int32 f, uint32 param_count);
	int32 check_matching(card* pcard, int32 findex, int32 extraargs);
	int32 check_matching_table(card* pcard, int32 findex, int32 table_index);
	int32 get_operation_value(card* pcard, int32 findex, int32 extraargs);
	int32 get_operation_value(card* pcard, int32 findex, int32 extraargs, std::vector<int32>* result);
	int32 get_function_value(int32 f, uint32 param_count);
	int32 get_function_value(int32 f, uint32 param_count, std::vector<int32>* result);
	int32 call_coroutine(int32 f, uint32 param_count, uint32* yield_value, uint16 step);
	int32 clone_function_ref(int32 func_ref);
	void* get_ref_object(int32 ref_handler);

	static void pushobject(lua_State* L, lua_obj* obj);
	static void pushobject(lua_State* L, int32 lua_ptr);
	static int pushExpandedTable(lua_State* L, int32 table_index);
	static int32 get_function_handle(lua_State* L, int32 index);
	static void set_duel_info(lua_State* L, duel* pduel);
	static duel* get_duel_info(lua_State* L);
	static void print_stacktrace(lua_State* L);

	template <size_t N, typename... TR>
	static inline int sprintf(char (&buffer)[N], const char* format, TR&&... args) {
		return std::snprintf(buffer, N, format, std::forward<TR>(args)...);
	}

	template<typename T>
	static inline void lua_table_iterate(lua_State* L, int idx, T&& func) {
		lua_pushnil(L);
		while(lua_next(L, idx) != 0) {
			func();
			lua_pop(L, 1);
		}
	}
};

#define COROUTINE_FINISH	1
#define COROUTINE_YIELD		2
#define COROUTINE_ERROR		3

static_assert(LUA_VERSION_NUM == 503, "Lua 5.3 is required, the core won't work with other lua versions");

#endif /* INTERPRETER_H_ */
