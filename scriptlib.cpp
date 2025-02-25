/*
 * Copyright (c) 2010-2015, Argon Sun (Fluorohydride)
 * Copyright (c) 2018-2025, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include "duel.h"
#include "field.h"
#include "lua_obj.h"
#include "scriptlib.h"

namespace scriptlib {

LuaParam get_lua_type(lua_State* L, int32_t index) {
	switch(auto type = lua_type(L, index); type) {
	case LUA_TFUNCTION:
		return LuaParam::FUNCTION;
	case LUA_TSTRING:
		return LuaParam::STRING;
	case LUA_TNUMBER:
		return LuaParam::INT;
	case LUA_TBOOLEAN:
		return LuaParam::BOOLEAN;
	case LUA_TTABLE:
		return LuaParam::TABLE;
	case LUA_TNIL:
		return LuaParam::NIL;
	case LUA_TNONE:
		return LuaParam::NONE;
	case LUA_TUSERDATA:
		if(auto* obj = *static_cast<lua_obj**>(lua_touserdata(L, index)); obj != nullptr) {
			return obj->lua_type;
		}
		[[fallthrough]];
	default:
		return LuaParam::UNKNOWN;
	}
}

const char* get_lua_type_name(lua_State* L, int32_t index) {
	switch(auto type = get_lua_type(L, index); type) {
	case LuaParam::FUNCTION:
		return "Function";
	case LuaParam::STRING:
		return "String";
	case LuaParam::INT:
		return "Int";
	case LuaParam::BOOLEAN:
		return "boolean";
	case LuaParam::TABLE:
		return "table";
	case LuaParam::NIL:
	case LuaParam::NONE:
		return "nil";
	case LuaParam::CARD:
		return "Card";
	case LuaParam::GROUP:
		return "Group";
	case LuaParam::EFFECT:
		return "Effect";
	case LuaParam::DELETED:
		return "Deleted";
	default:
		return "unknown";
	}
}

void check_action_permission(lua_State* L) {
	if(lua_get<duel*>(L)->lua->no_action)
		lua_error(L, "Action is not allowed here.");
}
int32_t push_return_cards(lua_State* L, int32_t/* status*/, lua_KContext ctx) {
	const auto pduel = lua_get<duel*>(L);
	bool cancelable = (bool)ctx;
	if(pduel->game_field->return_cards.canceled) {
		if(cancelable) {
			lua_pushnil(L);
		} else {
			group* pgroup = pduel->new_group();
			interpreter::pushobject(L, pgroup);
		}
	} else {
		group* pgroup = pduel->new_group(pduel->game_field->return_cards.list);
		interpreter::pushobject(L, pgroup);
	}
	return 1;
}
int32_t is_deleted_object(lua_State* L) {
	if(auto obj = lua_touserdata(L, 1)) {
		auto* ret = *static_cast<lua_obj**>(obj);
		lua_pushboolean(L, ret->lua_type == LuaParam::DELETED);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}
}
