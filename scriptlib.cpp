/*
 * scriptlib.cpp
 *
 *  Created on: 2010-7-29
 *      Author: Argon
 */
#include "scriptlib.h"
#include "duel.h"
#include "lua_obj.h"

int32 scriptlib::check_param(lua_State* L, int32 param_type, int32 index, int32 retfalse) {
	const char* type = nullptr;
	switch (param_type) {
	case PARAM_TYPE_CARD:
	case PARAM_TYPE_GROUP:
	case PARAM_TYPE_EFFECT:
		if(lua_isuserdata(L, index) && lua_get<lua_obj*>(L, index)->lua_type == param_type)
			return TRUE;
		type = param_type == PARAM_TYPE_CARD ? "Card" : param_type == PARAM_TYPE_GROUP ? "Group" : "Effect";
		break;
	case PARAM_TYPE_FUNCTION:
		if(lua_isfunction(L, index))
			return TRUE;
		type = "Function";
		break;
	case PARAM_TYPE_STRING:
		if(lua_isstring(L, index))
			return TRUE;
		type = "String";
		break;
	case PARAM_TYPE_INT:
		if(lua_isinteger(L, index) || lua_isnumber(L, index))
			return TRUE;
		type = "Int";
		break;
	case PARAM_TYPE_BOOLEAN:
		if(lua_isboolean(L, index))
			return TRUE;
		type = "boolean";
		break;
	}
	if(retfalse)
		return FALSE;
	luaL_error(L, "Parameter %d should be \"%s\".", index, type);
	return false;
}

int32 scriptlib::check_param_count(lua_State* L, int32 count) {
	if (lua_gettop(L) < count)
		luaL_error(L, "%d Parameters are needed.", count);
	return TRUE;
}
int32 scriptlib::check_action_permission(lua_State* L) {
	duel* pduel = interpreter::get_duel_info(L);
	if(pduel->lua->no_action)
		luaL_error(L, "Action is not allowed here.");
	return TRUE;
}