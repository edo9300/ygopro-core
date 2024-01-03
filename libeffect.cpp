/*
 * Copyright (c) 2010-2015, Argon Sun (Fluorohydride)
 * Copyright (c) 2017-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include "scriptlib.h"
#include "duel.h"
#include "field.h"
#include "card.h"
#include "effect.h"
#include "group.h"

#define LUA_MODULE Effect
using LUA_CLASS = effect;
#include "function_array_helper.h"

namespace {

using namespace scriptlib;

LUA_STATIC_FUNCTION(CreateEffect) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	effect* peffect = pduel->new_effect();
	peffect->effect_owner = pduel->game_field->core.reason_player;
	peffect->owner = pcard;
	interpreter::pushobject(L, peffect);
	return 1;
}
LUA_STATIC_FUNCTION(GlobalEffect) {
	effect* peffect = pduel->new_effect();
	peffect->effect_owner = 0;
	peffect->owner = pduel->game_field->temp_card;
	interpreter::pushobject(L, peffect);
	return 1;
}
LUA_FUNCTION(Clone) {
	check_param_count(L, 1);
	interpreter::pushobject(L, self->clone());
	return 1;
}
LUA_FUNCTION(Reset) {
	check_param_count(L, 1);
	if(self->owner == nullptr)
		return 0;
	if(self->is_flag(EFFECT_FLAG_FIELD_ONLY))
		pduel->game_field->remove_effect(self);
	else {
		if(self->handler)
			self->handler->remove_effect(self);
		else
			pduel->game_field->core.reseted_effects.insert(self);
	}
	return 0;
}
LUA_FUNCTION(GetFieldID) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->id);
	return 1;
}
LUA_FUNCTION(SetDescription) {
	check_param_count(L, 2);
	self->description = lua_get<uint64_t>(L, 2);
	return 0;
}
LUA_FUNCTION(SetCode) {
	check_param_count(L, 2);
	self->code = lua_get<uint32_t>(L, 2);
	return 0;
}
LUA_FUNCTION(SetRange) {
	check_param_count(L, 2);
	auto& range = self->range;
	range = lua_get<uint16_t>(L, 2);
	if((range & (LOCATION_MMZONE | LOCATION_EMZONE)) == (LOCATION_MMZONE | LOCATION_EMZONE))
		range |= LOCATION_MZONE;
	if(range & LOCATION_MZONE)
		range &= ~(LOCATION_MMZONE | LOCATION_EMZONE);
	if((range & (LOCATION_FZONE | LOCATION_STZONE | LOCATION_PZONE)) == (LOCATION_FZONE | LOCATION_STZONE | LOCATION_PZONE))
		range |= LOCATION_SZONE;
	if(range & LOCATION_SZONE)
		range &= ~(LOCATION_FZONE | LOCATION_STZONE | LOCATION_PZONE);
	return 0;
}
LUA_FUNCTION(SetTargetRange) {
	check_param_count(L, 3);
	self->s_range = lua_get<uint16_t>(L, 2);
	self->o_range = lua_get<uint16_t>(L, 3);
	self->flag[0] &= ~EFFECT_FLAG_ABSOLUTE_TARGET;
	return 0;
}
LUA_FUNCTION(SetAbsoluteRange) {
	check_param_count(L, 4);
	auto playerid = lua_get<uint8_t>(L, 2);
	auto s = lua_get<uint16_t>(L, 3);
	auto o = lua_get<uint16_t>(L, 4);
	if(playerid == 0) {
		self->s_range = s;
		self->o_range = o;
	} else {
		self->s_range = o;
		self->o_range = s;
	}
	self->flag[0] |= EFFECT_FLAG_ABSOLUTE_TARGET;
	return 0;
}
LUA_FUNCTION(SetCountLimit) {
	check_param_count(L, 2);
	auto count = lua_get<uint8_t>(L, 2);
	if(count == 0)
		lua_error(L, "The count must not be 0");
	uint8_t hopt_index = 0;
	uint32_t code = 0;
	if(lua_istable(L, 3)) {
		lua_pushnil(L);
		if(lua_next(L, 3) != 0) {
			code = lua_get<uint32_t>(L, -1);
			lua_pop(L, 1);
			if(!code) {
				hopt_index = 0;
				lua_pop(L, 1); //manually pop the key from the stack as there won't be a next iteration
			} else if(lua_next(L, 3) != 0) {
				hopt_index = lua_get<uint8_t>(L, -1);
				lua_pop(L, 1);
				lua_pop(L, 1); //manually pop the key from the stack as there won't be a next iteration
			}
		}
	} else
		code = lua_get<uint32_t, 0>(L, 3);
	uint8_t flag = lua_get<uint8_t, 0>(L, 4);
	if((flag & EFFECT_COUNT_CODE_CHAIN) != 0 && code == 0)
		flag |= EFFECT_COUNT_CODE_SINGLE;
	if((flag & (EFFECT_COUNT_CODE_DUEL | EFFECT_COUNT_CODE_CHAIN)) == (EFFECT_COUNT_CODE_DUEL | EFFECT_COUNT_CODE_CHAIN))
		lua_error(L, "Cannot use EFFECT_COUNT_CODE_DUEL together with EFFECT_COUNT_CODE_CHAIN");
	if((flag & EFFECT_COUNT_CODE_DUEL) != 0 && code == 0)
		lua_error(L, "EFFECT_COUNT_CODE_DUEL must have an associated code");
	self->flag[0] |= EFFECT_FLAG_COUNT_LIMIT;
	self->count_limit = count;
	self->count_limit_max = count;
	self->count_code = code;
	self->count_flag = flag;
	self->count_hopt_index = hopt_index;
	return 0;
}
LUA_FUNCTION(SetReset) {
	check_param_count(L, 2);
	auto v = lua_get<uint32_t>(L, 2);
	auto c = lua_get<uint16_t, 0>(L, 3);
	if(c == 0)
		c = 1;
	if(v & (RESET_PHASE) && !(v & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		v |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	self->reset_flag = v;
	self->reset_count = c;
	return 0;
}
LUA_FUNCTION(SetType) {
	check_param_count(L, 2);
	auto v = lua_get<uint16_t>(L, 2);
	if (v & 0x0ff0)
		v |= EFFECT_TYPE_ACTIONS;
	else
		v &= ~EFFECT_TYPE_ACTIONS;
	if(v & (EFFECT_TYPE_ACTIVATE | EFFECT_TYPE_IGNITION | EFFECT_TYPE_QUICK_O | EFFECT_TYPE_QUICK_F))
		v |= EFFECT_TYPE_FIELD;
	if(v & EFFECT_TYPE_ACTIVATE)
		self->range = LOCATION_SZONE + LOCATION_FZONE + LOCATION_HAND;
	if(v & EFFECT_TYPE_FLIP) {
		self->code = EVENT_FLIP;
		if(!(v & EFFECT_TYPE_TRIGGER_O))
			v |= EFFECT_TYPE_TRIGGER_F;
	}
	self->type = v;
	return 0;
}
LUA_FUNCTION(SetProperty) {
	check_param_count(L, 2);
	auto v1 = lua_get<uint32_t>(L, 2);
	auto v2 = lua_get<uint32_t, 0>(L, 3);
	self->flag[0] = (self->flag[0] & 0x4fu) | (v1 & ~0x4fu);
	self->flag[1] = v2;
	return 0;
}
LUA_FUNCTION(SetLabel) {
	check_param_count(L, 2);
	self->label.clear();
	lua_iterate_table_or_stack(L, 2, lua_gettop(L), [&] {
		self->label.push_back(lua_get<lua_Integer>(L, -1));
	});
	return 0;
}
LUA_FUNCTION(SetLabelObject) {
	check_param_count(L, 2);
	if(self->label_object)
		luaL_unref(L, LUA_REGISTRYINDEX, self->label_object);
	self->label_object = 0;
	if(lua_isnoneornil(L, 2))
		return 0;
	if(lua_get<lua_obj*>(L, 2) != nullptr || lua_istable(L, 2)) {
		lua_pushvalue(L, 2);
		self->label_object = luaL_ref(L, LUA_REGISTRYINDEX);
	} else
		lua_error(L, "Parameter 2 should be \"Card\" or \"Effect\" or \"Group\" or \"table\".");
	return 0;
}
LUA_FUNCTION(SetCategory) {
	check_param_count(L, 2);
	auto v = lua_get<uint32_t>(L, 2);
	self->category = v;
	return 0;
}
LUA_FUNCTION(SetHintTiming) {
	check_param_count(L, 2);
	auto vs = lua_get<uint32_t>(L, 2);
	auto vo = lua_get<uint32_t>(L, 3, vs);
	self->hint_timing[0] = vs;
	self->hint_timing[1] = vo;
	return 0;
}
LUA_FUNCTION(SetCondition) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	if(self->condition)
		luaL_unref(L, LUA_REGISTRYINDEX, self->condition);
	self->condition = interpreter::get_function_handle(L, findex);
	return 0;
}
LUA_FUNCTION(SetTarget) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	if(self->target)
		luaL_unref(L, LUA_REGISTRYINDEX, self->target);
	self->target = interpreter::get_function_handle(L, findex);
	return 0;
}
LUA_FUNCTION(SetCost) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	if(self->cost)
		luaL_unref(L, LUA_REGISTRYINDEX, self->cost);
	self->cost = interpreter::get_function_handle(L, findex);
	return 0;
}
LUA_FUNCTION(SetValue) {
	check_param_count(L, 2);
	if(self->value && self->is_flag(EFFECT_FLAG_FUNC_VALUE))
		luaL_unref(L, LUA_REGISTRYINDEX, self->value);
	if (lua_isfunction(L, 2)) {
		self->value = interpreter::get_function_handle(L, 2);
		self->flag[0] |= EFFECT_FLAG_FUNC_VALUE;
	} else {
		self->flag[0] &= ~EFFECT_FLAG_FUNC_VALUE;
		if(lua_isboolean(L, 2))
			self->value = lua_get<bool>(L, 2);
		else {
			auto value = lua_get<lua_Integer>(L, 2);
			if(value > INT32_MAX || value < INT32_MIN) {
				lua_pushvalue(L, 2);
				lua_pushcclosure(L, [](lua_State* L) -> int32_t {
					lua_pushvalue(L, lua_upvalueindex(1));
					return 1;
				}, 1);
				self->value = interpreter::get_function_handle(L, -1);
				self->flag[0] |= EFFECT_FLAG_FUNC_VALUE;
			} else
				self->value = static_cast<int32_t>(value);
		}
	}
	return 0;
}
LUA_FUNCTION(SetOperation) {
	check_param_count(L, 2);
	if(self->operation)
		luaL_unref(L, LUA_REGISTRYINDEX, self->operation);
	self->operation = 0;
	const auto findex = lua_get<function>(L, 2);
	if(findex)
		self->operation = interpreter::get_function_handle(L, findex);
	return 0;
}
LUA_FUNCTION(SetOwnerPlayer) {
	check_param_count(L, 1);
	auto p = lua_get<uint8_t>(L, 2);
	if(p != 0 && p != 1)
		return 0;
	self->effect_owner = p;
	return 0;
}
LUA_FUNCTION(GetDescription) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->description);
	return 1;
}
LUA_FUNCTION(GetCode) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->code);
	return 1;
}
LUA_FUNCTION(GetRange) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->range);
	return 1;
}
LUA_FUNCTION(GetTargetRange) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->s_range);
	lua_pushinteger(L, self->o_range);
	return 2;
}
LUA_FUNCTION(GetCountLimit) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->count_limit);
	lua_pushinteger(L, self->count_limit_max);
	lua_pushinteger(L, self->count_code);
	lua_pushinteger(L, self->count_flag); //return the count_code flags as their shifted representation
	lua_pushinteger(L, self->count_hopt_index);
	return 5;
}
LUA_FUNCTION(GetReset) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->reset_flag);
	lua_pushinteger(L, self->reset_count);
	return 2;
}
LUA_FUNCTION(GetType) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->type);
	return 1;
}
LUA_FUNCTION(GetProperty) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->flag[0]);
	lua_pushinteger(L, self->flag[1]);
	return 2;
}
LUA_FUNCTION(GetLabel) {
	check_param_count(L, 1);
	const auto& label = self->label;
	if(label.empty()) {
		lua_pushinteger(L, 0);
		return 1;
	}
	luaL_checkstack(L, static_cast<int>(label.size()), nullptr);
	for(const auto& lab : label)
		lua_pushinteger(L, lab);
	return static_cast<int32_t>(label.size());
}
LUA_FUNCTION(GetLabelObject) {
	check_param_count(L, 1);
	if(!self->label_object) {
		lua_pushnil(L);
		return 1;
	}
	lua_rawgeti(L, LUA_REGISTRYINDEX, self->label_object);
	if(lua_touserdata(L, -1) == nullptr && !lua_istable(L, -1)) {
		lua_pop(L, 1);
		lua_pushnil(L);
	}
	return 1;
}
LUA_FUNCTION(GetCategory) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->category);
	return 1;
}
LUA_FUNCTION(GetOwner) {
	check_param_count(L, 1);
	interpreter::pushobject(L, self->get_owner());
	return 1;
}
LUA_FUNCTION(GetHandler) {
	check_param_count(L, 1);
	interpreter::pushobject(L, self->get_handler());
	return 1;
}
LUA_FUNCTION(GetOwnerPlayer) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->get_owner_player());
	return 1;
}
LUA_FUNCTION(GetHandlerPlayer) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->get_handler_player());
	return 1;
}
LUA_FUNCTION(GetHintTiming) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->hint_timing[0]);
	lua_pushinteger(L, self->hint_timing[1]);
	return 2;
}
LUA_FUNCTION(GetCondition) {
	check_param_count(L, 1);
	interpreter::pushobject(L, self->condition);
	return 1;
}
LUA_FUNCTION(GetTarget) {
	check_param_count(L, 1);
	interpreter::pushobject(L, self->target);
	return 1;
}
LUA_FUNCTION(GetCost) {
	check_param_count(L, 1);
	interpreter::pushobject(L, self->cost);
	return 1;
}
LUA_FUNCTION(GetValue) {
	check_param_count(L, 1);
	if(self->is_flag(EFFECT_FLAG_FUNC_VALUE))
		interpreter::pushobject(L, self->value);
	else
		lua_pushinteger(L, self->value);
	return 1;
}
LUA_FUNCTION(GetOperation) {
	check_param_count(L, 1);
	interpreter::pushobject(L, self->operation);
	return 1;
}
LUA_FUNCTION(GetActiveType) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->get_active_type());
	return 1;
}
LUA_FUNCTION(IsActiveType) {
	check_param_count(L, 2);
	lua_pushboolean(L, (self->get_active_type() & lua_get<uint32_t>(L, 2)) != 0);
	return 1;
}
LUA_FUNCTION(IsHasProperty) {
	check_param_count(L, 2);
	auto tflag1 = lua_get<uint32_t>(L, 2);
	auto tflag2 = lua_get<uint32_t, 0>(L, 3);
	lua_pushboolean(L, ((!tflag1 || (self->flag[0] & tflag1)) && (!tflag2 || (self->flag[1] & tflag2))));
	return 1;
}
LUA_FUNCTION(IsHasCategory) {
	check_param_count(L, 2);
	lua_pushboolean(L, (self->category & lua_get<uint32_t>(L, 2)) != 0);
	return 1;
}
LUA_FUNCTION(IsHasType) {
	check_param_count(L, 2);
	lua_pushboolean(L, (self->type & lua_get<uint16_t>(L, 2)) != 0);
	return 1;
}
LUA_FUNCTION(IsActivatable) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 2);
	bool neglect_loc = lua_get<bool, false>(L, 3);
	bool neglect_target = lua_get<bool, false>(L, 4);
	lua_pushboolean(L, self->is_activateable(playerid, pduel->game_field->nil_event, 0, 0, neglect_target, neglect_loc));
	return 1;
}
LUA_FUNCTION(IsActivated) {
	check_param_count(L, 1);
	lua_pushboolean(L, (self->type & 0x7f0) != 0);
	return 1;
}
LUA_FUNCTION(GetActivateLocation) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->active_location);
	return 1;
}
LUA_FUNCTION(GetActivateSequence) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->active_sequence);
	return 1;
}
LUA_FUNCTION(CheckCountLimit) {
	check_param_count(L, 2);
	auto p = lua_get<uint8_t>(L, 2);
	lua_pushboolean(L, self->check_count_limit(p));
	return 1;
}
LUA_FUNCTION(UseCountLimit) {
	check_param_count(L, 2);
	auto p = lua_get<uint8_t>(L, 2);
	auto count = lua_get<uint32_t, 1>(L, 3);
	bool oath_only = lua_get<bool, false>(L, 4);
	if (!oath_only || self->count_flag & EFFECT_COUNT_CODE_OATH)
		while(count) {
			self->dec_count(p);
			--count;
		}
	return 0;
}
LUA_FUNCTION_EXISTING(GetLuaRef, get_lua_ref<effect>);
LUA_FUNCTION_EXISTING(FromLuaRef, from_lua_ref<effect>);
LUA_FUNCTION_EXISTING(IsDeleted, is_deleted_object);
}

void scriptlib::push_effect_lib(lua_State* L) {
	static constexpr auto effectlib = GET_LUA_FUNCTIONS_ARRAY();
	static_assert(effectlib.back().name == nullptr);
	lua_createtable(L, 0, static_cast<int>(effectlib.size() - 1));
	luaL_setfuncs(L, effectlib.data(), 0);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_setglobal(L, "Effect");
}
