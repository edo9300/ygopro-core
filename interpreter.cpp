/*
 * interpreter.cpp
 *
 *  Created on: 2010-4-28
 *      Author: Argon
 */

#include "lua_obj.h"
#include "duel.h"
#include "group.h"
#include "card.h"
#include "effect.h"
#include "scriptlib.h"
#include "interpreter.h"
#include <cmath>
#include <utility> //std::exchange

using namespace scriptlib;

// This function will be used by a lua library built with api check
void ocgcore_lua_api_check(void* L, const char* error_message) {
	auto pduel = lua_get<duel*>(static_cast<lua_State*>(L));
	pduel->handle_message(error_message, OCG_LOG_TYPE_ERROR);
}

interpreter::interpreter(duel* pd, const OCG_DuelOptions& options): coroutines(256), deleted(pd) {
	lua_state = luaL_newstate();
	current_state = lua_state;
	pduel = pd;
	no_action = 0;
	call_depth = 0;
	memcpy(lua_getextraspace(lua_state), &pd, LUA_EXTRASPACE);
	// Open basic and used functionality
	auto open_lib = [L=lua_state](const char* libname, lua_CFunction openf) {
		luaL_requiref(L, libname, openf, 1);
		lua_pop(L, 1);
	};
	open_lib("_G", luaopen_base);
	open_lib(LUA_STRLIBNAME, luaopen_string);
	open_lib(LUA_TABLIBNAME, luaopen_table);
	open_lib(LUA_MATHLIBNAME, luaopen_math);
	if(options.enableUnsafeLibraries != 0)
		open_lib(LUA_IOLIBNAME, luaopen_io);
	else {
		// Remove "dangerous" functions
		auto nil_out = [&](const char* name) {
			lua_pushnil(lua_state);
			lua_setglobal(lua_state, name);
		};
		nil_out("collectgarbage");
		nil_out("dofile");
		nil_out("loadfile");
	}
	// Open all card scripting libs
	scriptlib::push_card_lib(lua_state);
	scriptlib::push_effect_lib(lua_state);
	scriptlib::push_group_lib(lua_state);
	scriptlib::push_duel_lib(lua_state);
	scriptlib::push_debug_lib(lua_state);
}
interpreter::~interpreter() {
	lua_close(lua_state);
}
//creates a pointer to a lua_obj in the lua stack
static inline lua_obj** create_object(lua_State* L) {
	return static_cast<lua_obj**>(lua_newuserdata(L, sizeof(lua_obj*)));
}
void interpreter::register_card(card* pcard) {
	//create a card in by userdata
	luaL_checkstack(lua_state, 1, nullptr);
	luaL_checkstack(current_state, 1, nullptr);
	lua_obj** ppcard = create_object(lua_state);
	*ppcard = pcard;
	pcard->ref_handle = luaL_ref(lua_state, LUA_REGISTRYINDEX);
	//creates the pointer in the main state, then after taking a reference
	//pushes it in the stack of the current_state, as load_card_script push the metatable there
	lua_rawgeti(current_state, LUA_REGISTRYINDEX, pcard->ref_handle);
	//load script
	if(pcard->data.alias && (pcard->data.alias < pcard->data.code + 10) && (pcard->data.code < pcard->data.alias + 10))
		load_card_script(pcard->data.alias);
	else
		load_card_script(pcard->data.code);
	//set metatable of pointer to base script
	lua_setmetatable(current_state, -2);
	lua_pop(current_state, 1);
	//Initial
	if(pcard->data.code) {
		const bool forced = !(pcard->data.type & TYPE_NORMAL) || (pcard->data.type & TYPE_PENDULUM);
		pcard->set_status(STATUS_INITIALIZING, TRUE);
		add_param<PARAM_TYPE_CARD>(pcard);
		call_card_function(pcard, "initial_effect", 1, 0, forced);
		pcard->set_status(STATUS_INITIALIZING, FALSE);
	}
	pcard->cardid = pduel->game_field->infos.card_id++;
}
static inline void remove_object(lua_State* L, lua_obj* obj, lua_obj* replacement) {
	if(!obj || !obj->ref_handle)
		return;
	lua_rawgeti(L, LUA_REGISTRYINDEX, obj->ref_handle);
	lua_obj** lobj = static_cast<lua_obj**>(lua_touserdata(L, -1));
	if(lobj)
		*lobj = replacement;
	lua_pop(L, 1);
	luaL_unref(L, LUA_REGISTRYINDEX, obj->ref_handle);
	obj->ref_handle = 0;
}
void interpreter::register_effect(effect* peffect) {
	register_obj(peffect, "Effect");
}
void interpreter::unregister_effect(effect* peffect) {
	if (!peffect)
		return;
	if(peffect->condition)
		luaL_unref(lua_state, LUA_REGISTRYINDEX, peffect->condition);
	if(peffect->cost)
		luaL_unref(lua_state, LUA_REGISTRYINDEX, peffect->cost);
	if(peffect->target)
		luaL_unref(lua_state, LUA_REGISTRYINDEX, peffect->target);
	if(peffect->operation)
		luaL_unref(lua_state, LUA_REGISTRYINDEX, peffect->operation);
	if(peffect->value && peffect->is_flag(EFFECT_FLAG_FUNC_VALUE))
		luaL_unref(lua_state, LUA_REGISTRYINDEX, peffect->value);
	if(peffect->label_object)
		luaL_unref(lua_state, LUA_REGISTRYINDEX, peffect->label_object);
	remove_object(lua_state, peffect, &deleted);
}
void interpreter::register_group(group* pgroup) {
	register_obj(pgroup, "Group");
}
void interpreter::unregister_group(group* pgroup) {
	remove_object(lua_state, pgroup, &deleted);
}
void interpreter::register_obj(lua_obj* obj, const char* tablename) {
	if(!obj)
		return;
	luaL_checkstack(lua_state, 3, nullptr);
	lua_obj** pobj = create_object(lua_state);
	*pobj = obj;
	//set metatable current lua object
	lua_getglobal(lua_state, tablename);
	lua_setmetatable(lua_state, -2);
	//pops the lua object from the stack and takes a reference of it
	obj->ref_handle = luaL_ref(lua_state, LUA_REGISTRYINDEX);
}
bool interpreter::load_script(const char* buffer, int len, const char* script_name) {
	if(!buffer)
		return false;
	++no_action;
	if(luaL_loadbuffer(current_state, buffer, len, script_name) != LUA_OK
	   || lua_pcall(current_state, 0, 0, 0) != LUA_OK) {
		pduel->handle_message(lua_tostring_or_empty(current_state, -1), OCG_LOG_TYPE_ERROR);
		lua_pop(current_state, 1);
		--no_action;
		return false;
	}
	--no_action;
	return true;
}
bool interpreter::load_card_script(uint32_t code) {
	char code_buf[32];
	const char* class_name = format_to(code_buf, "c%u", code);
	luaL_checkstack(current_state, 1, nullptr);
	lua_getglobal(current_state, class_name);
	if(!lua_isnoneornil(current_state, -1))
		return true;
	//if script is not loaded, create and load it
	luaL_checkstack(current_state, 5, nullptr);
	lua_pop(current_state, 1);
	lua_pushinteger(current_state, code);
	lua_setglobal(current_state, "self_code");
	//create a table & set metatable
	lua_createtable(current_state, 0, 0);
	lua_setglobal(current_state, class_name);
	lua_getglobal(current_state, class_name);
	lua_getglobal(current_state, "Card");
	lua_setmetatable(current_state, -2);
	lua_pushstring(current_state, "__index");
	lua_pushvalue(current_state, -2);
	lua_rawset(current_state, -3);
	lua_getglobal(current_state, class_name);
	lua_setglobal(current_state, "self_table");
	const auto res = pduel->read_script(format_to(code_buf, "c%u.lua", code));
	lua_pushnil(current_state);
	lua_setglobal(current_state, "self_table");
	lua_pushnil(current_state);
	lua_setglobal(current_state, "self_code");
	return res;
}
void interpreter::push_param(lua_State* L, bool is_coroutine) {
	int32_t pushed = 0;
	luaL_checkstack(L, params.size(), nullptr);
	for (const auto& it : params) {
		switch(it.second) {
		case PARAM_TYPE_INT:
			lua_pushinteger(L, it.first.integer);
			break;
		case PARAM_TYPE_STRING:
			lua_pushstring(L, static_cast<const char*>(it.first.ptr));
			break;
		case PARAM_TYPE_BOOLEAN:
			lua_pushboolean(L, static_cast<bool>(it.first.integer));
			break;
		case PARAM_TYPE_CARD:
		case PARAM_TYPE_EFFECT:
		case PARAM_TYPE_GROUP:
			pushobject(L, static_cast<lua_obj*>(it.first.ptr));
			break;
		case PARAM_TYPE_FUNCTION:
			pushobject(L, static_cast<int32_t>(it.first.integer));
			break;
		case PARAM_TYPE_INDEX: {
			auto index = static_cast<int32_t>(it.first.integer);
			if(index > 0)
				lua_pushvalue(L, index);
			else if(is_coroutine) {
				//copy value from current_state to new stack
				lua_pushvalue(current_state, index);
				int32_t ref = luaL_ref(current_state, LUA_REGISTRYINDEX);
				lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
				luaL_unref(current_state, LUA_REGISTRYINDEX, ref);
			} else {
				//the calling function is pushed before the params, so the actual index is: index - pushed -1
				lua_pushvalue(L, index - pushed - 1);
			}
			break;
		}
		case PARAM_TYPE_DELETED:
			unreachable();
			break;
		}
		++pushed;
	}
	params.clear();
}
inline int interpreter::call_lua(lua_State* L, int nargs, int nresults) {
	++no_action;
	++call_depth;
	auto ret = lua_pcall(L, nargs, nresults, 0);
	--no_action;
	--call_depth;
	if(call_depth == 0) {
		pduel->release_script_group();
		pduel->restore_assumes();
	}
	return ret;
}
inline bool interpreter::ret_fail(const char* message) {
	interpreter::print_stacktrace(current_state);
	pduel->handle_message(message, OCG_LOG_TYPE_ERROR);
	params.clear();
	return false;
}
inline bool interpreter::ret_fail(const char* message, bool error) {
	if(error) {
		interpreter::print_stacktrace(current_state);
		pduel->handle_message(message, OCG_LOG_TYPE_ERROR);
	}
	params.clear();
	return false;
}
bool interpreter::call_function(int param_count, int ret_count) {
	push_param(current_state);
	auto ret = true;
	if(call_lua(current_state, param_count, ret_count) != LUA_OK) {
		print_stacktrace(current_state);
		pduel->handle_message(lua_tostring_or_empty(current_state, -1), OCG_LOG_TYPE_ERROR);
		lua_pop(current_state, 1);
		ret = false;
	}
	return ret;
}
bool interpreter::call_function(int32_t f, uint32_t param_count, int32_t ret_count) {
	if (!f)
		return ret_fail(R"("CallFunction": attempt to call a null function.)");
	if (param_count != params.size())
		return ret_fail(format(R"("CallFunction": incorrect parameter count (%u expected, %zu pushed))", param_count, params.size()));
	luaL_checkstack(current_state, 1, nullptr);
	pushobject(current_state, f);
	if (!lua_isfunction(current_state, -1))
		return ret_fail(R"("CallFunction": attempt to call an error function)");
	return call_function(param_count, ret_count);
}
bool interpreter::call_card_function(card* pcard, const char* f, uint32_t param_count, int32_t ret_count, bool forced) {
	if (param_count != params.size())
		return ret_fail(format(R"("CallCardFunction"(c%u.%s): incorrect parameter count)", pcard->data.code, f));
	luaL_checkstack(current_state, 1, nullptr);
	pushobject(current_state, pcard);
	lua_getfield(current_state, -1, f);
	if (!lua_isfunction(current_state, -1)) {
		lua_pop(current_state, 2);
		return ret_fail(format(R"("CallCardFunction"(c%u.%s): attempt to call an error function)", pcard->data.code, f), forced);
	}
	lua_remove(current_state, -2);
	return call_function(param_count, ret_count);
}
bool interpreter::call_code_function(uint32_t code, const char* f, uint32_t param_count, int32_t ret_count) {
	if (param_count != params.size())
		return ret_fail(R"("CallCodeFunction": incorrect parameter count)");
	load_card_script(code);
	lua_getfield(current_state, -1, f);
	if (!lua_isfunction(current_state, -1)) {
		lua_pop(current_state, 2);
		return ret_fail(R"("CallCodeFunction": attempt to call an error function)");
	}
	lua_remove(current_state, -2);
	return call_function(param_count, ret_count);
}
bool interpreter::check_condition(int32_t f, uint32_t param_count) {
	if(!f) {
		params.clear();
		return true;
	}
	auto result = false;
	if (call_function(f, param_count, 1)) {
		result = lua_toboolean(current_state, -1);
		lua_pop(current_state, 1);
	}
	return result;
}
bool interpreter::check_matching(card* pcard, int32_t findex, int32_t extraargs) {
	luaL_checkstack(current_state, extraargs + 2, nullptr);
	lua_pushvalue(current_state, findex);
	pushobject(current_state, pcard);
	for(int32_t i = 0; i < extraargs; ++i)
		lua_pushvalue(current_state, (int32_t)(-extraargs - 2));
	auto result = false;
	if(call_lua(current_state, 1 + extraargs, 1) != LUA_OK) {
		print_stacktrace(current_state);
		pduel->handle_message(lua_tostring_or_empty(current_state, -1), OCG_LOG_TYPE_ERROR);
	} else
		result = lua_toboolean(current_state, -1);
	lua_pop(current_state, 1);
	return result;
}
bool interpreter::check_matching_table(card* pcard, int32_t findex, int32_t table_index) {
	if(!findex || !lua_istable(current_state, table_index))
		return true;
	luaL_checkstack(current_state, 2, nullptr);
	lua_pushvalue(current_state, findex);
	pushobject(current_state, pcard);
	int extraargs = pushExpandedTable(current_state, table_index);
	auto result = false;
	if(call_lua(current_state, 1 + extraargs, 1) != LUA_OK) {
		print_stacktrace(current_state);
		pduel->handle_message(lua_tostring_or_empty(current_state, -1), OCG_LOG_TYPE_ERROR);
	} else
		result = lua_toboolean(current_state, -1);
	lua_pop(current_state, 1);
	return result;
}
lua_Integer interpreter::get_operation_value(card* pcard, int32_t findex, int32_t extraargs) {
	if(!findex || lua_isnoneornil(current_state, findex))
		return 0;
	luaL_checkstack(current_state, extraargs + 2, nullptr);
	lua_pushvalue(current_state, findex);
	pushobject(current_state, pcard);
	for(int32_t i = 0; i < extraargs; ++i)
		lua_pushvalue(current_state, (int32_t)(-extraargs - 2));
	lua_Integer result = 0;
	if(call_lua(current_state, 1 + extraargs, 1) != LUA_OK) {
		print_stacktrace(current_state);
		pduel->handle_message(lua_tostring_or_empty(current_state, -1), OCG_LOG_TYPE_ERROR);
	} else
		result = lua_get<lua_Integer>(current_state, -1);
	lua_pop(current_state, 1);
	return result;
}
bool interpreter::get_operation_value(card* pcard, int32_t findex, int32_t extraargs, std::vector<lua_Integer>& result) {
	if(!findex || lua_isnoneornil(current_state, findex))
		return false;
	luaL_checkstack(current_state, extraargs + 2, nullptr);
	lua_pushvalue(current_state, findex);
	int32_t stack_top = lua_gettop(current_state);
	lua_rawgeti(current_state, LUA_REGISTRYINDEX, pcard->ref_handle);
	for(int32_t i = 0; i < extraargs; ++i)
		lua_pushvalue(current_state, (int32_t)(-extraargs - 2));
	auto ret = call_lua(current_state, extraargs, LUA_MULTRET) == LUA_OK;
	if(!ret) {
		print_stacktrace(current_state);
		pduel->handle_message(lua_tostring_or_empty(current_state, -1), OCG_LOG_TYPE_ERROR);
		lua_pop(current_state, 1);
	} else {
		int32_t stack_newtop = lua_gettop(current_state);
		for(int32_t index = stack_top + 1; index <= stack_newtop; ++index) {
			lua_Integer return_value = 0;
			if(lua_isboolean(current_state, index))
				return_value = lua_get<bool>(current_state, index);
			else
				return_value = lua_get<lua_Integer, 0>(current_state, index);
			result.push_back(return_value);
		}
		lua_settop(current_state, stack_top);
		ret = true;
	}
	return ret;
}
lua_Integer interpreter::get_function_value(int32_t f, uint32_t param_count) {
	if(!f) {
		params.clear();
		return 0;
	}
	lua_Integer result = 0;
	if (call_function(f, param_count, 1)) {
		if(lua_isboolean(current_state, -1))
			result = lua_get<bool>(current_state, -1);
		else
			result = lua_get<lua_Integer, 0>(current_state, -1);
		lua_pop(current_state, 1);
	}
	return result;
}
bool interpreter::get_function_value(int32_t f, uint32_t param_count, std::vector<lua_Integer>& result) {
	if(!f) {
		params.clear();
		return false;
	}
	const int32_t stack_top = lua_gettop(current_state);
	auto res = call_function(f, param_count, LUA_MULTRET);
	if (res) {
		const int32_t stack_newtop = lua_gettop(current_state);
		for (int32_t index = stack_top + 1; index <= stack_newtop; ++index) {
			lua_Integer return_value = 0;
			if(lua_isboolean(current_state, index))
				return_value = lua_get<bool>(current_state, index);
			else
				return_value = lua_get<lua_Integer, 0>(current_state, index);
			result.push_back(return_value);
		}
		//pops all the results from the stack (lua_pop(current_state, stack_newtop - stack_top))
		lua_settop(current_state, stack_top);
	}
	return res;
}
#if LUA_VERSION_NUM <= 503
namespace {
int lua_resumec(lua_State* L, lua_State* from, int nargs, int* nresults) {
	auto ret = lua_resume(L, from, nargs);
	*nresults = lua_gettop(L);
	return ret;
}
}
#else
#define lua_resumec(state, from, nargs, res) lua_resume(state, from, nargs, res)
#endif
int32_t interpreter::call_coroutine(int32_t f, uint32_t param_count, lua_Integer* yield_value, uint16_t step) {
	if(yield_value)
		*yield_value = 0;
	if (!f)
		return ret_fail(R"("CallCoroutine": attempt to call a null function)");
	if (param_count != params.size())
		return ret_fail(R"("CallCoroutine": incorrect parameter count)");
	auto it = coroutines.find(f);
	lua_State* rthread;
	if (it == coroutines.end()) {
		rthread = lua_newthread(lua_state);
		const auto threadref = luaL_ref(lua_state, LUA_REGISTRYINDEX);
		pushobject(rthread, f);
		if(!lua_isfunction(rthread, -1)) {
			luaL_unref(lua_state, LUA_REGISTRYINDEX, threadref);
			return ret_fail(R"("CallCoroutine": attempt to call an error function)");
		}
		++call_depth;
		auto ret = coroutines.emplace(f, std::make_pair(rthread, threadref));
		it = ret.first;
	} else {
		if(step == 0) {
			auto ref = it->second.second;
			coroutines.erase(it);
			luaL_unref(lua_state, LUA_REGISTRYINDEX, ref);
			--call_depth;
			if(call_depth == 0) {
				pduel->release_script_group();
				pduel->restore_assumes();
			}
			return ret_fail("recursive event trigger detected.");
		}
		rthread = it->second.first;
	}
	push_param(rthread, true);
	int result, nresults;
	{
		auto prev_state = std::exchange(current_state, rthread);
		result = lua_resumec(current_state, prev_state, param_count, &nresults);
		current_state = prev_state;
	}
	if(result == LUA_YIELD)
		return COROUTINE_YIELD;
	if(result != LUA_OK) {
		print_stacktrace(current_state);
		pduel->handle_message(lua_tostring_or_empty(rthread, -1), OCG_LOG_TYPE_ERROR);
	} else if(yield_value) {
		if(nresults == 0)
			*yield_value = 0;
		else if(lua_isboolean(rthread, -1))
			*yield_value = lua_toboolean(rthread, -1);
		else
			*yield_value = static_cast<lua_Integer>(lua_tointeger(rthread, -1));
	}
	auto ref = it->second.second;
	coroutines.erase(it);
	luaL_unref(lua_state, LUA_REGISTRYINDEX, ref);
	--call_depth;
	if(call_depth == 0) {
		pduel->release_script_group();
		pduel->restore_assumes();
	}
	return (result == LUA_OK) ? COROUTINE_FINISH : COROUTINE_ERROR;
}
int32_t interpreter::clone_lua_ref(int32_t lua_ref) {
	lua_rawgeti(current_state, LUA_REGISTRYINDEX, lua_ref);
	return luaL_ref(current_state, LUA_REGISTRYINDEX);
}
void* interpreter::get_ref_object(int32_t ref_handler) {
	if(ref_handler == 0)
		return nullptr;
	lua_rawgeti(current_state, LUA_REGISTRYINDEX, ref_handler);
	auto obj = reinterpret_cast<lua_obj**>(lua_touserdata(current_state, -1));
	void* p = obj ? *obj : nullptr;
	lua_pop(current_state, 1);
	return p;
}
//Convert a pointer to a lua value, +1 -0
void interpreter::pushobject(lua_State* L, lua_obj* obj) {
	if(!obj || obj->ref_handle == 0)
		lua_pushnil(L);
	else
		lua_rawgeti(L, LUA_REGISTRYINDEX, obj->ref_handle);
}
void interpreter::pushobject(lua_State* L, int32_t lua_ptr) {
	if(!lua_ptr)
		lua_pushnil(L);
	else
		lua_rawgeti(L, LUA_REGISTRYINDEX, lua_ptr);
}
//Push all the elements of the table to the stack, +len(table) -0
int interpreter::pushExpandedTable(lua_State* L, int32_t table_index) {
	int extraargs = 0;
	lua_table_iterate(L, table_index, [&extraargs, &L] {
		luaL_checkstack(L, 1, nullptr);
		lua_pushvalue(L, -1);
		lua_insert(L, -3);
		++extraargs;
	});
	return extraargs;
}
int32_t interpreter::get_function_handle(lua_State* L, int32_t index) {
	lua_pushvalue(L, index);
	int32_t ref = luaL_ref(L, LUA_REGISTRYINDEX);
	return ref;
}

void interpreter::print_stacktrace(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	luaL_traceback(L, L, nullptr, 1);
	auto len = lua_rawlen(L, -1);
	/*checks for an empty stack*/
	if(len > sizeof("stack traceback:"))
		pduel->handle_message(lua_tostring_or_empty(L, -1), OCG_LOG_TYPE_FOR_DEBUG);
	lua_pop(L, 1);
}
