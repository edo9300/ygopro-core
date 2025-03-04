/*
 * Copyright (c) 2010-2015, Argon Sun (Fluorohydride)
 * Copyright (c) 2017-2025, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include "duel.h"
#include "effect.h"
#include "field.h"
#include "scriptlib.h"

#define LUA_MODULE Effect
using LUA_CLASS = effect;
#include "function_array_helper.h"

namespace {

using namespace scriptlib;

LUA_STATIC_FUNCTION(CreateEffect, card* pcard) {
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
	interpreter::pushobject(L, self->clone());
	return 1;
}
LUA_FUNCTION(Reset) {
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
	lua_pushinteger(L, self->id);
	return 1;
}
LUA_FUNCTION(SetDescription, uint64_t description) {
	self->description = description;
	return 0;
}
LUA_FUNCTION(SetCode, uint32_t code) {
	self->code = code;
	return 0;
}
LUA_FUNCTION(SetRange, uint16_t range) {
	if((range & (LOCATION_MMZONE | LOCATION_EMZONE)) == (LOCATION_MMZONE | LOCATION_EMZONE))
		range |= LOCATION_MZONE;
	if(range & LOCATION_MZONE)
		range &= ~(LOCATION_MMZONE | LOCATION_EMZONE);
	if((range & (LOCATION_FZONE | LOCATION_STZONE | LOCATION_PZONE)) == (LOCATION_FZONE | LOCATION_STZONE | LOCATION_PZONE))
		range |= LOCATION_SZONE;
	if(range & LOCATION_SZONE)
		range &= ~(LOCATION_FZONE | LOCATION_STZONE | LOCATION_PZONE);
	self->range = range;
	return 0;
}
LUA_FUNCTION(SetTargetRange, uint16_t self_range, uint16_t oppo_range) {
	self->s_range = self_range;
	self->o_range = oppo_range;
	self->flag[0] &= ~EFFECT_FLAG_ABSOLUTE_TARGET;
	return 0;
}
LUA_FUNCTION(SetAbsoluteRange, uint8_t playerid, uint16_t self_range, uint16_t oppo_range) {
	if(playerid == 0) {
		self->s_range = self_range;
		self->o_range = oppo_range;
	} else {
		self->s_range = oppo_range;
		self->o_range = self_range;
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
LUA_FUNCTION(SetReset, uint32_t reset_flag, std::optional<uint16_t> reset_count) {
	auto count = reset_count.value_or(1);
	if(count == 0)
		count = 1;
	if(reset_flag & (RESET_PHASE) && !(reset_flag & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		reset_flag |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	self->reset_flag = reset_flag;
	self->reset_count = count;
	return 0;
}
LUA_FUNCTION(SetType, uint16_t type) {
	if (type & 0x0ff0)
		type |= EFFECT_TYPE_ACTIONS;
	else
		type &= ~EFFECT_TYPE_ACTIONS;
	if(type & (EFFECT_TYPE_ACTIVATE | EFFECT_TYPE_IGNITION | EFFECT_TYPE_QUICK_O | EFFECT_TYPE_QUICK_F))
		type |= EFFECT_TYPE_FIELD;
	if(type & EFFECT_TYPE_ACTIVATE)
		self->range = LOCATION_SZONE + LOCATION_FZONE + LOCATION_HAND;
	if(type & EFFECT_TYPE_FLIP) {
		self->code = EVENT_FLIP;
		if(!(type & EFFECT_TYPE_TRIGGER_O))
			type |= EFFECT_TYPE_TRIGGER_F;
	}
	self->type = type;
	return 0;
}
LUA_FUNCTION(SetProperty, uint32_t prop1, std::optional<uint32_t> prop2) {
	self->flag[0] = (self->flag[0] & 0x4fu) | (prop1 & ~0x4fu);
	self->flag[1] = prop2.value_or(0);
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
		ensure_luaL_stack(luaL_unref, L, LUA_REGISTRYINDEX, self->label_object);
	self->label_object = 0;
	if(lua_isnoneornil(L, 2))
		return 0;
	if(lua_get<lua_obj*>(L, 2) != nullptr || lua_istable(L, 2)) {
		lua_pushvalue(L, 2);
		self->label_object = ensure_luaL_stack(luaL_ref, L, LUA_REGISTRYINDEX);
	} else
		lua_error(L, "Parameter 2 should be \"Card\" or \"Effect\" or \"Group\" or \"table\".");
	return 0;
}
LUA_FUNCTION(SetCategory, uint32_t category) {
	self->category = category;
	return 0;
}
LUA_FUNCTION(SetHintTiming, uint32_t self_timings, std::optional<uint32_t> opponent_timings) {
	self->hint_timing[0] = self_timings;
	self->hint_timing[1] = opponent_timings.value_or(self_timings);
	return 0;
}
LUA_FUNCTION(SetCondition, Function findex) {
	if(self->condition)
		ensure_luaL_stack(luaL_unref, L, LUA_REGISTRYINDEX, self->condition);
	self->condition = interpreter::get_function_handle(L, findex);
	return 0;
}
LUA_FUNCTION(SetTarget, Function findex) {
	if(self->target)
		ensure_luaL_stack(luaL_unref, L, LUA_REGISTRYINDEX, self->target);
	self->target = interpreter::get_function_handle(L, findex);
	return 0;
}
LUA_FUNCTION(SetCost, Function findex) {
	if(self->cost)
		ensure_luaL_stack(luaL_unref, L, LUA_REGISTRYINDEX, self->cost);
	self->cost = interpreter::get_function_handle(L, findex);
	return 0;
}
LUA_FUNCTION(SetValue, std::variant<Function, bool, lua_Integer> function_value) {
	if(self->value && self->is_flag(EFFECT_FLAG_FUNC_VALUE))
		ensure_luaL_stack(luaL_unref, L, LUA_REGISTRYINDEX, self->value);
	if(std::holds_alternative<Function>(function_value)) {
		self->value = interpreter::get_function_handle(L, std::get<Function>(function_value));
		self->flag[0] |= EFFECT_FLAG_FUNC_VALUE;
	} else {
		self->flag[0] &= ~EFFECT_FLAG_FUNC_VALUE;
		if(std::holds_alternative<bool>(function_value)) {
			self->value = std::get<bool>(function_value);
		} else {
			auto value = std::get<lua_Integer>(function_value);
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
LUA_FUNCTION(SetOperation, std::optional<Function> findex) {
	if(self->operation)
		ensure_luaL_stack(luaL_unref, L, LUA_REGISTRYINDEX, self->operation);
	self->operation = 0;
	if(findex.has_value())
		self->operation = interpreter::get_function_handle(L, findex.value());
	return 0;
}
LUA_FUNCTION(SetOwnerPlayer, uint8_t playerid) {
	if(playerid != 0 && playerid != 1)
		return 0;
	self->effect_owner = playerid;
	return 0;
}
LUA_FUNCTION(GetDescription) {
	lua_pushinteger(L, self->description);
	return 1;
}
LUA_FUNCTION(GetCode) {
	lua_pushinteger(L, self->code);
	return 1;
}
LUA_FUNCTION(GetRange) {
	lua_pushinteger(L, self->range);
	return 1;
}
LUA_FUNCTION(GetTargetRange) {
	lua_pushinteger(L, self->s_range);
	lua_pushinteger(L, self->o_range);
	return 2;
}
LUA_FUNCTION(GetCountLimit) {
	lua_pushinteger(L, self->count_limit);
	lua_pushinteger(L, self->count_limit_max);
	lua_pushinteger(L, self->count_code);
	lua_pushinteger(L, self->count_flag); //return the count_code flags as their shifted representation
	lua_pushinteger(L, self->count_hopt_index);
	return 5;
}
LUA_FUNCTION(GetReset) {
	lua_pushinteger(L, self->reset_flag);
	lua_pushinteger(L, self->reset_count);
	return 2;
}
LUA_FUNCTION(GetType) {
	lua_pushinteger(L, self->type);
	return 1;
}
LUA_FUNCTION(GetProperty) {
	lua_pushinteger(L, self->flag[0]);
	lua_pushinteger(L, self->flag[1]);
	return 2;
}
LUA_FUNCTION(GetLabel) {
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
	lua_pushinteger(L, self->category);
	return 1;
}
LUA_FUNCTION(GetOwner) {
	interpreter::pushobject(L, self->get_owner());
	return 1;
}
LUA_FUNCTION(GetHandler) {
	interpreter::pushobject(L, self->get_handler());
	return 1;
}
LUA_FUNCTION(GetOwnerPlayer) {
	lua_pushinteger(L, self->get_owner_player());
	return 1;
}
LUA_FUNCTION(GetHandlerPlayer) {
	lua_pushinteger(L, self->get_handler_player());
	return 1;
}
LUA_FUNCTION(GetHintTiming) {
	lua_pushinteger(L, self->hint_timing[0]);
	lua_pushinteger(L, self->hint_timing[1]);
	return 2;
}
LUA_FUNCTION(GetCondition) {
	interpreter::pushobject(L, self->condition);
	return 1;
}
LUA_FUNCTION(GetTarget) {
	interpreter::pushobject(L, self->target);
	return 1;
}
LUA_FUNCTION(GetCost) {
	interpreter::pushobject(L, self->cost);
	return 1;
}
LUA_FUNCTION(GetValue) {
	if(self->is_flag(EFFECT_FLAG_FUNC_VALUE))
		interpreter::pushobject(L, self->value);
	else
		lua_pushinteger(L, self->value);
	return 1;
}
LUA_FUNCTION(GetOperation) {
	interpreter::pushobject(L, self->operation);
	return 1;
}
LUA_FUNCTION(GetActiveType) {
	lua_pushinteger(L, self->get_active_type());
	return 1;
}
LUA_FUNCTION(IsActiveType, uint32_t type) {
	lua_pushboolean(L, (self->get_active_type() & type) != 0);
	return 1;
}
LUA_FUNCTION(IsHasProperty, uint32_t flag1, std::optional<uint32_t> flag2) {
	auto tflag2 = flag2.value_or(0);
	lua_pushboolean(L, ((!flag1 || (self->flag[0] & flag1)) && (!tflag2 || (self->flag[1] & tflag2))));
	return 1;
}
LUA_FUNCTION(IsHasCategory, uint32_t category) {
	lua_pushboolean(L, (self->category & category) != 0);
	return 1;
}
LUA_FUNCTION(IsHasType, uint16_t type) {
	lua_pushboolean(L, (self->type & type) != 0);
	return 1;
}
LUA_FUNCTION(IsActivatable, uint8_t playerid, std::optional<bool> ignore_location, std::optional<bool> ignore_target) {
	lua_pushboolean(L, self->is_activateable(playerid, pduel->game_field->nil_event, 0, 0, ignore_target.value_or(false), ignore_location.value_or(false)));
	return 1;
}
LUA_FUNCTION(IsActivated) {
	lua_pushboolean(L, (self->type & 0x7f0) != 0);
	return 1;
}
LUA_FUNCTION(GetActivateLocation) {
	lua_pushinteger(L, self->active_location);
	return 1;
}
LUA_FUNCTION(GetActivateSequence) {
	lua_pushinteger(L, self->active_sequence);
	return 1;
}
LUA_FUNCTION(CheckCountLimit, uint8_t playerid) {
	lua_pushboolean(L, self->check_count_limit(playerid));
	return 1;
}
LUA_FUNCTION(UseCountLimit, uint8_t playerid, std::optional<uint32_t> count, std::optional<bool> oath_only) {
	if (!oath_only.value_or(false) || self->count_flag & EFFECT_COUNT_CODE_OATH)
		for(auto i = count.value_or(1); i > 0; --i) {
			self->dec_count(playerid);
		}
	return 0;
}
LUA_FUNCTION(RestoreCountLimit, uint8_t playerid, std::optional<uint32_t> count, std::optional<bool> oath_only) {
	if(!oath_only.value_or(false) || self->count_flag & EFFECT_COUNT_CODE_OATH)
		for(auto i = count.value_or(1); i > 0; --i) {
			self->inc_count(playerid);
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
	ensure_luaL_stack(luaL_setfuncs, L, effectlib.data(), 0);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_setglobal(L, "Effect");
}
