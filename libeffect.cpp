/*
 * libeffect.cpp
 *
 *  Created on: 2010-7-20
 *      Author: Argon
 */

#include "scriptlib.h"
#include "duel.h"
#include "field.h"
#include "card.h"
#include "effect.h"
#include "group.h"

int32 scriptlib::effect_new(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	duel* pduel = pcard->pduel;
	effect* peffect = pduel->new_effect();
	peffect->effect_owner = pduel->game_field->core.reason_player;
	peffect->owner = pcard;
	interpreter::pushobject(L, peffect);
	return 1;
}
int32 scriptlib::effect_newex(lua_State* L) {
	duel* pduel = interpreter::get_duel_info(L);
	effect* peffect = pduel->new_effect();
	peffect->effect_owner = 0;
	peffect->owner = pduel->game_field->temp_card;
	interpreter::pushobject(L, peffect);
	return 1;
}
int32 scriptlib::effect_clone(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	interpreter::pushobject(L, peffect->clone());
	return 1;
}
int32 scriptlib::effect_reset(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	if(peffect->owner == nullptr)
		return 0;
	if(peffect->is_flag(EFFECT_FLAG_FIELD_ONLY))
		peffect->pduel->game_field->remove_effect(peffect);
	else {
		if(peffect->handler)
			peffect->handler->remove_effect(peffect);
		else
			peffect->pduel->game_field->core.reseted_effects.insert(peffect);
	}
	return 0;
}
int32 scriptlib::effect_get_field_id(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushinteger(L, peffect->id);
	return 1;
}
int32 scriptlib::effect_set_description(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	peffect->description = lua_get<uint64>(L, 2);
	return 0;
}
int32 scriptlib::effect_set_code(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	peffect->code = lua_get<uint32>(L, 2);
	return 0;
}
int32 scriptlib::effect_set_range(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	peffect->range = lua_get<uint16>(L, 2);
	return 0;
}
int32 scriptlib::effect_set_target_range(lua_State* L) {
	check_param_count(L, 3);
	auto peffect = lua_get<effect*, true>(L, 1);
	peffect->s_range = lua_get<uint16>(L, 2);
	peffect->o_range = lua_get<uint16>(L, 3);
	peffect->flag[0] &= ~EFFECT_FLAG_ABSOLUTE_TARGET;
	return 0;
}
int32 scriptlib::effect_set_absolute_range(lua_State* L) {
	check_param_count(L, 4);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto playerid = lua_get<uint8>(L, 2);
	auto s = lua_get<uint16>(L, 3);
	auto o = lua_get<uint16>(L, 4);
	if(playerid == 0) {
		peffect->s_range = s;
		peffect->o_range = o;
	} else {
		peffect->s_range = o;
		peffect->o_range = s;
	}
	peffect->flag[0] |= EFFECT_FLAG_ABSOLUTE_TARGET;
	return 0;
}
int32 scriptlib::effect_set_count_limit(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto v = lua_get<uint8>(L, 2);
	auto code = lua_get<uint32, 0>(L, 3);
	uint32 flag = 0;
	if(lua_gettop(L) >= 4)
		flag = lua_get<uint32, 0>(L, 4);
	else {
		flag = code & 0xf0000000;
		code &= 0xfffffff;
		if(code == 1) {
			flag |= EFFECT_COUNT_CODE_SINGLE;
			code = 0;
		}
	}
	if(v == 0)
		v = 1;
	peffect->flag[0] |= EFFECT_FLAG_COUNT_LIMIT;
	peffect->count_limit = v;
	peffect->count_limit_max = v;
	peffect->count_code = code;
	peffect->count_flag = flag;
	return 0;
}
int32 scriptlib::effect_set_reset(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto v = lua_get<uint32>(L, 2);
	auto c = lua_get<uint16, 0>(L, 3);
	if(c == 0)
		c = 1;
	if(v & (RESET_PHASE) && !(v & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		v |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	peffect->reset_flag = v;
	peffect->reset_count = c;
	return 0;
}
int32 scriptlib::effect_set_type(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto v = lua_get<uint16>(L, 2);
	if (v & 0x0ff0)
		v |= EFFECT_TYPE_ACTIONS;
	else
		v &= ~EFFECT_TYPE_ACTIONS;
	if(v & (EFFECT_TYPE_ACTIVATE | EFFECT_TYPE_IGNITION | EFFECT_TYPE_QUICK_O | EFFECT_TYPE_QUICK_F))
		v |= EFFECT_TYPE_FIELD;
	if(v & EFFECT_TYPE_ACTIVATE)
		peffect->range = LOCATION_SZONE + LOCATION_FZONE + LOCATION_HAND;
	if(v & EFFECT_TYPE_FLIP) {
		peffect->code = EVENT_FLIP;
		if(!(v & EFFECT_TYPE_TRIGGER_O))
			v |= EFFECT_TYPE_TRIGGER_F;
	}
	peffect->type = v;
	return 0;
}
int32 scriptlib::effect_set_property(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto v1 = lua_get<uint32>(L, 2);
	auto v2 = lua_get<uint32, 0>(L, 3);
	peffect->flag[0] = (peffect->flag[0] & 0x4f) | (v1 & ~0x4f);
	peffect->flag[1] = v2;
	return 0;
}
int32 scriptlib::effect_set_label(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	peffect->label.clear();
	for(int32 i = 2; i <= lua_gettop(L); ++i)
		peffect->label.push_back(lua_get<uint32>(L, i));
	return 0;
}
int32 scriptlib::effect_set_label_object(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	if(peffect->label_object) {
		lua_rawgeti(L, LUA_REGISTRYINDEX, peffect->label_object);
		if(lua_istable(L, -1))
			luaL_unref(L, LUA_REGISTRYINDEX, peffect->label_object);
		lua_pop(L, 1);
	}
	if(lua_isnil(L, 2)) {
		peffect->label_object = 0;
		return 0;
	}
	if(auto obj = lua_get<lua_obj*>(L, 2))
		peffect->label_object = obj->ref_handle;
	else if(lua_istable(L, 2)) {
		lua_pushvalue(L, 2);
		peffect->label_object = luaL_ref(L, LUA_REGISTRYINDEX);
	} else
		luaL_error(L, "Parameter 2 should be \"Card\" or \"Effect\" or \"Group\" or \"table\".");
	return 0;
}
int32 scriptlib::effect_set_category(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto v = lua_get<uint32>(L, 2);
	peffect->category = v;
	return 0;
}
int32 scriptlib::effect_set_hint_timing(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto vs = lua_get<uint32>(L, 2);
	auto vo = lua_get<uint32>(L, 3, vs);
	peffect->hint_timing[0] = vs;
	peffect->hint_timing[1] = vo;
	return 0;
}
int32 scriptlib::effect_set_condition(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	if(peffect->condition)
		luaL_unref(L, LUA_REGISTRYINDEX, peffect->condition);
	peffect->condition = interpreter::get_function_handle(L, 2);
	return 0;
}
int32 scriptlib::effect_set_target(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	if(peffect->target)
		luaL_unref(L, LUA_REGISTRYINDEX, peffect->target);
	peffect->target = interpreter::get_function_handle(L, 2);
	return 0;
}
int32 scriptlib::effect_set_cost(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	if(peffect->cost)
		luaL_unref(L, LUA_REGISTRYINDEX, peffect->cost);
	peffect->cost = interpreter::get_function_handle(L, 2);
	return 0;
}
int32 scriptlib::effect_set_value(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	if(peffect->value && peffect->is_flag(EFFECT_FLAG_FUNC_VALUE))
		luaL_unref(L, LUA_REGISTRYINDEX, peffect->value);
	if (lua_isfunction(L, 2)) {
		peffect->value = interpreter::get_function_handle(L, 2);
		peffect->flag[0] |= EFFECT_FLAG_FUNC_VALUE;
	} else {
		peffect->flag[0] &= ~EFFECT_FLAG_FUNC_VALUE;
		if(lua_isboolean(L, 2))
			peffect->value = lua_get<bool>(L, 2);
		else
			peffect->value = lua_get<int32>(L, 2);
	}
	return 0;
}
int32 scriptlib::effect_set_operation(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	if(peffect->operation)
		luaL_unref(L, LUA_REGISTRYINDEX, peffect->operation);
	if(!lua_isnil(L, 2)) {
		check_param(L, PARAM_TYPE_FUNCTION, 2);
		peffect->operation = interpreter::get_function_handle(L, 2);
	} else
		peffect->operation = 0;
	return 0;
}
int32 scriptlib::effect_set_owner_player(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto p = lua_get<uint8>(L, 2);
	if(p != 0 && p != 1)
		return 0;
	peffect->effect_owner = p;
	return 0;
}
int32 scriptlib::effect_get_description(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushinteger(L, peffect->description);
	return 1;
}
int32 scriptlib::effect_get_code(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushinteger(L, peffect->code);
	return 1;
}
int32 scriptlib::effect_get_count_limit(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushinteger(L, peffect->count_limit);
	lua_pushinteger(L, peffect->count_limit_max);
	lua_pushinteger(L, peffect->count_code);
	lua_pushinteger(L, peffect->count_flag);
	return 4;
}
int32 scriptlib::effect_get_reset(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushinteger(L, peffect->reset_flag);
	lua_pushinteger(L, peffect->reset_count);
	return 2;
}
int32 scriptlib::effect_get_type(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushinteger(L, peffect->type);
	return 1;
}
int32 scriptlib::effect_get_property(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushinteger(L, peffect->flag[0]);
	lua_pushinteger(L, peffect->flag[1]);
	return 2;
}
int32 scriptlib::effect_get_label(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	if(peffect->label.empty()) {
		lua_pushinteger(L, 0);
		return 1;
	}
	for(const auto& lab : peffect->label)
		lua_pushinteger(L, lab);
	return peffect->label.size();
}
int32 scriptlib::effect_get_label_object(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	if(!peffect->label_object) {
		lua_pushnil(L);
		return 1;
	}
	lua_rawgeti(L, LUA_REGISTRYINDEX, peffect->label_object);
	if(lua_isuserdata(L, -1))
	if(lua_isuserdata(L, -1) || lua_istable(L, -1))
		return 1;
	else {
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}
}
int32 scriptlib::effect_get_category(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushinteger(L, peffect->category);
	return 1;
}
int32 scriptlib::effect_get_owner(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	interpreter::pushobject(L, peffect->owner);
	return 1;
}
int32 scriptlib::effect_get_handler(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	interpreter::pushobject(L, peffect->get_handler());
	return 1;
}
int32 scriptlib::effect_get_owner_player(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushinteger(L, peffect->get_owner_player());
	return 1;
}
int32 scriptlib::effect_get_handler_player(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushinteger(L, peffect->get_handler_player());
	return 1;
}
int32 scriptlib::effect_get_condition(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	interpreter::pushobject(L, peffect->condition);
	return 1;
}
int32 scriptlib::effect_get_target(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	interpreter::pushobject(L, peffect->target);
	return 1;
}
int32 scriptlib::effect_get_cost(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	interpreter::pushobject(L, peffect->cost);
	return 1;
}
int32 scriptlib::effect_get_value(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	if(peffect->is_flag(EFFECT_FLAG_FUNC_VALUE))
		interpreter::pushobject(L, peffect->value);
	else
		lua_pushinteger(L, (int32)peffect->value);
	return 1;
}
int32 scriptlib::effect_get_operation(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	interpreter::pushobject(L, peffect->operation);
	return 1;
}
int32 scriptlib::effect_get_active_type(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushinteger(L, peffect->get_active_type());
	return 1;
}
int32 scriptlib::effect_is_active_type(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushboolean(L, peffect->get_active_type() & lua_get<uint32>(L, 2));
	return 1;
}
int32 scriptlib::effect_is_has_property(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto tflag1 = lua_get<uint32>(L, 2);
	auto tflag2 = lua_get<uint32, 0>(L, 3);
	lua_pushboolean(L, ((!tflag1 || (peffect->flag[0] & tflag1)) && (!tflag2 || (peffect->flag[1] & tflag2))));
	return 1;
}
int32 scriptlib::effect_is_has_category(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushboolean(L, peffect->category & lua_get<uint32>(L, 2));
	return 1;
}
int32 scriptlib::effect_is_has_type(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushboolean(L, peffect->type & lua_get<uint16>(L, 2));
	return 1;
}
int32 scriptlib::effect_is_activatable(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto playerid = lua_get<uint8>(L, 2);
	bool neglect_loc = lua_get<bool, false>(L, 3);
	bool neglect_target = lua_get<bool, false>(L, 4);
	lua_pushboolean(L, peffect->is_activateable(playerid, peffect->pduel->game_field->nil_event, 0, 0, neglect_target, neglect_loc));
	return 1;
}
int32 scriptlib::effect_is_activated(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushboolean(L, (peffect->type & 0x7f0));
	return 1;
}
int32 scriptlib::effect_get_activate_location(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushinteger(L, peffect->active_location);
	return 1;
}
int32 scriptlib::effect_get_activate_sequence(lua_State* L) {
	check_param_count(L, 1);
	auto peffect = lua_get<effect*, true>(L, 1);
	lua_pushinteger(L, peffect->active_sequence);
	return 1;
}
int32 scriptlib::effect_check_count_limit(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto p = lua_get<uint8>(L, 2);
	lua_pushboolean(L, peffect->check_count_limit(p));
	return 1;
}
int32 scriptlib::effect_use_count_limit(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto p = lua_get<uint8>(L, 2);
	auto count = lua_get<uint32, 1>(L, 3);
	bool oath_only = lua_get<bool, false>(L, 4);
	if (!oath_only || peffect->count_flag & EFFECT_COUNT_CODE_OATH)
		while(count) {
			peffect->dec_count(p);
			count--;
		}
	return 0;
}
