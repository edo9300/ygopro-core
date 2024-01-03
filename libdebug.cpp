/*
 * Copyright (c) 2012-2015, Argon Sun (Fluorohydride)
 * Copyright (c) 2017-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include "scriptlib.h"
#include "duel.h"
#include "field.h"
#include "card.h"

#define LUA_MODULE Debug
#include "function_array_helper.h"

namespace {

using namespace scriptlib;

LUA_STATIC_FUNCTION(Message) {
	int top = lua_gettop(L);
	if(top == 0)
		return 0;
	luaL_checkstack(L, 1, nullptr);
	for(int i = 1; i <= top; ++i) {
		const auto* str = luaL_tolstring(L, i, nullptr);
		if(str)
			pduel->handle_message(str, OCG_LOG_TYPE_FROM_SCRIPT);
		lua_pop(L, 1);
	}
	return 0;
}
LUA_STATIC_FUNCTION(AddCard) {
	check_param_count(L, 6);
	auto code = lua_get<uint32_t>(L, 1);
	auto owner = lua_get<uint8_t>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 3);
	auto location = lua_get<uint16_t>(L, 4);
	auto sequence = lua_get<uint16_t>(L, 5);
	auto position = lua_get<uint8_t>(L, 6);
	bool proc = lua_get<bool, false>(L, 7);
	if(owner != 0 && owner != 1)
		return 0;
	if(playerid != 0 && playerid != 1)
		return 0;
	auto& field = pduel->game_field;
	if(field->is_location_useable(playerid, location, sequence)) {
		card* pcard = pduel->new_card(code);
		pcard->owner = owner;
		if(location == LOCATION_EXTRA && (position == 0 || (pcard->data.type & TYPE_PENDULUM) == 0))
			position = POS_FACEDOWN_DEFENSE;
		pcard->sendto_param.position = position;
		bool pzone = false;
		if(location == LOCATION_PZONE) {
			location = LOCATION_SZONE;
			sequence = field->get_pzone_index(sequence, playerid);
			pzone = true;
		} else if(location == LOCATION_FZONE) {
			location = LOCATION_SZONE;
			sequence = 5;
		} else if(location == LOCATION_STZONE) {
			location = LOCATION_SZONE;
			sequence += 1 * pduel->game_field->is_flag(DUEL_3_COLUMNS_FIELD);

		} else if(location == LOCATION_MMZONE) {
			location = LOCATION_MZONE;
			sequence += 1 * pduel->game_field->is_flag(DUEL_3_COLUMNS_FIELD);

		} else if(location == LOCATION_EMZONE) {
			location = LOCATION_MZONE;
			sequence += 5;
		}
		field->add_card(playerid, pcard, location, sequence, pzone);
		pcard->current.position = position;
		if(!(location & LOCATION_ONFIELD) || (position & POS_FACEUP)) {
			pcard->enable_field_effect(true);
			field->adjust_instant();
		}
		if(proc)
			pcard->set_status(STATUS_PROC_COMPLETE, TRUE);
		interpreter::pushobject(L, pcard);
		return 1;
	} else if(location & LOCATION_ONFIELD) {
		card* pcard = pduel->new_card(code);
		pcard->owner = owner;
		card* fcard = field->get_field_card(playerid, location, sequence);
		fcard->xyz_add(pcard);
		if(proc)
			pcard->set_status(STATUS_PROC_COMPLETE, TRUE);
		interpreter::pushobject(L, pcard);
		return 1;
	}
	return 0;
}
LUA_STATIC_FUNCTION(SetPlayerInfo) {
	check_param_count(L, 4);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto lp = lua_get<uint32_t>(L, 2);
	auto startcount = lua_get<uint32_t>(L, 3);
	auto drawcount = lua_get<uint32_t>(L, 4);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto& player = pduel->game_field->player[playerid];
	player.lp = lp;
	player.start_lp = lp;
	player.start_count = startcount;
	player.draw_count = drawcount;
	return 0;
}
LUA_STATIC_FUNCTION(PreSummon) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto summon_type = lua_get<uint32_t>(L, 2);
	auto summon_location = lua_get<uint8_t, 0>(L, 3);
	auto summon_sequence = lua_get<uint8_t, 0>(L, 4);
	auto summon_pzone = lua_get<bool, false>(L, 5);
	pcard->summon.location = summon_location;
	pcard->summon.type = summon_type;
	pcard->summon.sequence = summon_sequence;
	pcard->summon.pzone = summon_pzone;
	return 0;
}
LUA_STATIC_FUNCTION(PreEquip) {
	check_param_count(L, 2);
	auto equip_card = lua_get<card*, true>(L, 1);
	auto target = lua_get<card*, true>(L, 2);
	if((equip_card->current.location != LOCATION_SZONE)
	   || (target->current.location != LOCATION_MZONE)
	   || (target->current.position & POS_FACEDOWN))
		lua_pushboolean(L, 0);
	else {
		equip_card->equip(target, FALSE);
		equip_card->effect_target_cards.insert(target);
		target->effect_target_owner.insert(equip_card);
		lua_pushboolean(L, 1);
	}
	return 1;
}
LUA_STATIC_FUNCTION(PreSetTarget) {
	check_param_count(L, 2);
	auto t_card = lua_get<card*, true>(L, 1);
	auto target = lua_get<card*, true>(L, 2);
	t_card->add_card_target(target);
	return 0;
}
LUA_STATIC_FUNCTION(PreAddCounter) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto countertype = lua_get<uint16_t>(L, 2);
	auto count = lua_get<uint16_t>(L, 3);
	uint16_t cttype = countertype & ~COUNTER_NEED_ENABLE;
	auto pr = pcard->counters.emplace(cttype, card::counter_map::mapped_type());
	auto cmit = pr.first;
	if(pr.second) {
		cmit->second[0] = 0;
		cmit->second[1] = 0;
	}
	if((countertype & COUNTER_WITHOUT_PERMIT) && !(countertype & COUNTER_NEED_ENABLE))
		cmit->second[0] += count;
	else
		cmit->second[1] += count;
	return 0;
}
LUA_STATIC_FUNCTION(ReloadFieldBegin) {
	check_param_count(L, 1);
	auto flag = lua_get<uint64_t>(L, 1);
	auto rule = lua_get<uint8_t, 3>(L, 2);
	bool build = lua_get<bool, false>(L, 3);
	pduel->clear();
#define CHECK(MR) case MR : { flag |= DUEL_MODE_MR##MR; break; }
	if(rule && !build) {
		switch (rule) {
		CHECK(1)
		CHECK(2)
		CHECK(3)
		CHECK(4)
		CHECK(5)
		}
#undef CHECK
	}
	pduel->game_field->core.duel_options = flag;
	return 0;
}
LUA_STATIC_FUNCTION(ReloadFieldEnd) {
	auto& field = pduel->game_field;
	auto& core = field->core;
	core.shuffle_hand_check[0] = false;
	core.shuffle_hand_check[1] = false;
	core.shuffle_deck_check[0] = false;
	core.shuffle_deck_check[1] = false;
	field->reload_field_info();
	if(lua_isyieldable(L))
		return yield();
	return 0;
}
template<int message_code, size_t max_len>
int32_t write_string_message(lua_State* L) {
	check_param_count(L, 1);
	check_param(L, LuaParam::STRING, 1);
	size_t len = 0;
	const char* pstr = lua_tolstring(L, 1, &len);
	if(len > max_len)
		len = max_len;
	auto message = lua_get<duel*>(L)->new_message(message_code);
	message->write<uint16_t>(static_cast<uint16_t>(len));
	message->write(pstr, len);
	message->write<uint8_t>(0);
	return 0;
}

LUA_FUNCTION_EXISTING(SetAIName, write_string_message<MSG_AI_NAME, 100>);
LUA_FUNCTION_EXISTING(ShowHint, write_string_message<MSG_SHOW_HINT, 1024>);

LUA_STATIC_FUNCTION(PrintStacktrace) {
	interpreter::print_stacktrace(L);
	return 0;
}

LUA_STATIC_FUNCTION(CardToStringWrapper) {
	const auto pcard = lua_get<card*>(L, 1);
	if(pcard) {
		luaL_checkstack(L, 4, nullptr);
		lua_getglobal(L, "Debug");
		lua_pushstring(L, "CardToString");
		lua_rawget(L, -2);
		if(!lua_isnil(L, -1)) {
			lua_pushvalue(L, 1);
			lua_call(L, 1, 1);
			return 1;
		}
		lua_settop(L, 1);
	}
	const char* kind = luaL_typename(L, 1);
	lua_pushfstring(L, "%s: %p", kind, lua_topointer(L, 1));
	return 1;
}

}

void scriptlib::push_debug_lib(lua_State* L) {
	static constexpr auto debuglib = GET_LUA_FUNCTIONS_ARRAY();
	static_assert(debuglib.back().name == nullptr);
	lua_createtable(L, 0, static_cast<int>(debuglib.size() - 1));
	luaL_setfuncs(L, debuglib.data(), 0);
	lua_setglobal(L, "Debug");
}
