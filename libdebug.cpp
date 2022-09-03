/*
 * libdebug.cpp
 *
 *  Created on: 2012-2-8
 *      Author: Argon
 */

#include "scriptlib.h"
#include "duel.h"
#include "field.h"
#include "card.h"

#define LUA_MODULE Debug
#include "function_array_helper.h"

namespace {

using namespace scriptlib;

LUA_FUNCTION(Message) {
	if(lua_gettop(L) == 0)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_getglobal(L, "tostring");
	lua_pushvalue(L, -2);
	lua_pcall(L, 1, 1, 0);
	pduel->handle_message(lua_tostring_or_empty(L, -1), OCG_LOG_TYPE_FROM_SCRIPT);
	return 0;
}
LUA_FUNCTION(AddCard) {
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
	const auto pduel = lua_get<duel*>(L);
	auto& field = pduel->game_field;
	if(field->is_location_useable(playerid, location, sequence)) {
		card* pcard = pduel->new_card(code);
		pcard->owner = owner;
		if(location == LOCATION_EXTRA && (position == 0 || (pcard->data.type & TYPE_PENDULUM) == 0))
			position = POS_FACEDOWN_DEFENSE;
		pcard->sendto_param.position = position;
		if(location == LOCATION_PZONE) {
			int32_t seq = field->get_pzone_index(sequence, playerid);
			field->add_card(playerid, pcard, LOCATION_SZONE, seq, TRUE);
		} else if(location == LOCATION_FZONE) {
			int32_t loc = LOCATION_SZONE;
			field->add_card(playerid, pcard, loc, 5);
		} else
			field->add_card(playerid, pcard, location, sequence);
		pcard->current.position = position;
		if(!(location & (LOCATION_ONFIELD | LOCATION_PZONE)) || (position & POS_FACEUP)) {
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
LUA_FUNCTION(SetPlayerInfo) {
	check_param_count(L, 4);
	const auto pduel = lua_get<duel*>(L);
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
LUA_FUNCTION(PreSummon) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto summon_type = lua_get<uint32_t>(L, 2);
	auto summon_location = lua_get<uint8_t, 0>(L, 3);
	pcard->summon_info = summon_type | (summon_location << 16);
	return 0;
}
LUA_FUNCTION(PreEquip) {
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
LUA_FUNCTION(PreSetTarget) {
	check_param_count(L, 2);
	auto t_card = lua_get<card*, true>(L, 1);
	auto target = lua_get<card*, true>(L, 2);
	t_card->add_card_target(target);
	return 0;
}
LUA_FUNCTION(PreAddCounter) {
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
LUA_FUNCTION(ReloadFieldBegin) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
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
LUA_FUNCTION(ReloadFieldEnd) {
	const auto pduel = lua_get<duel*>(L);
	auto& field = pduel->game_field;
	auto& core = field->core;
	core.shuffle_hand_check[0] = FALSE;
	core.shuffle_hand_check[1] = FALSE;
	core.shuffle_deck_check[0] = FALSE;
	core.shuffle_deck_check[1] = FALSE;
	field->reload_field_info();
	if(lua_isyieldable(L))
		return lua_yield(L, 0);
	return 0;
}
template<int message_code, size_t max_len>
int32_t write_string_message(lua_State* L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_STRING, 1);
	size_t len = 0;
	const char* pstr = lua_tolstring(L, 1, &len);
	if(len > max_len)
		len = max_len;
	const auto pduel = lua_get<duel*>(L);
	auto message = pduel->new_message(message_code);
	message->write<uint16_t>(static_cast<uint16_t>(len));
	message->write(pstr, len);
	message->write<uint8_t>(0);
	return 0;
}

LUA_FUNCTION_EXISTING(SetAIName, write_string_message<MSG_AI_NAME, 100>);
LUA_FUNCTION_EXISTING(ShowHint, write_string_message<MSG_SHOW_HINT, 1024>);

LUA_FUNCTION(PrintStacktrace) {
	interpreter::print_stacktrace(L);
	return 0;
}

}

void scriptlib::push_debug_lib(lua_State* L) {
	static constexpr auto debuglib = GET_LUA_FUNCTIONS_ARRAY();
	static_assert(debuglib.back().name == nullptr, "");
	lua_createtable(L, 0, debuglib.size() - 1);
	luaL_setfuncs(L, debuglib.data(), 0);
	lua_setglobal(L, "Debug");
}