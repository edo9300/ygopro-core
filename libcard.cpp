/*
 * libcard.cpp
 *
 *  Created on: 2010-5-6
 *      Author: Argon
 */

#include "scriptlib.h"
#include "duel.h"
#include "field.h"
#include "card.h"
#include "effect.h"
#include "group.h"

namespace {

using namespace scriptlib;

int32_t card_get_code(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	if (lua_gettop(L) > 1) {
		card* scard = 0;
		uint8_t playerid = PLAYER_NONE;
		if (lua_gettop(L) > 1 && !lua_isnoneornil(L, 2))
			scard = lua_get<card*, true>(L, 2);
		auto sumtype = lua_get<uint64_t, 0>(L, 3);
		if (lua_gettop(L) > 3)
			playerid = lua_get<uint8_t>(L, 4);
		else if (sumtype == SUMMON_TYPE_FUSION)
			playerid = pduel->game_field->core.reason_player;
		int32_t count = pcard->get_summon_code(scard, sumtype, playerid);
		if (count == 0) {
			lua_pushnil(L);
			return 1;
		}
		return count;
	}
	lua_pushinteger(L, pcard->get_code());
	uint32_t otcode = pcard->get_another_code();
	if(otcode) {
		lua_pushinteger(L, otcode);
		return 2;
	}
	return 1;
}
// GetOriginalCode(): get the original code printed on card
// return: 1 int
int32_t card_get_origin_code(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	if(pcard->data.alias) {
		int32_t dif = pcard->data.code - pcard->data.alias;
		if(dif > -10 && dif < 10)
			lua_pushinteger(L, pcard->data.alias);
		else
			lua_pushinteger(L, pcard->data.code);
	} else
		lua_pushinteger(L, pcard->data.code);
	return 1;
}
// GetOriginalCodeRule(): get the original code in duel (can be different from printed code)
// return: 1-2 int
int32_t card_get_origin_code_rule(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	effect_set eset;
	pcard->filter_effect(EFFECT_ADD_CODE, &eset);
	if(pcard->data.alias && !eset.size())
		lua_pushinteger(L, pcard->data.alias);
	else {
		lua_pushinteger(L, pcard->data.code);
		if(eset.size()) {
			uint32_t otcode = eset.back()->get_value(pcard);
			lua_pushinteger(L, otcode);
			return 2;
		}
	}
	return 1;
}
int32_t card_get_set_card(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	if (lua_gettop(L) > 1) {
		card* scard = 0;
		uint8_t playerid = PLAYER_NONE;
		if (lua_gettop(L) > 1 && !lua_isnoneornil(L, 2))
			scard = lua_get<card*, true>(L, 2);
		auto sumtype = lua_get<uint64_t, 0>(L, 3);
		if (lua_gettop(L) > 3)
			playerid = lua_get<uint8_t>(L, 4);
		else if (sumtype == SUMMON_TYPE_FUSION)
			playerid = pduel->game_field->core.reason_player;
		int32_t count = pcard->get_summon_set_card(scard, sumtype, playerid);
		if (count == 0) {
			lua_pushnil(L);
			return 1;
		}
		return count;
	}
	int32_t count = pcard->get_set_card();
	if (count == 0) {
		lua_pushnil(L);
		return 1;
	}
	return count;
}
int32_t card_get_origin_set_card(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	uint32_t n = 0;
	for(auto& setcode : pcard->get_origin_set_card()) {
		lua_pushinteger(L, setcode);
		++n;
	}
	return n;
}
int32_t card_get_pre_set_card(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	int32_t count = pcard->get_pre_set_card();
	if (count == 0) {
		lua_pushnil(L);
		return 1;
	}
	return count;
}
int32_t card_get_type(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	card* scard = 0;
	uint8_t playerid = PLAYER_NONE;
	if (lua_gettop(L) > 1 && !lua_isnoneornil(L, 2))
		scard = lua_get<card*, true>(L, 2);
	auto sumtype = lua_get<uint64_t, 0>(L, 3);
	if (lua_gettop(L) > 3)
		playerid = lua_get<uint8_t>(L, 4);
	else if (sumtype == SUMMON_TYPE_FUSION)
		playerid = pduel->game_field->core.reason_player;
	lua_pushinteger(L, pcard->get_type(scard, sumtype, playerid));
	return 1;
}
int32_t card_get_origin_type(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->data.type);
	return 1;
}
int32_t card_get_level(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->get_level());
	return 1;
}
int32_t card_get_rank(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->get_rank());
	return 1;
}
int32_t card_get_link(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->get_link());
	return 1;
}
int32_t card_get_synchro_level(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto scard = lua_get<card*, true>(L, 2);
	lua_pushinteger(L, pcard->get_synchro_level(scard));
	return 1;
}
int32_t card_get_ritual_level(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto scard = lua_get<card*, true>(L, 2);
	lua_pushinteger(L, pcard->get_ritual_level(scard));
	return 1;
}
int32_t card_get_origin_level(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	if((pcard->data.type & (TYPE_XYZ | TYPE_LINK)) || (pcard->status & STATUS_NO_LEVEL))
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->data.level);
	return 1;
}
int32_t card_get_origin_rank(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	if(!(pcard->data.type & TYPE_XYZ))
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->data.level);
	return 1;
}
int32_t card_is_xyz_level(lua_State* L) {
	check_param_count(L, 3);
	auto pcard = lua_get<card*, true>(L, 1);
	auto xyzcard = lua_get<card*, true>(L, 2);
	auto lv = lua_get<uint32_t>(L, 3);
	lua_pushboolean(L, pcard->check_xyz_level(xyzcard, lv));
	return 1;
}
int32_t card_get_lscale(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->get_lscale());
	return 1;
}
int32_t card_get_origin_lscale(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->data.lscale);
	return 1;
}
int32_t card_get_rscale(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->get_rscale());
	return 1;
}
int32_t card_get_origin_rscale(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->data.rscale);
	return 1;
}
int32_t card_get_link_marker(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->get_link_marker());
	return 1;
}
int32_t card_is_link_marker(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto dir = lua_get<uint32_t>(L, 2);
	lua_pushboolean(L, pcard->is_link_marker(dir));
	return 1;
}
int32_t card_get_linked_group(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	card_set cset;
	pcard->get_linked_cards(&cset);
	group* pgroup = pduel->new_group(cset);
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t card_get_linked_group_count(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	card_set cset;
	pcard->get_linked_cards(&cset);
	lua_pushinteger(L, cset.size());
	return 1;
}
int32_t card_get_linked_zone(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	uint32_t zone = pcard->get_linked_zone();
	auto cp = lua_get<uint8_t>(L, 2, pcard->current.controler);
	if(cp == 1 - pcard->current.controler)
		lua_pushinteger(L, (((zone & 0xffff) << 16) | (zone >> 16)));
	else
		lua_pushinteger(L, zone);
	return 1;
}
int32_t card_get_free_linked_zone(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	uint32_t zone = pcard->get_linked_zone(true);
	auto cp = lua_get<uint8_t>(L, 2, pcard->current.controler);
	if(cp == 1 - pcard->current.controler)
		lua_pushinteger(L, (((zone & 0xffff) << 16) | (zone >> 16)));
	else
		lua_pushinteger(L, zone);
	return 1;
}
int32_t card_get_mutual_linked_group(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	card_set cset;
	pcard->get_mutual_linked_cards(&cset);
	group* pgroup = pduel->new_group(cset);
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t card_get_mutual_linked_group_count(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	card_set cset;
	pcard->get_mutual_linked_cards(&cset);
	lua_pushinteger(L, cset.size());
	return 1;
}
int32_t card_get_mutual_linked_zone(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	uint32_t zone = pcard->get_mutual_linked_zone();
	auto cp = lua_get<uint8_t>(L, 2, pcard->current.controler);
	if(cp == 1 - pcard->current.controler)
		lua_pushinteger(L, (((zone & 0xffff) << 16) | (zone >> 16)));
	else
		lua_pushinteger(L, zone);
	return 1;
}
int32_t card_is_linked(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_link_state());
	return 1;
}
int32_t card_is_extra_linked(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_extra_link_state());
	return 1;
}
int32_t card_get_column_group(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto left = lua_get<uint8_t, 0>(L, 2);
	auto right = lua_get<uint8_t, 0>(L, 3);
	card_set cset;
	pcard->get_column_cards(&cset, left, right);
	group* pgroup = pduel->new_group(cset);
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t card_get_column_group_count(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	auto left = lua_get<uint8_t, 0>(L, 2);
	auto right = lua_get<uint8_t, 0>(L, 3);
	card_set cset;
	pcard->get_column_cards(&cset, left, right);
	lua_pushinteger(L, cset.size());
	return 1;
}
int32_t card_get_column_zone(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto loc = lua_get<uint16_t>(L, 2);
	auto left = lua_get<uint8_t, 0>(L, 3);
	auto right = lua_get<uint8_t, 0>(L, 4);
	auto cp = lua_get<uint8_t>(L, 5, pcard->current.controler);
	uint32_t zone = pcard->get_column_zone(loc, left, right);
	if(cp == 1 - pcard->current.controler)
		lua_pushinteger(L, (((zone & 0xffff) << 16) | (zone >> 16)));
	else
		lua_pushinteger(L, zone);
	return 1;
}
int32_t card_is_all_column(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_all_column());
	return 1;
}
int32_t card_get_attribute(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	card* scard = 0;
	uint8_t playerid = PLAYER_NONE;
	if (lua_gettop(L) > 1 && !lua_isnoneornil(L, 2))
		scard = lua_get<card*, true>(L, 2);
	auto sumtype = lua_get<uint64_t, 0>(L, 3);
	if (lua_gettop(L) > 3)
		playerid = lua_get<uint8_t>(L, 4);
	else if (sumtype == SUMMON_TYPE_FUSION)
		playerid = pduel->game_field->core.reason_player;
	lua_pushinteger(L, pcard->get_attribute(scard, sumtype, playerid));
	return 1;
}
int32_t card_get_origin_attribute(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	if(pcard->status & STATUS_NO_LEVEL)
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->data.attribute);
	return 1;
}
int32_t card_get_race(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	card* scard = 0;
	uint8_t playerid = PLAYER_NONE;
	if (lua_gettop(L) > 1 && !lua_isnoneornil(L, 2))
		scard = lua_get<card*, true>(L, 2);
	auto sumtype = lua_get<uint64_t, 0>(L, 3);
	if (lua_gettop(L) > 3)
		playerid = lua_get<uint8_t>(L, 4);
	else if (sumtype == SUMMON_TYPE_FUSION)
		playerid = pduel->game_field->core.reason_player;
	lua_pushinteger(L, pcard->get_race(scard, sumtype, playerid));
	return 1;
}
int32_t card_get_origin_race(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	if(pcard->status & STATUS_NO_LEVEL)
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->data.race);
	return 1;
}
int32_t card_get_attack(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	int32_t atk = pcard->get_attack();
	if(atk < 0)
		atk = 0;
	lua_pushinteger(L, atk);
	return 1;
}
int32_t card_get_origin_attack(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	int32_t atk = pcard->get_base_attack();
	if(atk < 0)
		atk = 0;
	lua_pushinteger(L, atk);
	return 1;
}
int32_t card_get_text_attack(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	if(pcard->status & STATUS_NO_LEVEL)
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->data.attack);
	return 1;
}
int32_t card_get_defense(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	int32_t def = pcard->get_defense();
	if(def < 0)
		def = 0;
	lua_pushinteger(L, def);
	return 1;
}
int32_t card_get_origin_defense(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	int32_t def = pcard->get_base_defense();
	if(def < 0)
		def = 0;
	lua_pushinteger(L, def);
	return 1;
}
int32_t card_get_text_defense(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	if(pcard->status & STATUS_NO_LEVEL)
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->data.defense);
	return 1;
}
int32_t card_get_previous_code_onfield(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->previous.code);
	if(pcard->previous.code2) {
		lua_pushinteger(L, pcard->previous.code2);
		return 2;
	}
	return 1;
}
int32_t card_get_previous_type_onfield(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->previous.type);
	return 1;
}
int32_t card_get_previous_level_onfield(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->previous.level);
	return 1;
}
int32_t card_get_previous_rank_onfield(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->previous.rank);
	return 1;
}
int32_t card_get_previous_attribute_onfield(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->previous.attribute);
	return 1;
}
int32_t card_get_previous_race_onfield(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->previous.race);
	return 1;
}
int32_t card_get_previous_attack_onfield(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->previous.attack);
	return 1;
}
int32_t card_get_previous_defense_onfield(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->previous.defense);
	return 1;
}
int32_t card_get_owner(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->owner);
	return 1;
}
int32_t card_get_controler(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->current.controler);
	return 1;
}
int32_t card_get_previous_controler(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->previous.controler);
	return 1;
}
int32_t card_get_reason(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->current.reason);
	return 1;
}
int32_t card_get_reason_card(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	interpreter::pushobject(L, pcard->current.reason_card);
	return 1;
}
int32_t card_get_reason_player(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->current.reason_player);
	return 1;
}
int32_t card_get_reason_effect(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	interpreter::pushobject(L, pcard->current.reason_effect);
	return 1;
}
int32_t card_set_reason(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto reason = lua_get<uint32_t>(L, 2);
	if (lua_get<bool, false>(L, 3))
		pcard->current.reason |= reason;
	else
		pcard->current.reason = reason;
	return 0;
}
int32_t card_set_reason_card(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto rcard = lua_get<card*, true>(L, 2);
	pcard->current.reason_card = rcard;
	return 0;
}
int32_t card_set_reason_player(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto rp = lua_get<uint8_t>(L, 2);
	if (rp <= (unsigned)PLAYER_ALL)
		pcard->current.reason_player = rp;
	return 0;
}
int32_t card_set_reason_effect(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto re = lua_get<effect*, true>(L, 2);
	pcard->current.reason_effect = re;
	return 0;
}
int32_t card_get_position(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->current.position);
	return 1;
}
int32_t card_get_previous_position(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->previous.position);
	return 1;
}
int32_t card_get_battle_position(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->temp.position);
	return 1;
}
int32_t card_get_location(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	if(pcard->get_status(STATUS_SUMMONING | STATUS_SUMMON_DISABLED | STATUS_ACTIVATE_DISABLED | STATUS_SPSUMMON_STEP))
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->current.location);
	return 1;
}
int32_t card_get_previous_location(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->previous.location);
	return 1;
}
int32_t card_get_sequence(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->current.sequence);
	return 1;
}
int32_t card_get_previous_sequence(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->previous.sequence);
	return 1;
}
int32_t card_get_summon_type(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->summon_info & 0xff00ffff);
	return 1;
}
int32_t card_get_summon_location(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, (pcard->summon_info >> 16) & 0xff);
	return 1;
}
int32_t card_get_summon_player(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->summon_player);
	return 1;
}
int32_t card_get_destination(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->sendto_param.location);
	return 1;
}
int32_t card_get_leave_field_dest(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->leave_field_redirect(REASON_EFFECT));
	return 1;
}
int32_t card_get_turnid(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->turnid);
	return 1;
}
int32_t card_get_fieldid(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->fieldid);
	return 1;
}
int32_t card_get_fieldidr(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->fieldid_r);
	return 1;
}
int32_t card_get_cardid(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->cardid);
	return 1;
}
int32_t card_is_origin_code_rule(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	uint32_t code1 = 0;
	uint32_t code2 = 0;
	effect_set eset;
	pcard->filter_effect(EFFECT_ADD_CODE, &eset);
	if(pcard->data.alias && !eset.size()){
		code1 = pcard->data.alias;
		code2 = 0;
	} else {
		code1 = pcard->data.code;
		if(eset.size())
			code2 = eset.back()->get_value(pcard);
	}
	uint32_t count = lua_gettop(L) - 1;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnoneornil(L, i + 2))
			continue;
		auto tcode = lua_get<uint32_t>(L, i + 2);
		if(code1 == tcode || (code2 && code2 == tcode)) {
			lua_pushboolean(L, TRUE);
			return 1;
		}
	}
	lua_pushboolean(L, FALSE);
	return 1;
}
int32_t card_is_code(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	uint32_t code1 = pcard->get_code();
	uint32_t code2 = pcard->get_another_code();
	uint32_t count = lua_gettop(L) - 1;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnoneornil(L, i + 2))
			continue;
		uint32_t tcode = lua_get<uint32_t>(L, i + 2);
		if(code1 == tcode || (code2 && code2 == tcode)) {
			lua_pushboolean(L, TRUE);
			return 1;
		}
	}
	lua_pushboolean(L, FALSE);
	return 1;
}
int32_t card_is_summon_code(lua_State* L) {
	check_param_count(L, 5);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto scard = lua_get<card*>(L, 2);
	auto sumtype = lua_get<uint64_t>(L, 3);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 4);
	effect_set eset;
	std::set<uint32_t> codes;
	pcard->filter_effect(EFFECT_ADD_CODE, &eset, FALSE);
	pcard->filter_effect(EFFECT_REMOVE_CODE, &eset, FALSE);
	pcard->filter_effect(EFFECT_CHANGE_CODE, &eset);
	uint32_t code1 = pcard->get_code();
	uint32_t code2 = pcard->get_another_code();
	codes.insert(code1);
	if (code2)
		codes.insert(code2);
	for (const auto& peff : eset) {
		if (!peff->operation)
			continue;
		pduel->lua->add_param<PARAM_TYPE_CARD>(scard);
		pduel->lua->add_param<PARAM_TYPE_INT>(sumtype);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		if (!pduel->lua->check_condition(peff->operation, 3))
			continue;
		if (peff->code == EFFECT_ADD_CODE)
			codes.insert(peff->get_value(pcard));
		else if (peff->code == EFFECT_REMOVE_CODE) {
			auto cit = codes.find(peff->get_value(pcard));
			if (cit != codes.end())
				codes.erase(cit);
		} else {
			codes.clear();
			codes.insert(peff->get_value(pcard));
		}
	}
	uint32_t count = lua_gettop(L) - 4;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnoneornil(L, i + 5))
			continue;
		auto tcode = lua_get<uint32_t>(L, i + 5);
		if(codes.find(tcode) != codes.end()) {
			lua_pushboolean(L, TRUE);
			return 1;
		}
	}
	lua_pushboolean(L, FALSE);
	return 1;
}
int32_t card_is_set_card(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	uint32_t set_code = lua_get<uint32_t>(L, 2);
	if (lua_gettop(L) > 2) {
		card* scard = 0;
		uint8_t playerid = PLAYER_NONE;
		if (lua_gettop(L) > 2 && !lua_isnoneornil(L, 3))
			scard = lua_get<card*, true>(L, 3);
		auto sumtype = lua_get<uint64_t, 0>(L, 4);
		playerid = lua_get<uint8_t, PLAYER_NONE>(L, 5);
		lua_pushboolean(L, pcard->is_sumon_set_card(set_code, scard, sumtype, playerid));
	} else
		lua_pushboolean(L, pcard->is_set_card(set_code));
	return 1;
}
int32_t card_is_origin_set_card(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	uint32_t set_code = lua_get<uint32_t>(L, 2);
	lua_pushboolean(L, pcard->is_origin_set_card(set_code));
	return 1;
}
int32_t card_is_pre_set_card(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	uint32_t set_code = lua_get<uint32_t>(L, 2);
	lua_pushboolean(L, pcard->is_pre_set_card(set_code));
	return 1;
}
int32_t card_is_type(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto ttype = lua_get<uint32_t>(L, 2);
	card* scard = 0;
	uint8_t playerid = PLAYER_NONE;
	if (lua_gettop(L) > 2 && !lua_isnoneornil(L, 3))
		scard = lua_get<card*, true>(L, 3);
	auto sumtype = lua_get<uint64_t, 0>(L, 4);
	if (lua_gettop(L) > 4)
		playerid = lua_get<uint8_t>(L, 5);
	else if (sumtype == SUMMON_TYPE_FUSION)
		playerid = pduel->game_field->core.reason_player;
	lua_pushboolean(L, pcard->get_type(scard, sumtype, playerid) & ttype);
	return 1;
}
inline int32_t is_prop(lua_State* L, uint32_t val) {
	uint32_t count = lua_gettop(L) - 1;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnoneornil(L, i + 2))
			continue;
		if(val == lua_get<uint32_t>(L, i + 2)) {
			lua_pushboolean(L, TRUE);
			return 1;
		}
	}
	lua_pushboolean(L, FALSE);
	return 1;
}
int32_t card_is_level(lua_State* L) {
	check_param_count(L, 2);
	return is_prop(L, lua_get<card*, true>(L, 1)->get_level());
}
int32_t card_is_rank(lua_State* L) {
	check_param_count(L, 2);
	return is_prop(L, lua_get<card*, true>(L, 1)->get_rank());
}
int32_t card_is_link(lua_State* L) {
	check_param_count(L, 2);
	return is_prop(L, lua_get<card*, true>(L, 1)->get_link());
}
int32_t card_is_attack(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	if(!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !pcard->is_affected_by_effect(EFFECT_PRE_MONSTER))
		lua_pushboolean(L, 0);
	else
		is_prop(L, pcard->get_attack());
	return 1;
}
int32_t card_is_defense(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	if((pcard->data.type & TYPE_LINK)
	   || (!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !pcard->is_affected_by_effect(EFFECT_PRE_MONSTER)))
		lua_pushboolean(L, 0);
	else
		is_prop(L, pcard->get_defense());
	return 1;
}
int32_t card_is_race(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto trace = lua_get<uint32_t>(L, 2);
	card* scard = 0;
	auto playerid = PLAYER_NONE;
	if(lua_gettop(L) > 2 && !lua_isnoneornil(L, 3))
		scard = lua_get<card*, true>(L, 3);
	auto sumtype = lua_get<uint64_t, 0>(L, 4);
	if(lua_gettop(L) > 4)
		playerid = lua_get<uint8_t>(L, 5);
	else if(sumtype==SUMMON_TYPE_FUSION)
		playerid = pduel->game_field->core.reason_player;
	lua_pushboolean(L, pcard->get_race(scard, sumtype, playerid) & trace);
	return 1;
}
int32_t card_is_attribute(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto tattrib = lua_get<uint32_t>(L, 2);
	card* scard = 0;
	uint8_t playerid = PLAYER_NONE;
	if(lua_gettop(L) > 2 && !lua_isnoneornil(L, 3))
		scard = lua_get<card*, true>(L, 3);
	auto sumtype = lua_get<uint64_t, 0>(L, 4);
	if(lua_gettop(L) > 4)
		playerid = lua_get<uint8_t>(L, 5);
	else if(sumtype==SUMMON_TYPE_FUSION)
		playerid = pduel->game_field->core.reason_player;
	lua_pushboolean(L, pcard->get_attribute(scard, sumtype, playerid) & tattrib);
	return 1;
}
int32_t card_is_reason(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto treason = lua_get<uint32_t>(L, 2);
	lua_pushboolean(L, pcard->current.reason & treason);
	return 1;
}
int32_t card_is_summon_type(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnoneornil(L, i + 2))
			continue;
		auto ttype = lua_get<uint32_t>(L, i + 2);
		if(((pcard->summon_info & 0xff00ffff) & ttype) == ttype) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t card_is_status(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto tstatus = lua_get<uint32_t>(L, 2);
	lua_pushboolean(L, pcard->status & tstatus);
	return 1;
}
int32_t card_is_not_tuner(lua_State* L) {
	check_param_count(L, 3);
	auto pcard = lua_get<card*, true>(L, 1);
	auto scard = lua_get<card*, true>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 3);
	lua_pushboolean(L, pcard->is_not_tuner(scard, playerid));
	return 1;
}
int32_t card_set_status(lua_State* L) {
	check_param_count(L, 3);
	auto pcard = lua_get<card*, true>(L, 1);
	if(pcard->status & STATUS_COPYING_EFFECT)
		return 0;
	auto tstatus = lua_get<uint32_t>(L, 2);
	auto enable = lua_get<bool>(L, 3);
	pcard->set_status(tstatus, enable);
	return 0;
}
int32_t card_is_gemini_state(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, !!pcard->is_affected_by_effect(EFFECT_GEMINI_STATUS));
	return 1;
}
int32_t card_enable_gemini_state(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	effect* deffect = pduel->new_effect();
	deffect->owner = pcard;
	deffect->code = EFFECT_GEMINI_STATUS;
	deffect->type = EFFECT_TYPE_SINGLE;
	deffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_CLIENT_HINT;
	deffect->description = 64;
	deffect->reset_flag = RESET_EVENT + 0x1fe0000;
	pcard->add_effect(deffect);
	return 0;
}
int32_t card_set_turn_counter(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto ct = lua_get<uint16_t>(L, 2);
	pcard->count_turn(ct);
	return 0;
}
int32_t card_get_turn_counter(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->turn_counter);
	return 1;
}
int32_t card_set_material(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	if(!lua_isnoneornil(L, 2)) {
		card* matcard{ nullptr };
		group* pgroup{ nullptr };
		get_card_or_group(L, 2, matcard, pgroup);
		if(matcard) {
			card_set mats{ matcard };
			pcard->set_material(&mats);
		} else
			pcard->set_material(&pgroup->container);
	} else
		pcard->set_material(nullptr);
	return 0;
}
int32_t card_get_material(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	group* pgroup = pduel->new_group(pcard->material_cards);
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t card_get_material_count(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->material_cards.size());
	return 1;
}
int32_t card_get_equip_group(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	group* pgroup = pduel->new_group(pcard->equiping_cards);
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t card_get_equip_count(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->equiping_cards.size());
	return 1;
}
int32_t card_get_equip_target(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	interpreter::pushobject(L, pcard->equiping_target);
	return 1;
}
int32_t card_get_pre_equip_target(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	interpreter::pushobject(L, pcard->pre_equip_target);
	return 1;
}
int32_t card_check_equip_target(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	auto target = lua_get<card*, true>(L, 2);
	if(pcard->is_affected_by_effect(EFFECT_EQUIP_LIMIT, target)
		&& ((!pcard->is_affected_by_effect(EFFECT_OLDUNION_STATUS) || target->get_union_count() == 0)
			&& (!pcard->is_affected_by_effect(EFFECT_UNION_STATUS) || target->get_old_union_count() == 0)))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t card_check_union_target(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	auto target = lua_get<card*, true>(L, 2);
	if(pcard->is_affected_by_effect(EFFECT_UNION_LIMIT, target)
		&& ((!pcard->is_affected_by_effect(EFFECT_OLDUNION_STATUS) || target->get_union_count() == 0)
			&& (!pcard->is_affected_by_effect(EFFECT_UNION_STATUS) || target->get_old_union_count() == 0)))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t card_get_union_count(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->get_union_count());
	lua_pushinteger(L, pcard->get_old_union_count());
	return 2;
}
int32_t card_get_overlay_group(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	group* pgroup = pduel->new_group(pcard->xyz_materials);
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t card_get_overlay_count(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->xyz_materials.size());
	return 1;
}
int32_t card_get_overlay_target(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	interpreter::pushobject(L, pcard->overlay_target);
	return 1;
}
int32_t card_check_remove_overlay_card(lua_State* L) {
	check_param_count(L, 4);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	group* pgroup = pduel->new_group(pcard);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto count = lua_get<uint16_t>(L, 3);
	auto reason = lua_get<uint32_t>(L, 4);
	lua_pushboolean(L, pduel->game_field->is_player_can_remove_overlay_card(playerid, pgroup, 0, 0, count, reason));
	return 1;
}
int32_t card_remove_overlay_card(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 5);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	group* pgroup = pduel->new_group(pcard);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto min = lua_get<uint16_t>(L, 3);
	auto max = lua_get<uint16_t>(L, 4);
	auto reason = lua_get<uint32_t>(L, 5);
	pduel->game_field->remove_overlay_card(reason, pgroup, playerid, 0, 0, min, max);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t card_get_attacked_group(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	group* pgroup = pduel->new_group();
	for(auto& cit : pcard->attacked_cards) {
		if(cit.second.first)
			pgroup->container.insert(cit.second.first);
	}
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t card_get_attacked_group_count(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->attacked_cards.size());
	return 1;
}
int32_t card_get_attacked_count(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->attacked_count);
	return 1;
}
int32_t card_get_battled_group(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	group* pgroup = pduel->new_group();
	for(auto& cit : pcard->battled_cards) {
		if(cit.second.first)
			pgroup->container.insert(cit.second.first);
	}
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t card_get_battled_group_count(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->battled_cards.size());
	return 1;
}
int32_t card_get_attack_announced_count(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->attack_announce_count);
	return 1;
}
int32_t card_is_direct_attacked(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->attacked_cards.findcard(0));
	return 1;
}
int32_t card_set_card_target(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto ocard = lua_get<card*, true>(L, 2);
	pcard->add_card_target(ocard);
	return 0;
}
int32_t card_get_card_target(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	group* pgroup = pduel->new_group(pcard->effect_target_cards);
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t card_get_first_card_target(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	if(pcard->effect_target_cards.size())
		interpreter::pushobject(L, *pcard->effect_target_cards.begin());
	else lua_pushnil(L);
	return 1;
}
int32_t card_get_card_target_count(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->effect_target_cards.size());
	return 1;
}
int32_t card_is_has_card_target(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto rcard = lua_get<card*, true>(L, 2);
	lua_pushboolean(L, pcard->effect_target_cards.count(rcard));
	return 1;
}
int32_t card_cancel_card_target(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto rcard = lua_get<card*, true>(L, 2);
	pcard->cancel_card_target(rcard);
	return 0;
}
int32_t card_get_owner_target(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	group* pgroup = pduel->new_group(pcard->effect_target_owner);
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t card_get_owner_target_count(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushinteger(L, pcard->effect_target_owner.size());
	return 1;
}
int32_t card_get_activate_effect(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	int32_t count = 0;
	for(auto& eit : pcard->field_effect) {
		if(eit.second->type & EFFECT_TYPE_ACTIVATE) {
			interpreter::pushobject(L, eit.second);
			++count;
		}
	}
	return count;
}
int32_t card_check_activate_effect(lua_State* L) {
	check_param_count(L, 4);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	bool neglect_con = lua_get<bool>(L, 2);
	bool neglect_cost = lua_get<bool>(L, 3);
	bool copy_info = lua_get<bool>(L, 4);
	tevent pe;
	for(const auto& eit : pcard->field_effect) {
		effect* peffect = eit.second;
		if((peffect->type & EFFECT_TYPE_ACTIVATE)
		        && pduel->game_field->check_event_c(peffect, pduel->game_field->core.reason_player, neglect_con, neglect_cost, copy_info, &pe)) {
			if(!copy_info || (peffect->code == EVENT_FREE_CHAIN)) {
				interpreter::pushobject(L, peffect);
				return 1;
			} else {
				interpreter::pushobject(L, peffect);
				interpreter::pushobject(L, pe.event_cards);
				lua_pushinteger(L, pe.event_player);
				lua_pushinteger(L, pe.event_value);
				interpreter::pushobject(L, pe.reason_effect);
				lua_pushinteger(L, pe.reason);
				lua_pushinteger(L, pe.reason_player);
				return 7;
			}
		}
	}
	return 0;
}
int32_t card_register_effect(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto peffect = lua_get<effect*, true>(L, 2);
	bool forced = lua_get<bool, false>(L, 3);
	if(peffect->owner == pduel->game_field->temp_card)
		return 0;
	if(!forced && pduel->game_field->core.reason_effect && !pcard->is_affect_by_effect(pduel->game_field->core.reason_effect)) {
		pduel->game_field->core.reseted_effects.insert(peffect);
		return 0;
	}
	int32_t id = -1;
	if (!peffect->handler)
		id = pcard->add_effect(peffect);
	lua_pushinteger(L, id);
	return 1;
}
int32_t card_is_has_effect(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto code = lua_get<uint32_t>(L, 2);
	if(!pcard) {
		lua_pushnil(L);
		return 1;
	}
	effect_set eset;
	pcard->filter_effect(code, &eset);
	int32_t size = eset.size();
	if(!size) {
		lua_pushnil(L);
		return 1;
	}
	auto check_player = lua_get<uint8_t, PLAYER_NONE>(L, 3);
	for(const auto& peff : eset) {
		if(check_player == PLAYER_NONE || peff->check_count_limit(check_player))
			interpreter::pushobject(L, peff);
		else
			--size;
	}
	if(!size) {
		lua_pushnil(L);
		return 1;
	}
	return size;
}
int32_t card_get_card_effect(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	auto code = lua_get<uint32_t, 0>(L, 2);
	int32_t count = pcard->get_card_effect(code);
	if (count == 0) {
		lua_pushnil(L);
		return 1;
	}
	return count;
}
int32_t card_reset_effect(lua_State* L) {
	check_param_count(L, 3);
	auto pcard = lua_get<card*, true>(L, 1);
	auto code = lua_get<uint32_t>(L, 2);
	auto type = lua_get<uint32_t>(L, 3);
	pcard->reset(code, type);
	return 0;
}
int32_t card_get_effect_count(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto code = lua_get<uint32_t>(L, 2);
	effect_set eset;
	pcard->filter_effect(code, &eset);
	lua_pushinteger(L, eset.size());
	return 1;
}
int32_t card_register_flag_effect(lua_State* L) {
	check_param_count(L, 5);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	auto reset = lua_get<uint32_t>(L, 3);
	auto flag = lua_get<uint32_t>(L, 4);
	auto count = lua_get<uint16_t>(L, 5);
	auto lab = lua_get<uint32_t, 0>(L, 6);
	auto desc = lua_get<uint64_t, 0>(L, 7);
	if(count == 0)
		count = 1;
	if(reset & (RESET_PHASE) && !(reset & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		reset |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	effect* peffect = pduel->new_effect();
	peffect->owner = pcard;
	peffect->handler = 0;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = code;
	peffect->reset_flag = reset;
	peffect->flag[0] = flag | EFFECT_FLAG_CANNOT_DISABLE;
	peffect->reset_count = count;
	peffect->label = { lab };
	peffect->description = desc;
	pcard->add_effect(peffect);
	interpreter::pushobject(L, peffect);
	return 1;
}
int32_t card_get_flag_effect(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	lua_pushinteger(L, pcard->single_effect.count(code));
	return 1;
}
int32_t card_reset_flag_effect(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	pcard->reset(code, RESET_CODE);
	return 0;
}
int32_t card_set_flag_effect_label(lua_State* L) {
	check_param_count(L, 3);
	auto pcard = lua_get<card*, true>(L, 1);
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	auto lab = lua_get<uint32_t>(L, 3);
	auto eit = pcard->single_effect.find(code);
	if(eit == pcard->single_effect.end())
		lua_pushboolean(L, FALSE);
	else {
		eit->second->label = { lab };
		lua_pushboolean(L, TRUE);
	}
	return 1;
}
int32_t card_get_flag_effect_label(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	auto rg = pcard->single_effect.equal_range(code);
	int32_t count = 0;
	for(; rg.first != rg.second; ++rg.first, ++count)
		lua_pushinteger(L, rg.first->second->label.size() ? rg.first->second->label[0] : 0);
	if(!count) {
		lua_pushnil(L);
		return 1;
	}
	return count;
}
int32_t card_create_relation(lua_State* L) {
	check_param_count(L, 3);
	auto pcard = lua_get<card*, true>(L, 1);
	auto rcard = lua_get<card*, true>(L, 2);
	auto reset = lua_get<uint32_t>(L, 3);
	pcard->create_relation(rcard, reset);
	return 0;
}
int32_t card_release_relation(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto rcard = lua_get<card*, true>(L, 2);
	pcard->release_relation(rcard);
	return 0;
}
int32_t card_create_effect_relation(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto peffect = lua_get<effect*, true>(L, 2);
	pcard->create_relation(peffect);
	return 0;
}
int32_t card_release_effect_relation(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto peffect = lua_get<effect*, true>(L, 2);
	pcard->release_relation(peffect);
	return 0;
}
int32_t card_clear_effect_relation(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	pcard->clear_relate_effect();
	return 0;
}
int32_t card_is_relate_to_effect(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto peffect = lua_get<effect*, true>(L, 2);
	lua_pushboolean(L, pcard->is_has_relation(peffect));
	return 1;
}
int32_t card_is_relate_to_chain(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto chain_count = lua_get<uint8_t>(L, 2);
	if(chain_count > pduel->game_field->core.current_chain.size() || chain_count < 1)
		chain_count = (uint8_t)pduel->game_field->core.current_chain.size();
	lua_pushboolean(L, pcard->is_has_relation(pduel->game_field->core.current_chain[chain_count - 1]));
	return 1;
}
int32_t card_is_relate_to_card(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto rcard = lua_get<card*, true>(L, 2);
	lua_pushboolean(L, pcard->is_has_relation(rcard));
	return 1;
}
int32_t card_is_relate_to_battle(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->fieldid_r == pduel->game_field->core.pre_field[0] || pcard->fieldid_r == pduel->game_field->core.pre_field[1]);
	return 1;
}
int32_t card_copy_effect(lua_State* L) {
	check_param_count(L, 3);
	auto pcard = lua_get<card*, true>(L, 1);
	auto code = lua_get<uint32_t>(L, 2);
	auto reset = lua_get<uint32_t>(L, 3);
	auto count = lua_get<uint8_t, 1>(L, 4);
	if(count == 0)
		count = 1;
	if(reset & RESET_PHASE && !(reset & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		reset |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	lua_pushinteger(L, pcard->copy_effect(code, reset, count));
	return 1;
}
int32_t card_replace_effect(lua_State* L) {
	check_param_count(L, 3);
	auto pcard = lua_get<card*, true>(L, 1);
	auto code = lua_get<uint32_t>(L, 2);
	auto reset = lua_get<uint32_t>(L, 3);
	auto count = lua_get<uint8_t, 0>(L, 4);
	if(count == 0)
		count = 1;
	if(reset & RESET_PHASE && !(reset & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		reset |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	lua_pushinteger(L, pcard->replace_effect(code, reset, count));
	return 1;
}
int32_t card_enable_unsummonable(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	if(!pcard->is_status(STATUS_COPYING_EFFECT)) {
		effect* peffect = pduel->new_effect();
		peffect->owner = pcard;
		peffect->code = EFFECT_UNSUMMONABLE_CARD;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_UNCOPYABLE;
		pcard->add_effect(peffect);
	}
	return 0;
}
int32_t card_enable_revive_limit(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	if(!pcard->is_status(STATUS_COPYING_EFFECT)) {
		effect* peffect1 = pduel->new_effect();
		peffect1->owner = pcard;
		peffect1->code = EFFECT_UNSUMMONABLE_CARD;
		peffect1->type = EFFECT_TYPE_SINGLE;
		peffect1->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_UNCOPYABLE;
		pcard->add_effect(peffect1);
		effect* peffect2 = pduel->new_effect();
		peffect2->owner = pcard;
		peffect2->code = EFFECT_REVIVE_LIMIT;
		peffect2->type = EFFECT_TYPE_SINGLE;
		peffect2->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_UNCOPYABLE;
		pcard->add_effect(peffect2);
	}
	return 0;
}
int32_t card_complete_procedure(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	pcard->set_status(STATUS_PROC_COMPLETE, TRUE);
	return 0;
}
int32_t card_is_disabled(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_status(STATUS_DISABLED));
	return 1;
}
int32_t card_is_destructable(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	effect* peffect = 0;
	if(lua_gettop(L) > 1)
		peffect = lua_get<effect*, true>(L, 2);
	if(peffect)
		lua_pushboolean(L, pcard->is_destructable_by_effect(peffect, pduel->game_field->core.reason_player));
	else
		lua_pushboolean(L, pcard->is_destructable());
	return 1;
}
int32_t card_is_summonable(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_summonable_card());
	return 1;
}
int32_t card_is_fusion_summonable_card(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	auto summon_type = lua_get<uint32_t, 0>(L, 2);
	lua_pushboolean(L, pcard->is_fusion_summonable_card(summon_type));
	return 1;
}
int32_t card_is_msetable(lua_State* L) {
	check_param_count(L, 3);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	bool ign = lua_get<bool>(L, 2);
	effect* peffect = 0;
	if(!lua_isnoneornil(L, 3))
		peffect = lua_get<effect*, true>(L, 3);
	auto minc = lua_get<uint16_t, 0>(L, 4);
	auto zone = lua_get<uint32_t, 0xff>(L, 5);
	lua_pushboolean(L, pcard->is_setable_mzone(pduel->game_field->core.reason_player, ign, peffect, minc, zone));
	return 1;
}
int32_t card_is_ssetable(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	bool ign = lua_get<bool, false>(L, 2);
	lua_pushboolean(L, pcard->is_setable_szone(pduel->game_field->core.reason_player, ign));
	return 1;
}
int32_t card_is_special_summonable(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto sumtype = lua_get<uint32_t, 0>(L, 2);
	lua_pushboolean(L, pcard->is_special_summonable(pduel->game_field->core.reason_player, sumtype));
	return 1;
}
inline int32_t spsummonable_rule(lua_State* L, uint32_t cardtype, uint32_t sumtype, uint32_t offset) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	if(!(pcard->data.type & cardtype))
		return 0;
	group* must = nullptr;
	group* materials = nullptr;
	if(auto _pcard = lua_get<card*>(L, 2 + offset))
		must = pduel->new_group(_pcard);
	else
		must = lua_get<group*>(L, 2 + offset);
	if(auto _pcard = lua_get<card*>(L, 3 + offset))
		materials = pduel->new_group(_pcard);
	else
		materials = lua_get<group*>(L, 3 + offset);
	auto minc = lua_get<uint16_t, 0>(L, 4 + offset);
	auto maxc = lua_get<uint16_t, 0>(L, 5 + offset);
	auto& core = pduel->game_field->core;
	uint8_t p = core.reason_player;
	core.must_use_mats = must;
	core.only_use_mats = materials;
	core.forced_summon_minc = minc;
	core.forced_summon_maxc = maxc;
	lua_pushboolean(L, pcard->is_special_summonable(p, sumtype));
	core.must_use_mats = nullptr;
	core.only_use_mats = nullptr;
	core.forced_summon_minc = 0;
	core.forced_summon_maxc = 0;
	return 1;
}
int32_t card_is_synchro_summonable(lua_State* L) {
	return spsummonable_rule(L, TYPE_SYNCHRO, SUMMON_TYPE_SYNCHRO, 0);
}
int32_t card_is_xyz_summonable(lua_State* L) {
	return spsummonable_rule(L, TYPE_XYZ, SUMMON_TYPE_XYZ, 0);
}
int32_t card_is_link_summonable(lua_State* L) {
	return spsummonable_rule(L, TYPE_LINK, SUMMON_TYPE_LINK, 0);
}
int32_t card_is_procedure_summonable(lua_State* L) {
	check_param_count(L, 3);
	auto cardtype = lua_get<uint32_t, true>(L, 2);
	auto sumtype = lua_get<uint32_t, true>(L, 3);
	return spsummonable_rule(L, cardtype, sumtype, 2);
}
int32_t card_is_can_be_summoned(lua_State* L) {
	check_param_count(L, 3);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto ign = lua_get<bool>(L, 2);
	effect* peffect = 0;
	if(!lua_isnoneornil(L, 3))
		peffect = lua_get<effect*, true>(L, 3);
	auto minc = lua_get<uint16_t, 0>(L, 4);
	auto zone = lua_get<uint16_t, 0x1f>(L, 5);
	lua_pushboolean(L, pcard->is_can_be_summoned(pduel->game_field->core.reason_player, ign, peffect, minc, zone));
	return 1;
}
int32_t card_is_can_be_special_summoned(lua_State* L) {
	check_param_count(L, 6);
	auto pcard = lua_get<card*, true>(L, 1);
	auto peffect = lua_get<effect*, true>(L, 2);
	auto sumtype = lua_get<uint32_t>(L, 3);
	auto sumplayer = lua_get<uint8_t>(L, 4);
	auto nocheck = lua_get<bool>(L, 5);
	auto nolimit = lua_get<bool>(L, 6);
	auto sumpos = lua_get<uint8_t, POS_FACEUP>(L, 7);
	auto toplayer = lua_get<uint8_t>(L, 8, sumplayer);
	auto zone = lua_get<uint32_t, 0xff>(L, 9);
	lua_pushboolean(L, pcard->is_can_be_special_summoned(peffect, sumtype, sumpos, sumplayer, toplayer, nocheck, nolimit, zone));
	return 1;
}
int32_t card_is_able_to_hand(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto p = lua_get<uint8_t>(L, 2, pduel->game_field->core.reason_player);
	lua_pushboolean(L, pcard->is_capable_send_to_hand(p));
	return 1;
}
int32_t card_is_able_to_grave(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_capable_send_to_grave(pduel->game_field->core.reason_player));
	return 1;
}
int32_t card_is_able_to_deck(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_capable_send_to_deck(pduel->game_field->core.reason_player));
	return 1;
}
int32_t card_is_able_to_extra(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_capable_send_to_extra(pduel->game_field->core.reason_player));
	return 1;
}
int32_t card_is_able_to_remove(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto p = lua_get<uint8_t>(L, 2, pduel->game_field->core.reason_player);
	auto pos = lua_get<uint8_t, POS_FACEUP>(L, 3);
	auto reason = lua_get<uint32_t, REASON_EFFECT>(L, 4);
	lua_pushboolean(L, pcard->is_removeable(p, pos, reason));
	return 1;
}
int32_t card_is_able_to_hand_as_cost(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_capable_cost_to_hand(pduel->game_field->core.reason_player));
	return 1;
}
int32_t card_is_able_to_grave_as_cost(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_capable_cost_to_grave(pduel->game_field->core.reason_player));
	return 1;
}
int32_t card_is_able_to_deck_as_cost(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_capable_cost_to_deck(pduel->game_field->core.reason_player));
	return 1;
}
int32_t card_is_able_to_extra_as_cost(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_capable_cost_to_extra(pduel->game_field->core.reason_player));
	return 1;
}
int32_t card_is_able_to_deck_or_extra_as_cost(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto p = pduel->game_field->core.reason_player;
	lua_pushboolean(L, pcard->is_extra_deck_monster() ? pcard->is_capable_cost_to_extra(p) : pcard->is_capable_cost_to_deck(p));
	return 1;
}
int32_t card_is_able_to_remove_as_cost(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto pos = lua_get<uint8_t, POS_FACEUP>(L, 2);
	lua_pushboolean(L, pcard->is_removeable_as_cost(pduel->game_field->core.reason_player, pos));
	return 1;
}
int32_t card_is_releasable(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_releasable_by_nonsummon(pduel->game_field->core.reason_player));
	return 1;
}
int32_t card_is_releasable_by_effect(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto p = pduel->game_field->core.reason_player;
	effect* re = pduel->game_field->core.reason_effect;
	lua_pushboolean(L, pcard->is_releasable_by_nonsummon(p) && pcard->is_releasable_by_effect(p, re));
	return 1;
}
int32_t card_is_discardable(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto p = pduel->game_field->core.reason_player;
	effect* pe = pduel->game_field->core.reason_effect;
	auto reason = lua_get<uint32_t, REASON_COST>(L, 2);
	if((reason != REASON_COST || !pcard->is_affected_by_effect(EFFECT_CANNOT_USE_AS_COST))
	        && pduel->game_field->is_player_can_discard_hand(p, pcard, pe, reason))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t card_can_attack(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_capable_attack());
	return 1;
}
int32_t card_can_chain_attack(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	if(pcard != pduel->game_field->core.attacker) {
		lua_pushboolean(L, 0);
		return 1;
	}
	auto ac = lua_get<uint8_t, 2>(L, 2);
	bool monsteronly = lua_get<bool, false>(L, 3);
	if(pcard->is_status(STATUS_BATTLE_DESTROYED)
			|| pcard->current.controler != pduel->game_field->infos.turn_player
			|| pcard->fieldid_r != pduel->game_field->core.pre_field[0]
			|| !pcard->is_capable_attack_announce(pduel->game_field->infos.turn_player)
			|| (ac != 0 && pcard->announce_count >= ac)
			|| (ac == 2 && pcard->is_affected_by_effect(EFFECT_EXTRA_ATTACK))) {
		lua_pushboolean(L, 0);
		return 1;
	}
	pduel->game_field->core.select_cards.clear();
	pduel->game_field->get_attack_target(pcard, &pduel->game_field->core.select_cards, TRUE);
	if(pduel->game_field->core.select_cards.empty() && (monsteronly || !pcard->direct_attackable))
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, 1);
	return 1;
}
int32_t card_is_faceup(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_position(POS_FACEUP));
	return 1;
}
int32_t card_is_attack_pos(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_position(POS_ATTACK));
	return 1;
}
int32_t card_is_facedown(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_position(POS_FACEDOWN));
	return 1;
}
int32_t card_is_defense_pos(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_position(POS_DEFENSE));
	return 1;
}
int32_t card_is_position(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto pos = lua_get<uint8_t>(L, 2);
	lua_pushboolean(L, pcard->is_position(pos));
	return 1;
}
int32_t card_is_pre_position(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto pos = lua_get<uint8_t>(L, 2);
	lua_pushboolean(L, pcard->previous.position & pos);
	return 1;
}
int32_t card_is_controler(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto con = lua_get<uint8_t>(L, 2);
	lua_pushboolean(L, pcard->current.controler == con);
	return 1;
}
int32_t card_is_onfield(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	if((pcard->current.location & LOCATION_ONFIELD)
			&& !pcard->get_status(STATUS_SUMMONING | STATUS_SUMMON_DISABLED | STATUS_ACTIVATE_DISABLED | STATUS_SPSUMMON_STEP))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t card_is_location(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto loc = lua_get<uint16_t>(L, 2);
	if(pcard->current.location == LOCATION_MZONE) {
		if((loc & LOCATION_MZONE) && !pcard->get_status(STATUS_SUMMONING | STATUS_SUMMON_DISABLED | STATUS_SPSUMMON_STEP))
			lua_pushboolean(L, 1);
		else
			lua_pushboolean(L, 0);
	} else if(pcard->current.location == LOCATION_SZONE) {
		if(pcard->current.is_location(loc) && !pcard->is_status(STATUS_ACTIVATE_DISABLED))
			lua_pushboolean(L, 1);
		else
			lua_pushboolean(L, 0);
	} else
		lua_pushboolean(L, pcard->current.location & loc);
	return 1;
}
int32_t card_is_pre_location(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto loc = lua_get<uint16_t>(L, 2);
	lua_pushboolean(L, pcard->previous.is_location(loc));
	return 1;
}
int32_t card_is_level_below(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto lvl = lua_get<uint32_t>(L, 2);
	uint32_t plvl = pcard->get_level();
	lua_pushboolean(L, plvl > 0 && plvl <= lvl);
	return 1;
}
int32_t card_is_level_above(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto lvl = lua_get<uint32_t>(L, 2);
	uint32_t plvl = pcard->get_level();
	lua_pushboolean(L, plvl > 0 && plvl >= lvl);
	return 1;
}
int32_t card_is_rank_below(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto rnk = lua_get<uint32_t>(L, 2);
	uint32_t prnk = pcard->get_rank();
	lua_pushboolean(L, prnk > 0 && prnk <= rnk);
	return 1;
}
int32_t card_is_rank_above(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto rnk = lua_get<uint32_t>(L, 2);
	uint32_t prnk = pcard->get_rank();
	lua_pushboolean(L, prnk > 0 && prnk >= rnk);
	return 1;
}
int32_t card_is_link_below(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto lnk = lua_get<uint32_t>(L, 2);
	uint32_t plnk = pcard->get_link();
	lua_pushboolean(L, plnk > 0 && plnk <= lnk);
	return 1;
}
int32_t card_is_link_above(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto lnk = lua_get<uint32_t>(L, 2);
	uint32_t plnk = pcard->get_link();
	lua_pushboolean(L, plnk > 0 && plnk >= lnk);
	return 1;
}
int32_t card_is_attack_below(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto atk = lua_get<int32_t>(L, 2);
	if(!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !pcard->is_affected_by_effect(EFFECT_PRE_MONSTER))
		lua_pushboolean(L, 0);
	else {
		int32_t _atk = pcard->get_attack();
		lua_pushboolean(L, _atk >= 0 && _atk <= atk);
	}
	return 1;
}
int32_t card_is_attack_above(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto atk = lua_get<int32_t>(L, 2);
	if(!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !pcard->is_affected_by_effect(EFFECT_PRE_MONSTER))
		lua_pushboolean(L, 0);
	else {
		lua_pushboolean(L, pcard->get_attack() >= atk);
	}
	return 1;
}
int32_t card_is_defense_below(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto def = lua_get<int32_t>(L, 2);
	if((pcard->data.type & TYPE_LINK)
	   || (!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !(pcard->current.location & LOCATION_MZONE)))
		lua_pushboolean(L, 0);
	else {
		int32_t _def = pcard->get_defense();
		lua_pushboolean(L, _def >= 0 && _def <= def);
	}
	return 1;
}
int32_t card_is_defense_above(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto def = lua_get<int32_t>(L, 2);
	if((pcard->data.type & TYPE_LINK)
	   || (!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !pcard->is_affected_by_effect(EFFECT_PRE_MONSTER)))
		lua_pushboolean(L, 0);
	else {
		lua_pushboolean(L, pcard->get_defense() >= def);
	}
	return 1;
}
int32_t card_is_public(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_position(POS_FACEUP));
	return 1;
}
int32_t card_is_forbidden(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_status(STATUS_FORBIDDEN));
	return 1;
}
int32_t card_is_able_to_change_controler(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_capable_change_control());
	return 1;
}
int32_t card_is_controler_can_be_changed(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	bool ign = lua_get<bool, false>(L, 2);
	auto zone = lua_get<uint32_t, 0xff>(L, 3);
	lua_pushboolean(L, pcard->is_control_can_be_changed(ign, zone));
	return 1;
}
int32_t card_add_counter(lua_State* L) {
	check_param_count(L, 3);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto countertype = lua_get<uint16_t>(L, 2);
	auto count = lua_get<uint16_t>(L, 3);
	bool singly = lua_get<bool, false>(L, 4);
	if(pcard->is_affect_by_effect(pduel->game_field->core.reason_effect))
		lua_pushboolean(L, pcard->add_counter(pduel->game_field->core.reason_player, countertype, count, singly));
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t card_remove_counter(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 5);
	const auto pduel = lua_get<duel*>(L);
	auto countertype = lua_get<uint16_t>(L, 3);
	if(countertype == 0) {
		luaL_error(L, "Counter type cannot be 0, use Card.RemoveAllCounters instead");
		unreachable();
	}
	auto pcard = lua_get<card*, true>(L, 1);
	auto rplayer = lua_get<uint8_t>(L, 2);
	auto count = lua_get<uint16_t>(L, 4);
	auto reason = lua_get<uint32_t>(L, 5);
	pduel->game_field->remove_counter(reason, pcard, rplayer, 0, 0, countertype, count);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushboolean(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t card_remove_all_counters(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	uint32_t total = 0;
	for(const auto& cmit : pcard->counters) {
		auto message = pduel->new_message(MSG_REMOVE_COUNTER);
		message->write<uint16_t>(cmit.first);
		message->write<uint8_t>(pcard->current.controler);
		message->write<uint8_t>(pcard->current.location);
		message->write<uint8_t>(pcard->current.sequence);
		message->write<uint16_t>(cmit.second[0] + cmit.second[1]);
		total += cmit.second[0] + cmit.second[1];
	}
	pcard->counters.clear();
	lua_pushinteger(L, total);
	return 1;
}
int32_t card_get_counter(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto countertype = lua_get<uint16_t>(L, 2);
	if(countertype == 0) {
		luaL_error(L, "Counter type cannot be 0, use Card.GetAllCounters instead");
		unreachable();
	}
	lua_pushinteger(L, pcard->get_counter(countertype));
	return 1;
}
int32_t card_get_all_counters(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_newtable(L);
	for(const auto& counter : pcard->counters) {
		lua_pushinteger(L, counter.first);
		lua_pushinteger(L, counter.second[0] + counter.second[1]);
		lua_settable(L, -3);
	}
	return 1;
}
int32_t card_has_counters(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->counters.size());
	return 1;
}
int32_t card_enable_counter_permit(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto countertype = lua_get<uint16_t>(L, 2);
	uint16_t prange;
	if(lua_gettop(L) > 2)
		prange = lua_get<uint16_t>(L, 3);
	else if(pcard->data.type & TYPE_MONSTER)
		prange = LOCATION_MZONE;
	else
		prange = LOCATION_SZONE | LOCATION_FZONE;
	effect* peffect = pduel->new_effect();
	peffect->owner = pcard;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = EFFECT_COUNTER_PERMIT | countertype;
	peffect->value = prange;
	if(lua_gettop(L) > 3 && lua_isfunction(L, 4))
		peffect->target = interpreter::get_function_handle(L, 4);
	pcard->add_effect(peffect);
	return 0;
}
int32_t card_set_counter_limit(lua_State* L) {
	check_param_count(L, 3);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto countertype = lua_get<uint16_t>(L, 2);
	auto limit = lua_get<uint32_t>(L, 3);
	effect* peffect = pduel->new_effect();
	peffect->owner = pcard;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = EFFECT_COUNTER_LIMIT | countertype;
	peffect->value = limit;
	pcard->add_effect(peffect);
	return 0;
}
int32_t card_is_can_change_position(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_capable_change_position_by_effect(pduel->game_field->core.reason_player));
	return 1;
}
int32_t card_is_can_turn_set(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	lua_pushboolean(L, pcard->is_capable_turn_set(pduel->game_field->core.reason_player));
	return 1;
}
int32_t card_is_can_add_counter(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto countertype = lua_get<uint16_t>(L, 2);
	auto count = lua_get<uint16_t, 0>(L, 3);
	bool singly = lua_get<bool, false>(L, 4);
	auto loc = lua_get<uint16_t, 0>(L, 5);
	lua_pushboolean(L, pcard->is_can_add_counter(pduel->game_field->core.reason_player, countertype, count, singly, loc));
	return 1;
}
int32_t card_is_can_remove_counter(lua_State* L) {
	check_param_count(L, 5);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto countertype = lua_get<uint16_t>(L, 3);
	auto count = lua_get<uint16_t>(L, 4);
	auto reason = lua_get<uint32_t>(L, 5);
	lua_pushboolean(L, pduel->game_field->is_player_can_remove_counter(playerid, pcard, 0, 0, countertype, count, reason));
	return 1;
}
int32_t card_is_can_be_fusion_material(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	card* fcard = 0;
	if(lua_gettop(L) >= 2 && !lua_isnoneornil(L, 2))
		fcard = lua_get<card*, true>(L, 2);
	auto summon_type = lua_get<uint64_t, SUMMON_TYPE_FUSION>(L, 3);
	auto playerid = lua_get<uint8_t>(L, 3, pduel->game_field->core.reason_player);
	lua_pushboolean(L, pcard->is_can_be_fusion_material(fcard, summon_type, playerid));
	return 1;
}
int32_t card_is_can_be_synchro_material(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	card* scard = 0;
	card* tuner = 0;
	if(lua_gettop(L) >= 2)
		scard = lua_get<card*, true>(L, 2);
	if(lua_gettop(L) >= 3 && !lua_isnoneornil(L, 3))
		tuner = lua_get<card*, true>(L, 3);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 4);
	lua_pushboolean(L, pcard->is_can_be_synchro_material(scard, playerid, tuner));
	return 1;
}
int32_t card_is_can_be_ritual_material(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	card* scard = 0;
	if(lua_gettop(L) >= 2 && !lua_isnoneornil(L, 2))
		scard = lua_get<card*, true>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 3, pduel->game_field->core.reason_player);
	lua_pushboolean(L, pcard->is_can_be_ritual_material(scard, playerid));
	return 1;
}
int32_t card_is_can_be_xyz_material(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	card* scard = nullptr;
	if(!lua_isnoneornil(L, 2))
		lua_get<card*, true>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 3, lua_get<duel*>(L)->game_field->core.reason_player);
	auto reason = lua_get<uint32_t, REASON_XYZ | REASON_MATERIAL>(L, 4);
	lua_pushboolean(L, pcard->is_can_be_xyz_material(scard, playerid, reason));
	return 1;
}
int32_t card_is_can_be_link_material(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	card* scard = 0;
	if(lua_gettop(L) >= 2)
		scard = lua_get<card*, true>(L, 2);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 3);
	lua_pushboolean(L, pcard->is_can_be_link_material(scard, playerid));
	return 1;
}
int32_t card_is_can_be_material(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	uint64_t sumtype = lua_get<uint64_t>(L, 2);
	card* scard = 0;
	if(lua_gettop(L) >= 3)
		scard = lua_get<card*, true>(L, 3);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 4);
	lua_pushboolean(L, pcard->is_can_be_material(scard, sumtype, playerid));
	return 1;
}
int32_t card_check_fusion_material(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	group* pgroup = 0;
	if(lua_gettop(L) > 1 && !lua_isnoneornil(L, 2))
		pgroup = lua_get<group*, true>(L, 2);
	group* cg = 0;
	if(auto _pcard = lua_get<card*>(L, 3))
		cg = pduel->new_group(_pcard);
	else
		cg = lua_get<group*>(L, 3);
	auto chkf = lua_get<uint32_t, PLAYER_NONE>(L, 4);
	lua_pushboolean(L, pcard->fusion_check(pgroup, cg, chkf));
	return 1;
}
int32_t card_check_fusion_substitute(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto fcard = lua_get<card*, true>(L, 2);
	lua_pushboolean(L, pcard->check_fusion_substitute(fcard));
	return 1;
}
int32_t card_is_immune_to_effect(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto peffect = lua_get<effect*, true>(L, 2);
	lua_pushboolean(L, !pcard->is_affect_by_effect(peffect));
	return 1;
}
int32_t card_is_can_be_effect_target(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	effect* peffect = pduel->game_field->core.reason_effect;
	if(lua_gettop(L) > 1)
		peffect = lua_get<effect*, true>(L, 2);
	lua_pushboolean(L, pcard->is_capable_be_effect_target(peffect, pduel->game_field->core.reason_player));
	return 1;
}
int32_t card_is_can_be_battle_target(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto bcard = lua_get<card*, true>(L, 2);
	lua_pushboolean(L, pcard->is_capable_be_battle_target(bcard));
	return 1;
}
int32_t card_add_monster_attribute(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto type = lua_get<uint32_t>(L, 2);
	auto attribute = lua_get<uint32_t, 0>(L, 3);
	auto race = lua_get<uint32_t, 0>(L, 4);
	auto level = lua_get<uint32_t, 0>(L, 5);
	auto atk = lua_get<int32_t, 0>(L, 6);
	auto def = lua_get<int32_t, 0>(L, 7);
	pcard->set_status(STATUS_NO_LEVEL, FALSE);
	// pre-monster
	effect* peffect = pduel->new_effect();
	peffect->owner = pcard;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = EFFECT_PRE_MONSTER;
	peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
	peffect->reset_flag = RESET_CHAIN + RESET_EVENT + 0x47e0000;
	peffect->value = type;
	pcard->add_effect(peffect);
	//attribute
	if(attribute) {
		peffect = pduel->new_effect();
		peffect->owner = pcard;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_ADD_ATTRIBUTE;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = RESET_EVENT + 0x47e0000;
		peffect->value = attribute;
		pcard->add_effect(peffect);
	}
	//race
	if(race) {
		peffect = pduel->new_effect();
		peffect->owner = pcard;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_ADD_RACE;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = RESET_EVENT + 0x47e0000;
		peffect->value = race;
		pcard->add_effect(peffect);
	}
	//level
	if(level) {
		peffect = pduel->new_effect();
		peffect->owner = pcard;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_CHANGE_LEVEL;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = RESET_EVENT + 0x47e0000;
		peffect->value = level;
		pcard->add_effect(peffect);
	}
	//atk
	if(atk) {
		peffect = pduel->new_effect();
		peffect->owner = pcard;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_SET_BASE_ATTACK;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = RESET_EVENT + 0x47e0000;
		peffect->value = atk;
		pcard->add_effect(peffect);
	}
	//def
	if(def) {
		peffect = pduel->new_effect();
		peffect->owner = pcard;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_SET_BASE_DEFENSE;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = RESET_EVENT + 0x47e0000;
		peffect->value = def;
		pcard->add_effect(peffect);
	}
	return 0;
}
int32_t card_add_monster_attribute_complete(lua_State* /*L*/) {
	return 0;
}
int32_t card_cancel_to_grave(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	bool cancel = lua_get<bool, true>(L, 2);
	if(cancel)
		pcard->set_status(STATUS_LEAVE_CONFIRMED, FALSE);
	else {
		pduel->game_field->core.leave_confirmed.insert(pcard);
		pcard->set_status(STATUS_LEAVE_CONFIRMED, TRUE);
	}
	return 0;
}
int32_t card_get_tribute_requirement(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	int32_t rcount = pcard->get_summon_tribute_count();
	lua_pushinteger(L, rcount & 0xffff);
	lua_pushinteger(L, (rcount >> 16) & 0xffff);
	return 2;
}
int32_t card_get_battle_target(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	if(pduel->game_field->core.attacker == pcard)
		interpreter::pushobject(L, pduel->game_field->core.attack_target);
	else if(pduel->game_field->core.attack_target == pcard)
		interpreter::pushobject(L, pduel->game_field->core.attacker);
	else lua_pushnil(L);
	return 1;
}
int32_t card_get_attackable_target(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	card_vector targets;
	bool chain_attack = pduel->game_field->core.chain_attacker_id == pcard->fieldid;
	pduel->game_field->get_attack_target(pcard, &targets, chain_attack);
	group* newgroup = pduel->new_group(targets);
	interpreter::pushobject(L, newgroup);
	lua_pushboolean(L, (int32_t)pcard->direct_attackable);
	return 2;
}
int32_t card_set_hint(lua_State* L) {
	check_param_count(L, 3);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto type = lua_get<uint8_t>(L, 2);
	auto value = lua_get<uint32_t>(L, 3);
	if(type >= CHINT_DESC_ADD)
		return 0;
	auto message = pduel->new_message(MSG_CARD_HINT);
	message->write(pcard->get_info_location());
	message->write<uint8_t>(type);
	message->write<uint64_t>(value);
	return 0;
}
int32_t card_reverse_in_deck(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	if(pcard->current.location != LOCATION_DECK)
		return 0;
	pcard->current.position = POS_FACEUP_DEFENSE;
	if(pcard->current.sequence == pduel->game_field->player[pcard->current.controler].list_main.size() - 1) {
		auto message = pduel->new_message(MSG_DECK_TOP);
		message->write<uint8_t>(pcard->current.controler);
		message->write<uint32_t>(0);
		message->write<uint32_t>(pcard->data.code);
		message->write<uint32_t>(pcard->current.position);
	}
	return 0;
}
int32_t card_set_unique_onfield(lua_State* L) {
	check_param_count(L, 4);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	pcard->unique_pos[0] = lua_get<uint8_t>(L, 2);
	pcard->unique_pos[1] = lua_get<uint8_t>(L, 3);
	if(lua_isfunction(L, 4)) {
		pcard->unique_code = 1;
		pcard->unique_function = interpreter::get_function_handle(L, 4);
	} else
		pcard->unique_code = lua_get<uint32_t>(L, 4);
	auto location = lua_get<uint8_t, LOCATION_ONFIELD>(L, 5) & LOCATION_ONFIELD;
	pcard->unique_location = location;
	effect* peffect = pduel->new_effect();
	peffect->owner = pcard;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = EFFECT_UNIQUE_CHECK;
	peffect->flag[0] = EFFECT_FLAG_COPY_INHERIT;
	pcard->add_effect(peffect);
	pcard->unique_effect = peffect;
	if(pcard->current.location & location)
		pduel->game_field->add_unique_card(pcard);
	return 0;
}
int32_t card_check_unique_onfield(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto check_player = lua_get<uint8_t>(L, 2);
	auto check_location = lua_get<uint8_t, LOCATION_ONFIELD>(L, 3) & LOCATION_ONFIELD;
	auto icard = lua_get<card*>(L, 4);
	lua_pushboolean(L, pduel->game_field->check_unique_onfield(pcard, check_player, check_location, icard) ? 0 : 1);
	return 1;
}
int32_t card_reset_negate_effect(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	int32_t count = lua_gettop(L) - 1;
	for(int32_t i = 0; i < count; ++i)
		pcard->reset(lua_get<uint32_t>(L, i + 2), RESET_CARD);
	return 0;
}
int32_t card_assume_prop(lua_State* L) {
	check_param_count(L, 3);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto assume = lua_get<uint32_t>(L, 2);
	if ((assume < ASSUME_CODE) || (assume > ASSUME_LINKMARKER))
		return 0;
	pcard->assume[assume] = lua_get<uint32_t>(L, 3);
	pduel->assumes.insert(pcard);
	return 0;
}
int32_t card_set_spsummon_once(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	if(pcard->status & STATUS_COPYING_EFFECT)
		return 0;
	pcard->spsummon_code = lua_get<uint32_t>(L, 2);
	pduel->game_field->core.global_flag |= GLOBALFLAG_SPSUMMON_ONCE;
	return 0;
}
#define CARD_INFO_FUNC(attr) int32_t card_##attr(lua_State* L) {\
	check_param_count(L, 1);\
	auto pcard = lua_get<card*, true>(L, 1);\
	if(lua_gettop(L) > 1) {\
		pcard->data.attr = lua_get<uint32_t>(L, 2);\
		return 0;\
	} else \
		lua_pushinteger(L, pcard->data.attr);\
	return 1;\
}
CARD_INFO_FUNC(code)
CARD_INFO_FUNC(alias)
CARD_INFO_FUNC(type)
CARD_INFO_FUNC(level)
CARD_INFO_FUNC(attribute)
CARD_INFO_FUNC(race)
CARD_INFO_FUNC(attack)
CARD_INFO_FUNC(defense)
CARD_INFO_FUNC(rscale)
CARD_INFO_FUNC(lscale)
CARD_INFO_FUNC(link_marker)
#undef CARD_INFO_FUNC
int32_t card_setcode(lua_State* L) { 
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	if(lua_gettop(L) > 1) {
		pcard->data.setcodes.clear();
		if(lua_istable(L, 2)) {
			lua_table_iterate(L, 2, [&setcodes = pcard->data.setcodes, &L] {
				setcodes.insert(lua_get<uint16_t>(L, -1));
			});
		} else
			pcard->data.setcodes.insert(lua_get<uint16_t>(L, 2));
		return 0;
	} else {
		for(auto& setcode : pcard->data.setcodes)
			lua_pushinteger(L, setcode);
	}
	return pcard->data.setcodes.size();
}
int32_t card_recreate(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto code = lua_get<uint32_t>(L, 2);
	if (pcard->recreate(code)) {
		pcard->data.alias = lua_get<uint32_t>(L, 3, pcard->data.alias);
		if(lua_gettop(L) > 3 && !lua_isnoneornil(L, 4)) {
			if(lua_istable(L, 4)) {
				lua_table_iterate(L, 4, [&setcodes = pcard->data.setcodes, &L] {
					setcodes.insert(lua_get<uint16_t>(L, -1));
				});
			} else
				pcard->data.setcodes.insert(lua_get<uint16_t>(L, 4));
		}
		pcard->data.type = lua_get<uint32_t>(L, 5, pcard->data.type);
		pcard->data.level = lua_get<uint32_t>(L, 6, pcard->data.level);
		pcard->data.attribute = lua_get<uint32_t>(L, 7, pcard->data.attribute);
		pcard->data.race = lua_get<uint32_t>(L, 8, pcard->data.race);
		pcard->data.attack = lua_get<int32_t>(L, 9, pcard->data.attack);
		pcard->data.defense = lua_get<int32_t>(L, 10, pcard->data.defense);
		pcard->data.lscale = lua_get<uint32_t>(L, 11, pcard->data.lscale);
		pcard->data.rscale = lua_get<uint32_t>(L, 12, pcard->data.rscale);
		pcard->data.link_marker = lua_get<uint32_t>(L, 13, pcard->data.link_marker);
		if (lua_get<bool, false>(L, 14))
			pcard->replace_effect(code, 0, 0, true);
	}
	return 0;
}
int32_t card_cover(lua_State* L) {
	check_param_count(L, 1);
	auto pcard = lua_get<card*, true>(L, 1);
	if(lua_gettop(L) > 1) {
		pcard->cover = lua_get<uint32_t>(L, 2);
		return 0;
	} else
		lua_pushinteger(L, pcard->cover);
	return 1;
}

