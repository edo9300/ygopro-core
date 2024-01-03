/*
 * Copyright (c) 2010-2015, Argon Sun (Fluorohydride)
 * Copyright (c) 2017-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <algorithm>
#include "scriptlib.h"
#include "duel.h"
#include "field.h"
#include "card.h"
#include "effect.h"
#include "group.h"

#define LUA_MODULE Card
using LUA_CLASS = card;
#include "function_array_helper.h"

namespace {

using namespace scriptlib;

LUA_FUNCTION(GetCode) {
	check_param_count(L, 1);
	if (lua_gettop(L) > 1) {
		card* scard = 0;
		uint8_t playerid = PLAYER_NONE;
		if (!lua_isnoneornil(L, 2))
			scard = lua_get<card*, true>(L, 2);
		auto sumtype = lua_get<uint64_t, 0>(L, 3);
		if (lua_gettop(L) > 3)
			playerid = lua_get<uint8_t>(L, 4);
		else if (sumtype == SUMMON_TYPE_FUSION)
			playerid = pduel->game_field->core.reason_player;
		std::set<uint32_t> codes;
		self->get_summon_code(codes, scard, sumtype, playerid);
		if (codes.empty()) {
			lua_pushnil(L);
			return 1;
		}
		luaL_checkstack(L, static_cast<int>(codes.size()), nullptr);
		for(uint32_t code : codes)
			lua_pushinteger(pduel->lua->current_state, code);
		return static_cast<int32_t>(codes.size());
	}
	lua_pushinteger(L, self->get_code());
	uint32_t otcode = self->get_another_code();
	if(otcode) {
		lua_pushinteger(L, otcode);
		return 2;
	}
	return 1;
}
// GetOriginalCode(): get the original code printed on card
// return: 1 int
LUA_FUNCTION(GetOriginalCode) {
	check_param_count(L, 1);
	if(self->data.alias) {
		int32_t dif = self->data.code - self->data.alias;
		if(dif > -10 && dif < 10)
			lua_pushinteger(L, self->data.alias);
		else
			lua_pushinteger(L, self->data.code);
	} else
		lua_pushinteger(L, self->data.code);
	return 1;
}
// GetOriginalCodeRule(): get the original code in duel (can be different from printed code)
// return: 1-2 int
LUA_FUNCTION(GetOriginalCodeRule) {
	check_param_count(L, 1);
	effect_set eset;
	self->filter_effect(EFFECT_ADD_CODE, &eset);
	if(self->data.alias && !eset.size())
		lua_pushinteger(L, self->data.alias);
	else {
		lua_pushinteger(L, self->data.code);
		if(eset.size()) {
			uint32_t otcode = eset.back()->get_value(self);
			lua_pushinteger(L, otcode);
			return 2;
		}
	}
	return 1;
}
LUA_FUNCTION(GetSetCard) {
	check_param_count(L, 1);
	std::set<uint16_t> setcodes;
	if (lua_gettop(L) > 1) {
		card* scard = 0;
		uint8_t playerid = PLAYER_NONE;
		if (!lua_isnoneornil(L, 2))
			scard = lua_get<card*, true>(L, 2);
		auto sumtype = lua_get<uint64_t, 0>(L, 3);
		if (lua_gettop(L) > 3)
			playerid = lua_get<uint8_t>(L, 4);
		else if (sumtype == SUMMON_TYPE_FUSION)
			playerid = pduel->game_field->core.reason_player;
		self->get_summon_set_card(setcodes, scard, sumtype, playerid);
	} else {
		self->get_set_card(setcodes);
	}
	if (setcodes.empty()) {
		lua_pushnil(L);
		return 1;
	}
	luaL_checkstack(L, static_cast<int>(setcodes.size()), nullptr);
	for(const auto setcode : setcodes)
		lua_pushinteger(L, setcode);
	return static_cast<int32_t>(setcodes.size());
}
LUA_FUNCTION(GetOriginalSetCard) {
	check_param_count(L, 1);
	const auto& setcodes = self->get_origin_set_card();
	luaL_checkstack(L, static_cast<int>(setcodes.size()), nullptr);
	for(auto& setcode : setcodes)
		lua_pushinteger(L, setcode);
	return static_cast<int32_t>(setcodes.size());
}
LUA_FUNCTION(GetPreviousSetCard) {
	check_param_count(L, 1);
	std::set<uint16_t> setcodes;
	self->get_pre_set_card(setcodes);
	if (setcodes.empty()) {
		lua_pushnil(L);
		return 1;
	}
	luaL_checkstack(L, static_cast<int>(setcodes.size()), nullptr);
	for(auto& setcode : setcodes)
		lua_pushinteger(L, setcode);
	return static_cast<int32_t>(setcodes.size());
}
LUA_FUNCTION(GetType) {
	check_param_count(L, 1);
	card* scard = 0;
	uint8_t playerid = PLAYER_NONE;
	if (lua_gettop(L) > 1 && !lua_isnoneornil(L, 2))
		scard = lua_get<card*, true>(L, 2);
	auto sumtype = lua_get<uint64_t, 0>(L, 3);
	if (lua_gettop(L) > 3)
		playerid = lua_get<uint8_t>(L, 4);
	else if (sumtype == SUMMON_TYPE_FUSION)
		playerid = pduel->game_field->core.reason_player;
	lua_pushinteger(L, self->get_type(scard, sumtype, playerid));
	return 1;
}
LUA_FUNCTION(GetOriginalType) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->data.type);
	return 1;
}
LUA_FUNCTION(GetLevel) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->get_level());
	return 1;
}
LUA_FUNCTION(GetRank) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->get_rank());
	return 1;
}
LUA_FUNCTION(GetLink) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->get_link());
	return 1;
}
LUA_FUNCTION(GetSynchroLevel) {
	check_param_count(L, 2);
	auto scard = lua_get<card*, true>(L, 2);
	lua_pushinteger(L, self->get_synchro_level(scard));
	return 1;
}
LUA_FUNCTION(GetRitualLevel) {
	check_param_count(L, 2);
	auto scard = lua_get<card*, true>(L, 2);
	lua_pushinteger(L, self->get_ritual_level(scard));
	return 1;
}
LUA_FUNCTION(GetOriginalLevel) {
	check_param_count(L, 1);
	if((self->data.type & (TYPE_XYZ | TYPE_LINK)) || (self->status & STATUS_NO_LEVEL))
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, self->data.level);
	return 1;
}
LUA_FUNCTION(GetOriginalRank) {
	check_param_count(L, 1);
	if(!(self->data.type & TYPE_XYZ))
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, self->data.level);
	return 1;
}
LUA_FUNCTION(IsXyzLevel) {
	check_param_count(L, 3);
	auto xyzcard = lua_get<card*, true>(L, 2);
	auto lv = lua_get<uint32_t>(L, 3);
	lua_pushboolean(L, self->check_xyz_level(xyzcard, lv));
	return 1;
}
LUA_FUNCTION(GetLeftScale) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->get_lscale());
	return 1;
}
LUA_FUNCTION(GetOriginalLeftScale) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->data.lscale);
	return 1;
}
LUA_FUNCTION(GetRightScale) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->get_rscale());
	return 1;
}
LUA_FUNCTION(GetOriginalRightScale) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->data.rscale);
	return 1;
}
LUA_FUNCTION(GetLinkMarker) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->get_link_marker());
	return 1;
}
LUA_FUNCTION(IsLinkMarker) {
	check_param_count(L, 2);
	auto dir = lua_get<uint32_t>(L, 2);
	lua_pushboolean(L, self->is_link_marker(dir));
	return 1;
}
LUA_FUNCTION(GetLinkedGroup) {
	check_param_count(L, 1);
	card_set cset;
	self->get_linked_cards(&cset);
	group* pgroup = pduel->new_group(cset);
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(GetLinkedGroupCount) {
	check_param_count(L, 1);
	card_set cset;
	self->get_linked_cards(&cset);
	lua_pushinteger(L, cset.size());
	return 1;
}
LUA_FUNCTION(GetLinkedZone) {
	check_param_count(L, 1);
	uint32_t zone = self->get_linked_zone();
	auto cp = lua_get<uint8_t>(L, 2, self->current.controler);
	if(cp == 1 - self->current.controler)
		lua_pushinteger(L, (((zone & 0xffff) << 16) | (zone >> 16)));
	else
		lua_pushinteger(L, zone);
	return 1;
}
LUA_FUNCTION(GetFreeLinkedZone) {
	check_param_count(L, 1);
	uint32_t zone = self->get_linked_zone(true);
	auto cp = lua_get<uint8_t>(L, 2, self->current.controler);
	if(cp == 1 - self->current.controler)
		lua_pushinteger(L, (((zone & 0xffff) << 16) | (zone >> 16)));
	else
		lua_pushinteger(L, zone);
	return 1;
}
LUA_FUNCTION(GetMutualLinkedGroup) {
	check_param_count(L, 1);
	card_set cset;
	self->get_mutual_linked_cards(&cset);
	group* pgroup = pduel->new_group(cset);
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(GetMutualLinkedGroupCount) {
	check_param_count(L, 1);
	card_set cset;
	self->get_mutual_linked_cards(&cset);
	lua_pushinteger(L, cset.size());
	return 1;
}
LUA_FUNCTION(GetMutualLinkedZone) {
	check_param_count(L, 1);
	uint32_t zone = self->get_mutual_linked_zone();
	auto cp = lua_get<uint8_t>(L, 2, self->current.controler);
	if(cp == 1 - self->current.controler)
		lua_pushinteger(L, (((zone & 0xffff) << 16) | (zone >> 16)));
	else
		lua_pushinteger(L, zone);
	return 1;
}
LUA_FUNCTION(IsLinked) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_link_state());
	return 1;
}
LUA_FUNCTION(IsExtraLinked) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_extra_link_state());
	return 1;
}
LUA_FUNCTION(GetColumnGroup) {
	check_param_count(L, 1);
	auto left = lua_get<uint8_t, 0>(L, 2);
	auto right = lua_get<uint8_t, 0>(L, 3);
	card_set cset;
	self->get_column_cards(&cset, left, right);
	group* pgroup = pduel->new_group(cset);
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(GetColumnGroupCount) {
	check_param_count(L, 1);
	auto left = lua_get<uint8_t, 0>(L, 2);
	auto right = lua_get<uint8_t, 0>(L, 3);
	card_set cset;
	self->get_column_cards(&cset, left, right);
	lua_pushinteger(L, cset.size());
	return 1;
}
LUA_FUNCTION(GetColumnZone) {
	check_param_count(L, 2);
	auto loc = lua_get<uint16_t>(L, 2);
	auto left = lua_get<uint8_t, 0>(L, 3);
	auto right = lua_get<uint8_t, 0>(L, 4);
	auto cp = lua_get<uint8_t>(L, 5, self->current.controler);
	uint32_t zone = self->get_column_zone(loc, left, right);
	if(cp == 1 - self->current.controler)
		lua_pushinteger(L, (((zone & 0xffff) << 16) | (zone >> 16)));
	else
		lua_pushinteger(L, zone);
	return 1;
}
LUA_FUNCTION(IsAllColumn) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_all_column());
	return 1;
}
LUA_FUNCTION(GetAttribute) {
	check_param_count(L, 1);
	card* scard = 0;
	uint8_t playerid = PLAYER_NONE;
	if (lua_gettop(L) > 1 && !lua_isnoneornil(L, 2))
		scard = lua_get<card*, true>(L, 2);
	auto sumtype = lua_get<uint64_t, 0>(L, 3);
	if (lua_gettop(L) > 3)
		playerid = lua_get<uint8_t>(L, 4);
	else if (sumtype == SUMMON_TYPE_FUSION)
		playerid = pduel->game_field->core.reason_player;
	lua_pushinteger(L, self->get_attribute(scard, sumtype, playerid));
	return 1;
}
LUA_FUNCTION(GetOriginalAttribute) {
	check_param_count(L, 1);
	if(self->status & STATUS_NO_LEVEL)
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, self->data.attribute);
	return 1;
}
LUA_FUNCTION(GetRace) {
	check_param_count(L, 1);
	card* scard = 0;
	uint8_t playerid = PLAYER_NONE;
	if (lua_gettop(L) > 1 && !lua_isnoneornil(L, 2))
		scard = lua_get<card*, true>(L, 2);
	auto sumtype = lua_get<uint64_t, 0>(L, 3);
	if (lua_gettop(L) > 3)
		playerid = lua_get<uint8_t>(L, 4);
	else if (sumtype == SUMMON_TYPE_FUSION)
		playerid = pduel->game_field->core.reason_player;
	lua_pushinteger(L, self->get_race(scard, sumtype, playerid));
	return 1;
}
LUA_FUNCTION(GetOriginalRace) {
	check_param_count(L, 1);
	if(self->status & STATUS_NO_LEVEL)
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, self->data.race);
	return 1;
}
LUA_FUNCTION(GetAttack) {
	check_param_count(L, 1);
	int32_t atk = self->get_attack();
	if(atk < 0)
		atk = 0;
	lua_pushinteger(L, atk);
	return 1;
}
LUA_FUNCTION(GetBaseAttack) {
	check_param_count(L, 1);
	int32_t atk = self->get_base_attack();
	if(atk < 0)
		atk = 0;
	lua_pushinteger(L, atk);
	return 1;
}
LUA_FUNCTION(GetTextAttack) {
	check_param_count(L, 1);
	if(self->status & STATUS_NO_LEVEL)
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, self->data.attack);
	return 1;
}
LUA_FUNCTION(GetDefense) {
	check_param_count(L, 1);
	int32_t def = self->get_defense();
	if(def < 0)
		def = 0;
	lua_pushinteger(L, def);
	return 1;
}
LUA_FUNCTION(GetBaseDefense) {
	check_param_count(L, 1);
	int32_t def = self->get_base_defense();
	if(def < 0)
		def = 0;
	lua_pushinteger(L, def);
	return 1;
}
LUA_FUNCTION(GetTextDefense) {
	check_param_count(L, 1);
	if(self->status & STATUS_NO_LEVEL)
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, self->data.defense);
	return 1;
}
LUA_FUNCTION(GetPreviousCodeOnField) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->previous.code);
	if(self->previous.code2) {
		lua_pushinteger(L, self->previous.code2);
		return 2;
	}
	return 1;
}
LUA_FUNCTION(GetPreviousTypeOnField) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->previous.type);
	return 1;
}
LUA_FUNCTION(GetPreviousLevelOnField) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->previous.level);
	return 1;
}
LUA_FUNCTION(GetPreviousRankOnField) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->previous.rank);
	return 1;
}
LUA_FUNCTION(GetPreviousAttributeOnField) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->previous.attribute);
	return 1;
}
LUA_FUNCTION(GetPreviousRaceOnField) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->previous.race);
	return 1;
}
LUA_FUNCTION(GetPreviousAttackOnField) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->previous.attack);
	return 1;
}
LUA_FUNCTION(GetPreviousDefenseOnField) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->previous.defense);
	return 1;
}
LUA_FUNCTION(GetOwner) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->owner);
	return 1;
}
LUA_FUNCTION(GetControler) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->current.controler);
	return 1;
}
LUA_FUNCTION(GetPreviousControler) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->previous.controler);
	return 1;
}
LUA_FUNCTION(GetReason) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->current.reason);
	return 1;
}
LUA_FUNCTION(GetReasonCard) {
	check_param_count(L, 1);
	interpreter::pushobject(L, self->current.reason_card);
	return 1;
}
LUA_FUNCTION(GetReasonPlayer) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->current.reason_player);
	return 1;
}
LUA_FUNCTION(GetReasonEffect) {
	check_param_count(L, 1);
	interpreter::pushobject(L, self->current.reason_effect);
	return 1;
}
LUA_FUNCTION(SetReason) {
	check_param_count(L, 2);
	auto reason = lua_get<uint32_t>(L, 2);
	if (lua_get<bool, false>(L, 3))
		self->current.reason |= reason;
	else
		self->current.reason = reason;
	return 0;
}
LUA_FUNCTION(SetReasonCard) {
	check_param_count(L, 2);
	auto rcard = lua_get<card*, true>(L, 2);
	self->current.reason_card = rcard;
	return 0;
}
LUA_FUNCTION(SetReasonPlayer) {
	check_param_count(L, 2);
	auto rp = lua_get<uint8_t>(L, 2);
	if (rp <= (unsigned)PLAYER_ALL)
		self->current.reason_player = rp;
	return 0;
}
LUA_FUNCTION(SetReasonEffect) {
	check_param_count(L, 2);
	auto re = lua_get<effect*, true>(L, 2);
	self->current.reason_effect = re;
	return 0;
}
LUA_FUNCTION(GetPosition) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->current.position);
	return 1;
}
LUA_FUNCTION(GetPreviousPosition) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->previous.position);
	return 1;
}
LUA_FUNCTION(GetBattlePosition) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->temp.position);
	return 1;
}
LUA_FUNCTION(GetLocation) {
	check_param_count(L, 1);
	if(self->get_status(STATUS_SUMMONING | STATUS_SUMMON_DISABLED | STATUS_ACTIVATE_DISABLED | STATUS_SPSUMMON_STEP))
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, self->current.location);
	return 1;
}
LUA_FUNCTION(GetPreviousLocation) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->previous.location);
	return 1;
}
LUA_FUNCTION(GetSequence) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->current.sequence);
	return 1;
}
LUA_FUNCTION(GetPreviousSequence) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->previous.sequence);
	return 1;
}
LUA_FUNCTION(GetSummonType) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->summon.type & 0xff00ffff);
	return 1;
}
LUA_FUNCTION(GetSummonLocation) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->summon.location);
	return 1;
}
LUA_FUNCTION(GetSummonPlayer) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->summon.player);
	return 1;
}
LUA_FUNCTION(GetDestination) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->sendto_param.location);
	return 1;
}
LUA_FUNCTION(GetLeaveFieldDest) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->leave_field_redirect(REASON_EFFECT));
	return 1;
}
LUA_FUNCTION(GetTurnID) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->turnid);
	return 1;
}
LUA_FUNCTION(GetFieldID) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->fieldid);
	return 1;
}
LUA_FUNCTION(GetRealFieldID) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->fieldid_r);
	return 1;
}
LUA_FUNCTION(GetCardID) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->cardid);
	return 1;
}
LUA_FUNCTION(IsOriginalCodeRule) {
	check_param_count(L, 2);
	uint32_t code1 = 0;
	uint32_t code2 = 0;
	effect_set eset;
	self->filter_effect(EFFECT_ADD_CODE, &eset);
	if(self->data.alias && !eset.size()){
		code1 = self->data.alias;
		code2 = 0;
	} else {
		code1 = self->data.code;
		if(eset.size())
			code2 = eset.back()->get_value(self);
	}
	bool found = lua_find_in_table_or_in_stack(L, 2, lua_gettop(L), [L, code1, code2] {
		if(lua_isnoneornil(L, -1))
			return false;
		uint32_t tcode = lua_get<uint32_t>(L, -1);
		return code1 == tcode || (code2 && code2 == tcode);
	});
	lua_pushboolean(L, found);
	return 1;
}
LUA_FUNCTION(IsOriginalCode) {
	check_param_count(L, 2);
	const auto original_code = [self]() {
		if(self->data.alias) {
			int32_t dif = self->data.code - self->data.alias;
			if(dif > -10 && dif < 10)
				return self->data.alias;
			else
				return self->data.code;
		} else
			return self->data.code;
	}();
	bool found = lua_find_in_table_or_in_stack(L, 2, lua_gettop(L), [L, original_code] {
		if(lua_isnoneornil(L, -1))
			return false;
		return original_code == lua_get<uint32_t>(L, -1);
	});
	lua_pushboolean(L, found);
	return 1;
}
LUA_FUNCTION(IsCode) {
	check_param_count(L, 2);
	uint32_t code1 = self->get_code();
	uint32_t code2 = self->get_another_code();
	bool found = lua_find_in_table_or_in_stack(L, 2, lua_gettop(L), [L, code1, code2] {
		if(lua_isnoneornil(L, -1))
			return false;
		uint32_t tcode = lua_get<uint32_t>(L, -1);
		return code1 == tcode || (code2 && code2 == tcode);
	});
	lua_pushboolean(L, found);
	return 1;
}
LUA_FUNCTION(IsSummonCode) {
	check_param_count(L, 5);
	auto scard = lua_get<card*>(L, 2);
	auto sumtype = lua_get<uint64_t>(L, 3);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 4);
	effect_set eset;
	std::set<uint32_t> codes;
	self->filter_effect(EFFECT_ADD_CODE, &eset, FALSE);
	self->filter_effect(EFFECT_REMOVE_CODE, &eset, FALSE);
	self->filter_effect(EFFECT_CHANGE_CODE, &eset);
	uint32_t code1 = self->get_code();
	uint32_t code2 = self->get_another_code();
	codes.insert(code1);
	if (code2)
		codes.insert(code2);
	for (const auto& peff : eset) {
		if (!peff->operation)
			continue;
		pduel->lua->add_param<LuaParam::CARD>(scard);
		pduel->lua->add_param<LuaParam::INT>(sumtype);
		pduel->lua->add_param<LuaParam::INT>(playerid);
		if (!pduel->lua->check_condition(peff->operation, 3))
			continue;
		if (peff->code == EFFECT_ADD_CODE)
			codes.insert(peff->get_value(self));
		else if (peff->code == EFFECT_REMOVE_CODE) {
			auto cit = codes.find(peff->get_value(self));
			if (cit != codes.end())
				codes.erase(cit);
		} else {
			codes.clear();
			codes.insert(peff->get_value(self));
		}
	}
	bool found = lua_find_in_table_or_in_stack(L, 5, lua_gettop(L), [L, &codes] {
		if(lua_isnoneornil(L, -1))
			return false;
		uint32_t tcode = lua_get<uint32_t>(L, -1);
		return codes.find(tcode) != codes.end();
	});
	lua_pushboolean(L, found);
	return 1;
}
inline bool set_contains_setcode(const std::set<uint16_t>& setcodes, uint16_t to_match_setcode) {
	return std::any_of(setcodes.cbegin(), setcodes.cend(), [to_match_setcode](uint16_t setcode) {
		return card::match_setcode(to_match_setcode, setcode);
	});
}
LUA_FUNCTION(IsSetCard) {
	check_param_count(L, 2);
	std::set<uint16_t> setcodes;
	if (lua_gettop(L) > 2) {
		card* scard = 0;
		uint8_t playerid = PLAYER_NONE;
		if (!lua_isnoneornil(L, 3))
			scard = lua_get<card*, true>(L, 3);
		auto sumtype = lua_get<uint64_t, 0>(L, 4);
		playerid = lua_get<uint8_t, PLAYER_NONE>(L, 5);
		self->get_summon_set_card(setcodes, scard, sumtype, playerid);
	} else
		self->get_set_card(setcodes);
	bool found = lua_find_in_table_or_in_stack(L, 2, 2, [L, &setcodes] {
		return set_contains_setcode(setcodes, lua_get<uint16_t>(L, -1));
	});
	lua_pushboolean(L, found);
	return 1;
}
LUA_FUNCTION(IsOriginalSetCard) {
	check_param_count(L, 2);
	const auto& setcodes = self->get_origin_set_card();
	bool found = lua_find_in_table_or_in_stack(L, 2, lua_gettop(L), [L, &setcodes] {
		return set_contains_setcode(setcodes, lua_get<uint16_t>(L, -1));
	});
	lua_pushboolean(L, found);
	return 1;
}
LUA_FUNCTION(IsPreviousSetCard) {
	check_param_count(L, 2);
	std::set<uint16_t> setcodes;
	self->get_pre_set_card(setcodes);
	bool found = lua_find_in_table_or_in_stack(L, 2, 2, [L, &setcodes] {
		return set_contains_setcode(setcodes, lua_get<uint16_t>(L, -1));
	});
	lua_pushboolean(L, found);
	return 1;
}
LUA_FUNCTION(IsType) {
	check_param_count(L, 2);
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
	lua_pushboolean(L, self->get_type(scard, sumtype, playerid) & ttype);
	return 1;
}
LUA_FUNCTION(IsExactType) {
	check_param_count(L, 2);
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
	lua_pushboolean(L, (self->get_type(scard, sumtype, playerid) & ttype) == ttype);
	return 1;
}
LUA_FUNCTION(IsOriginalType) {
	check_param_count(L, 2);
	auto ttype = lua_get<uint32_t>(L, 2);
	lua_pushboolean(L, (self->data.type & ttype) != 0);
	return 1;
}
inline int32_t is_prop(lua_State* L, uint32_t val) {
	bool found = lua_find_in_table_or_in_stack(L, 2, lua_gettop(L), [L, val] {
		if(lua_isnoneornil(L, -1))
			return false;
		return val == lua_get<uint32_t>(L, -1);
	});
	lua_pushboolean(L, found);
	return 1;
}
LUA_FUNCTION(IsLevel) {
	check_param_count(L, 2);
	return is_prop(L, self->get_level());
}
LUA_FUNCTION(IsRank) {
	check_param_count(L, 2);
	return is_prop(L, self->get_rank());
}
LUA_FUNCTION(IsLink) {
	check_param_count(L, 2);
	return is_prop(L, self->get_link());
}
LUA_FUNCTION(IsAttack) {
	check_param_count(L, 2);
	if(!(self->data.type & TYPE_MONSTER) && !(self->get_type() & TYPE_MONSTER) && !self->is_affected_by_effect(EFFECT_PRE_MONSTER))
		lua_pushboolean(L, 0);
	else
		is_prop(L, self->get_attack());
	return 1;
}
LUA_FUNCTION(IsDefense) {
	check_param_count(L, 2);
	if((self->data.type & TYPE_LINK)
	   || (!(self->data.type & TYPE_MONSTER) && !(self->get_type() & TYPE_MONSTER) && !self->is_affected_by_effect(EFFECT_PRE_MONSTER)))
		lua_pushboolean(L, 0);
	else
		is_prop(L, self->get_defense());
	return 1;
}
LUA_FUNCTION(IsRace) {
	check_param_count(L, 2);
	auto trace = lua_get<uint64_t>(L, 2);
	card* scard = 0;
	auto playerid = PLAYER_NONE;
	if(lua_gettop(L) > 2 && !lua_isnoneornil(L, 3))
		scard = lua_get<card*, true>(L, 3);
	auto sumtype = lua_get<uint64_t, 0>(L, 4);
	if(lua_gettop(L) > 4)
		playerid = lua_get<uint8_t>(L, 5);
	else if(sumtype==SUMMON_TYPE_FUSION)
		playerid = pduel->game_field->core.reason_player;
	lua_pushboolean(L, (self->get_race(scard, sumtype, playerid) & trace) != 0);
	return 1;
}
LUA_FUNCTION(IsOriginalRace) {
	check_param_count(L, 2);
	auto trace = lua_get<uint64_t>(L, 2);
	if(self->status & STATUS_NO_LEVEL)
		lua_pushboolean(L, FALSE);
	else
		lua_pushboolean(L, (self->data.race & trace) != 0);
	return 1;
}
LUA_FUNCTION(IsAttribute) {
	check_param_count(L, 2);
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
	lua_pushboolean(L, self->get_attribute(scard, sumtype, playerid) & tattrib);
	return 1;
}
LUA_FUNCTION(IsOriginalAttribute) {
	check_param_count(L, 2);
	auto tattrib = lua_get<uint32_t>(L, 2);
	if(self->status & STATUS_NO_LEVEL)
		lua_pushboolean(L, FALSE);
	else
		lua_pushboolean(L, (self->data.attribute & tattrib) != 0);
	return 1;
}
LUA_FUNCTION(IsReason) {
	check_param_count(L, 2);
	auto treason = lua_get<uint32_t>(L, 2);
	lua_pushboolean(L, (self->current.reason & treason) != 0);
	return 1;
}
LUA_FUNCTION(IsSummonType) {
	check_param_count(L, 2);
	bool found = lua_find_in_table_or_in_stack(L, 2, lua_gettop(L), [L, summon_info = self->summon.type & 0xff00ffff] {
		if(lua_isnoneornil(L, -1))
			return false;
		auto ttype = lua_get<uint32_t>(L, -1);
		return (summon_info & ttype) == ttype;
	});
	lua_pushboolean(L, found);
	return 1;
}
LUA_FUNCTION(IsSummonLocation) {
	check_param_count(L, 1);
	auto loc = lua_get<uint16_t>(L, 2);
	lua_pushboolean(L, card_state::is_location(self->summon, loc));
	return 1;
}
LUA_FUNCTION(IsSummonPlayer) {
	check_param_count(L, 1);
	auto player = lua_get<uint8_t>(L, 2);
	lua_pushboolean(L, self->summon.player == player);
	return 1;
}
LUA_FUNCTION(IsStatus) {
	check_param_count(L, 2);
	auto tstatus = lua_get<uint32_t>(L, 2);
	lua_pushboolean(L, (self->status & tstatus) != 0);
	return 1;
}
LUA_FUNCTION(IsNotTuner) {
	check_param_count(L, 3);
	auto scard = lua_get<card*, true>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 3);
	lua_pushboolean(L, self->is_not_tuner(scard, playerid));
	return 1;
}
LUA_FUNCTION(SetStatus) {
	check_param_count(L, 3);
	if(self->status & STATUS_COPYING_EFFECT)
		return 0;
	auto tstatus = lua_get<uint32_t>(L, 2);
	auto enable = lua_get<bool>(L, 3);
	self->set_status(tstatus, enable);
	return 0;
}
LUA_FUNCTION(IsGeminiState) {
	check_param_count(L, 1);
	lua_pushboolean(L, !!self->is_affected_by_effect(EFFECT_GEMINI_STATUS));
	return 1;
}
LUA_FUNCTION(EnableGeminiState) {
	check_param_count(L, 1);
	effect* deffect = pduel->new_effect();
	deffect->owner = self;
	deffect->code = EFFECT_GEMINI_STATUS;
	deffect->type = EFFECT_TYPE_SINGLE;
	deffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_CLIENT_HINT;
	deffect->description = 64;
	deffect->reset_flag = RESET_EVENT + 0x1fe0000;
	self->add_effect(deffect);
	return 0;
}
LUA_FUNCTION(SetTurnCounter) {
	check_param_count(L, 2);
	auto ct = lua_get<uint16_t>(L, 2);
	self->count_turn(ct);
	return 0;
}
LUA_FUNCTION(GetTurnCounter) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->turn_counter);
	return 1;
}
LUA_FUNCTION(SetMaterial) {
	check_param_count(L, 2);
	if(auto [pcard, pgroup] = lua_get_card_or_group<true>(L, 2); pcard) {
		self->set_material({ pcard });
	} else if(pgroup) {
		self->set_material(pgroup->container);
	} else {
		self->set_material({});
	}
	return 0;
}
LUA_FUNCTION(GetMaterial) {
	check_param_count(L, 1);
	group* pgroup = pduel->new_group(self->material_cards);
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(GetMaterialCount) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->material_cards.size());
	return 1;
}
LUA_FUNCTION(GetEquipGroup) {
	check_param_count(L, 1);
	group* pgroup = pduel->new_group(self->equiping_cards);
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(GetEquipCount) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->equiping_cards.size());
	return 1;
}
LUA_FUNCTION(GetEquipTarget) {
	check_param_count(L, 1);
	interpreter::pushobject(L, self->equiping_target);
	return 1;
}
LUA_FUNCTION(GetPreviousEquipTarget) {
	check_param_count(L, 1);
	interpreter::pushobject(L, self->pre_equip_target);
	return 1;
}
LUA_FUNCTION(CheckEquipTarget) {
	check_param_count(L, 1);
	auto target = lua_get<card*, true>(L, 2);
	if(self->is_affected_by_effect(EFFECT_EQUIP_LIMIT, target)
		&& ((!self->is_affected_by_effect(EFFECT_OLDUNION_STATUS) || target->get_union_count() == 0)
			&& (!self->is_affected_by_effect(EFFECT_UNION_STATUS) || target->get_old_union_count() == 0)))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
LUA_FUNCTION(CheckUnionTarget) {
	check_param_count(L, 1);
	auto target = lua_get<card*, true>(L, 2);
	if(self->is_affected_by_effect(EFFECT_UNION_LIMIT, target)
		&& ((!self->is_affected_by_effect(EFFECT_OLDUNION_STATUS) || target->get_union_count() == 0)
			&& (!self->is_affected_by_effect(EFFECT_UNION_STATUS) || target->get_old_union_count() == 0)))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
LUA_FUNCTION(GetUnionCount) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->get_union_count());
	lua_pushinteger(L, self->get_old_union_count());
	return 2;
}
LUA_FUNCTION(GetOverlayGroup) {
	check_param_count(L, 1);
	group* pgroup = pduel->new_group(self->xyz_materials);
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(GetOverlayCount) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->xyz_materials.size());
	return 1;
}
LUA_FUNCTION(GetOverlayTarget) {
	check_param_count(L, 1);
	interpreter::pushobject(L, self->overlay_target);
	return 1;
}
LUA_FUNCTION(CheckRemoveOverlayCard) {
	check_param_count(L, 4);
	group* pgroup = pduel->new_group(self);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto count = lua_get<uint16_t>(L, 3);
	auto reason = lua_get<uint32_t>(L, 4);
	lua_pushboolean(L, pduel->game_field->is_player_can_remove_overlay_card(playerid, pgroup, 0, 0, count, reason));
	return 1;
}
LUA_FUNCTION(RemoveOverlayCard) {
	check_action_permission(L);
	check_param_count(L, 5);
	group* pgroup = pduel->new_group(self);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto min = lua_get<uint16_t>(L, 3);
	auto max = lua_get<uint16_t>(L, 4);
	auto reason = lua_get<uint32_t>(L, 5);
	pduel->game_field->remove_overlay_card(reason, pgroup, playerid, 0, 0, min, max);
	return yieldk({
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(GetAttackedGroup) {
	check_param_count(L, 1);
	group* pgroup = pduel->new_group();
	for(auto& cit : self->attacked_cards) {
		if(cit.second.first)
			pgroup->container.insert(cit.second.first);
	}
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(GetAttackedGroupCount) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->attacked_cards.size());
	return 1;
}
LUA_FUNCTION(GetAttackedCount) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->attacked_count);
	return 1;
}
LUA_FUNCTION(GetBattledGroup) {
	check_param_count(L, 1);
	group* pgroup = pduel->new_group();
	for(auto& cit : self->battled_cards) {
		if(cit.second.first)
			pgroup->container.insert(cit.second.first);
	}
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(GetBattledGroupCount) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->battled_cards.size());
	return 1;
}
LUA_FUNCTION(GetAttackAnnouncedCount) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->attack_announce_count);
	return 1;
}
LUA_FUNCTION(IsDirectAttacked) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->attacked_cards.findcard(0));
	return 1;
}
LUA_FUNCTION(SetCardTarget) {
	check_param_count(L, 2);
	auto ocard = lua_get<card*, true>(L, 2);
	self->add_card_target(ocard);
	return 0;
}
LUA_FUNCTION(GetCardTarget) {
	check_param_count(L, 1);
	group* pgroup = pduel->new_group(self->effect_target_cards);
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(GetFirstCardTarget) {
	check_param_count(L, 1);
	if(self->effect_target_cards.size())
		interpreter::pushobject(L, *self->effect_target_cards.begin());
	else lua_pushnil(L);
	return 1;
}
LUA_FUNCTION(GetCardTargetCount) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->effect_target_cards.size());
	return 1;
}
LUA_FUNCTION(IsHasCardTarget) {
	check_param_count(L, 2);
	auto rcard = lua_get<card*, true>(L, 2);
	lua_pushboolean(L, self->effect_target_cards.find(rcard) != self->effect_target_cards.end());
	return 1;
}
LUA_FUNCTION(CancelCardTarget) {
	check_param_count(L, 2);
	auto rcard = lua_get<card*, true>(L, 2);
	self->cancel_card_target(rcard);
	return 0;
}
LUA_FUNCTION(GetOwnerTarget) {
	check_param_count(L, 1);
	group* pgroup = pduel->new_group(self->effect_target_owner);
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(GetOwnerTargetCount) {
	check_param_count(L, 1);
	lua_pushinteger(L, self->effect_target_owner.size());
	return 1;
}
LUA_FUNCTION(GetActivateEffect) {
	check_param_count(L, 1);
	effect_set eset;
	for(auto& eit : self->field_effect) {
		if(eit.second->type & EFFECT_TYPE_ACTIVATE)
			eset.push_back(eit.second);
	}
	luaL_checkstack(L, static_cast<int>(eset.size()), nullptr);
	for(const auto& peffect : eset)
		interpreter::pushobject(L, peffect);
	return static_cast<int32_t>(eset.size());
}
LUA_FUNCTION(CheckActivateEffect) {
	check_param_count(L, 4);
	bool neglect_con = lua_get<bool>(L, 2);
	bool neglect_cost = lua_get<bool>(L, 3);
	bool copy_info = lua_get<bool>(L, 4);
	tevent pe;
	for(const auto& eit : self->field_effect) {
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
LUA_FUNCTION(RegisterEffect) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 2);
	bool forced = lua_get<bool, false>(L, 3);
	if(peffect->owner == pduel->game_field->temp_card)
		return 0;
	if(!forced && pduel->game_field->core.reason_effect && !self->is_affect_by_effect(pduel->game_field->core.reason_effect)) {
		pduel->game_field->core.reseted_effects.insert(peffect);
		return 0;
	}
	int32_t id = -1;
	if (!peffect->handler)
		id = self->add_effect(peffect);
	lua_pushinteger(L, id);
	return 1;
}
LUA_FUNCTION(IsHasEffect) {
	check_param_count(L, 2);
	auto code = lua_get<uint32_t>(L, 2);
	effect_set eset;
	self->filter_effect(code, &eset);
	auto size = eset.size();
	if(!size) {
		lua_pushnil(L);
		return 1;
	}
	auto check_player = lua_get<uint8_t, PLAYER_NONE>(L, 3);
	luaL_checkstack(L, static_cast<int>(eset.size()), nullptr); // we waste a bit of stack space but keeps checking simple
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
	return static_cast<int32_t>(size);
}
LUA_FUNCTION(GetCardEffect) {
	check_param_count(L, 1);
	auto code = lua_get<uint32_t, 0>(L, 2);
	effect_set eset;
	self->get_card_effect(code, &eset);
	if(eset.empty()) {
		lua_pushnil(L);
		return 1;
	}
	luaL_checkstack(L, static_cast<int>(eset.size()), nullptr);
	for(const auto& peff : eset)
		interpreter::pushobject(L, peff);
	return static_cast<int32_t>(eset.size());
}
LUA_FUNCTION(ResetEffect) {
	check_param_count(L, 3);
	auto code = lua_get<uint32_t>(L, 2);
	auto type = lua_get<uint32_t>(L, 3);
	self->reset(code, type);
	return 0;
}
LUA_FUNCTION(GetEffectCount) {
	check_param_count(L, 2);
	auto code = lua_get<uint32_t>(L, 2);
	effect_set eset;
	self->filter_effect(code, &eset);
	lua_pushinteger(L, eset.size());
	return 1;
}
LUA_FUNCTION(RegisterFlagEffect) {
	check_param_count(L, 5);
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	auto reset = lua_get<uint32_t>(L, 3);
	auto flag = lua_get<uint32_t>(L, 4);
	auto count = lua_get<uint16_t>(L, 5);
	auto lab = lua_get<lua_Integer, 0>(L, 6);
	auto desc = lua_get<uint64_t, 0>(L, 7);
	if(count == 0)
		count = 1;
	if(reset & (RESET_PHASE) && !(reset & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		reset |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	effect* peffect = pduel->new_effect();
	peffect->owner = self;
	peffect->handler = 0;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = code;
	peffect->reset_flag = reset;
	peffect->flag[0] = flag | EFFECT_FLAG_CANNOT_DISABLE;
	peffect->reset_count = count;
	peffect->label = { lab };
	peffect->description = desc;
	self->add_effect(peffect);
	interpreter::pushobject(L, peffect);
	return 1;
}
LUA_FUNCTION(GetFlagEffect) {
	check_param_count(L, 2);
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	lua_pushinteger(L, self->single_effect.count(code));
	return 1;
}
LUA_FUNCTION(ResetFlagEffect) {
	check_param_count(L, 2);
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	self->reset(code, RESET_CODE);
	return 0;
}
LUA_FUNCTION(SetFlagEffectLabel) {
	check_param_count(L, 3);
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	auto lab = lua_get<uint32_t>(L, 3);
	auto eit = self->single_effect.find(code);
	if(eit == self->single_effect.end())
		lua_pushboolean(L, FALSE);
	else {
		eit->second->label = { lab };
		lua_pushboolean(L, TRUE);
	}
	return 1;
}
LUA_FUNCTION(GetFlagEffectLabel) {
	check_param_count(L, 2);
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	auto rg = self->single_effect.equal_range(code);
	auto count = std::distance(rg.first, rg.second);
	if(!count) {
		lua_pushnil(L);
		return 1;
	}
	luaL_checkstack(L, count, nullptr);
	for(; rg.first != rg.second; ++rg.first) {
		const auto& label = rg.first->second->label;
		lua_pushinteger(L, label.size() ? label[0] : 0);
	}
	return count;
}
LUA_FUNCTION(CreateRelation) {
	check_param_count(L, 3);
	auto rcard = lua_get<card*, true>(L, 2);
	auto reset = lua_get<uint32_t>(L, 3);
	self->create_relation(rcard, reset);
	return 0;
}
LUA_FUNCTION(ReleaseRelation) {
	check_param_count(L, 2);
	auto rcard = lua_get<card*, true>(L, 2);
	self->release_relation(rcard);
	return 0;
}
LUA_FUNCTION(CreateEffectRelation) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 2);
	self->create_relation(peffect);
	return 0;
}
LUA_FUNCTION(ReleaseEffectRelation) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 2);
	self->release_relation(peffect);
	return 0;
}
LUA_FUNCTION(ClearEffectRelation) {
	check_param_count(L, 1);
	self->clear_relate_effect();
	return 0;
}
LUA_FUNCTION(IsRelateToEffect) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 2);
	lua_pushboolean(L, self->is_has_relation(peffect));
	return 1;
}
LUA_FUNCTION(IsRelateToChain) {
	check_param_count(L, 2);
	auto chain_count = lua_get<uint8_t>(L, 2);
	if(chain_count > pduel->game_field->core.current_chain.size() || chain_count < 1)
		chain_count = (uint8_t)pduel->game_field->core.current_chain.size();
	lua_pushboolean(L, self->is_has_relation(pduel->game_field->core.current_chain[chain_count - 1]));
	return 1;
}
LUA_FUNCTION(IsRelateToCard) {
	check_param_count(L, 2);
	auto rcard = lua_get<card*, true>(L, 2);
	lua_pushboolean(L, self->is_has_relation(rcard));
	return 1;
}
LUA_FUNCTION(IsRelateToBattle) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->fieldid_r == pduel->game_field->core.pre_field[0] || self->fieldid_r == pduel->game_field->core.pre_field[1]);
	return 1;
}
LUA_FUNCTION(CopyEffect) {
	check_param_count(L, 3);
	auto code = lua_get<uint32_t>(L, 2);
	auto reset = lua_get<uint32_t>(L, 3);
	auto count = lua_get<uint8_t, 1>(L, 4);
	if(count == 0)
		count = 1;
	if(reset & RESET_PHASE && !(reset & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		reset |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	lua_pushinteger(L, self->copy_effect(code, reset, count));
	return 1;
}
LUA_FUNCTION(ReplaceEffect) {
	check_param_count(L, 3);
	auto code = lua_get<uint32_t>(L, 2);
	auto reset = lua_get<uint32_t>(L, 3);
	auto count = lua_get<uint8_t, 0>(L, 4);
	if(count == 0)
		count = 1;
	if(reset & RESET_PHASE && !(reset & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		reset |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	lua_pushinteger(L, self->replace_effect(code, reset, count));
	return 1;
}
LUA_FUNCTION(EnableUnsummonable) {
	check_param_count(L, 1);
	if(!self->is_status(STATUS_COPYING_EFFECT)) {
		effect* peffect = pduel->new_effect();
		peffect->owner = self;
		peffect->code = EFFECT_UNSUMMONABLE_CARD;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_UNCOPYABLE;
		self->add_effect(peffect);
	}
	return 0;
}
LUA_FUNCTION(EnableReviveLimit) {
	check_param_count(L, 1);
	if(!self->is_status(STATUS_COPYING_EFFECT)) {
		effect* peffect1 = pduel->new_effect();
		peffect1->owner = self;
		peffect1->code = EFFECT_UNSUMMONABLE_CARD;
		peffect1->type = EFFECT_TYPE_SINGLE;
		peffect1->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_UNCOPYABLE;
		self->add_effect(peffect1);
		effect* peffect2 = pduel->new_effect();
		peffect2->owner = self;
		peffect2->code = EFFECT_REVIVE_LIMIT;
		peffect2->type = EFFECT_TYPE_SINGLE;
		peffect2->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_UNCOPYABLE;
		self->add_effect(peffect2);
	}
	return 0;
}
LUA_FUNCTION(CompleteProcedure) {
	check_param_count(L, 1);
	self->set_status(STATUS_PROC_COMPLETE, TRUE);
	return 0;
}
LUA_FUNCTION(IsDisabled) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_status(STATUS_DISABLED));
	return 1;
}
LUA_FUNCTION(IsDestructable) {
	check_param_count(L, 1);
	effect* peffect = 0;
	if(lua_gettop(L) > 1)
		peffect = lua_get<effect*, true>(L, 2);
	if(peffect)
		lua_pushboolean(L, self->is_destructable_by_effect(peffect, pduel->game_field->core.reason_player));
	else
		lua_pushboolean(L, self->is_destructable());
	return 1;
}
LUA_FUNCTION(IsSummonableCard) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_summonable_card());
	return 1;
}
LUA_FUNCTION(IsSpecialSummonable) {
	check_param_count(L, 1);
	auto sumtype = lua_get<uint32_t, 0>(L, 2);
	lua_pushboolean(L, self->is_special_summonable(pduel->game_field->core.reason_player, sumtype));
	return 1;
}
LUA_FUNCTION(IsFusionSummonableCard) {
	check_param_count(L, 1);
	auto summon_type = lua_get<uint32_t, 0>(L, 2);
	lua_pushboolean(L, self->is_fusion_summonable_card(summon_type));
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
LUA_STATIC_FUNCTION(IsSynchroSummonable) {
	return spsummonable_rule(L, TYPE_SYNCHRO, SUMMON_TYPE_SYNCHRO, 0);
}
LUA_STATIC_FUNCTION(IsXyzSummonable) {
	return spsummonable_rule(L, TYPE_XYZ, SUMMON_TYPE_XYZ, 0);
}
LUA_STATIC_FUNCTION(IsLinkSummonable) {
	return spsummonable_rule(L, TYPE_LINK, SUMMON_TYPE_LINK, 0);
}
LUA_STATIC_FUNCTION(IsProcedureSummonable) {
	check_param_count(L, 3);
	auto cardtype = lua_get<uint32_t, true>(L, 2);
	auto sumtype = lua_get<uint32_t, true>(L, 3);
	return spsummonable_rule(L, cardtype, sumtype, 2);
}
LUA_FUNCTION(IsSummonable) {
	check_param_count(L, 2);
	auto ign = lua_get<bool>(L, 2);
	effect* peffect = 0;
	if(!lua_isnoneornil(L, 3))
		peffect = lua_get<effect*, true>(L, 3);
	auto minc = lua_get<uint16_t, 0>(L, 4);
	auto zone = lua_get<uint16_t, 0x1f>(L, 5);
	lua_pushboolean(L, self->is_can_be_summoned(pduel->game_field->core.reason_player, ign, peffect, minc, zone));
	return 1;
}
LUA_FUNCTION(IsMSetable) {
	check_param_count(L, 2);
	bool ign = lua_get<bool>(L, 2);
	effect* peffect = 0;
	if(!lua_isnoneornil(L, 3))
		peffect = lua_get<effect*, true>(L, 3);
	auto minc = lua_get<uint16_t, 0>(L, 4);
	auto zone = lua_get<uint32_t, 0xff>(L, 5);
	lua_pushboolean(L, self->is_setable_mzone(pduel->game_field->core.reason_player, ign, peffect, minc, zone));
	return 1;
}
LUA_FUNCTION(IsSSetable) {
	check_param_count(L, 1);
	bool ign = lua_get<bool, false>(L, 2);
	lua_pushboolean(L, self->is_setable_szone(pduel->game_field->core.reason_player, ign));
	return 1;
}
LUA_FUNCTION(IsCanBeSpecialSummoned) {
	check_param_count(L, 6);
	auto peffect = lua_get<effect*, true>(L, 2);
	auto sumtype = lua_get<uint32_t>(L, 3);
	auto sumplayer = lua_get<uint8_t>(L, 4);
	auto nocheck = lua_get<bool>(L, 5);
	auto nolimit = lua_get<bool>(L, 6);
	auto sumpos = lua_get<uint8_t, POS_FACEUP>(L, 7);
	auto toplayer = lua_get<uint8_t>(L, 8, sumplayer);
	auto zone = lua_get<uint32_t, 0xff>(L, 9);
	lua_pushboolean(L, self->is_can_be_special_summoned(peffect, sumtype, sumpos, sumplayer, toplayer, nocheck, nolimit, zone));
	return 1;
}
LUA_FUNCTION(IsAbleToHand) {
	check_param_count(L, 1);
	auto p = lua_get<uint8_t>(L, 2, pduel->game_field->core.reason_player);
	lua_pushboolean(L, self->is_capable_send_to_hand(p));
	return 1;
}
LUA_FUNCTION(IsAbleToDeck) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_capable_send_to_deck(pduel->game_field->core.reason_player));
	return 1;
}
LUA_FUNCTION(IsAbleToExtra) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_capable_send_to_extra(pduel->game_field->core.reason_player));
	return 1;
}
LUA_FUNCTION(IsAbleToGrave) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_capable_send_to_grave(pduel->game_field->core.reason_player));
	return 1;
}
LUA_FUNCTION(IsAbleToRemove) {
	check_param_count(L, 1);
	auto p = lua_get<uint8_t>(L, 2, pduel->game_field->core.reason_player);
	auto pos = lua_get<uint8_t, POS_FACEUP>(L, 3);
	auto reason = lua_get<uint32_t, REASON_EFFECT>(L, 4);
	lua_pushboolean(L, self->is_removeable(p, pos, reason));
	return 1;
}
LUA_FUNCTION(IsAbleToHandAsCost) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_capable_cost_to_hand(pduel->game_field->core.reason_player));
	return 1;
}
LUA_FUNCTION(IsAbleToDeckAsCost) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_capable_cost_to_deck(pduel->game_field->core.reason_player));
	return 1;
}
LUA_FUNCTION(IsAbleToExtraAsCost) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_capable_cost_to_extra(pduel->game_field->core.reason_player));
	return 1;
}
LUA_FUNCTION(IsAbleToDeckOrExtraAsCost) {
	check_param_count(L, 1);
	auto p = pduel->game_field->core.reason_player;
	lua_pushboolean(L, self->is_extra_deck_monster() ? self->is_capable_cost_to_extra(p) : self->is_capable_cost_to_deck(p));
	return 1;
}
LUA_FUNCTION(IsAbleToGraveAsCost) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_capable_cost_to_grave(pduel->game_field->core.reason_player));
	return 1;
}
LUA_FUNCTION(IsAbleToRemoveAsCost) {
	check_param_count(L, 1);
	auto pos = lua_get<uint8_t, POS_FACEUP>(L, 2);
	lua_pushboolean(L, self->is_removeable_as_cost(pduel->game_field->core.reason_player, pos));
	return 1;
}
LUA_FUNCTION(IsReleasable) {
	check_param_count(L, 1);
	const auto reason = lua_get<uint32_t, REASON_COST>(L, 2);
	lua_pushboolean(L, self->is_releasable_by_nonsummon(pduel->game_field->core.reason_player, reason));
	return 1;
}
LUA_FUNCTION(IsReleasableByEffect) {
	check_param_count(L, 1);
	auto p = pduel->game_field->core.reason_player;
	effect* re = pduel->game_field->core.reason_effect;
	lua_pushboolean(L, self->is_releasable_by_nonsummon(p, REASON_EFFECT) && self->is_releasable_by_effect(p, re));
	return 1;
}
LUA_FUNCTION(IsDiscardable) {
	check_param_count(L, 1);
	auto p = pduel->game_field->core.reason_player;
	effect* pe = pduel->game_field->core.reason_effect;
	auto reason = lua_get<uint32_t, REASON_COST>(L, 2);
	if((reason != REASON_COST || !self->is_affected_by_effect(EFFECT_CANNOT_USE_AS_COST))
	        && pduel->game_field->is_player_can_discard_hand(p, self, pe, reason))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
LUA_FUNCTION(CanAttack) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_capable_attack());
	return 1;
}
LUA_FUNCTION(CanChainAttack) {
	check_param_count(L, 1);
	if(self != pduel->game_field->core.attacker) {
		lua_pushboolean(L, 0);
		return 1;
	}
	auto ac = lua_get<uint8_t, 2>(L, 2);
	bool monsteronly = lua_get<bool, false>(L, 3);
	if(self->is_status(STATUS_BATTLE_DESTROYED)
			|| self->current.controler != pduel->game_field->infos.turn_player
			|| self->fieldid_r != pduel->game_field->core.pre_field[0]
			|| !self->is_capable_attack_announce(pduel->game_field->infos.turn_player)
			|| (ac != 0 && self->announce_count >= ac)
			|| (ac == 2 && self->is_affected_by_effect(EFFECT_EXTRA_ATTACK))) {
		lua_pushboolean(L, 0);
		return 1;
	}
	pduel->game_field->core.select_cards.clear();
	pduel->game_field->get_attack_target(self, &pduel->game_field->core.select_cards, TRUE);
	if(pduel->game_field->core.select_cards.empty() && (monsteronly || !self->direct_attackable))
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, 1);
	return 1;
}
LUA_FUNCTION(IsFaceup) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_position(POS_FACEUP));
	return 1;
}
LUA_FUNCTION(IsAttackPos) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_position(POS_ATTACK));
	return 1;
}
LUA_FUNCTION(IsFacedown) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_position(POS_FACEDOWN));
	return 1;
}
LUA_FUNCTION(IsDefensePos) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_position(POS_DEFENSE));
	return 1;
}
LUA_FUNCTION(IsPosition) {
	check_param_count(L, 2);
	auto pos = lua_get<uint8_t>(L, 2);
	lua_pushboolean(L, self->is_position(pos));
	return 1;
}
LUA_FUNCTION(IsPreviousPosition) {
	check_param_count(L, 2);
	auto pos = lua_get<uint8_t>(L, 2);
	lua_pushboolean(L, (self->previous.position & pos) != 0);
	return 1;
}
LUA_FUNCTION(IsControler) {
	check_param_count(L, 2);
	auto con = lua_get<uint8_t>(L, 2);
	lua_pushboolean(L, self->current.controler == con);
	return 1;
}
LUA_FUNCTION(IsPreviousControler) {
	check_param_count(L, 2);
	auto con = lua_get<uint8_t>(L, 2);
	lua_pushboolean(L, self->previous.controler == con);
	return 1;
}
LUA_FUNCTION(IsOnField) {
	check_param_count(L, 1);
	if((self->current.location & LOCATION_ONFIELD)
			&& !self->get_status(STATUS_SUMMONING | STATUS_SUMMON_DISABLED | STATUS_ACTIVATE_DISABLED | STATUS_SPSUMMON_STEP))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
LUA_FUNCTION(IsLocation) {
	check_param_count(L, 2);
	auto loc = lua_get<uint16_t>(L, 2);
	if(self->current.location == LOCATION_MZONE) {
		if(self->current.is_location(loc) && !self->get_status(STATUS_SUMMONING | STATUS_SUMMON_DISABLED | STATUS_SPSUMMON_STEP))
			lua_pushboolean(L, 1);
		else
			lua_pushboolean(L, 0);
	} else if(self->current.location == LOCATION_SZONE) {
		if(self->current.is_location(loc) && !self->is_status(STATUS_ACTIVATE_DISABLED))
			lua_pushboolean(L, 1);
		else
			lua_pushboolean(L, 0);
	} else
		lua_pushboolean(L, (self->current.location & loc) != 0);
	return 1;
}
LUA_FUNCTION(IsPreviousLocation) {
	check_param_count(L, 2);
	auto loc = lua_get<uint16_t>(L, 2);
	lua_pushboolean(L, self->previous.is_location(loc));
	return 1;
}
LUA_FUNCTION(IsLevelBelow) {
	check_param_count(L, 2);
	auto lvl = lua_get<uint32_t>(L, 2);
	uint32_t plvl = self->get_level();
	lua_pushboolean(L, plvl > 0 && plvl <= lvl);
	return 1;
}
LUA_FUNCTION(IsLevelAbove) {
	check_param_count(L, 2);
	auto lvl = lua_get<uint32_t>(L, 2);
	uint32_t plvl = self->get_level();
	lua_pushboolean(L, plvl > 0 && plvl >= lvl);
	return 1;
}
LUA_FUNCTION(IsRankBelow) {
	check_param_count(L, 2);
	auto rnk = lua_get<uint32_t>(L, 2);
	uint32_t prnk = self->get_rank();
	lua_pushboolean(L, prnk > 0 && prnk <= rnk);
	return 1;
}
LUA_FUNCTION(IsRankAbove) {
	check_param_count(L, 2);
	auto rnk = lua_get<uint32_t>(L, 2);
	uint32_t prnk = self->get_rank();
	lua_pushboolean(L, prnk > 0 && prnk >= rnk);
	return 1;
}
LUA_FUNCTION(IsLinkBelow) {
	check_param_count(L, 2);
	auto lnk = lua_get<uint32_t>(L, 2);
	uint32_t plnk = self->get_link();
	lua_pushboolean(L, plnk > 0 && plnk <= lnk);
	return 1;
}
LUA_FUNCTION(IsLinkAbove) {
	check_param_count(L, 2);
	auto lnk = lua_get<uint32_t>(L, 2);
	uint32_t plnk = self->get_link();
	lua_pushboolean(L, plnk > 0 && plnk >= lnk);
	return 1;
}
LUA_FUNCTION(IsAttackBelow) {
	check_param_count(L, 2);
	auto atk = lua_get<int32_t>(L, 2);
	if(!(self->data.type & TYPE_MONSTER) && !(self->get_type() & TYPE_MONSTER) && !self->is_affected_by_effect(EFFECT_PRE_MONSTER))
		lua_pushboolean(L, 0);
	else {
		int32_t _atk = self->get_attack();
		lua_pushboolean(L, _atk >= 0 && _atk <= atk);
	}
	return 1;
}
LUA_FUNCTION(IsAttackAbove) {
	check_param_count(L, 2);
	auto atk = lua_get<int32_t>(L, 2);
	if(!(self->data.type & TYPE_MONSTER) && !(self->get_type() & TYPE_MONSTER) && !self->is_affected_by_effect(EFFECT_PRE_MONSTER))
		lua_pushboolean(L, 0);
	else {
		lua_pushboolean(L, self->get_attack() >= atk);
	}
	return 1;
}
LUA_FUNCTION(IsDefenseBelow) {
	check_param_count(L, 2);
	auto def = lua_get<int32_t>(L, 2);
	if((self->data.type & TYPE_LINK)
	   || (!(self->data.type & TYPE_MONSTER) && !(self->get_type() & TYPE_MONSTER) && !(self->current.location & LOCATION_MZONE)))
		lua_pushboolean(L, 0);
	else {
		int32_t _def = self->get_defense();
		lua_pushboolean(L, _def >= 0 && _def <= def);
	}
	return 1;
}
LUA_FUNCTION(IsDefenseAbove) {
	check_param_count(L, 2);
	auto def = lua_get<int32_t>(L, 2);
	if((self->data.type & TYPE_LINK)
	   || (!(self->data.type & TYPE_MONSTER) && !(self->get_type() & TYPE_MONSTER) && !self->is_affected_by_effect(EFFECT_PRE_MONSTER)))
		lua_pushboolean(L, 0);
	else {
		lua_pushboolean(L, self->get_defense() >= def);
	}
	return 1;
}
LUA_FUNCTION(IsPublic) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_position(POS_FACEUP));
	return 1;
}
LUA_FUNCTION(IsForbidden) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_status(STATUS_FORBIDDEN));
	return 1;
}
LUA_FUNCTION(IsAbleToChangeControler) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_capable_change_control());
	return 1;
}
LUA_FUNCTION(IsControlerCanBeChanged) {
	check_param_count(L, 1);
	bool ign = lua_get<bool, false>(L, 2);
	auto zone = lua_get<uint32_t, 0xff>(L, 3);
	lua_pushboolean(L, self->is_control_can_be_changed(ign, zone));
	return 1;
}
LUA_FUNCTION(AddCounter) {
	check_param_count(L, 3);
	auto countertype = lua_get<uint16_t>(L, 2);
	auto count = lua_get<uint16_t>(L, 3);
	bool singly = lua_get<bool, false>(L, 4);
	if(self->is_affect_by_effect(pduel->game_field->core.reason_effect))
		lua_pushboolean(L, self->add_counter(pduel->game_field->core.reason_player, countertype, count, singly));
	else
		lua_pushboolean(L, 0);
	return 1;
}
LUA_FUNCTION(RemoveCounter) {
	check_action_permission(L);
	check_param_count(L, 5);
	auto countertype = lua_get<uint16_t>(L, 3);
	if(countertype == 0)
		lua_error(L, "Counter type cannot be 0, use Card.RemoveAllCounters instead");
	auto rplayer = lua_get<uint8_t>(L, 2);
	auto count = lua_get<uint16_t>(L, 4);
	auto reason = lua_get<uint32_t>(L, 5);
	pduel->game_field->remove_counter(reason, self, rplayer, 0, 0, countertype, count);
	return yieldk({
		lua_pushboolean(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(RemoveAllCounters) {
	check_action_permission(L);
	check_param_count(L, 1);
	uint32_t total = 0;
	for(const auto& cmit : self->counters) {
		auto message = pduel->new_message(MSG_REMOVE_COUNTER);
		message->write<uint16_t>(cmit.first);
		message->write<uint8_t>(self->current.controler);
		message->write<uint8_t>(self->current.location);
		message->write<uint8_t>(self->current.sequence);
		message->write<uint16_t>(cmit.second[0] + cmit.second[1]);
		total += cmit.second[0] + cmit.second[1];
	}
	self->counters.clear();
	lua_pushinteger(L, total);
	return 1;
}
LUA_FUNCTION(GetCounter) {
	check_param_count(L, 2);
	auto countertype = lua_get<uint16_t>(L, 2);
	if(countertype == 0)
		lua_error(L, "Counter type cannot be 0, use Card.GetAllCounters instead");
	lua_pushinteger(L, self->get_counter(countertype));
	return 1;
}
LUA_FUNCTION(GetAllCounters) {
	check_param_count(L, 1);
	lua_createtable(L, self->counters.size(), 0);
	for(const auto& counter : self->counters) {
		lua_pushinteger(L, counter.first);
		lua_pushinteger(L, counter.second[0] + counter.second[1]);
		lua_settable(L, -3);
	}
	return 1;
}
LUA_FUNCTION(HasCounters) {
	check_param_count(L, 1);
	lua_pushboolean(L, !self->counters.empty());
	return 1;
}
LUA_FUNCTION(EnableCounterPermit) {
	check_param_count(L, 2);
	auto countertype = lua_get<uint16_t>(L, 2);
	uint16_t prange;
	if(lua_gettop(L) > 2)
		prange = lua_get<uint16_t>(L, 3);
	else if(self->data.type & TYPE_MONSTER)
		prange = LOCATION_MZONE;
	else
		prange = LOCATION_SZONE | LOCATION_FZONE;
	effect* peffect = pduel->new_effect();
	peffect->owner = self;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = EFFECT_COUNTER_PERMIT | countertype;
	peffect->value = prange;
	if(lua_gettop(L) > 3 && lua_isfunction(L, 4))
		peffect->target = interpreter::get_function_handle(L, 4);
	self->add_effect(peffect);
	return 0;
}
LUA_FUNCTION(SetCounterLimit) {
	check_param_count(L, 3);
	auto countertype = lua_get<uint16_t>(L, 2);
	auto limit = lua_get<uint32_t>(L, 3);
	effect* peffect = pduel->new_effect();
	peffect->owner = self;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = EFFECT_COUNTER_LIMIT | countertype;
	peffect->value = limit;
	self->add_effect(peffect);
	return 0;
}
LUA_FUNCTION(IsCanChangePosition) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_capable_change_position_by_effect(pduel->game_field->core.reason_player));
	return 1;
}
LUA_FUNCTION(IsCanTurnSet) {
	check_param_count(L, 1);
	lua_pushboolean(L, self->is_capable_turn_set(pduel->game_field->core.reason_player));
	return 1;
}
LUA_FUNCTION(IsCanAddCounter) {
	check_param_count(L, 2);
	auto countertype = lua_get<uint16_t>(L, 2);
	auto count = lua_get<uint16_t, 0>(L, 3);
	bool singly = lua_get<bool, false>(L, 4);
	auto loc = lua_get<uint16_t, 0>(L, 5);
	lua_pushboolean(L, self->is_can_add_counter(pduel->game_field->core.reason_player, countertype, count, singly, loc));
	return 1;
}
LUA_FUNCTION(IsCanRemoveCounter) {
	check_param_count(L, 5);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto countertype = lua_get<uint16_t>(L, 3);
	auto count = lua_get<uint16_t>(L, 4);
	auto reason = lua_get<uint32_t>(L, 5);
	lua_pushboolean(L, pduel->game_field->is_player_can_remove_counter(playerid, self, 0, 0, countertype, count, reason));
	return 1;
}
LUA_FUNCTION(IsCanBeFusionMaterial) {
	check_param_count(L, 1);
	card* fcard = nullptr;
	if(lua_gettop(L) >= 2 && !lua_isnoneornil(L, 2))
		fcard = lua_get<card*, true>(L, 2);
	auto summon_type = lua_get<uint64_t, SUMMON_TYPE_FUSION>(L, 3);
	auto playerid = lua_get<uint8_t>(L, 4, pduel->game_field->core.reason_player);
	lua_pushboolean(L, self->is_can_be_fusion_material(fcard, summon_type, playerid));
	return 1;
}
LUA_FUNCTION(IsCanBeSynchroMaterial) {
	check_param_count(L, 1);
	card* scard = nullptr;
	card* tuner = nullptr;
	if(lua_gettop(L) >= 2)
		scard = lua_get<card*, true>(L, 2);
	if(lua_gettop(L) >= 3 && !lua_isnoneornil(L, 3))
		tuner = lua_get<card*, true>(L, 3);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 4);
	lua_pushboolean(L, self->is_can_be_synchro_material(scard, playerid, tuner));
	return 1;
}
LUA_FUNCTION(IsCanBeRitualMaterial) {
	check_param_count(L, 1);
	card* scard = nullptr;
	if(lua_gettop(L) >= 2 && !lua_isnoneornil(L, 2))
		scard = lua_get<card*, true>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 3, pduel->game_field->core.reason_player);
	lua_pushboolean(L, self->is_can_be_ritual_material(scard, playerid));
	return 1;
}
LUA_FUNCTION(IsCanBeXyzMaterial) {
	check_param_count(L, 1);
	card* scard = nullptr;
	if(!lua_isnoneornil(L, 2))
		scard = lua_get<card*, true>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 3, pduel->game_field->core.reason_player);
	auto reason = lua_get<uint32_t, REASON_XYZ | REASON_MATERIAL>(L, 4);
	lua_pushboolean(L, self->is_can_be_xyz_material(scard, playerid, reason));
	return 1;
}
LUA_FUNCTION(IsCanBeLinkMaterial) {
	check_param_count(L, 1);
	card* scard = 0;
	if(lua_gettop(L) >= 2)
		scard = lua_get<card*, true>(L, 2);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 3);
	lua_pushboolean(L, self->is_can_be_link_material(scard, playerid));
	return 1;
}
LUA_FUNCTION(IsCanBeMaterial) {
	check_param_count(L, 2);
	uint64_t sumtype = lua_get<uint64_t>(L, 2);
	card* scard = nullptr;
	if(lua_gettop(L) >= 3)
		scard = lua_get<card*, true>(L, 3);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 4);
	lua_pushboolean(L, self->is_can_be_material(scard, sumtype, playerid));
	return 1;
}
LUA_FUNCTION(CheckFusionMaterial) {
	check_param_count(L, 1);
	group* pgroup = nullptr;
	if(lua_gettop(L) > 1 && !lua_isnoneornil(L, 2))
		pgroup = lua_get<group*, true>(L, 2);
	group* cg = nullptr;
	if(auto _pcard = lua_get<card*>(L, 3))
		cg = pduel->new_group(_pcard);
	else
		cg = lua_get<group*>(L, 3);
	auto chkf = lua_get<uint32_t, PLAYER_NONE>(L, 4);
	lua_pushboolean(L, self->fusion_check(pgroup, cg, chkf));
	return 1;
}
LUA_FUNCTION(CheckFusionSubstitute) {
	check_param_count(L, 2);
	auto fcard = lua_get<card*, true>(L, 2);
	lua_pushboolean(L, self->check_fusion_substitute(fcard));
	return 1;
}
LUA_FUNCTION(IsImmuneToEffect) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 2);
	lua_pushboolean(L, !self->is_affect_by_effect(peffect));
	return 1;
}
LUA_FUNCTION(IsCanBeDisabledByEffect) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 2);
	bool is_monster_effect = lua_get<bool, true>(L, 3);
	lua_pushboolean(L, self->is_can_be_disabled_by_effect(peffect, is_monster_effect));
	return 1;
}
LUA_FUNCTION(IsCanBeEffectTarget) {
	check_param_count(L, 1);
	effect* peffect = pduel->game_field->core.reason_effect;
	if(lua_gettop(L) > 1)
		peffect = lua_get<effect*, true>(L, 2);
	lua_pushboolean(L, self->is_capable_be_effect_target(peffect, pduel->game_field->core.reason_player));
	return 1;
}
LUA_FUNCTION(IsCanBeBattleTarget) {
	check_param_count(L, 2);
	auto bcard = lua_get<card*, true>(L, 2);
	lua_pushboolean(L, self->is_capable_be_battle_target(bcard));
	return 1;
}
LUA_FUNCTION(AddMonsterAttribute) {
	check_param_count(L, 2);
	auto type = lua_get<uint32_t>(L, 2);
	auto attribute = lua_get<uint32_t, 0>(L, 3);
	auto race = lua_get<uint64_t, 0>(L, 4);
	auto level = lua_get<uint32_t, 0>(L, 5);
	auto atk = lua_get<int32_t, 0>(L, 6);
	auto def = lua_get<int32_t, 0>(L, 7);
	self->set_status(STATUS_NO_LEVEL, FALSE);
	constexpr auto resets = RESET_TURN_SET | RESET_TOGRAVE | RESET_REMOVE | RESET_TEMP_REMOVE | RESET_TOHAND | RESET_TODECK | RESET_OVERLAY;
	// pre-monster
	{
		effect* peffect = pduel->new_effect();
		peffect->owner = self;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_PRE_MONSTER;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = RESET_CHAIN | RESET_EVENT | resets;
		peffect->value = type;
		self->add_effect(peffect);
	}

	auto register_property_effect = [&](auto value, auto effect_code) {
		auto peffect = pduel->new_effect();
		peffect->owner = self;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = effect_code;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = RESET_EVENT | resets;
		peffect->value = value;
		self->add_effect(peffect);
	};

	if(attribute)
		register_property_effect(attribute, EFFECT_ADD_ATTRIBUTE);

	if(race)
		register_property_effect(race, EFFECT_ADD_RACE);

	if(level)
		register_property_effect(level, EFFECT_CHANGE_LEVEL);

	if(atk)
		register_property_effect(atk, EFFECT_SET_BASE_ATTACK);

	if(def)
		register_property_effect(def, EFFECT_SET_BASE_DEFENSE);
	return 0;
}
LUA_FUNCTION(AddMonsterAttributeComplete) {
	return 0;
}
LUA_FUNCTION(CancelToGrave) {
	check_param_count(L, 1);
	bool cancel = lua_get<bool, true>(L, 2);
	if(cancel)
		self->set_status(STATUS_LEAVE_CONFIRMED, FALSE);
	else {
		pduel->game_field->core.leave_confirmed.insert(self);
		self->set_status(STATUS_LEAVE_CONFIRMED, TRUE);
	}
	return 0;
}
LUA_FUNCTION(GetTributeRequirement) {
	check_param_count(L, 1);
	int32_t rcount = self->get_summon_tribute_count();
	lua_pushinteger(L, rcount & 0xffff);
	lua_pushinteger(L, (rcount >> 16) & 0xffff);
	return 2;
}
LUA_FUNCTION(GetBattleTarget) {
	check_param_count(L, 1);
	if(pduel->game_field->core.attacker == self)
		interpreter::pushobject(L, pduel->game_field->core.attack_target);
	else if(pduel->game_field->core.attack_target == self)
		interpreter::pushobject(L, pduel->game_field->core.attacker);
	else lua_pushnil(L);
	return 1;
}
LUA_FUNCTION(GetAttackableTarget) {
	check_param_count(L, 1);
	card_vector targets;
	bool chain_attack = pduel->game_field->core.chain_attacker_id == self->fieldid;
	pduel->game_field->get_attack_target(self, &targets, chain_attack);
	group* newgroup = pduel->new_group(targets);
	interpreter::pushobject(L, newgroup);
	lua_pushboolean(L, self->direct_attackable);
	return 2;
}
LUA_FUNCTION(SetHint) {
	check_param_count(L, 3);
	auto type = lua_get<uint8_t>(L, 2);
	auto value = lua_get<uint64_t>(L, 3);
	if(type >= CHINT_DESC_ADD)
		return 0;
	auto message = pduel->new_message(MSG_CARD_HINT);
	message->write(self->get_info_location());
	message->write<uint8_t>(type);
	message->write<uint64_t>(value);
	return 0;
}
LUA_FUNCTION(ReverseInDeck) {
	check_param_count(L, 1);
	if(self->current.location != LOCATION_DECK)
		return 0;
	self->current.position = POS_FACEUP_DEFENSE;
	if(self->current.sequence == pduel->game_field->player[self->current.controler].list_main.size() - 1) {
		auto message = pduel->new_message(MSG_DECK_TOP);
		message->write<uint8_t>(self->current.controler);
		message->write<uint32_t>(0);
		message->write<uint32_t>(self->data.code);
		message->write<uint32_t>(self->current.position);
	}
	return 0;
}
LUA_FUNCTION(SetUniqueOnField) {
	check_param_count(L, 4);
	self->unique_pos[0] = lua_get<uint8_t>(L, 2);
	self->unique_pos[1] = lua_get<uint8_t>(L, 3);
	if(lua_isfunction(L, 4)) {
		self->unique_code = 1;
		self->unique_function = interpreter::get_function_handle(L, 4);
	} else
		self->unique_code = lua_get<uint32_t>(L, 4);
	auto location = lua_get<uint8_t, LOCATION_ONFIELD>(L, 5) & LOCATION_ONFIELD;
	self->unique_location = location;
	effect* peffect = pduel->new_effect();
	peffect->owner = self;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = EFFECT_UNIQUE_CHECK;
	peffect->flag[0] = EFFECT_FLAG_COPY_INHERIT;
	self->add_effect(peffect);
	self->unique_effect = peffect;
	if(self->current.location & location)
		pduel->game_field->add_unique_card(self);
	return 0;
}
LUA_FUNCTION(CheckUniqueOnField) {
	check_param_count(L, 2);
	auto check_player = lua_get<uint8_t>(L, 2);
	auto check_location = lua_get<uint8_t, LOCATION_ONFIELD>(L, 3) & LOCATION_ONFIELD;
	auto icard = lua_get<card*>(L, 4);
	lua_pushboolean(L, pduel->game_field->check_unique_onfield(self, check_player, check_location, icard) ? 0 : 1);
	return 1;
}
LUA_FUNCTION(ResetNegateEffect) {
	check_param_count(L, 1);
	lua_iterate_table_or_stack(L, 2, lua_gettop(L), [L, self] {
		self->reset(lua_get<uint32_t>(L, -1), RESET_CARD);
	});
	return 0;
}
LUA_FUNCTION(AssumeProperty) {
	check_param_count(L, 3);
	auto assume = lua_get<uint32_t>(L, 2);
	if ((assume < ASSUME_CODE) || (assume > ASSUME_LINKMARKER))
		return 0;
	self->assume[assume] = lua_get<uint64_t>(L, 3);
	pduel->assumes.insert(self);
	return 0;
}
LUA_FUNCTION(SetSPSummonOnce) {
	check_param_count(L, 2);
	if(self->status & STATUS_COPYING_EFFECT)
		return 0;
	self->spsummon_code = lua_get<uint32_t>(L, 2);
	pduel->game_field->core.global_flag |= GLOBALFLAG_SPSUMMON_ONCE;
	return 0;
}
#define CARD_INFO_FUNC(lua_name,attr) \
LUA_FUNCTION(lua_name) { \
	check_param_count(L, 1);\
	if(lua_gettop(L) > 1) { \
		self->data.attr = lua_get<decltype(self->data.attr)>(L, 2); \
		return 0; \
	} else \
		lua_pushinteger(L, self->data.attr); \
	return 1;\
}
CARD_INFO_FUNC(Code, code)
CARD_INFO_FUNC(Alias, alias)

LUA_FUNCTION(Setcode) {
	check_param_count(L, 1);
	if(lua_gettop(L) > 1) {
		self->data.setcodes.clear();
		if(lua_istable(L, 2)) {
			lua_table_iterate(L, 2, [&setcodes = self->data.setcodes, &L] {
				setcodes.insert(lua_get<uint16_t>(L, -1));
			});
		} else
			self->data.setcodes.insert(lua_get<uint16_t>(L, 2));
		return 0;
	} else {
		luaL_checkstack(L, static_cast<int>(self->data.setcodes.size()), nullptr);
		for(auto& setcode : self->data.setcodes)
			lua_pushinteger(L, setcode);
	}
	return static_cast<int32_t>(self->data.setcodes.size());
}

CARD_INFO_FUNC(Type, type)
CARD_INFO_FUNC(Level, level)
CARD_INFO_FUNC(Attribute, attribute)
CARD_INFO_FUNC(Race, race)
CARD_INFO_FUNC(Attack, attack)
CARD_INFO_FUNC(Defense, defense)
CARD_INFO_FUNC(Rscale, rscale)
CARD_INFO_FUNC(Lscale, lscale)
CARD_INFO_FUNC(LinkMarker, link_marker)
#undef CARD_INFO_FUNC
LUA_FUNCTION(Recreate) {
	check_param_count(L, 2);
	auto code = lua_get<uint32_t>(L, 2);
	if (self->recreate(code)) {
		self->data.alias = lua_get<uint32_t>(L, 3, self->data.alias);
		if(lua_gettop(L) > 3 && !lua_isnoneornil(L, 4)) {
			if(lua_istable(L, 4)) {
				lua_table_iterate(L, 4, [&setcodes = self->data.setcodes, &L] {
					setcodes.insert(lua_get<uint16_t>(L, -1));
				});
			} else
				self->data.setcodes.insert(lua_get<uint16_t>(L, 4));
		}
		self->data.type = lua_get<uint32_t>(L, 5, self->data.type);
		self->data.level = lua_get<uint32_t>(L, 6, self->data.level);
		self->data.attribute = lua_get<uint32_t>(L, 7, self->data.attribute);
		self->data.race = lua_get<uint64_t>(L, 8, self->data.race);
		self->data.attack = lua_get<int32_t>(L, 9, self->data.attack);
		self->data.defense = lua_get<int32_t>(L, 10, self->data.defense);
		self->data.lscale = lua_get<uint32_t>(L, 11, self->data.lscale);
		self->data.rscale = lua_get<uint32_t>(L, 12, self->data.rscale);
		self->data.link_marker = lua_get<uint32_t>(L, 13, self->data.link_marker);
		if (lua_get<bool, false>(L, 14))
			self->replace_effect(code, 0, 0, true);
	}
	return 0;
}
LUA_FUNCTION(Cover) {
	check_param_count(L, 1);
	if(lua_gettop(L) > 1) {
		self->cover = lua_get<uint32_t>(L, 2);
		return 0;
	} else
		lua_pushinteger(L, self->cover);
	return 1;
}
LUA_FUNCTION_EXISTING(GetLuaRef, get_lua_ref<card>);
LUA_FUNCTION_EXISTING(FromLuaRef, from_lua_ref<card>);
LUA_FUNCTION_EXISTING(IsDeleted, is_deleted_object);
}

void scriptlib::push_card_lib(lua_State* L) {
	static constexpr auto cardlib = GET_LUA_FUNCTIONS_ARRAY();
	static_assert(cardlib.back().name == nullptr);
	lua_createtable(L, 0, static_cast<int>(cardlib.size() - 1));
	luaL_setfuncs(L, cardlib.data(), 0);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_setglobal(L, "Card");
}
