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

bool is_in_noaction_state(lua_State* L) {
	return duel::from(L)->lua->no_action;
}
int32_t push_return_cards(lua_State* L, int32_t/* status*/, lua_KContext ctx) {
	const auto pduel = duel::from(L);
	bool cancelable = (bool)ctx;
	if(pduel->game_field->return_cards.canceled) {
		if(cancelable) {
			lua_pushnil(L);
		} else {
			auto pgroup = pduel->new_group();
			interpreter::pushobject(L, pgroup);
		}
	} else {
		auto pgroup = pduel->new_group(pduel->game_field->return_cards.list);
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
std::any* set_any_temp_storage(lua_State* L, std::any&& storage) {
	return &duel::from(L)->lua->temp_lua_parsing_storage.emplace_back(std::move(storage));
}
void clear_any_temp_storage(lua_State* L) {
	duel::from(L)->lua->temp_lua_parsing_storage.clear();
}
}