static constexpr luaL_Reg cardlib[] = {
	{ "GetCode", card_get_code },
	{ "GetOriginalCode", card_get_origin_code },
	{ "GetOriginalCodeRule", card_get_origin_code_rule },
	{ "GetSetCard", card_get_set_card },
	{ "GetOriginalSetCard", card_get_origin_set_card },
	{ "GetPreviousSetCard", card_get_pre_set_card },
	{ "GetType", card_get_type },
	{ "GetOriginalType", card_get_origin_type },
	{ "GetLevel", card_get_level },
	{ "GetRank", card_get_rank },
	{ "GetLink", card_get_link },
	{ "GetSynchroLevel", card_get_synchro_level },
	{ "GetRitualLevel", card_get_ritual_level },
	{ "GetOriginalLevel", card_get_origin_level },
	{ "GetOriginalRank", card_get_origin_rank },
	{ "IsXyzLevel", card_is_xyz_level },
	{ "GetLeftScale", card_get_lscale },
	{ "GetOriginalLeftScale", card_get_origin_lscale },
	{ "GetRightScale", card_get_rscale },
	{ "GetOriginalRightScale", card_get_origin_rscale },
	{ "GetLinkMarker", card_get_link_marker },
	{ "IsLinkMarker", card_is_link_marker },
	{ "GetLinkedGroup", card_get_linked_group },
	{ "GetLinkedGroupCount", card_get_linked_group_count },
	{ "GetLinkedZone", card_get_linked_zone },
	{ "GetFreeLinkedZone", card_get_free_linked_zone },
	{ "GetMutualLinkedGroup", card_get_mutual_linked_group },
	{ "GetMutualLinkedGroupCount", card_get_mutual_linked_group_count },
	{ "GetMutualLinkedZone", card_get_mutual_linked_zone },
	{ "IsLinked", card_is_linked },
	{ "IsExtraLinked", card_is_extra_linked },
	{ "GetColumnGroup", card_get_column_group },
	{ "GetColumnGroupCount", card_get_column_group_count },
	{ "GetColumnZone", card_get_column_zone },
	{ "IsAllColumn", card_is_all_column },
	{ "GetAttribute", card_get_attribute },
	{ "GetOriginalAttribute", card_get_origin_attribute },
	{ "GetRace", card_get_race },
	{ "GetOriginalRace", card_get_origin_race },
	{ "GetAttack", card_get_attack },
	{ "GetBaseAttack", card_get_origin_attack },
	{ "GetTextAttack", card_get_text_attack },
	{ "GetDefense",	card_get_defense },
	{ "GetBaseDefense", card_get_origin_defense },
	{ "GetTextDefense", card_get_text_defense },
	{ "GetPreviousCodeOnField", card_get_previous_code_onfield },
	{ "GetPreviousTypeOnField", card_get_previous_type_onfield },
	{ "GetPreviousLevelOnField", card_get_previous_level_onfield },
	{ "GetPreviousRankOnField", card_get_previous_rank_onfield },
	{ "GetPreviousAttributeOnField", card_get_previous_attribute_onfield },
	{ "GetPreviousRaceOnField", card_get_previous_race_onfield },
	{ "GetPreviousAttackOnField", card_get_previous_attack_onfield },
	{ "GetPreviousDefenseOnField", card_get_previous_defense_onfield },
	{ "GetOwner", card_get_owner },
	{ "GetControler", card_get_controler },
	{ "GetPreviousControler", card_get_previous_controler },
	{ "GetReason", card_get_reason },
	{ "GetReasonCard", card_get_reason_card },
	{ "GetReasonPlayer", card_get_reason_player },
	{ "GetReasonEffect", card_get_reason_effect },
	{ "SetReason", card_set_reason },
	{ "SetReasonCard", card_set_reason_card },
	{ "SetReasonPlayer", card_set_reason_player },
	{ "SetReasonEffect", card_set_reason_effect },
	{ "GetPosition", card_get_position },
	{ "GetPreviousPosition", card_get_previous_position },
	{ "GetBattlePosition", card_get_battle_position },
	{ "GetLocation", card_get_location },
	{ "GetPreviousLocation", card_get_previous_location },
	{ "GetSequence", card_get_sequence },
	{ "GetPreviousSequence", card_get_previous_sequence },
	{ "GetSummonType", card_get_summon_type },
	{ "GetSummonLocation", card_get_summon_location },
	{ "GetSummonPlayer", card_get_summon_player },
	{ "GetDestination", card_get_destination },
	{ "GetLeaveFieldDest", card_get_leave_field_dest },
	{ "GetTurnID", card_get_turnid },
	{ "GetFieldID", card_get_fieldid },
	{ "GetRealFieldID", card_get_fieldidr },
	{ "GetCardID", card_get_cardid },
	{ "IsOriginalCodeRule", card_is_origin_code_rule },
	{ "IsCode", card_is_code },
	{ "IsSummonCode", card_is_summon_code },
	{ "IsSetCard", card_is_set_card },
	{ "IsOriginalSetCard", card_is_origin_set_card },
	{ "IsPreviousSetCard", card_is_pre_set_card },
	{ "IsType", card_is_type },
	{ "IsLevel", card_is_level },
	{ "IsRank", card_is_rank },
	{ "IsLink", card_is_link },
	{ "IsAttack", card_is_attack },
	{ "IsDefense", card_is_defense },
	{ "IsRace", card_is_race },
	{ "IsAttribute", card_is_attribute },
	{ "IsReason", card_is_reason },
	{ "IsSummonType", card_is_summon_type },
	{ "IsStatus", card_is_status },
	{ "IsNotTuner", card_is_not_tuner },
	{ "SetStatus", card_set_status },
	{ "IsGeminiState", card_is_gemini_state },
	{ "EnableGeminiState", card_enable_gemini_state },
	{ "SetTurnCounter",	card_set_turn_counter },
	{ "GetTurnCounter", card_get_turn_counter },
	{ "SetMaterial", card_set_material },
	{ "GetMaterial", card_get_material },
	{ "GetMaterialCount", card_get_material_count },
	{ "GetEquipGroup", card_get_equip_group },
	{ "GetEquipCount", card_get_equip_count },
	{ "GetEquipTarget", card_get_equip_target },
	{ "GetPreviousEquipTarget", card_get_pre_equip_target },
	{ "CheckEquipTarget", card_check_equip_target },
	{ "CheckUnionTarget", card_check_union_target },
	{ "GetUnionCount", card_get_union_count },
	{ "GetOverlayGroup", card_get_overlay_group },
	{ "GetOverlayCount", card_get_overlay_count },
	{ "GetOverlayTarget", card_get_overlay_target },
	{ "CheckRemoveOverlayCard", card_check_remove_overlay_card },
	{ "RemoveOverlayCard", card_remove_overlay_card },
	{ "GetAttackedGroup", card_get_attacked_group },
	{ "GetAttackedGroupCount", card_get_attacked_group_count },
	{ "GetAttackedCount", card_get_attacked_count },
	{ "GetBattledGroup", card_get_battled_group },
	{ "GetBattledGroupCount", card_get_battled_group_count },
	{ "GetAttackAnnouncedCount", card_get_attack_announced_count },
	{ "IsDirectAttacked", card_is_direct_attacked },
	{ "SetCardTarget", card_set_card_target },
	{ "GetCardTarget", card_get_card_target },
	{ "GetFirstCardTarget", card_get_first_card_target },
	{ "GetCardTargetCount", card_get_card_target_count },
	{ "IsHasCardTarget", card_is_has_card_target },
	{ "CancelCardTarget", card_cancel_card_target },
	{ "GetOwnerTarget", card_get_owner_target },
	{ "GetOwnerTargetCount", card_get_owner_target_count },
	{ "GetActivateEffect", card_get_activate_effect },
	{ "CheckActivateEffect", card_check_activate_effect },
	{ "RegisterEffect", card_register_effect },
	{ "IsHasEffect", card_is_has_effect },
	{ "GetCardEffect", card_get_card_effect },
	{ "ResetEffect", card_reset_effect },
	{ "GetEffectCount", card_get_effect_count },
	{ "RegisterFlagEffect", card_register_flag_effect },
	{ "GetFlagEffect", card_get_flag_effect },
	{ "ResetFlagEffect", card_reset_flag_effect },
	{ "SetFlagEffectLabel", card_set_flag_effect_label },
	{ "GetFlagEffectLabel", card_get_flag_effect_label },
	{ "CreateRelation", card_create_relation },
	{ "ReleaseRelation", card_release_relation },
	{ "CreateEffectRelation", card_create_effect_relation },
	{ "ReleaseEffectRelation", card_release_effect_relation },
	{ "ClearEffectRelation", card_clear_effect_relation },
	{ "IsRelateToEffect", card_is_relate_to_effect },
	{ "IsRelateToChain", card_is_relate_to_chain },
	{ "IsRelateToCard", card_is_relate_to_card },
	{ "IsRelateToBattle", card_is_relate_to_battle },
	{ "CopyEffect", card_copy_effect },
	{ "ReplaceEffect", card_replace_effect },
	{ "EnableUnsummonable", card_enable_unsummonable },
	{ "EnableReviveLimit", card_enable_revive_limit },
	{ "CompleteProcedure", card_complete_procedure },
	{ "IsDisabled", card_is_disabled },
	{ "IsDestructable", card_is_destructable },
	{ "IsSummonableCard", card_is_summonable },
	{ "IsFusionSummonableCard", card_is_fusion_summonable_card },
	{ "IsSpecialSummonable", card_is_special_summonable },
	{ "IsSynchroSummonable", card_is_synchro_summonable },
	{ "IsXyzSummonable", card_is_xyz_summonable },
	{ "IsLinkSummonable", card_is_link_summonable },
	{ "IsProcedureSummonable", card_is_procedure_summonable },
	{ "IsSummonable", card_is_can_be_summoned },
	{ "IsMSetable", card_is_msetable },
	{ "IsSSetable", card_is_ssetable },
	{ "IsCanBeSpecialSummoned", card_is_can_be_special_summoned },
	{ "IsAbleToHand", card_is_able_to_hand },
	{ "IsAbleToDeck", card_is_able_to_deck },
	{ "IsAbleToExtra", card_is_able_to_extra },
	{ "IsAbleToGrave", card_is_able_to_grave },
	{ "IsAbleToRemove", card_is_able_to_remove },
	{ "IsAbleToHandAsCost", card_is_able_to_hand_as_cost },
	{ "IsAbleToDeckAsCost", card_is_able_to_deck_as_cost },
	{ "IsAbleToExtraAsCost", card_is_able_to_extra_as_cost },
	{ "IsAbleToDeckOrExtraAsCost", card_is_able_to_deck_or_extra_as_cost },
	{ "IsAbleToGraveAsCost", card_is_able_to_grave_as_cost },
	{ "IsAbleToRemoveAsCost", card_is_able_to_remove_as_cost },
	{ "IsReleasable", card_is_releasable },
	{ "IsReleasableByEffect", card_is_releasable_by_effect },
	{ "IsDiscardable", card_is_discardable },
	{ "CanAttack", card_can_attack },
	{ "CanChainAttack", card_can_chain_attack },
	{ "IsFaceup", card_is_faceup },
	{ "IsAttackPos", card_is_attack_pos },
	{ "IsFacedown", card_is_facedown },
	{ "IsDefensePos", card_is_defense_pos },
	{ "IsPosition", card_is_position },
	{ "IsPreviousPosition", card_is_pre_position },
	{ "IsControler", card_is_controler },
	{ "IsOnField", card_is_onfield },
	{ "IsLocation", card_is_location },
	{ "IsPreviousLocation", card_is_pre_location },
	{ "IsLevelBelow", card_is_level_below },
	{ "IsLevelAbove", card_is_level_above },
	{ "IsRankBelow", card_is_rank_below },
	{ "IsRankAbove", card_is_rank_above },
	{ "IsLinkBelow", card_is_link_below },
	{ "IsLinkAbove", card_is_link_above },
	{ "IsAttackBelow", card_is_attack_below },
	{ "IsAttackAbove", card_is_attack_above },
	{ "IsDefenseBelow", card_is_defense_below },
	{ "IsDefenseAbove", card_is_defense_above },
	{ "IsPublic", card_is_public },
	{ "IsForbidden", card_is_forbidden },
	{ "IsAbleToChangeControler", card_is_able_to_change_controler },
	{ "IsControlerCanBeChanged", card_is_controler_can_be_changed },
	{ "AddCounter", card_add_counter },
	{ "RemoveCounter", card_remove_counter },
	{ "RemoveAllCounters", card_remove_all_counters },
	{ "GetCounter", card_get_counter },
	{ "GetAllCounters", card_get_all_counters },
	{ "HasCounters", card_has_counters },
	{ "EnableCounterPermit", card_enable_counter_permit },
	{ "SetCounterLimit", card_set_counter_limit },
	{ "IsCanChangePosition", card_is_can_change_position },
	{ "IsCanTurnSet", card_is_can_turn_set },
	{ "IsCanAddCounter", card_is_can_add_counter },
	{ "IsCanRemoveCounter", card_is_can_remove_counter },
	{ "IsCanBeFusionMaterial", card_is_can_be_fusion_material },
	{ "IsCanBeSynchroMaterial", card_is_can_be_synchro_material },
	{ "IsCanBeRitualMaterial", card_is_can_be_ritual_material },
	{ "IsCanBeXyzMaterial", card_is_can_be_xyz_material },
	{ "IsCanBeLinkMaterial", card_is_can_be_link_material },
	{ "IsCanBeMaterial", card_is_can_be_material },
	{ "CheckFusionMaterial", card_check_fusion_material },
	{ "CheckFusionSubstitute", card_check_fusion_substitute },
	{ "IsImmuneToEffect", card_is_immune_to_effect },
	{ "IsCanBeEffectTarget", card_is_can_be_effect_target },
	{ "IsCanBeBattleTarget", card_is_can_be_battle_target },
	{ "AddMonsterAttribute", card_add_monster_attribute },
	{ "AddMonsterAttributeComplete", card_add_monster_attribute_complete },
	{ "CancelToGrave", card_cancel_to_grave },
	{ "GetTributeRequirement", card_get_tribute_requirement },
	{ "GetBattleTarget", card_get_battle_target },
	{ "GetAttackableTarget", card_get_attackable_target },
	{ "SetHint", card_set_hint },
	{ "ReverseInDeck", card_reverse_in_deck },
	{ "SetUniqueOnField", card_set_unique_onfield },
	{ "CheckUniqueOnField", card_check_unique_onfield },
	{ "ResetNegateEffect", card_reset_negate_effect },
	{ "AssumeProperty", card_assume_prop },
	{ "SetSPSummonOnce", card_set_spsummon_once },
	{ "Code", card_code },
	{ "Alias", card_alias },
	{ "Setcode", card_setcode },
	{ "Type", card_type },
	{ "Level", card_level },
	{ "Attribute", card_attribute },
	{ "Race", card_race },
	{ "Attack", card_attack },
	{ "Defense", card_defense },
	{ "Rscale", card_rscale },
	{ "Lscale", card_lscale },
	{ "LinkMarker", card_link_marker },
	{ "Recreate", card_recreate },
	{ "Cover", card_cover },
	{ "GetLuaRef", get_lua_ref<card> },
	{ "FromLuaRef", from_lua_ref<card> },
	{ "IsDeleted", is_deleted_object },
	{ nullptr, nullptr }
};
}

void scriptlib::push_card_lib(lua_State* L) {
	luaL_newlib(L, cardlib);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_setglobal(L, "Card");
}