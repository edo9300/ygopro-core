/*
 * libduel.cpp
 *
 *  Created on: 2010-5-6
 *      Author: Argon
 */

#include <algorithm>
#include "scriptlib.h"
#include "duel.h"
#include "field.h"
#include "card.h"
#include "effect.h"
#include "group.h"

namespace {

using namespace scriptlib;

int32_t duel_enable_global_flag(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.global_flag |= lua_get<uint32_t>(L, 1);
	return 0;
}

int32_t duel_get_lp(lua_State* L) {
	check_param_count(L, 1);
	auto p = lua_get<int8_t>(L, 1);
	if(p != 0 && p != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->player[p].lp);
	return 1;
}
int32_t duel_set_lp(lua_State* L) {
	check_param_count(L, 2);
	auto p = lua_get<int8_t>(L, 1);
	auto lp = lua_get<int32_t>(L, 2);
	if(lp < 0) lp = 0;
	if(p != 0 && p != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->player[p].lp = lp;
	auto message = pduel->new_message(MSG_LPUPDATE);
	message->write<uint8_t>(p);
	message->write<uint32_t>(lp);
	return 0;
}
int32_t duel_get_turn_player(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->infos.turn_player);
	return 1;
}
int32_t duel_get_turn_count(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	if(lua_gettop(L) > 0) {
		auto playerid = lua_get<uint8_t>(L, 1);
		if(playerid != 0 && playerid != 1)
			return 0;
		lua_pushinteger(L, pduel->game_field->infos.turn_id_by_player[playerid]);
	} else
		lua_pushinteger(L, pduel->game_field->infos.turn_id);
	return 1;
}
int32_t duel_get_draw_count(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	lua_pushinteger(L, pduel->game_field->get_draw_count(playerid));
	return 1;
}
int32_t duel_register_effect(lua_State* L) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_effect(peffect, playerid);
	return 0;
}
int32_t duel_register_flag_effect(lua_State* L) {
	check_param_count(L, 5);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	auto reset = lua_get<uint32_t>(L, 3);
	auto flag = lua_get<uint32_t>(L, 4);
	auto count = lua_get<uint16_t>(L, 5);
	auto lab = lua_get<uint32_t, 0>(L, 6);
	if(count == 0)
		count = 1;
	if(reset & (RESET_PHASE) && !(reset & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		reset |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	const auto pduel = lua_get<duel*>(L);
	effect* peffect = pduel->new_effect();
	peffect->effect_owner = playerid;
	peffect->owner = pduel->game_field->temp_card;
	peffect->handler = 0;
	peffect->type = EFFECT_TYPE_FIELD;
	peffect->code = code;
	peffect->reset_flag = reset;
	peffect->flag[0] = flag | EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_PLAYER_TARGET | EFFECT_FLAG_FIELD_ONLY;
	peffect->s_range = 1;
	peffect->o_range = 0;
	peffect->reset_count = count;
	peffect->label.push_back(lab);
	pduel->game_field->add_effect(peffect, playerid);
	interpreter::pushobject(L, peffect);
	return 1;
}
int32_t duel_get_flag_effect(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	const auto pduel = lua_get<duel*>(L);
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, code, &eset);
	lua_pushinteger(L, eset.size());
	return 1;
}
int32_t duel_reset_flag_effect(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	const auto pduel = lua_get<duel*>(L);
	auto pr = pduel->game_field->effects.aura_effect.equal_range(code);
	for(; pr.first != pr.second; ) {
		auto rm = pr.first++;
		effect* peffect = rm->second;
		if(peffect->code == code && peffect->is_target_player(playerid))
			pduel->game_field->remove_effect(peffect);
	}
	return 0;
}
int32_t duel_set_flag_effect_label(lua_State* L) {
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	auto lab = lua_get<int32_t>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, code, &eset);
	if(!eset.size())
		lua_pushboolean(L, FALSE);
	else {
		eset[0]->label.clear();
		eset[0]->label.push_back(lab);
		lua_pushboolean(L, TRUE);
	}
	return 1;
}
int32_t duel_get_flag_effect_label(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	const auto pduel = lua_get<duel*>(L);
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, code, &eset);
	if(!eset.size()) {
		lua_pushnil(L);
		return 1;
	}
	for(auto& eff : eset)
		lua_pushinteger(L, eff->label.size() ? eff->label[0] : 0);
	return eset.size();
}
int32_t duel_destroy(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto reason = lua_get<uint32_t>(L, 2);
	auto dest = lua_get<uint16_t, LOCATION_GRAVE>(L, 3);
	if(pcard)
		pduel->game_field->destroy(pcard, pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, PLAYER_NONE, dest, 0);
	else
		pduel->game_field->destroy(&(pgroup->container), pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, PLAYER_NONE, dest, 0);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_remove(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto pos = lua_get<uint8_t, 0>(L, 2);
	auto reason = lua_get<uint32_t>(L, 3);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 4);
	if (pcard)
		pduel->game_field->send_to(pcard, pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, LOCATION_REMOVED, 0, pos);
	else
		pduel->game_field->send_to(&(pgroup->container), pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, LOCATION_REMOVED, 0, pos);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_sendto_grave(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto reason = lua_get<uint32_t>(L, 2);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 3);
	if (pcard)
		pduel->game_field->send_to(pcard, pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, LOCATION_GRAVE, 0, POS_FACEUP);
	else
		pduel->game_field->send_to(&(pgroup->container), pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, LOCATION_GRAVE, 0, POS_FACEUP);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_summon(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 4);
	const auto pduel = lua_get<duel*>(L);
	if(pduel->game_field->core.effect_damage_step)
		return 0;
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto pcard = lua_get<card*, true>(L, 2);
	bool ignore_count = lua_get<bool>(L, 3);
	auto peffect = lua_get<effect*>(L, 4);
	auto min_tribute = lua_get<uint8_t, 0>(L, 5);
	auto zone = lua_get<uint32_t, 0x1f>(L, 6);
	pduel->game_field->core.summon_cancelable = FALSE;
	pduel->game_field->summon(playerid, pcard, peffect, ignore_count, min_tribute, zone);
	if(pduel->game_field->core.current_chain.size()) {
		pduel->game_field->core.reserved = pduel->game_field->core.subunits.back();
		pduel->game_field->core.subunits.pop_back();
		pduel->game_field->core.summoning_card = pcard;
	}
	return lua_yield(L, 0);
}
int32_t duel_special_summon_rule(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	if(pduel->game_field->core.effect_damage_step)
		return 0;
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto pcard = lua_get<card*, true>(L, 2);
	auto sumtype = lua_get<uint32_t, 0>(L, 3);
	pduel->game_field->core.summon_cancelable = FALSE;
	pduel->game_field->special_summon_rule(playerid, pcard, sumtype);
	if(pduel->game_field->core.current_chain.size()) {
		pduel->game_field->core.reserved = pduel->game_field->core.subunits.back();
		pduel->game_field->core.subunits.pop_back();
		pduel->game_field->core.summoning_card = pcard;
	}
	return lua_yield(L, 0);
}
inline int32_t spsummon_rule(lua_State* L, uint32_t summon_type, uint32_t offset) {
	check_action_permission(L);
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	if(pduel->game_field->core.effect_damage_step)
		return 0;
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto pcard = lua_get<card*, true>(L, 2);
	group* must = nullptr;
	if(auto _pcard = lua_get<card*>(L, 3 + offset)) {
		must = pduel->new_group(_pcard);
		must->is_readonly = TRUE;
	} else if(auto pgroup = lua_get<group*>(L, 3 + offset)) {
		must = pduel->new_group(pgroup);
		must->is_readonly = TRUE;
	}
	group* materials = nullptr;
	if(auto _pcard = lua_get<card*>(L, 4 + offset)) {
		materials = pduel->new_group(_pcard);
		materials->is_readonly = TRUE;
	} else if(auto pgroup = lua_get<group*>(L, 4 + offset)) {
		materials = pduel->new_group(pgroup);
		materials->is_readonly = TRUE;
	}
	auto minc = lua_get<uint16_t, 0>(L, 5 + offset);
	auto maxc = lua_get<uint16_t, 0>(L, 6 + offset);
	pduel->game_field->core.must_use_mats = must;
	pduel->game_field->core.only_use_mats = materials;
	pduel->game_field->core.forced_summon_minc = minc;
	pduel->game_field->core.forced_summon_maxc = maxc;
	pduel->game_field->core.summon_cancelable = FALSE;
	pduel->game_field->special_summon_rule(playerid, pcard, summon_type);
	if(pduel->game_field->core.current_chain.size()) {
		pduel->game_field->core.reserved = pduel->game_field->core.subunits.back();
		pduel->game_field->core.subunits.pop_back();
		pduel->game_field->core.summoning_card = pcard;
	}
	return lua_yield(L, 0);
}
int32_t duel_synchro_summon(lua_State* L) {
	return spsummon_rule(L, SUMMON_TYPE_SYNCHRO, 0);
}
int32_t duel_xyz_summon(lua_State* L) {
	return spsummon_rule(L, SUMMON_TYPE_XYZ, 0);
}
int32_t duel_link_summon(lua_State* L) {
	return spsummon_rule(L, SUMMON_TYPE_LINK, 0);
}
int32_t duel_procedure_summon(lua_State* L) {
	check_param_count(L, 3);
	auto sumtype = lua_get<uint32_t>(L, 3);
	return spsummon_rule(L, sumtype, 1);
}
inline int32_t spsummon_rule_group(lua_State* L, uint32_t summon_type, uint32_t offset) {
	(void)offset;
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	const auto playerid = lua_get<uint8_t>(L, 1);
	pduel->game_field->core.summon_cancelable = FALSE;
	pduel->game_field->special_summon_rule_group(playerid, summon_type);
	if(pduel->game_field->core.current_chain.size()) {
		pduel->game_field->core.reserved = pduel->game_field->core.subunits.back();
		pduel->game_field->core.subunits.pop_back();
		pduel->game_field->core.summoning_proc_group_type = summon_type;
	}
	return lua_yield(L, 0);
}
int32_t duel_pendulum_summon(lua_State* L) {
	return spsummon_rule_group(L, SUMMON_TYPE_PENDULUM, 0);
}
int32_t duel_procedure_summon_group(lua_State* L) {
	check_param_count(L, 2);
	auto sumtype = lua_get<uint32_t>(L, 2);
	return spsummon_rule_group(L, sumtype, 1);
}
int32_t duel_setm(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 4);
	const auto pduel = lua_get<duel*>(L);
	if(pduel->game_field->core.effect_damage_step)
		return 0;
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto pcard = lua_get<card*, true>(L, 2);
	auto ignore_count = lua_get<bool>(L, 3);
	auto peffect = lua_get<effect*>(L, 4);
	auto min_tribute = lua_get<uint8_t, 0>(L, 5);
	auto zone = lua_get<uint32_t, 0x1f>(L, 6);
	pduel->game_field->core.summon_cancelable = FALSE;
	pduel->game_field->mset(playerid, pcard, peffect, ignore_count, min_tribute, zone);
	if(pduel->game_field->core.current_chain.size()) {
		pduel->game_field->core.reserved = pduel->game_field->core.subunits.back();
		pduel->game_field->core.subunits.pop_back();
		pduel->game_field->core.summoning_card = pcard;
	}
	return lua_yield(L, 0);
}
int32_t duel_sets(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto toplayer = lua_get<uint8_t>(L, 3, playerid);
	if(toplayer != 0 && toplayer != 1)
		toplayer = playerid;
	bool confirm = lua_get<bool, true>(L, 4);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 2, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	if(pcard) {
		pgroup = pduel->new_group(pcard);
	} else if(pgroup->container.empty()) {
		return 0;
	}
	pduel->game_field->add_process(PROCESSOR_SSET_G, 0, pduel->game_field->core.reason_effect, pgroup, playerid, toplayer, confirm);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_create_token(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid >= PLAYER_NONE) {
		lua_pushboolean(L, 0);
		return 1;
	}
	auto code = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	card* pcard = pduel->new_card(code);
	pcard->owner = playerid;
	pcard->current.location = 0;
	pcard->current.controler = playerid;
	interpreter::pushobject(L, pcard);
	return 1;
}
int32_t duel_special_summon(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 7);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto sumtype = lua_get<uint32_t>(L, 2);
	auto sumplayer = lua_get<uint8_t>(L, 3);
	if(sumplayer >= PLAYER_NONE)
		return 0;
	auto playerid = lua_get<uint8_t>(L, 4);
	auto nocheck = lua_get<bool>(L, 5);
	auto nolimit = lua_get<bool>(L, 6);
	auto positions = lua_get<uint8_t>(L, 7);
	auto zone = lua_get<uint32_t, 0xff>(L, 8);
	if(pcard) {
		pduel->game_field->core.temp_set.clear();
		pduel->game_field->core.temp_set.insert(pcard);
		pduel->game_field->special_summon(&pduel->game_field->core.temp_set, sumtype, sumplayer, playerid, nocheck, nolimit, positions, zone);
	} else
		pduel->game_field->special_summon(&(pgroup->container), sumtype, sumplayer, playerid, nocheck, nolimit, positions, zone);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_special_summon_step(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 7);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto sumtype = lua_get<uint32_t>(L, 2);
	auto sumplayer = lua_get<uint8_t>(L, 3);
	if(sumplayer >= PLAYER_NONE)
		return 0;
	auto playerid = lua_get<uint8_t>(L, 4);
	auto nocheck = lua_get<bool>(L, 5);
	auto nolimit = lua_get<bool>(L, 6);
	auto positions = lua_get<uint8_t>(L, 7);
	auto zone = lua_get<uint32_t, 0xff>(L, 8);
	pduel->game_field->special_summon_step(pcard, sumtype, sumplayer, playerid, nocheck, nolimit, positions, zone);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushboolean(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_special_summon_complete(lua_State* L) {
	check_action_permission(L);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->special_summon_complete(pduel->game_field->core.reason_effect, pduel->game_field->core.reason_player);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_sendto_hand(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 2);
	if(playerid > PLAYER_NONE)
		return 0;
	auto reason = lua_get<uint32_t>(L, 3);
	if(pcard)
		pduel->game_field->send_to(pcard, pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, LOCATION_HAND, 0, POS_FACEUP);
	else
		pduel->game_field->send_to(&(pgroup->container), pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, LOCATION_HAND, 0, POS_FACEUP);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_sendto_deck(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 4);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 2);
	if(playerid > PLAYER_NONE)
		return 0;
	auto sequence = lua_get<int32_t>(L, 3);
	auto reason = lua_get<uint32_t>(L, 4);
	uint16_t location = (sequence == -2) ? 0 : LOCATION_DECK;
	if(pcard)
		pduel->game_field->send_to(pcard, pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, location, sequence, POS_FACEUP);
	else
		pduel->game_field->send_to(&(pgroup->container), pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, location, sequence, POS_FACEUP);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_sendto_extra(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 2);
	if(playerid > PLAYER_NONE)
		return 0;
	auto reason = lua_get<uint32_t>(L, 3);
	if(pcard)
		pduel->game_field->send_to(pcard, pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, LOCATION_EXTRA, 0, POS_FACEUP);
	else
		pduel->game_field->send_to(&(pgroup->container), pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, LOCATION_EXTRA, 0, POS_FACEUP);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_sendto(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto location = lua_get<uint16_t>(L, 2);
	auto reason = lua_get<uint32_t>(L, 3);
	auto pos = lua_get<uint8_t, POS_FACEUP>(L, 4);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 5);
	if(playerid > PLAYER_NONE)
		return 0;
	auto sequence = lua_get<uint32_t, 0>(L, 6);
	if(pcard)
		pduel->game_field->send_to(pcard, pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, location, sequence, pos, TRUE);
	else
		pduel->game_field->send_to(&(pgroup->container), pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, location, sequence, pos, TRUE);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_remove_cards(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto message = pduel->new_message(MSG_REMOVE_CARDS);
	if(pcard) {
		message->write<uint32_t>(1);
		message->write(pcard->get_info_location());
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
		pduel->game_field->remove_card(pcard);
	} else {
		message->write<uint32_t>(pgroup->container.size());
		for(auto& card : pgroup->container) {
			message->write(card->get_info_location());
			card->enable_field_effect(false);
			card->cancel_field_effect();
		}
		for(auto& card : pgroup->container)
			pduel->game_field->remove_card(card);
	}
	return 0;
}
int32_t duel_get_operated_group(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group(pduel->game_field->core.operated_set);
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t duel_is_can_add_counter(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	if(lua_gettop(L) == 1)
		lua_pushboolean(L, pduel->game_field->is_player_can_action(playerid, EFFECT_CANNOT_PLACE_COUNTER));
	else {
		check_param_count(L, 4);
		auto countertype = lua_get<uint16_t>(L, 2);
		auto count = lua_get<uint32_t>(L, 3);
		auto pcard = lua_get<card*, true>(L, 4);
		lua_pushboolean(L, pduel->game_field->is_player_can_place_counter(playerid, pcard, countertype, count));
	}
	return 1;
}
int32_t duel_remove_counter(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 6);
	auto rplayer = lua_get<uint8_t>(L, 1);
	if(rplayer != 0 && rplayer != 1)
		return 0;
	auto self = lua_get<uint8_t>(L, 2);
	auto oppo = lua_get<uint8_t>(L, 3);
	auto countertype = lua_get<uint16_t>(L, 4);
	auto count = lua_get<uint32_t>(L, 5);
	auto reason = lua_get<uint32_t>(L, 6);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->remove_counter(reason, 0, rplayer, self, oppo, countertype, count);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushboolean(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_is_can_remove_counter(lua_State* L) {
	check_param_count(L, 6);
	auto rplayer = lua_get<uint8_t>(L, 1);
	if(rplayer != 0 && rplayer != 1)
		return 0;
	auto self = lua_get<uint8_t>(L, 2);
	auto oppo = lua_get<uint8_t>(L, 3);
	auto countertype = lua_get<uint16_t>(L, 4);
	auto count = lua_get<uint32_t>(L, 5);
	auto reason = lua_get<uint32_t>(L, 6);
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->is_player_can_remove_counter(rplayer, 0, self, oppo, countertype, count, reason));
	return 1;
}
int32_t duel_get_counter(lua_State* L) {
	check_param_count(L, 4);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto self = lua_get<uint8_t>(L, 2);
	auto oppo = lua_get<uint8_t>(L, 3);
	auto countertype = lua_get<uint16_t>(L, 4);
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->get_field_counter(playerid, self, oppo, countertype));
	return 1;
}
int32_t duel_change_form(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto au = lua_get<uint8_t>(L, 2);
	auto ad = lua_get<uint8_t>(L, 3, au);
	auto du = lua_get<uint8_t>(L, 4, au);
	auto dd = lua_get<uint8_t>(L, 5, au);
	uint32_t flag = 0;
	if(lua_get<bool, false>(L, 6)) flag |= NO_FLIP_EFFECT;
	if(pcard) {
		pduel->game_field->core.temp_set.clear();
		pduel->game_field->core.temp_set.insert(pcard);
		pduel->game_field->change_position(&pduel->game_field->core.temp_set, pduel->game_field->core.reason_effect, pduel->game_field->core.reason_player, au, ad, du, dd, flag, TRUE);
	} else
		pduel->game_field->change_position(&(pgroup->container), pduel->game_field->core.reason_effect, pduel->game_field->core.reason_player, au, ad, du, dd, flag, TRUE);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_release(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto reason = lua_get<uint32_t>(L, 2);
	if(pcard)
		pduel->game_field->release(pcard, pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player);
	else
		pduel->game_field->release(&(pgroup->container), pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_move_to_field(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 6);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto move_player = lua_get<uint8_t>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 3);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto destination = lua_get<uint16_t>(L, 4);
	auto positions = lua_get<uint8_t>(L, 5);
	auto enable = lua_get<bool>(L, 6);
	auto zone = lua_get<uint32_t, 0xff>(L, 7);
	pcard->enable_field_effect(false);
	pduel->game_field->adjust_instant();
	pduel->game_field->move_to_field(pcard, move_player, playerid, destination, positions, enable, 0, zone);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushboolean(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_return_to_field(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	if(!(pcard->current.reason & REASON_TEMPORARY))
		return 0;
	auto pos = lua_get<uint8_t>(L, 2, pcard->previous.position);
	auto zone = lua_get<uint32_t, 0xff>(L, 3);
	pcard->enable_field_effect(false);
	pduel->game_field->adjust_instant();
	pduel->game_field->refresh_location_info_instant();
	pduel->game_field->move_to_field(pcard, pcard->previous.controler, pcard->previous.controler, pcard->previous.location, pos, TRUE, 1, zone, FALSE, LOCATION_REASON_TOFIELD | LOCATION_REASON_RETURN);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushboolean(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_move_sequence(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto seq = lua_get<uint8_t>(L, 2);
	auto playerid = pcard->current.controler;
	if(pcard->is_affect_by_effect(pduel->game_field->core.reason_effect)) {
		pduel->game_field->move_card(playerid, pcard, pcard->current.location, seq);
		pduel->game_field->raise_single_event(pcard, 0, EVENT_MOVE, pduel->game_field->core.reason_effect, 0, pduel->game_field->core.reason_player, playerid, 0);
		pduel->game_field->raise_event(pcard, EVENT_MOVE, pduel->game_field->core.reason_effect, 0, pduel->game_field->core.reason_player, playerid, 0);
		pduel->game_field->process_single_event();
		pduel->game_field->process_instant_event();
	}
	return 0;
}
int32_t duel_swap_sequence(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard1 = lua_get<card*, true>(L, 1);
	auto pcard2 = lua_get<card*, true>(L, 2);
	uint8_t player = pcard1->current.controler;
	uint8_t location = pcard1->current.location;
	if(pcard2->current.controler == player
		&& (location == LOCATION_MZONE || location == LOCATION_SZONE) && pcard2->current.location == location
		&& pcard1->is_affect_by_effect(pduel->game_field->core.reason_effect)
		&& pcard2->is_affect_by_effect(pduel->game_field->core.reason_effect)) {
		pduel->game_field->swap_card(pcard1, pcard2);
		card_set swapped{ pcard1, pcard2 };
		pduel->game_field->raise_single_event(pcard1, 0, EVENT_MOVE, pduel->game_field->core.reason_effect, 0, pduel->game_field->core.reason_player, player, 0);
		pduel->game_field->raise_single_event(pcard2, 0, EVENT_MOVE, pduel->game_field->core.reason_effect, 0, pduel->game_field->core.reason_player, player, 0);
		pduel->game_field->raise_event(&swapped, EVENT_MOVE, pduel->game_field->core.reason_effect, 0, pduel->game_field->core.reason_player, player, 0);
		pduel->game_field->process_single_event();
		pduel->game_field->process_instant_event();
	}
	return 0;
}
int32_t duel_activate_effect(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto peffect = lua_get<effect*, true>(L, 1);
	pduel->game_field->add_process(PROCESSOR_ACTIVATE_EFFECT, 0, peffect, 0, 0, 0);
	return 0;
}
int32_t duel_set_chain_limit(lua_State* L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 1);
	const auto pduel = lua_get<duel*>(L);
	int32_t f = interpreter::get_function_handle(L, 1);
	pduel->game_field->core.chain_limit.emplace_back(f, pduel->game_field->core.reason_player);
	return 0;
}
int32_t duel_set_chain_limit_p(lua_State* L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 1);
	const auto pduel = lua_get<duel*>(L);
	int32_t f = interpreter::get_function_handle(L, 1);
	pduel->game_field->core.chain_limit_p.emplace_back(f, pduel->game_field->core.reason_player);
	return 0;
}
int32_t duel_get_chain_material(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, EFFECT_CHAIN_MATERIAL, &eset);
	if(!eset.size())
		return 0;
	interpreter::pushobject(L, eset[0]);
	return 1;
}
int32_t duel_confirm_decktop(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto count = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	if(count >= pduel->game_field->player[playerid].list_main.size())
		count = pduel->game_field->player[playerid].list_main.size();
	else if(pduel->game_field->player[playerid].list_main.size() > count) {
		if(pduel->game_field->core.global_flag & GLOBALFLAG_DECK_REVERSE_CHECK) {
			card* pcard = *(pduel->game_field->player[playerid].list_main.rbegin() + count);
			if(pduel->game_field->core.deck_reversed) {
				auto message = pduel->new_message(MSG_DECK_TOP);
				message->write<uint8_t>(playerid);
				message->write<uint32_t>(count);
				message->write<uint32_t>(pcard->data.code);
				message->write<uint32_t>(pcard->current.position);
			}
		}
	}
	auto cit = pduel->game_field->player[playerid].list_main.rbegin();
	auto message = pduel->new_message(MSG_CONFIRM_DECKTOP);
	message->write<uint8_t>(playerid);
	message->write<uint32_t>(count);
	for(uint32_t i = 0; i < count && cit != pduel->game_field->player[playerid].list_main.rend(); ++i, ++cit) {
		message->write<uint32_t>((*cit)->data.code);
		message->write<uint8_t>((*cit)->current.controler);
		message->write<uint8_t>((*cit)->current.location);
		message->write<uint32_t>((*cit)->current.sequence);
	}
	return lua_yield(L, 0);
}
int32_t duel_confirm_extratop(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto count = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	if(count >= pduel->game_field->player[playerid].list_extra.size() - pduel->game_field->player[playerid].extra_p_count)
		count = pduel->game_field->player[playerid].list_extra.size() - pduel->game_field->player[playerid].extra_p_count;
	auto cit = pduel->game_field->player[playerid].list_extra.rbegin() + pduel->game_field->player[playerid].extra_p_count;
	auto message = pduel->new_message(MSG_CONFIRM_EXTRATOP);
	message->write<uint8_t>(playerid);
	message->write<uint32_t>(count);
	for(uint32_t i = 0; i < count && cit != pduel->game_field->player[playerid].list_extra.rend(); ++i, ++cit) {
		message->write<uint32_t>((*cit)->data.code);
		message->write<uint8_t>((*cit)->current.controler);
		message->write<uint8_t>((*cit)->current.location);
		message->write<uint32_t>((*cit)->current.sequence);
	}
	return lua_yield(L, 0);
}
int32_t duel_confirm_cards(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 2, pcard, pgroup);
	if(pgroup && pgroup->container.empty())
		return 0;
	const auto pduel = lua_get<duel*>(L);
	auto message = pduel->new_message(MSG_CONFIRM_CARDS);
	message->write<uint8_t>(playerid);
	if(pcard) {
		message->write<uint32_t>(1);
		message->write<uint32_t>(pcard->data.code);
		message->write<uint8_t>(pcard->current.controler);
		message->write<uint8_t>(pcard->current.location);
		message->write<uint32_t>(pcard->current.sequence);
	} else {
		message->write<uint32_t>(pgroup->container.size());
		for(auto& _pcard : pgroup->container) {
			message->write<uint32_t>(_pcard->data.code);
			message->write<uint8_t>(_pcard->current.controler);
			message->write<uint8_t>(_pcard->current.location);
			message->write<uint32_t>(_pcard->current.sequence);
		}
	}
	return lua_yield(L, 0);
}
int32_t duel_sort_decktop(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto sort_player = lua_get<uint8_t>(L, 1);
	auto target_player = lua_get<uint8_t>(L, 2);
	auto count = lua_get<uint32_t>(L, 3);
	if(sort_player != 0 && sort_player != 1)
		return 0;
	if(target_player != 0 && target_player != 1)
		return 0;
	if(count < 1 || count > 64)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_SORT_DECK, 0, 0, 0, sort_player + (target_player << 16), count, FALSE);
	return lua_yield(L, 0);
}
int32_t duel_sort_deckbottom(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto sort_player = lua_get<uint8_t>(L, 1);
	auto target_player = lua_get<uint8_t>(L, 2);
	auto count = lua_get<uint32_t>(L, 3);
	if(sort_player != 0 && sort_player != 1)
		return 0;
	if(target_player != 0 && target_player != 1)
		return 0;
	if(count < 1 || count > 64)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_SORT_DECK, 0, 0, 0, sort_player + (target_player << 16), count, TRUE);
	return lua_yield(L, 0);
}
int32_t duel_check_event(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto ev = lua_get<uint32_t>(L, 1);
	if(!/*bool get_info = */lua_get<bool, false>(L, 2)) {
		lua_pushboolean(L, pduel->game_field->check_event(ev));
		return 1;
	} else {
		tevent pe;
		if(pduel->game_field->check_event(ev, &pe)) {
			lua_pushboolean(L, 1);
			interpreter::pushobject(L, pe.event_cards);
			lua_pushinteger(L, pe.event_player);
			lua_pushinteger(L, pe.event_value);
			interpreter::pushobject(L, pe.reason_effect);
			lua_pushinteger(L, pe.reason);
			lua_pushinteger(L, pe.reason_player);
			return 7;
		} else {
			lua_pushboolean(L, 0);
			return 1;
		}
	}
}
int32_t duel_raise_event(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 7);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto code = lua_get<uint32_t>(L, 2);
	effect* peffect = 0;
	if(!lua_isnoneornil(L, 3))
		peffect = lua_get<effect*, true>(L, 3);
	auto r = lua_get<uint32_t>(L, 4);
	auto rp = lua_get<uint8_t>(L, 5);
	auto ep = lua_get<uint8_t>(L, 6);
	auto ev = lua_get<uint32_t>(L, 7);
	if(pcard)
		pduel->game_field->raise_event(pcard, code, peffect, r, rp, ep, ev);
	else
		pduel->game_field->raise_event(&pgroup->container, code, peffect, r, rp, ep, ev);
	pduel->game_field->process_instant_event();
	return lua_yield(L, 0);
}
int32_t duel_raise_single_event(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 7);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto code = lua_get<uint32_t>(L, 2);
	auto peffect = lua_get<effect*>(L, 3);
	auto r = lua_get<uint32_t>(L, 4);
	auto rp = lua_get<uint8_t>(L, 5);
	auto ep = lua_get<uint8_t>(L, 6);
	auto ev = lua_get<uint32_t>(L, 7);
	pduel->game_field->raise_single_event(pcard, 0, code, peffect, r, rp, ep, ev);
	pduel->game_field->process_single_event();
	return lua_yield(L, 0);
}
int32_t duel_check_timing(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto tm = lua_get<uint32_t>(L, 1);
	lua_pushboolean(L, (pduel->game_field->core.hint_timing[0]&tm) || (pduel->game_field->core.hint_timing[1]&tm));
	return 1;
}
int32_t duel_get_environment(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	uint32_t code = 0;
	uint8_t p = 2;
	auto IsEnabled = [&](card* pcard) {
		if(pcard && pcard->is_position(POS_FACEUP) && pcard->get_status(STATUS_EFFECT_ENABLED)) {
			code = pcard->get_code();
			p = pcard->current.controler;
			return true;
		}
		return false;
	};
	if(!IsEnabled(pduel->game_field->player[0].list_szone[5]) && !IsEnabled(pduel->game_field->player[1].list_szone[5])) {
		effect_set eset;
		pduel->game_field->filter_field_effect(EFFECT_CHANGE_ENVIRONMENT, &eset);
		if(eset.size()) {
			const auto peffect = eset.back();
			code = peffect->get_value();
			p = peffect->get_handler_player();
		}
	}
	lua_pushinteger(L, code);
	lua_pushinteger(L, p);
	return 2;
}
int32_t duel_is_environment(lua_State* L) {
	check_param_count(L, 1);
	auto code = lua_get<uint32_t>(L, 1);
	auto playerid = lua_get<uint8_t, PLAYER_ALL>(L, 2);
	auto loc = lua_get<uint16_t, LOCATION_FZONE + LOCATION_ONFIELD>(L, 3);
	if(playerid != 0 && playerid != 1 && playerid != PLAYER_ALL)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	auto RetTrue = [L] {
		lua_pushboolean(L, TRUE);
		return 1;
	};
	auto IsEnabled = [](card* pcard) { return pcard && pcard->is_position(POS_FACEUP) && pcard->get_status(STATUS_EFFECT_ENABLED); };
	auto CheckFzone = [&](uint8_t player) {
		if(playerid == player || playerid == PLAYER_ALL) {
			const auto& pcard = pduel->game_field->player[player].list_szone[5];
			if(IsEnabled(pcard) && code == pcard->get_code())
				return true;
		}
		return false;
	};
	auto CheckSzone = [&](uint8_t player) {
		if(playerid == player || playerid == PLAYER_ALL) {
			for(auto& pcard : pduel->game_field->player[player].list_szone) {
				if(IsEnabled(pcard) && code == pcard->get_code())
					return true;
			}
		}
		return false;
	};
	auto CheckMzone = [&](uint8_t player) {
		if(playerid == player || playerid == PLAYER_ALL) {
			for(auto& pcard : pduel->game_field->player[player].list_mzone) {
				if(IsEnabled(pcard) && code == pcard->get_code())
					return true;
			}
		}
		return false;
	};
	auto ShouldApplyChangeEnv = [&]() {
		if((loc & (LOCATION_FZONE | LOCATION_SZONE)) == 0)
			return false;
		return !(IsEnabled(pduel->game_field->player[0].list_szone[5]) || IsEnabled(pduel->game_field->player[1].list_szone[5]));
	};

	if(loc & LOCATION_FZONE && (CheckFzone(0) || CheckFzone(1)))
		return RetTrue();

	if(loc & LOCATION_SZONE && (CheckSzone(0) || CheckSzone(1)))
		return RetTrue();

	if(loc & LOCATION_MZONE && (CheckMzone(0) || CheckMzone(1)))
		return RetTrue();

	if(ShouldApplyChangeEnv()) {
		effect_set eset;
		pduel->game_field->filter_field_effect(EFFECT_CHANGE_ENVIRONMENT, &eset);
		if(eset.size()) {
			const auto& peffect = eset.back();
			if((playerid == PLAYER_ALL || playerid == peffect->get_handler_player()) && code == (uint32_t)peffect->get_value())
				return RetTrue();
		}
	}
	lua_pushboolean(L, FALSE);
	return 1;
}
int32_t duel_win(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto reason = lua_get<uint32_t>(L, 2);
	if (playerid != 0 && playerid != 1 && playerid != 2)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	if (playerid == 0) {
		if (pduel->game_field->is_player_affected_by_effect(1, EFFECT_CANNOT_LOSE_EFFECT))
			return 0;
	}
	else if (playerid == 1) {
		if (pduel->game_field->is_player_affected_by_effect(0, EFFECT_CANNOT_LOSE_EFFECT))
			return 0;
	}
	else {
		if (pduel->game_field->is_player_affected_by_effect(0, EFFECT_CANNOT_LOSE_EFFECT) && pduel->game_field->is_player_affected_by_effect(1, EFFECT_CANNOT_LOSE_EFFECT))
			return 0;
		else if (pduel->game_field->is_player_affected_by_effect(0, EFFECT_CANNOT_LOSE_EFFECT))
			playerid = 0;
		else if (pduel->game_field->is_player_affected_by_effect(1, EFFECT_CANNOT_LOSE_EFFECT))
			playerid = 1;
	}
	if (pduel->game_field->core.win_player == 5) {
		pduel->game_field->core.win_player = playerid;
		pduel->game_field->core.win_reason = reason;
	} else if ((pduel->game_field->core.win_player == 0 && playerid == 1) || (pduel->game_field->core.win_player == 1 && playerid == 0)) {
		pduel->game_field->core.win_player = 2;
		pduel->game_field->core.win_reason = reason;
	}
	return 0;
}
int32_t duel_draw(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto count = lua_get<uint32_t>(L, 2);
	auto reason = lua_get<uint32_t>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->draw(pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, count);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_damage(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto amount = lua_get<int32_t>(L, 2);
	if(amount < 0)
		amount = 0;
	auto reason = lua_get<uint32_t>(L, 3);
	bool is_step = lua_get<bool, false>(L, 4);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->damage(pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, 0, playerid, amount, is_step);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_recover(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto amount = lua_get<int32_t>(L, 2);
	if(amount < 0)
		amount = 0;
	auto reason = lua_get<uint32_t>(L, 3);
	bool is_step = lua_get<bool, false>(L, 4);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->recover(pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, amount, is_step);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_rd_complete(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.subunits.splice(pduel->game_field->core.subunits.end(), pduel->game_field->core.recover_damage_reserve);
	return lua_yield(L, 0);
}
int32_t duel_equip(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	const auto pduel = lua_get<duel*>(L);
	auto equip_card = lua_get<card*, true>(L, 2);
	auto target = lua_get<card*, true>(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	bool up = lua_get<bool, true>(L, 4);
	bool step = lua_get<bool, false>(L, 5);
	pduel->game_field->equip(playerid, equip_card, target, up, step);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushboolean(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_equip_complete(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	card_set etargets;
	for(auto& equip_card : pduel->game_field->core.equiping_cards) {
		if(equip_card->is_position(POS_FACEUP))
			equip_card->enable_field_effect(true);
		etargets.insert(equip_card->equiping_target);
	}
	pduel->game_field->adjust_instant();
	for(auto& equip_target : etargets)
		pduel->game_field->raise_single_event(equip_target, &pduel->game_field->core.equiping_cards, EVENT_EQUIP,
		                                      pduel->game_field->core.reason_effect, 0, pduel->game_field->core.reason_player, PLAYER_NONE, 0);
	pduel->game_field->raise_event(&pduel->game_field->core.equiping_cards, EVENT_EQUIP,
	                               pduel->game_field->core.reason_effect, 0, pduel->game_field->core.reason_player, PLAYER_NONE, 0);
	pduel->game_field->core.hint_timing[0] |= TIMING_EQUIP;
	pduel->game_field->core.hint_timing[1] |= TIMING_EQUIP;
	pduel->game_field->process_single_event();
	pduel->game_field->process_instant_event();
	return lua_yield(L, 0);
}
int32_t duel_get_control(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	uint16_t reset_phase = lua_get<uint16_t, 0>(L, 3) & 0x3ff;
	auto reset_count = lua_get<uint8_t, 0>(L, 4);
	auto zone = lua_get<uint32_t, 0xff>(L, 5);
	auto chose_player = lua_get<uint8_t>(L, 6, pduel->game_field->core.reason_player);
	if(pcard)
		pduel->game_field->get_control(pcard, pduel->game_field->core.reason_effect, chose_player, playerid, reset_phase, reset_count, zone);
	else
		pduel->game_field->get_control(&pgroup->container, pduel->game_field->core.reason_effect, chose_player, playerid, reset_phase, reset_count, zone);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_swap_control(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	card* pcard1 = nullptr;
	card* pcard2 = nullptr;
	group* pgroup1 = nullptr;
	group* pgroup2 = nullptr;
	auto obj1 = lua_get<lua_obj*>(L, 1);
	auto obj2 = lua_get<lua_obj*>(L, 2);
	if((!obj1 || !obj2) ||
		((obj1->lua_type | obj2->lua_type) & ~(PARAM_TYPE_CARD | PARAM_TYPE_GROUP)) ||
	    obj1->lua_type != obj2->lua_type) {
		luaL_error(L, "Parameter %d should be \"Card\" or \"Group\".", 1);
		unreachable();
	}
	if(obj1->lua_type == PARAM_TYPE_CARD) {
		pcard1 = static_cast<card*>(obj1);
		pcard2 = static_cast<card*>(obj2);
	} else if(obj1->lua_type == PARAM_TYPE_GROUP) {
		pgroup1 = static_cast<group*>(obj1);
		pgroup2 = static_cast<group*>(obj2);
	}
	const auto pduel = lua_get<duel*>(L);
	uint16_t reset_phase = lua_get<uint16_t, 0>(L, 3) & 0x3ff;
	auto reset_count = lua_get<uint8_t, 0>(L, 4);
	if(pcard1)
		pduel->game_field->swap_control(pduel->game_field->core.reason_effect, pduel->game_field->core.reason_player, pcard1, pcard2, reset_phase, reset_count);
	else
		pduel->game_field->swap_control(pduel->game_field->core.reason_effect, pduel->game_field->core.reason_player, &pgroup1->container, &pgroup2->container, reset_phase, reset_count);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushboolean(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_check_lp_cost(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto cost = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->check_lp_cost(playerid, cost));
	return 1;
}
int32_t duel_pay_lp_cost(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto cost = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_PAY_LPCOST, 0, 0, 0, playerid, cost);
	return lua_yield(L, 0);
}
int32_t duel_discard_deck(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto count = lua_get<uint16_t>(L, 2);
	auto reason = lua_get<uint32_t>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_DISCARD_DECK, 0, 0, 0, playerid + (count << 16), reason);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_discard_hand(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 5);
	if(!lua_isnoneornil(L, 2))
		check_param(L, PARAM_TYPE_FUNCTION, 2);
	card* pexception = 0;
	group* pexgroup = 0;
	uint32_t extraargs = 0;
	if(lua_gettop(L) >= 6) {
		if((pexception = lua_get<card*>(L, 6)) == nullptr)
			pexgroup = lua_get<group*>(L, 6);
		extraargs = lua_gettop(L) - 6;
	}
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto min = lua_get<uint16_t>(L, 3);
	auto max = lua_get<uint16_t>(L, 4);
	auto reason = lua_get<uint32_t>(L, 5);
	group* pgroup = pduel->new_group();
	pduel->game_field->filter_matching_card(2, playerid, LOCATION_HAND, 0, pgroup, pexception, pexgroup, extraargs);
	pduel->game_field->core.select_cards.assign(pgroup->container.begin(), pgroup->container.end());
	if(pduel->game_field->core.select_cards.size() == 0) {
		lua_pushinteger(L, 0);
		return 1;
	}
	pduel->game_field->add_process(PROCESSOR_DISCARD_HAND, 0, NULL, NULL, playerid, min + (max << 16), reason);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_disable_shuffle_check(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	bool disable = lua_get<bool, true>(L, 1);
	pduel->game_field->core.shuffle_check_disabled = disable;
	return 0;
}
int32_t duel_disable_self_destroy_check(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	bool disable = lua_get<bool, true>(L, 1);
	pduel->game_field->core.selfdes_disabled = disable;
	return 0;
}
int32_t duel_shuffle_deck(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->shuffle(playerid, LOCATION_DECK);
	return 0;
}
int32_t duel_shuffle_extra(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if (playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->shuffle(playerid, LOCATION_EXTRA);
	return 0;
}
int32_t duel_shuffle_hand(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->shuffle(playerid, LOCATION_HAND);
	return 0;
}
int32_t duel_shuffle_setcard(lua_State* L) {
	check_param_count(L, 1);
	auto pgroup = lua_get<group*, true>(L, 1);
	if(pgroup->container.size() <= 0)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	card* ms[7];
	uint8_t seq[7];
	uint8_t tp = 2;
	uint8_t loc = 0;
	uint8_t ct = 0;
	for(auto& pcard : pgroup->container) {
		if((loc != 0 && (pcard->current.location != loc)) || (pcard->current.location != LOCATION_MZONE && pcard->current.location != LOCATION_SZONE)
			|| (pcard->current.position & POS_FACEUP) || (pcard->current.sequence > 4) || (tp != 2 && (pcard->current.controler != tp)))
			return 0;
		tp = pcard->current.controler;
		loc = pcard->current.location;
		ms[ct] = pcard;
		seq[ct] = pcard->current.sequence;
		ct++;
	}
	for(int32_t i = ct - 1; i > 0; --i) {
		int32_t s = pduel->get_next_integer(0, i);
		std::swap(ms[i], ms[s]);
	}
	auto& field = pduel->game_field;
	auto& list = (loc == LOCATION_MZONE) ? field->player[tp].list_mzone : field->player[tp].list_szone;
	auto message = pduel->new_message(MSG_SHUFFLE_SET_CARD);
	message->write<uint8_t>(loc);
	message->write<uint8_t>(ct);
	for(uint32_t i = 0; i < ct; ++i) {
		card* pcard = ms[i];
		message->write(pcard->get_info_location());
		list[seq[i]] = pcard;
		pcard->current.sequence = seq[i];
		field->raise_single_event(pcard, 0, EVENT_MOVE, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, tp, 0);
	}
	field->raise_event(&pgroup->container, EVENT_MOVE, field->core.reason_effect, 0, field->core.reason_player, tp, 0);
	field->process_single_event();
	field->process_instant_event();
	for(uint32_t i = 0; i < ct; ++i) {
		if(ms[i]->xyz_materials.size()) {
			message->write(ms[i]->get_info_location());
		} else {
			message->write(loc_info{});
		}
	}
	return 0;
}
int32_t duel_change_attacker(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto attacker = lua_get<card*, true>(L, 1);
	if(pduel->game_field->core.attacker == attacker)
		return 0;
	auto ignore_count = lua_get<bool, false>(L, 2);
	card* attack_target = pduel->game_field->core.attack_target;
	pduel->game_field->core.attacker->announce_count++;
	pduel->game_field->core.attacker->announced_cards.addcard(attack_target);
	pduel->game_field->attack_all_target_check();
	pduel->game_field->core.attacker = attacker;
	attacker->attack_controler = attacker->current.controler;
	pduel->game_field->core.pre_field[0] = attacker->fieldid_r;
	if(!ignore_count) {
		attacker->attack_announce_count++;
		if(pduel->game_field->infos.phase == PHASE_DAMAGE) {
			attacker->attacked_count++;
			attacker->attacked_cards.addcard(attack_target);
		}
	}
	return 0;
}
int32_t duel_change_attack_target(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto target = lua_get<card*>(L, 1);
	auto ignore = lua_get<bool, false>(L, 2);
	card* attacker = pduel->game_field->core.attacker;
	if(!attacker || !attacker->is_capable_attack() || attacker->is_status(STATUS_ATTACK_CANCELED)) {
		lua_pushboolean(L, 0);
		return 1;
	}
	card_vector cv;
	pduel->game_field->get_attack_target(attacker, &cv, pduel->game_field->core.chain_attack);
	if(((target && std::find(cv.begin(), cv.end(), target) != cv.end()) || ignore) ||
		(!target && !attacker->is_affected_by_effect(EFFECT_CANNOT_DIRECT_ATTACK))) {
		pduel->game_field->core.attack_target = target;
		pduel->game_field->core.attack_rollback = FALSE;
		pduel->game_field->core.opp_mzone.clear();
		uint8_t turnp = pduel->game_field->infos.turn_player;
		for(auto& pcard : pduel->game_field->player[1 - turnp].list_mzone) {
			if(pcard)
				pduel->game_field->core.opp_mzone.insert(pcard->fieldid_r);
		}
		auto message = pduel->new_message(MSG_ATTACK);
		message->write(attacker->get_info_location());
		if(target) {
			if(target->current.controler != turnp) {
				pduel->game_field->raise_single_event(target, 0, EVENT_BE_BATTLE_TARGET, 0, REASON_REPLACE, 0, turnp, 0);
				pduel->game_field->raise_event(target, EVENT_BE_BATTLE_TARGET, 0, REASON_REPLACE, 0, turnp, 0);
			}else{
				pduel->game_field->raise_single_event(target, 0, EVENT_BE_BATTLE_TARGET, 0, REASON_REPLACE, 0, 1 - turnp, 0);
				pduel->game_field->raise_event(target, EVENT_BE_BATTLE_TARGET, 0, REASON_REPLACE, 0, 1 - turnp, 0);
			}
			pduel->game_field->process_single_event();
			pduel->game_field->process_instant_event();
			message->write(target->get_info_location());
		} else {
			pduel->game_field->core.attack_player = TRUE;
			message->write(loc_info{});
		}
		lua_pushboolean(L, 1);
	} else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t duel_attack_cost_paid(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	auto paid = lua_get<uint8_t, 1>(L, 1);
	pduel->game_field->core.attack_cost_paid = paid;
	return 0;
}
int32_t duel_force_attack(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto attacker = lua_get<card*, true>(L, 1);
	auto attack_target = lua_get<card*, true>(L, 2);
	pduel->game_field->core.set_forced_attack = true;
	pduel->game_field->core.forced_attacker = attacker;
	pduel->game_field->core.forced_attack_target = attack_target;
	return lua_yield(L, 0);
}
int32_t duel_calculate_damage(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto attacker = lua_get<card*, true>(L, 1);
	auto attack_target = lua_get<card*>(L, 2);
	auto new_attack = lua_get<bool, false>(L, 3);
	if(attacker == attack_target)
		return 0;
	pduel->game_field->add_process(PROCESSOR_DAMAGE_STEP, 0, (effect*)attacker, (group*)attack_target, 0, new_attack);
	return lua_yield(L, 0);
}
int32_t duel_get_battle_damage(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	lua_pushinteger(L, pduel->game_field->core.battle_damage[playerid]);
	return 1;
}
int32_t duel_change_battle_damage(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto dam = lua_get<uint32_t>(L, 2);
	bool check = lua_get<bool, true>(L, 3);
	if(check && pduel->game_field->core.battle_damage[playerid] == 0)
		return 0;
	pduel->game_field->core.battle_damage[playerid] = dam;
	return 0;
}
int32_t duel_change_target(lua_State* L) {
	check_param_count(L, 2);
	auto count = lua_get<uint8_t>(L, 1);
	auto pgroup = lua_get<group*, true>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->change_target(count, pgroup);
	return 0;
}
int32_t duel_change_target_player(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto count = lua_get<uint8_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->change_target_player(count, playerid);
	return 0;
}
int32_t duel_change_target_param(lua_State* L) {
	check_param_count(L, 2);
	auto count = lua_get<uint8_t>(L, 1);
	auto param = lua_get<int32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->change_target_param(count, param);
	return 0;
}
int32_t duel_break_effect(lua_State* L) {
	check_action_permission(L);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->break_effect();
	pduel->game_field->raise_event((card*)0, EVENT_BREAK_EFFECT, 0, 0, PLAYER_NONE, PLAYER_NONE, 0);
	pduel->game_field->process_instant_event();
	return lua_yield(L, 0);
}
int32_t duel_change_effect(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	const auto pduel = lua_get<duel*>(L);
	int32_t pf = interpreter::get_function_handle(L, 2);
	pduel->game_field->change_chain_effect(lua_get<uint8_t>(L, 1), pf);
	return 0;
}
int32_t duel_negate_activate(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->negate_chain(lua_get<uint8_t>(L, 1)));
	return 1;
}
int32_t duel_negate_effect(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->disable_chain(lua_get<uint8_t>(L, 1)));
	return 1;
}
int32_t duel_negate_related_chain(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto reset_flag = lua_get<uint32_t>(L, 2);
	if(pduel->game_field->core.current_chain.size() < 2)
		return 0;
	if(!pcard->is_affect_by_effect(pduel->game_field->core.reason_effect))
		return 0;
	for(auto it = pduel->game_field->core.current_chain.rbegin(); it != pduel->game_field->core.current_chain.rend(); ++it) {
		if(it->triggering_effect->get_handler() == pcard && pcard->is_has_relation(*it)) {
			effect* negeff = pduel->new_effect();
			negeff->owner = pduel->game_field->core.reason_effect->get_handler();
			negeff->type = EFFECT_TYPE_SINGLE;
			negeff->code = EFFECT_DISABLE_CHAIN;
			negeff->value = it->chain_id;
			negeff->reset_flag = RESET_CHAIN | RESET_EVENT | reset_flag;
			pcard->add_effect(negeff);
		}
	}
	return 0;
}
int32_t duel_disable_summon(lua_State* L) {
	check_param_count(L, 1);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	uint8_t sumplayer = PLAYER_NONE;
	if(pcard) {
		sumplayer = pcard->summon_player;
		pcard->set_status(STATUS_SUMMONING, FALSE);
		pcard->set_status(STATUS_SUMMON_DISABLED, TRUE);
		if((pcard->summon_info & SUMMON_TYPE_PENDULUM) != SUMMON_TYPE_PENDULUM)
			pcard->set_status(STATUS_PROC_COMPLETE, FALSE);
	} else {
		for(auto& _pcard : pgroup->container) {
			sumplayer = _pcard->summon_player;
			_pcard->set_status(STATUS_SUMMONING, FALSE);
			_pcard->set_status(STATUS_SUMMON_DISABLED, TRUE);
			if((_pcard->summon_info & SUMMON_TYPE_PENDULUM) != SUMMON_TYPE_PENDULUM)
				_pcard->set_status(STATUS_PROC_COMPLETE, FALSE);
		}
	}
	uint32_t event_code = 0;
	if(pduel->game_field->check_event(EVENT_SUMMON))
		event_code = EVENT_SUMMON_NEGATED;
	else if(pduel->game_field->check_event(EVENT_FLIP_SUMMON))
		event_code = EVENT_FLIP_SUMMON_NEGATED;
	else if(pduel->game_field->check_event(EVENT_SPSUMMON))
		event_code = EVENT_SPSUMMON_NEGATED;
	effect* reason_effect = pduel->game_field->core.reason_effect;
	uint8_t reason_player = pduel->game_field->core.reason_player;
	if(pcard)
		pduel->game_field->raise_event(pcard, event_code, reason_effect, REASON_EFFECT, reason_player, sumplayer, 0);
	else
		pduel->game_field->raise_event(&pgroup->container, event_code, reason_effect, REASON_EFFECT, reason_player, sumplayer, 0);
	pduel->game_field->process_instant_event();
	return 0;
}
int32_t duel_increase_summon_count(lua_State* L) {
	card* pcard = 0;
	effect* pextra = 0;
	if(lua_gettop(L) > 0)
		pcard = lua_get<card*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	uint8_t playerid = pduel->game_field->core.reason_player;
	if(pcard && (pextra = pcard->is_affected_by_effect(EFFECT_EXTRA_SUMMON_COUNT)) != nullptr)
		pextra->get_value(pcard);
	else
		pduel->game_field->core.summon_count[playerid]++;
	return 0;
}
int32_t duel_check_summon_count(lua_State* L) {
	card* pcard = 0;
	if(lua_gettop(L) > 0)
		pcard = lua_get<card*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	uint8_t playerid = pduel->game_field->core.reason_player;
	if((pcard && pcard->is_affected_by_effect(EFFECT_EXTRA_SUMMON_COUNT))
	        || pduel->game_field->core.summon_count[playerid] < pduel->game_field->get_summon_count_limit(playerid))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t duel_get_location_count(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto location = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	auto uplayer = lua_get<uint8_t>(L, 3, pduel->game_field->core.reason_player);
	auto reason = lua_get<uint32_t, LOCATION_REASON_TOFIELD>(L, 4);
	auto zone = lua_get<uint32_t, 0xff>(L, 5);
	uint32_t list = 0;
	lua_pushinteger(L, pduel->game_field->get_useable_count(NULL, playerid, location, uplayer, reason, zone, &list));
	lua_pushinteger(L, list);
	return 2;
}
int32_t duel_get_mzone_count(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	bool swapped = false;
	card* mcard = nullptr;
	group* mgroup = nullptr;
	uint32_t used_location[2] = { 0, 0 };
	card_vector list_mzone[2];
	if(!lua_isnoneornil(L, 2)) {
		get_card_or_group(L, 2, mcard, mgroup);
		for(int32_t p = 0; p < 2; p++) {
			uint32_t digit = 1;
			for(auto& pcard : pduel->game_field->player[p].list_mzone) {
				if(pcard && pcard != mcard && !(mgroup && mgroup->has_card(pcard))) {
					used_location[p] |= digit;
					list_mzone[p].push_back(pcard);
				} else
					list_mzone[p].push_back(0);
				digit <<= 1;
			}
			used_location[p] |= pduel->game_field->player[p].used_location & 0xff00;
			std::swap(used_location[p], pduel->game_field->player[p].used_location);
			pduel->game_field->player[p].list_mzone.swap(list_mzone[p]);
		}
		swapped = true;
	}
	auto uplayer = lua_get<uint8_t>(L, 3, pduel->game_field->core.reason_player);
	auto reason = lua_get<uint32_t, LOCATION_REASON_TOFIELD>(L, 4);
	auto zone = lua_get<uint32_t, 0xff>(L, 5);
	uint32_t list = 0;
	lua_pushinteger(L, pduel->game_field->get_useable_count(NULL, playerid, LOCATION_MZONE, uplayer, reason, zone, &list));
	lua_pushinteger(L, list);
	if(swapped) {
		pduel->game_field->player[0].used_location = used_location[0];
		pduel->game_field->player[1].used_location = used_location[1];
		pduel->game_field->player[0].list_mzone.swap(list_mzone[0]);
		pduel->game_field->player[1].list_mzone.swap(list_mzone[1]);
	}
	return 2;
}
int32_t duel_get_location_count_fromex(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	auto uplayer = lua_get<uint8_t>(L, 2, pduel->game_field->core.reason_player);
	bool swapped = false;
	card* mcard = nullptr;
	group* mgroup = nullptr;
	uint32_t used_location[2] = {0, 0};
	card_vector list_mzone[2];
	if(!lua_isnoneornil(L, 3)) {
		get_card_or_group(L, 3, mcard, mgroup);
		for(int32_t p = 0; p < 2; p++) {
			uint32_t digit = 1;
			for(auto& pcard : pduel->game_field->player[p].list_mzone) {
				if(pcard && pcard != mcard && !(mgroup && mgroup->has_card(pcard))) {
					used_location[p] |= digit;
					list_mzone[p].push_back(pcard);
				} else
					list_mzone[p].push_back(0);
				digit <<= 1;
			}
			used_location[p] |= pduel->game_field->player[p].used_location & 0xff00;
			std::swap(used_location[p], pduel->game_field->player[p].used_location);
			pduel->game_field->player[p].list_mzone.swap(list_mzone[p]);
		}
		swapped = true;
	}
	bool use_temp_card = false;
	card* scard = 0;
	if(!lua_isnoneornil(L, 4)) {
		if((scard = lua_get<card*>(L, 4)) == nullptr) {
			use_temp_card = true;
			auto type = lua_get<uint32_t>(L, 4);
			scard = pduel->game_field->temp_card;
			scard->current.location = LOCATION_EXTRA;
			scard->data.type = TYPE_MONSTER | type;
			if(type & TYPE_PENDULUM)
				scard->current.position = POS_FACEUP_DEFENSE;
			else
				scard->current.position = POS_FACEDOWN_DEFENSE;
		}
	}
	auto zone = lua_get<uint32_t, 0xff>(L, 5);
	uint32_t list = 0;
	lua_pushinteger(L, pduel->game_field->get_useable_count_fromex(scard, playerid, uplayer, zone, &list));
	lua_pushinteger(L, list);
	if(swapped) {
		pduel->game_field->player[0].used_location = used_location[0];
		pduel->game_field->player[1].used_location = used_location[1];
		pduel->game_field->player[0].list_mzone.swap(list_mzone[0]);
		pduel->game_field->player[1].list_mzone.swap(list_mzone[1]);
	}
	if(use_temp_card) {
		scard->current.location = 0;
		scard->data.type = 0;
		scard->current.position = 0;
	}
	return 2;
}
int32_t duel_get_usable_mzone_count(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	auto uplayer = lua_get<uint8_t>(L, 2, pduel->game_field->core.reason_player);
	uint32_t zone = 0xff;
	uint32_t flag1, flag2;
	int32_t ct1 = pduel->game_field->get_tofield_count(NULL, playerid, LOCATION_MZONE, uplayer, LOCATION_REASON_TOFIELD, zone, &flag1);
	int32_t ct2 = pduel->game_field->get_spsummonable_count_fromex(NULL, playerid, uplayer, zone, &flag2);
	int32_t ct3 = field::field_used_count[~(flag1 | flag2) & 0x1f];
	int32_t count = ct1 + ct2 - ct3;
	int32_t limit = pduel->game_field->get_mzone_limit(playerid, uplayer, LOCATION_REASON_TOFIELD);
	if(count > limit)
		count = limit;
	lua_pushinteger(L, count);
	return 1;
}
int32_t duel_get_linked_group(lua_State* L) {
	check_param_count(L, 3);
	auto rplayer = lua_get<uint8_t>(L, 1);
	if(rplayer != 0 && rplayer != 1)
		return 0;
	auto location1 = lua_get<uint16_t>(L, 2);
	if(location1 == 1)
		location1 = LOCATION_MZONE;
	auto location2 = lua_get<uint16_t>(L, 3);
	if(location2 == 1)
		location2 = LOCATION_MZONE;
	const auto pduel = lua_get<duel*>(L);
	card_set cset;
	pduel->game_field->get_linked_cards(rplayer, location1, location2, &cset);
	group* pgroup = pduel->new_group(std::move(cset));
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t duel_get_linked_group_count(lua_State* L) {
	check_param_count(L, 3);
	auto rplayer = lua_get<uint8_t>(L, 1);
	if(rplayer != 0 && rplayer != 1)
		return 0;
	auto location1 = lua_get<uint16_t>(L, 2);
	if(location1 == 1)
		location1 = LOCATION_MZONE;
	auto location2 = lua_get<uint16_t>(L, 3);
	if(location2 == 1)
		location2 = LOCATION_MZONE;
	const auto pduel = lua_get<duel*>(L);
	card_set cset;
	pduel->game_field->get_linked_cards(rplayer, location1, location2, &cset);
	lua_pushinteger(L, cset.size());
	return 1;
}
int32_t duel_get_linked_zone(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->get_linked_zone(playerid));
	return 1;
}
int32_t duel_get_free_linked_zone(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->get_linked_zone(playerid, true));
	return 1;
}
int32_t duel_get_field_card(lua_State* L) {
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto location = lua_get<uint16_t>(L, 2);
	auto sequence = lua_get<uint32_t>(L, 3);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	card* pcard = pduel->game_field->get_field_card(playerid, location, sequence);
	if(!pcard || pcard->get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP))
		return 0;
	interpreter::pushobject(L, pcard);
	return 1;
}
int32_t duel_check_location(lua_State* L) {
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto location = lua_get<uint16_t>(L, 2);
	auto sequence = lua_get<uint32_t>(L, 3);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->is_location_useable(playerid, location, sequence));
	return 1;
}
int32_t duel_get_current_chain(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	const auto real = lua_get<bool, false>(L, 1);
	const auto& core = pduel->game_field->core;
	lua_pushinteger(L, real ? core.real_chain_count : core.current_chain.size());
	return 1;
}
int32_t duel_get_chain_info(lua_State* L) {
	check_param_count(L, 1);
	uint32_t args = lua_gettop(L) - 1;
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(lua_get<uint8_t>(L, 1));
	if(!ch)
		return 0;
	for(uint32_t i = 0; i < args; ++i) {
		auto flag = lua_get<uint32_t>(L, 2 + i);
		switch(flag) {
		case CHAININFO_CHAIN_COUNT:
			lua_pushinteger(L, ch->chain_count);
			break;
		case CHAININFO_TRIGGERING_EFFECT:
			interpreter::pushobject(L, ch->triggering_effect);
			break;
		case CHAININFO_TRIGGERING_PLAYER:
			lua_pushinteger(L, ch->triggering_player);
			break;
		case CHAININFO_TRIGGERING_CONTROLER:
			lua_pushinteger(L, ch->triggering_controler);
			break;
		case CHAININFO_TRIGGERING_LOCATION:
			lua_pushinteger(L, ch->triggering_location);
			break;
		case CHAININFO_TRIGGERING_SEQUENCE:
			lua_pushinteger(L, ch->triggering_sequence);
			break;
		case CHAININFO_TRIGGERING_POSITION:
			lua_pushinteger(L, ch->triggering_position);
			break;
		case CHAININFO_TRIGGERING_CODE:
			lua_pushinteger(L, ch->triggering_state.code);
			break;
		case CHAININFO_TRIGGERING_CODE2:
			lua_pushinteger(L, ch->triggering_state.code2);
			break;
		case CHAININFO_TRIGGERING_LEVEL:
			lua_pushinteger(L, ch->triggering_state.level);
			break;
		case CHAININFO_TRIGGERING_RANK:
			lua_pushinteger(L, ch->triggering_state.rank);
			break;
		case CHAININFO_TRIGGERING_ATTRIBUTE:
			lua_pushinteger(L, ch->triggering_state.attribute);
			break;
		case CHAININFO_TRIGGERING_RACE:
			lua_pushinteger(L, ch->triggering_state.race);
			break;
		case CHAININFO_TRIGGERING_ATTACK:
			lua_pushinteger(L, ch->triggering_state.attack);
			break;
		case CHAININFO_TRIGGERING_DEFENSE:
			lua_pushinteger(L, ch->triggering_state.defense);
			break;
		case CHAININFO_TARGET_CARDS:
			interpreter::pushobject(L, ch->target_cards);
			break;
		case CHAININFO_TARGET_PLAYER:
			lua_pushinteger(L, ch->target_player);
			break;
		case CHAININFO_TARGET_PARAM:
			lua_pushinteger(L, ch->target_param);
			break;
		case CHAININFO_DISABLE_REASON:
			interpreter::pushobject(L, ch->disable_reason);
			break;
		case CHAININFO_DISABLE_PLAYER:
			lua_pushinteger(L, ch->disable_player);
			break;
		case CHAININFO_CHAIN_ID:
			lua_pushinteger(L, ch->chain_id);
			break;
		case CHAININFO_TYPE:
			if((ch->triggering_effect->card_type & (TYPE_MONSTER | TYPE_SPELL | TYPE_TRAP)) == (TYPE_TRAP | TYPE_MONSTER))
				lua_pushinteger(L, TYPE_MONSTER);
			else lua_pushinteger(L, (ch->triggering_effect->card_type & (TYPE_MONSTER | TYPE_SPELL | TYPE_TRAP)));
			break;
		case CHAININFO_EXTTYPE:
			lua_pushinteger(L, ch->triggering_effect->card_type);
			break;
		default:
			lua_pushnil(L);
			break;
		}
	}
	return args;
}
int32_t duel_get_chain_event(lua_State* L) {
	check_param_count(L, 1);
	auto count = lua_get<uint8_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(count);
	if(!ch)
		return 0;
	interpreter::pushobject(L, ch->evt.event_cards);
	lua_pushinteger(L, ch->evt.event_player);
	lua_pushinteger(L, ch->evt.event_value);
	interpreter::pushobject(L, ch->evt.reason_effect);
	lua_pushinteger(L, ch->evt.reason);
	lua_pushinteger(L, ch->evt.reason_player);
	return 6;
}
int32_t duel_get_first_target(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(0);
	if(!ch || !ch->target_cards || ch->target_cards->container.size() == 0)
		return 0;
	for(auto& pcard : ch->target_cards->container)
		interpreter::pushobject(L, pcard);
	return ch->target_cards->container.size();
}
int32_t duel_get_current_phase(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->infos.phase);
	return 1;
}
int32_t duel_skip_phase(lua_State* L) {
	check_param_count(L, 4);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto phase = lua_get<uint16_t>(L, 2);
	auto reset = lua_get<uint32_t>(L, 3);
	auto count = lua_get<uint16_t>(L, 4);
	auto value = lua_get<uint32_t, 0>(L, 5);
	if(count <= 0)
		count = 1;
	const auto pduel = lua_get<duel*>(L);
	int32_t code = 0;
	if(phase == PHASE_DRAW)
		code = EFFECT_SKIP_DP;
	else if(phase == PHASE_STANDBY)
		code = EFFECT_SKIP_SP;
	else if(phase == PHASE_MAIN1)
		code = EFFECT_SKIP_M1;
	else if(phase == PHASE_BATTLE)
		code = EFFECT_SKIP_BP;
	else if(phase == PHASE_MAIN2)
		code = EFFECT_SKIP_M2;
	else if(phase == PHASE_END)
		code = EFFECT_SKIP_EP;
	else
		return 0;
	effect* peffect = pduel->new_effect();
	peffect->owner = pduel->game_field->temp_card;
	peffect->effect_owner = playerid;
	peffect->type = EFFECT_TYPE_FIELD;
	peffect->code = code;
	peffect->reset_flag = (reset & 0x3ff) | RESET_PHASE | RESET_SELF_TURN;
	peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_PLAYER_TARGET;
	peffect->s_range = 1;
	peffect->o_range = 0;
	peffect->reset_count = count;
	peffect->value = value;
	pduel->game_field->add_effect(peffect, playerid);
	return 0;
}
int32_t duel_is_attack_cost_paid(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->core.attack_cost_paid);
	return 1;
}
int32_t duel_is_damage_calculated(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->core.damage_calculated);
	return 1;
}
int32_t duel_get_attacker(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	card* pcard = pduel->game_field->core.attacker;
	interpreter::pushobject(L, pcard);
	return 1;
}
int32_t duel_get_attack_target(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	card* pcard = pduel->game_field->core.attack_target;
	interpreter::pushobject(L, pcard);
	return 1;
}
int32_t duel_get_battle_monster(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 1);
	card* attacker = pduel->game_field->core.attacker;
	card* defender = pduel->game_field->core.attack_target;
	for(int32_t i = 0; i < 2; i++) {
		if(attacker && attacker->current.controler == playerid)
			interpreter::pushobject(L, attacker);
		else if(defender && defender->current.controler == playerid)
			interpreter::pushobject(L, defender);
		else
			lua_pushnil(L);
		playerid = 1 - playerid;
	}
	return 2;
}
int32_t duel_disable_attack(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_ATTACK_DISABLE, 0, 0, 0, 0, 0);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushboolean(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_chain_attack(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.chain_attack = TRUE;
	pduel->game_field->core.chain_attacker_id = pduel->game_field->core.attacker->fieldid;
	if(lua_gettop(L) > 0)
		pduel->game_field->core.chain_attack_target = lua_get<card*, true>(L, 1);
	return 0;
}
int32_t duel_readjust(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	card* adjcard = pduel->game_field->core.reason_effect->get_handler();
	pduel->game_field->core.readjust_map[adjcard]++;
	if(pduel->game_field->core.readjust_map[adjcard] > 3) {
		pduel->game_field->send_to(adjcard, 0, REASON_RULE, pduel->game_field->core.reason_player, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
		return lua_yield(L, 0);
	}
	pduel->game_field->core.re_adjust = TRUE;
	return 0;
}
int32_t duel_adjust_instantly(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	if(lua_gettop(L) > 0) {
		auto pcard = lua_get<card*, true>(L, 1);
		pcard->filter_disable_related_cards();
	}
	pduel->game_field->adjust_instant();
	return 0;
}
/**
 * \brief Duel.GetFieldGroup
 * \param playerid, location1, location2
 * \return Group
 */
int32_t duel_get_field_group(lua_State* L) {
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto location1 = lua_get<uint16_t>(L, 2);
	auto location2 = lua_get<uint16_t>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group();
	pduel->game_field->filter_field_card(playerid, location1, location2, pgroup);
	interpreter::pushobject(L, pgroup);
	return 1;
}
/**
 * \brief Duel.GetFieldGroupCount
 * \param playerid, location1, location2
 * \return Integer
 */
int32_t duel_get_field_group_count(lua_State* L) {
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto location1 = lua_get<uint16_t>(L, 2);
	auto location2 = lua_get<uint16_t>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	uint32_t count = pduel->game_field->filter_field_card(playerid, location1, location2, 0);
	lua_pushinteger(L, count);
	return 1;
}
/**
 * \brief Duel.GetDeckTop
 * \param playerid, count
 * \return Group
 */
int32_t duel_get_decktop_group(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto count = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group();
	auto cit = pduel->game_field->player[playerid].list_main.rbegin();
	for(uint32_t i = 0; i < count && cit != pduel->game_field->player[playerid].list_main.rend(); ++i, ++cit)
		pgroup->container.insert(*cit);
	interpreter::pushobject(L, pgroup);
	return 1;
}
/**
 * \brief Duel.GetExtraTopGroup
 * \param playerid, count
 * \return Group
 */
int32_t duel_get_extratop_group(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto count = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group();
	auto cit = pduel->game_field->player[playerid].list_extra.rbegin() + pduel->game_field->player[playerid].extra_p_count;
	for(uint32_t i = 0; i < count && cit != pduel->game_field->player[playerid].list_extra.rend(); ++i, ++cit)
		pgroup->container.insert(*cit);
	interpreter::pushobject(L, pgroup);
	return 1;
}
/**
* \brief Duel.GetMatchingGroup
* \param filter_func, self, location1, location2, exception card, (extraargs...)
* \return Group
*/
int32_t duel_get_matching_group(lua_State* L) {
	check_param_count(L, 5);
	if(!lua_isnoneornil(L, 1))
		check_param(L, PARAM_TYPE_FUNCTION, 1);
	card* pexception = 0;
	group* pexgroup = 0;
	if((pexception = lua_get<card*>(L, 5)) == nullptr)
		pexgroup = lua_get<group*>(L, 5);
	uint32_t extraargs = lua_gettop(L) - 5;
	const auto pduel = lua_get<duel*>(L);
	auto self = lua_get<uint8_t>(L, 2);
	auto location1 = lua_get<uint16_t>(L, 3);
	auto location2 = lua_get<uint16_t>(L, 4);
	group* pgroup = pduel->new_group();
	pduel->game_field->filter_matching_card(1, self, location1, location2, pgroup, pexception, pexgroup, extraargs);
	interpreter::pushobject(L, pgroup);
	return 1;
}
/**
* \brief Duel.GetMatchingGroupCount
* \param filter_func, self, location1, location2, exception card, (extraargs...)
* \return Integer
*/
int32_t duel_get_matching_count(lua_State* L) {
	check_param_count(L, 5);
	if(!lua_isnoneornil(L, 1))
		check_param(L, PARAM_TYPE_FUNCTION, 1);
	card* pexception = 0;
	group* pexgroup = 0;
	if((pexception = lua_get<card*>(L, 5)) == nullptr)
		pexgroup = lua_get<group*>(L, 5);
	uint32_t extraargs = lua_gettop(L) - 5;
	const auto pduel = lua_get<duel*>(L);
	auto self = lua_get<uint8_t>(L, 2);
	auto location1 = lua_get<uint16_t>(L, 3);
	auto location2 = lua_get<uint16_t>(L, 4);
	group* pgroup = pduel->new_group();
	pduel->game_field->filter_matching_card(1, self, location1, location2, pgroup, pexception, pexgroup, extraargs);
	uint32_t count = pgroup->container.size();
	lua_pushinteger(L, count);
	return 1;
}
/**
* \brief Duel.GetFirstMatchingCard
* \param filter_func, self, location1, location2, exception card, (extraargs...)
* \return Card | nil
*/
int32_t duel_get_first_matching_card(lua_State* L) {
	check_param_count(L, 5);
	if(!lua_isnoneornil(L, 1))
		check_param(L, PARAM_TYPE_FUNCTION, 1);
	card* pexception = 0;
	group* pexgroup = 0;
	if((pexception = lua_get<card*>(L, 5)) == nullptr)
		pexgroup = lua_get<group*>(L, 5);
	uint32_t extraargs = lua_gettop(L) - 5;
	const auto pduel = lua_get<duel*>(L);
	auto self = lua_get<uint8_t>(L, 2);
	auto location1 = lua_get<uint16_t>(L, 3);
	auto location2 = lua_get<uint16_t>(L, 4);
	card* pret = 0;
	pduel->game_field->filter_matching_card(1, self, location1, location2, 0, pexception, pexgroup, extraargs, &pret);
	if(pret)
		interpreter::pushobject(L, pret);
	else lua_pushnil(L);
	return 1;
}
/**
* \brief Duel.IsExistingMatchingCard
* \param filter_func, self, location1, location2, count, exception card, (extraargs...)
* \return boolean
*/
int32_t duel_is_existing_matching_card(lua_State* L) {
	check_param_count(L, 6);
	if(!lua_isnoneornil(L, 1))
		check_param(L, PARAM_TYPE_FUNCTION, 1);
	card* pexception = 0;
	group* pexgroup = 0;
	if((pexception = lua_get<card*>(L, 6)) == nullptr)
		pexgroup = lua_get<group*>(L, 6);
	uint32_t extraargs = lua_gettop(L) - 6;
	const auto pduel = lua_get<duel*>(L);
	auto self = lua_get<uint8_t>(L, 2);
	auto location1 = lua_get<uint16_t>(L, 3);
	auto location2 = lua_get<uint16_t>(L, 4);
	auto fcount = lua_get<uint32_t>(L, 5);
	lua_pushboolean(L, pduel->game_field->filter_matching_card(1, self, location1, location2, 0, pexception, pexgroup, extraargs, 0, fcount));
	return 1;
}
/**
* \brief Duel.SelectMatchingCards
* \param playerid, filter_func, self, location1, location2, min, max, exception card, (extraargs...)
* \return Group
*/
int32_t duel_select_matching_cards(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 8);
	if(!lua_isnoneornil(L, 2))
		check_param(L, PARAM_TYPE_FUNCTION, 2);
	card* pexception = 0;
	group* pexgroup = 0;
	bool cancelable = false;
	uint8_t lastarg = 8;
	if(lua_isboolean(L, lastarg)) {
		check_param_count(L, 9);
		cancelable = lua_get<bool, false>(L, lastarg);
		lastarg++;
	}
	if((pexception = lua_get<card*>(L, lastarg)) == nullptr)
		pexgroup = lua_get<group*>(L, lastarg);
	uint32_t extraargs = lua_gettop(L) - lastarg;
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	auto self = lua_get<uint8_t>(L, 3);
	auto location1 = lua_get<uint16_t>(L, 4);
	auto location2 = lua_get<uint16_t>(L, 5);
	auto min = lua_get<uint16_t>(L, 6);
	auto max = lua_get<uint16_t>(L, 7);
	group* pgroup = pduel->new_group();
	pduel->game_field->filter_matching_card(2, self, location1, location2, pgroup, pexception, pexgroup, extraargs);
	pduel->game_field->core.select_cards.assign(pgroup->container.begin(), pgroup->container.end());
	pduel->game_field->add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + (cancelable << 16), min + (max << 16));
	return lua_yieldk(L, 0, (lua_KContext)cancelable, push_return_cards);
}
int32_t duel_select_cards_code(lua_State * L) {
	using container = std::pair<uint32_t, uint32_t>;
	check_action_permission(L);
	check_param_count(L, 6);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.select_cards.clear();
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto min = lua_get<uint16_t>(L, 2);
	auto max = lua_get<uint16_t>(L, 3);
	bool cancelable = lua_get<bool>(L, 4);
	/*bool ret_index = */(void)lua_get<bool>(L, 5);
	for(int32_t i = 6, tot = lua_gettop(L); i <= tot; ++i) {
		auto cardobj = new container(lua_get<uint32_t>(L, i), i - 5);
		pduel->game_field->core.select_cards.push_back((card*)cardobj);
	}
	pduel->game_field->add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + (cancelable << 16), min + (max << 16), TRUE);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		int ret = 1;
		if(!pduel->game_field->return_cards.canceled) {
			bool ret_index = lua_get<bool>(L, 5);
			for(auto& code : pduel->game_field->return_cards.list) {
				auto obj = (container*)code;
				if(ret_index) {
					lua_newtable(L);
					lua_pushinteger(L, 1);
				}
				lua_pushinteger(L, obj->first);
				if(ret_index) {
					lua_settable(L, -3);
					lua_pushinteger(L, 2);
					lua_pushinteger(L, obj->second);
					lua_settable(L, -3);
				}
			}
			ret = (int)pduel->game_field->return_cards.list.size();
		} else
			lua_pushnil(L);
		for(auto& obj : pduel->game_field->core.select_cards)
			delete ((container*)obj);
		return ret;
	});
}
/**
* \brief Duel.GetReleaseGroup
* \param playerid
* \return Group
*/
int32_t duel_get_release_group(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	bool hand = lua_get<bool, false>(L, 2);
	bool oppo = lua_get<bool, false>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group();
	pduel->game_field->get_release_list(playerid, &pgroup->container, &pgroup->container, &pgroup->container, FALSE, hand, 0, 0, 0, 0, oppo);
	interpreter::pushobject(L, pgroup);
	return 1;
}
/**
* \brief Duel.GetReleaseGroupCount
* \param playerid
* \return Integer
*/
int32_t duel_get_release_group_count(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	bool hand = lua_get<bool, false>(L, 2);
	bool oppo = lua_get<bool, false>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->get_release_list(playerid, 0, 0, 0, FALSE, hand, 0, 0, 0, 0, oppo));
	return 1;
}
static int32_t check_release_group(lua_State* L, uint8_t use_hand) {
	check_param_count(L, 4);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	bool use_con = !lua_isnoneornil(L, 2) && check_param(L, PARAM_TYPE_FUNCTION, 2);
	card* pexception = 0;
	group* pexgroup = 0;
	uint32_t lastarg = 4;
	const auto pduel = lua_get<duel*>(L);
	auto min = lua_get<uint16_t>(L, 3);
	auto max = min;
	bool check_field = false;
	uint8_t zone = 0xff;
	card* to_check = nullptr;
	uint8_t toplayer = playerid;
	bool use_oppo = false;
	if(lua_isboolean(L, lastarg)) {
		use_hand = lua_get<bool>(L, lastarg);
		lastarg++;
		max = lua_get<uint16_t>(L, lastarg, min);
		lastarg++;
		check_field = lua_get<bool, false>(L, lastarg);
		lastarg++;
		to_check = lua_get<card*>(L, lastarg);
		lastarg++;
		toplayer = lua_get<uint8_t>(L, lastarg, playerid);
		lastarg++;
		zone = lua_get<uint32_t, 0xff>(L, lastarg);
		lastarg++;
		use_oppo = lua_get<bool, false>(L, lastarg);
		lastarg++;
	}
	if((pexception = lua_get<card*>(L, lastarg)) == nullptr)
		pexgroup = lua_get<group*>(L, lastarg);
	uint32_t extraargs = lua_gettop(L) - lastarg;
	int32_t result = pduel->game_field->check_release_list(playerid, min, max, use_con, use_hand, 2, extraargs, pexception, pexgroup, check_field, toplayer, zone, to_check, use_oppo);
	pduel->game_field->core.must_select_cards.clear();
	lua_pushboolean(L, result);
	return 1;

}
int32_t duel_check_release_group(lua_State* L) {
	return check_release_group(L, FALSE);
}
int32_t duel_check_release_group_ex(lua_State* L) {
	return check_release_group(L, TRUE);
}
static int32_t select_release_group(lua_State* L, uint8_t use_hand) {
	check_action_permission(L);
	check_param_count(L, 5);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	bool use_con = !lua_isnoneornil(L, 2) && check_param(L, PARAM_TYPE_FUNCTION, 2);
	card* pexception = 0;
	group* pexgroup = 0;
	bool cancelable = false;
	uint8_t lastarg = 5;
	bool check_field = false;
	card* to_check = nullptr;
	uint8_t toplayer = playerid;
	uint8_t zone = 0xff;
	bool use_oppo = false;
	if(lua_isboolean(L, lastarg)) {
		use_hand = lua_get<bool>(L, lastarg);
		lastarg++;
		cancelable = lua_get<bool, false>(L, lastarg);
		lastarg++;
		check_field = lua_get<bool, false>(L, lastarg);
		lastarg++;
		to_check = lua_get<card*>(L, lastarg);
		lastarg++;
		toplayer = lua_get<uint8_t>(L, lastarg, playerid);
		lastarg++;
		zone = lua_get<uint32_t, 0xff>(L, lastarg);
		lastarg++;
		use_oppo = lua_get<bool, false>(L, lastarg);
		lastarg++;
	}
	if((pexception = lua_get<card*>(L, lastarg)) == nullptr)
		pexgroup = lua_get<group*>(L, lastarg);
	uint32_t extraargs = lua_gettop(L) - lastarg;
	const auto pduel = lua_get<duel*>(L);
	auto min = lua_get<uint16_t>(L, 3);
	auto max = lua_get<uint16_t>(L, 4);
	pduel->game_field->core.release_cards.clear();
	pduel->game_field->core.release_cards_ex.clear();
	pduel->game_field->core.release_cards_ex_oneof.clear();
	pduel->game_field->get_release_list(playerid, &pduel->game_field->core.release_cards, &pduel->game_field->core.release_cards_ex, &pduel->game_field->core.release_cards_ex_oneof, use_con, use_hand, 2, extraargs, pexception, pexgroup, use_oppo);
	pduel->game_field->add_process(PROCESSOR_SELECT_RELEASE, 0, 0, (group*)to_check, playerid + (cancelable << 16), (max << 16) + min, check_field + (toplayer << 8) + (zone << 16));
	return lua_yieldk(L, 0, (lua_KContext)cancelable, push_return_cards);
}
int32_t duel_select_release_group(lua_State* L) {
	return select_release_group(L, false);
}
int32_t duel_select_release_group_ex(lua_State* L) {
	return select_release_group(L, true);
}
/**
* \brief Duel.GetTributeGroup
* \param targetcard
* \return Group
*/
int32_t duel_get_tribute_group(lua_State* L) {
	check_param_count(L, 1);
	auto target = lua_get<card*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group();
	pduel->game_field->get_summon_release_list(target, &(pgroup->container), &(pgroup->container), &(pgroup->container));
	interpreter::pushobject(L, pgroup);
	return 1;
}
/**
* \brief Duel.GetTributeCount
* \param targetcard
* \return Integer
*/
int32_t duel_get_tribute_count(lua_State* L) {
	check_param_count(L, 1);
	auto target = lua_get<card*, true>(L, 1);
	group* mg = 0;
	if(!lua_isnoneornil(L, 2))
		mg = lua_get<group*, true>(L, 2);
	bool ex = lua_get<bool, false>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->get_summon_release_list(target, NULL, NULL, NULL, mg, ex));
	return 1;
}
int32_t duel_check_tribute(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto target = lua_get<card*, true>(L, 1);
	auto min = lua_get<uint16_t>(L, 2);
	auto max = lua_get<uint16_t>(L, 3, min);
	group* mg = 0;
	if(!lua_isnoneornil(L, 4))
		mg = lua_get<group*, true>(L, 4);
	auto toplayer = lua_get<uint8_t>(L, 5, target->current.controler);
	auto zone = lua_get<uint32_t, 0x1f>(L, 6);
	lua_pushboolean(L, pduel->game_field->check_tribute(target, min, max, mg, toplayer, zone));
	return 1;
}
int32_t duel_select_tribute(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 4);
	auto target = lua_get<card*, true>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto min = lua_get<uint16_t>(L, 3);
	auto max = lua_get<uint16_t>(L, 4);
	group* mg = 0;
	if(!lua_isnoneornil(L, 5))
		mg = lua_get<group*, true>(L, 5);
	auto toplayer = lua_get<uint8_t>(L, 6, playerid);
	if(toplayer != 0 && toplayer != 1)
		return 0;
	uint32_t ex = FALSE;
	if(toplayer != playerid)
		ex = TRUE;
	auto zone = lua_get<uint32_t, 0x1f>(L, 7);
	auto cancelable = lua_get<bool, false>(L, 8);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.release_cards.clear();
	pduel->game_field->core.release_cards_ex.clear();
	pduel->game_field->core.release_cards_ex_oneof.clear();
	pduel->game_field->get_summon_release_list(target, &pduel->game_field->core.release_cards, &pduel->game_field->core.release_cards_ex, &pduel->game_field->core.release_cards_ex_oneof, mg, ex);
	pduel->game_field->select_tribute_cards(0, playerid, cancelable, min, max, toplayer, zone);
	return lua_yieldk(L, 0, (lua_KContext)cancelable, push_return_cards);
}
/**
* \brief Duel.GetTargetCount
* \param filter_func, self, location1, location2, exception card, (extraargs...)
* \return Group
*/
int32_t duel_get_target_count(lua_State* L) {
	check_param_count(L, 5);
	if(!lua_isnoneornil(L, 1))
		check_param(L, PARAM_TYPE_FUNCTION, 1);
	card* pexception = 0;
	group* pexgroup = 0;
	if((pexception = lua_get<card*>(L, 5)) == nullptr)
		pexgroup = lua_get<group*>(L, 5);
	uint32_t extraargs = lua_gettop(L) - 5;
	const auto pduel = lua_get<duel*>(L);
	auto self = lua_get<uint8_t>(L, 2);
	auto location1 = lua_get<uint16_t>(L, 3);
	auto location2 = lua_get<uint16_t>(L, 4);
	group* pgroup = pduel->new_group();
	uint32_t count = 0;
	pduel->game_field->filter_matching_card(1, self, location1, location2, pgroup, pexception, pexgroup, extraargs, 0, 0, TRUE);
	count = pgroup->container.size();
	lua_pushinteger(L, count);
	return 1;
}
/**
* \brief Duel.IsExistingTarget
* \param filter_func, self, location1, location2, count, exception card, (extraargs...)
* \return boolean
*/
int32_t duel_is_existing_target(lua_State* L) {
	check_param_count(L, 6);
	if(!lua_isnoneornil(L, 1))
		check_param(L, PARAM_TYPE_FUNCTION, 1);
	card* pexception = 0;
	group* pexgroup = 0;
	if((pexception = lua_get<card*>(L, 6)) == nullptr)
		pexgroup = lua_get<group*>(L, 6);
	uint32_t extraargs = lua_gettop(L) - 6;
	const auto pduel = lua_get<duel*>(L);
	auto self = lua_get<uint8_t>(L, 2);
	auto location1 = lua_get<uint16_t>(L, 3);
	auto location2 = lua_get<uint16_t>(L, 4);
	auto count = lua_get<uint16_t>(L, 5);
	lua_pushboolean(L, pduel->game_field->filter_matching_card(1, self, location1, location2, 0, pexception, pexgroup, extraargs, 0, count, TRUE));
	return 1;
}
/**
* \brief Duel.SelectTarget
* \param playerid, filter_func, self, location1, location2, min, max, exception card, (extraargs...)
* \return Group
*/
int32_t duel_select_target(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 8);
	if(!lua_isnoneornil(L, 2))
		check_param(L, PARAM_TYPE_FUNCTION, 2);
	card* pexception = 0;
	group* pexgroup = 0;
	bool cancelable = false;
	uint8_t lastarg = 8;
	if(lua_isboolean(L, lastarg)) {
		check_param_count(L, 9);
		cancelable = lua_get<bool, false>(L, lastarg);
		lastarg++;
	}
	if((pexception = lua_get<card*>(L, lastarg)) == nullptr)
		pexgroup = lua_get<group*>(L, lastarg);
	uint32_t extraargs = lua_gettop(L) - lastarg;
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	auto self = lua_get<uint8_t>(L, 3);
	auto location1 = lua_get<uint16_t>(L, 4);
	auto location2 = lua_get<uint16_t>(L, 5);
	auto min = lua_get<uint16_t>(L, 6);
	auto max = lua_get<uint16_t>(L, 7);
	if(pduel->game_field->core.current_chain.size() == 0)
		return 0;
	group* pgroup = pduel->new_group();
	pduel->game_field->filter_matching_card(2, self, location1, location2, pgroup, pexception, pexgroup, extraargs, 0, 0, TRUE);
	pduel->game_field->core.select_cards.assign(pgroup->container.begin(), pgroup->container.end());
	pduel->game_field->add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + (cancelable << 16), min + (max << 16));
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		if(pduel->game_field->return_cards.canceled) {
			lua_pushnil(L);
			return 1;
		}
		chain* ch = pduel->game_field->get_chain(0);
		if(ch) {
			if(!ch->target_cards) {
				ch->target_cards = pduel->new_group();
				ch->target_cards->is_readonly = TRUE;
			}
			ch->target_cards->container.insert(pduel->game_field->return_cards.list.begin(), pduel->game_field->return_cards.list.end());
			effect* peffect = ch->triggering_effect;
			if(peffect->type & EFFECT_TYPE_CONTINUOUS) {
				interpreter::pushobject(L, ch->target_cards);
			} else {
				group* pret = pduel->new_group(pduel->game_field->return_cards.list);
				for(auto& pcard : pret->container) {
					pcard->create_relation(*ch);
					if(peffect->is_flag(EFFECT_FLAG_CARD_TARGET)) {
						auto message = pduel->new_message(MSG_BECOME_TARGET);
						message->write<uint32_t>(1);
						message->write(pcard->get_info_location());
					}
				}
				interpreter::pushobject(L, pret);
			}
		}
		return 1;
	});
}
int32_t duel_select_fusion_material(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 3);
	group* cg = 0;
	if(auto _pcard = lua_get<card*>(L, 4))
		cg = pduel->new_group(_pcard);
	else
		cg = lua_get<group*>(L, 4);
	auto chkf = lua_get<uint32_t, PLAYER_NONE>(L, 5);
	pduel->game_field->add_process(PROCESSOR_SELECT_FUSION, 0, 0, (group*)pgroup, playerid + (chkf << 16), 0, 0, 0, cg, (card*)pcard);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		group* pgroup = pduel->new_group(pduel->game_field->core.fusion_materials);
		interpreter::pushobject(L, pgroup);
		return 1;
	});
}
int32_t duel_set_fusion_material(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pgroup = lua_get<group*, true>(L, 1);
	pduel->game_field->core.fusion_materials = pgroup->container;
	return 0;
}
int32_t duel_get_ritual_material(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	bool check_level = lua_get<bool, true>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group();
	pduel->game_field->get_ritual_material(playerid, pduel->game_field->core.reason_effect, &pgroup->container, check_level);
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t duel_release_ritual_material(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pgroup = lua_get<group*, true>(L, 1);
	pduel->game_field->ritual_release(&pgroup->container);
	return lua_yield(L, 0);
}
int32_t duel_get_fusion_material(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group();
	pduel->game_field->get_fusion_material(playerid, &pgroup->container);
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t duel_is_summon_cancelable(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->core.summon_cancelable);
	return 1;
}
int32_t duel_set_must_select_cards(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	if(auto pcard = lua_get<card*>(L, 1)) {
		pduel->game_field->core.must_select_cards = { pcard };
	} else if(auto pgroup = lua_get<group*>(L, 1)) {
		pduel->game_field->core.must_select_cards.assign(pgroup->container.begin(), pgroup->container.end());
	} else {
		luaL_error(L, "Parameter %d should be \"Card\" or \"Group\".", 1);
		unreachable();
	}
	return 0;
}
int32_t duel_grab_must_select_cards(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group(pduel->game_field->core.must_select_cards);
	pduel->game_field->core.must_select_cards.clear();
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t duel_set_target_card(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(0);
	if(!ch)
		return 0;
	if(!ch->target_cards) {
		ch->target_cards = pduel->new_group();
		ch->target_cards->is_readonly = TRUE;
	}
	group* targets = ch->target_cards;
	effect* peffect = ch->triggering_effect;
	if(peffect->type & EFFECT_TYPE_CONTINUOUS) {
		if(pcard)
			targets->container.insert(pcard);
		else
			targets->container = pgroup->container;
	} else {
		if(pcard) {
			targets->container.insert(pcard);
			pcard->create_relation(*ch);
		} else {
			targets->container.insert(pgroup->container.begin(), pgroup->container.end());
			for(auto& _pcard : pgroup->container)
				_pcard->create_relation(*ch);
		}
		if(peffect->is_flag(EFFECT_FLAG_CARD_TARGET)) {
			auto message = pduel->new_message(MSG_BECOME_TARGET);
			if(pcard) {
				message->write<uint32_t>(1);
				message->write(pcard->get_info_location());
			} else {
				message->write<uint32_t>(pgroup->container.size());
				for(auto& _pcard : pgroup->container)
					message->write(_pcard->get_info_location());
			}
		}
	}
	return 0;
}
int32_t duel_clear_target_card(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(0);
	if(ch && ch->target_cards)
		ch->target_cards->container.clear();
	return 0;
}
int32_t duel_set_target_player(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(0);
	if(ch)
		ch->target_player = playerid;
	return 0;
}
int32_t duel_set_target_param(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	auto param = lua_get<uint32_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(0);
	if(ch)
		ch->target_param = param;
	return 0;
}
/**
* \brief Duel.SetOperationInfo
* \param target_group, target_count, target_player, targ
* \return N/A
*/
int32_t duel_set_operation_info(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 6);
	const auto pduel = lua_get<duel*>(L);
	auto ct = lua_get<uint32_t>(L, 1);
	auto cate = lua_get<uint32_t>(L, 2);
	auto count = lua_get<uint8_t>(L, 4);
	auto playerid = lua_get<uint8_t>(L, 5);
	auto param = lua_get<int32_t>(L, 6);
	auto pobj = lua_get<lua_obj*>(L, 3);
	chain* ch = pduel->game_field->get_chain(ct);
	if(!ch)
		return 0;
	optarget opt{ nullptr, count, playerid, param };
	if(pobj && (pobj->lua_type & (PARAM_TYPE_CARD | PARAM_TYPE_GROUP))) {
		opt.op_cards = pduel->new_group(pobj);
		// spear creting and similar stuff, they set CATEGORY_SPECIAL_SUMMON with PLAYER_ALL to increase the summon counters
		// and the core always assume the group is exactly 2 cards
		if(cate == 0x200 && playerid == PLAYER_ALL && opt.op_cards->container.size() != 2) {
			luaL_error(L, "Called Duel.SetOperationInfo with CATEGORY_SPECIAL_SUMMON and PLAYER_ALL but the group size wasn't exactly 2.");
			unreachable();
		}
		opt.op_cards->is_readonly = TRUE;
	}
	auto omit = ch->opinfos.find(cate);
	if(omit != ch->opinfos.end() && omit->second.op_cards)
		pduel->delete_group(omit->second.op_cards);
	ch->opinfos[cate] = std::move(opt);
	return 0;
}
int32_t duel_get_operation_info(lua_State* L) {
	check_param_count(L, 2);
	auto ct = lua_get<uint32_t>(L, 1);
	auto cate = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(ct);
	if(!ch)
		return 0;
	auto oit = ch->opinfos.find(cate);
	if(oit != ch->opinfos.end()) {
		optarget& opt = oit->second;
		lua_pushboolean(L, 1);
		if(opt.op_cards)
			interpreter::pushobject(L, opt.op_cards);
		else
			lua_pushnil(L);
		lua_pushinteger(L, opt.op_count);
		lua_pushinteger(L, opt.op_player);
		lua_pushinteger(L, opt.op_param);
		return 5;
	} else {
		lua_pushboolean(L, 0);
		return 1;
	}
}
int32_t duel_set_possible_operation_info(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 6);
	const auto pduel = lua_get<duel*>(L);
	auto ct = lua_get<uint32_t>(L, 1);
	auto cate = lua_get<uint32_t>(L, 2);
	auto count = lua_get<uint8_t>(L, 4);
	auto playerid = lua_get<uint8_t>(L, 5);
	auto param = lua_get<int32_t>(L, 6);
	auto pobj = lua_get<lua_obj*>(L, 3);
	chain* ch = pduel->game_field->get_chain(ct);
	if(!ch)
		return 0;
	optarget opt{ nullptr, count, playerid, param };
	if(pobj && (pobj->lua_type & (PARAM_TYPE_CARD | PARAM_TYPE_GROUP))) {
		opt.op_cards = pduel->new_group(pobj);
		opt.op_cards->is_readonly = TRUE;
	}
	auto omit = ch->possibleopinfos.find(cate);
	if(omit != ch->possibleopinfos.end() && omit->second.op_cards)
		pduel->delete_group(omit->second.op_cards);
	ch->possibleopinfos[cate] = std::move(opt);
	return 0;
}
int32_t duel_get_possible_operation_info(lua_State* L) {
	check_param_count(L, 2);
	auto ct = lua_get<uint32_t>(L, 1);
	auto cate = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(ct);
	if(!ch)
		return 0;
	auto oit = ch->possibleopinfos.find(cate);
	if(oit != ch->possibleopinfos.end()) {
		optarget& opt = oit->second;
		lua_pushboolean(L, 1);
		if(opt.op_cards)
			interpreter::pushobject(L, opt.op_cards);
		else
			lua_pushnil(L);
		lua_pushinteger(L, opt.op_count);
		lua_pushinteger(L, opt.op_player);
		lua_pushinteger(L, opt.op_param);
		return 5;
	} else {
		lua_pushboolean(L, 0);
		return 1;
	}
}
int32_t duel_get_operation_count(lua_State* L) {
	check_param_count(L, 1);
	auto ct = lua_get<uint32_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(ct);
	if(!ch)
		return 0;
	lua_pushinteger(L, ch->opinfos.size());
	return 1;
}
int32_t duel_clear_operation_info(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	auto ct = lua_get<uint32_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(ct);
	if(!ch)
		return 0;
	for(auto& oit : ch->opinfos) {
		if(oit.second.op_cards)
			pduel->delete_group(oit.second.op_cards);
	}
	ch->opinfos.clear();
	for(auto& oit : ch->possibleopinfos) {
		if(oit.second.op_cards)
			pduel->delete_group(oit.second.op_cards);
	}
	ch->possibleopinfos.clear();
	return 0;
}
int32_t duel_overlay(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto target = lua_get<card*, true>(L, 1);
	if(target->overlay_target != nullptr) {
		luaL_error(L, "Attempt to overlay materials to a card that is an overlay material.");
		unreachable();
	}
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 2, pcard, pgroup);
	if(pcard) {
		if(pcard == target) {
			luaL_error(L, "Attempt to overlay a card with itself.");
			unreachable();
		}
		target->xyz_overlay(card_set{ pcard });
	} else {
		if(pgroup->has_card(target)) {
			luaL_error(L, "Attempt to overlay a card with itself.");
			unreachable();
		}
		target->xyz_overlay(pgroup->container);
	}
	if(target->current.location & LOCATION_ONFIELD)
		pduel->game_field->adjust_all();
	return lua_yield(L, 0);
}
int32_t duel_get_overlay_group(lua_State* L) {
	check_param_count(L, 3);
	auto rplayer = lua_get<uint8_t>(L, 1);
	if(rplayer != 0 && rplayer != 1)
		return 0;
	auto self = lua_get<uint8_t>(L, 2);
	auto oppo = lua_get<uint8_t>(L, 3);
	group* targetsgroup = lua_get<group*>(L, 4);
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group();
	pduel->game_field->get_overlay_group(rplayer, self, oppo, &pgroup->container, targetsgroup);
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t duel_get_overlay_count(lua_State* L) {
	check_param_count(L, 3);
	auto rplayer = lua_get<uint8_t>(L, 1);
	if(rplayer != 0 && rplayer != 1)
		return 0;
	auto self = lua_get<uint8_t>(L, 2);
	auto oppo = lua_get<uint8_t>(L, 3);
	group* pgroup = lua_get<group*>(L, 4);
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->get_overlay_count(rplayer, self, oppo, pgroup));
	return 1;
}
int32_t duel_check_remove_overlay_card(lua_State* L) {
	check_param_count(L, 5);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto self = lua_get<uint8_t>(L, 2);
	auto oppo = lua_get<uint8_t>(L, 3);
	auto count = lua_get<uint16_t>(L, 4);
	auto reason = lua_get<uint32_t>(L, 5);
	group* pgroup = lua_get<group*>(L, 6);
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->is_player_can_remove_overlay_card(playerid, pgroup, self, oppo, count, reason));
	return 1;
}
int32_t duel_remove_overlay_card(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 6);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto self = lua_get<uint8_t>(L, 2);
	auto oppo = lua_get<uint8_t>(L, 3);
	auto min = lua_get<uint16_t>(L, 4);
	auto max = lua_get<uint16_t>(L, 5);
	auto reason = lua_get<uint32_t>(L, 6);
	group* pgroup = lua_get<group*>(L, 7);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->remove_overlay_card(reason, pgroup, playerid, self, oppo, min, max);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_hint(lua_State* L) {
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto htype = lua_get<uint8_t>(L, 1);
	auto desc = lua_get<uint64_t>(L, 3);
	if(htype == HINT_OPSELECTED)
		playerid = 1 - playerid;
	const auto pduel = lua_get<duel*>(L);
	auto message = pduel->new_message(MSG_HINT);
	message->write<uint8_t>(htype);
	message->write<uint8_t>(playerid);
	message->write<uint64_t>(desc);
	return 0;
}
int32_t duel_hint_selection(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	bool selection = lua_get<bool, false>(L, 2);
	auto message = pduel->new_message(selection ? MSG_CARD_SELECTED : MSG_BECOME_TARGET);
	if(pcard) {
		message->write<uint32_t>(1);
		message->write(pcard->get_info_location());
	} else {
		message->write<uint32_t>(pgroup->container.size());
		for(auto& pcard : pgroup->container) {
			message->write(pcard->get_info_location());
		}
	}
	return 0;
}
int32_t duel_select_effect_yesno(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto desc = lua_get<uint64_t, 95>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_SELECT_EFFECTYN, 0, 0, (group*)pcard, playerid, desc);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushboolean(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_select_yesno(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto desc = lua_get<uint64_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, playerid, desc);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushboolean(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_select_option(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	uint32_t count = lua_gettop(L) - 1;
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	uint8_t i = lua_isboolean(L, 2);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.select_options.clear();
	for(; i < count; ++i)
		pduel->game_field->core.select_options.push_back(lua_get<uint64_t>(L, i + 2));
	pduel->game_field->add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, playerid, 0);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		auto playerid = lua_get<uint8_t>(L, 1);
		bool sel_hint = lua_get<bool, true>(L, 2);
		if(sel_hint && !pduel->game_field->core.select_options.empty()) {
			auto message = pduel->new_message(MSG_HINT);
			message->write<uint8_t>(HINT_OPSELECTED);
			message->write<uint8_t>(playerid);
			message->write<uint64_t>(pduel->game_field->core.select_options[pduel->game_field->returns.at<int32_t>(0)]);
		}
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_select_position(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto pcard = lua_get<card*, true>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto positions = lua_get<uint8_t>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_SELECT_POSITION, 0, 0, 0, playerid + (positions << 16), pcard->data.code);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_select_disable_field(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 5);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto count = lua_get<uint8_t>(L, 2);
	auto location1 = lua_get<uint16_t>(L, 3);
	auto location2 = lua_get<uint16_t>(L, 4);
	const auto pduel = lua_get<duel*>(L);
	bool all_field = lua_get<bool, false>(L, 6);
	uint32_t filter = 0xe0e0e0e0;
	if(all_field){
		filter = pduel->game_field->is_flag(DUEL_EMZONE) ? 0x800080 : 0xE000E0;
		filter |= pduel->game_field->get_pzone_zones_flag();
	}
	filter |= lua_get<uint32_t>(L, 5, filter);
	uint32_t ct1 = 0, ct2 = 0, ct3 = 0, ct4 = 0, plist = 0, flag = 0xffffffff;
	if(location1 & LOCATION_MZONE) {
		ct1 = pduel->game_field->get_useable_count(NULL, playerid, LOCATION_MZONE, PLAYER_NONE, 0, 0xff, &plist);
		if (all_field) {
			plist &= ~0x60;
			if (!pduel->game_field->is_location_useable(playerid, LOCATION_MZONE, 5))
				plist |= 0x20;
			else
				ct1++;
			if (!pduel->game_field->is_location_useable(playerid, LOCATION_MZONE, 6))
				plist |= 0x40;
			else
				ct1++;
		}
		flag = (flag & 0xffffff00) | plist;
	}
	if(location1 & LOCATION_SZONE) {
		ct2 = pduel->game_field->get_useable_count(NULL, playerid, LOCATION_SZONE, PLAYER_NONE, 0, 0xff, &plist);
		if (all_field) {
			plist &= ~0xe0;
			if (!pduel->game_field->is_location_useable(playerid, LOCATION_SZONE, 5))
				plist |= 0x20;
			else
				ct2++;
			if (!pduel->game_field->is_location_useable(playerid, LOCATION_SZONE, 6))
				plist |= 0x40;
			else
				ct2++;
			if (!pduel->game_field->is_location_useable(playerid, LOCATION_SZONE, 7))
				plist |= 0x80;
			else
				ct2++;
		}
		flag = (flag & 0xffff00ff) | (plist << 8);
	}
	if(location2 & LOCATION_MZONE) {
		ct3 = pduel->game_field->get_useable_count(NULL, 1 - playerid, LOCATION_MZONE, PLAYER_NONE, 0, 0xff, &plist);
		if (all_field) {
			plist &= ~0x60;
			if (!pduel->game_field->is_location_useable(1 - playerid, LOCATION_MZONE, 5))
				plist |= 0x20;
			else
				ct3++;
			if (!pduel->game_field->is_location_useable(1 - playerid, LOCATION_MZONE, 6))
				plist |= 0x40;
			else
				ct3++;
		}
		flag = (flag & 0xff00ffff) | (plist << 16);
	}
	if(location2 & LOCATION_SZONE) {
		ct4 = pduel->game_field->get_useable_count(NULL, 1 - playerid, LOCATION_SZONE, PLAYER_NONE, 0, 0xff, &plist);
		if (all_field) {
			plist &= ~0xe0;
			if (!pduel->game_field->is_location_useable(1 - playerid, LOCATION_SZONE, 5))
				plist |= 0x20;
			else
				ct4++;
			if (!pduel->game_field->is_location_useable(1 - playerid, LOCATION_SZONE, 6))
				plist |= 0x40;
			else
				ct4++;
			if (!pduel->game_field->is_location_useable(1 - playerid, LOCATION_SZONE, 7))
				plist |= 0x80;
			else
				ct4++;
		}
		flag = (flag & 0xffffff) | (plist << 24);
	}
	flag |= filter | ((all_field) ? 0x800080 : 0xe0e0e0e0);
	if(count > ct1 + ct2 + ct3 + ct4)
		count = ct1 + ct2 + ct3 + ct4;
	if(count == 0)
		return 0;
	pduel->game_field->add_process(PROCESSOR_SELECT_DISFIELD, 0, 0, 0, playerid, flag, count);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		auto playerid = lua_get<uint8_t>(L, 1);
		auto count = lua_get<uint8_t>(L, 2);
		uint32_t dfflag = 0;
		uint8_t pa = 0;
		for(uint32_t i = 0; i < count; ++i) {
			uint8_t p = pduel->game_field->returns.at<int8_t>(pa);
			uint8_t l = pduel->game_field->returns.at<int8_t>(pa + 1);
			uint8_t s = pduel->game_field->returns.at<int8_t>(pa + 2);
			dfflag |= 0x1u << (s + (p == playerid ? 0 : 16) + (l == LOCATION_MZONE ? 0 : 8));
			pa += 3;
		}
		lua_pushinteger(L, dfflag);
		return 1;
	});
}
int32_t duel_select_field_zone(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 4);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto count = lua_get<uint8_t>(L, 2);
	auto location1 = lua_get<uint16_t>(L, 3);
	auto location2 = lua_get<uint16_t>(L, 4);
	const auto pduel = lua_get<duel*>(L);
	auto filter = lua_get<uint32_t, 0xe0e0e0e0>(L, 5);
	filter |= pduel->game_field->is_flag(DUEL_EMZONE) ? 0x800080 : 0xE000E0;
	filter |= pduel->game_field->get_pzone_zones_flag();
	uint32_t flag = 0xffffffff;
	if(location1 & LOCATION_MZONE)
		flag &= 0xffffff00;
	if(location1 & LOCATION_SZONE)
		flag &= 0xffff00ff;
	if(location2 & LOCATION_MZONE)
		flag &= 0xff00ffff;
	if(location2 & LOCATION_SZONE)
		flag &= 0xffffff;
	flag |= filter;
	if (flag == 0xffffffff)
		return 0;
	pduel->game_field->add_process(PROCESSOR_SELECT_DISFIELD, 0, 0, 0, playerid, flag, count);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		auto playerid = lua_get<uint8_t>(L, 1);
		auto count = lua_get<uint8_t>(L, 2);
		uint32_t dfflag = 0;
		uint8_t pa = 0;
		for(uint32_t i = 0; i < count; ++i) {
			uint8_t p = pduel->game_field->returns.at<int8_t>(pa);
			uint8_t l = pduel->game_field->returns.at<int8_t>(pa + 1);
			uint8_t s = pduel->game_field->returns.at<int8_t>(pa + 2);
			dfflag |= 0x1u << (s + (p == playerid ? 0 : 16) + (l == LOCATION_MZONE ? 0 : 8));
			pa += 3;
		}
		lua_pushinteger(L, dfflag);
		return 1;
	});
}
int32_t duel_announce_race(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto count = lua_get<uint16_t>(L, 2);
	auto available = lua_get<uint32_t>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_ANNOUNCE_RACE, 0, 0, 0, playerid + (count << 16), available);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_announce_attribute(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto count = lua_get<uint16_t>(L, 2);
	auto available = lua_get<uint32_t>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_ANNOUNCE_ATTRIB, 0, 0, 0, playerid + (count << 16), available);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_announce_level(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto min = lua_get<uint32_t, 1>(L, 2);
	auto max = lua_get<uint32_t, 12>(L, 3);
	auto paramcount = lua_gettop(L);
	if(min > max)
		std::swap(min, max);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.select_options.clear();
	if(paramcount > 3) {
		std::set<uint32_t> excluded;
		for(int32_t j = 4; j <= paramcount; ++j)
			excluded.insert(lua_get<uint32_t>(L, j));
		auto it = excluded.begin();
		while(it != excluded.end() && *it < min) it++;
		for(uint32_t i = min; i <= max; ++i) {
			if(it != excluded.end() && *it == i) {
				it++;
				continue;
			}
			pduel->game_field->core.select_options.push_back(i);
		}
	} else {
		for(uint32_t i = min; i <= max; ++i)
			pduel->game_field->core.select_options.push_back(i);
	}
	if(pduel->game_field->core.select_options.empty())
		return 0;
	pduel->game_field->add_process(PROCESSOR_ANNOUNCE_NUMBER, 0, 0, 0, playerid, 0);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->core.select_options[pduel->game_field->returns.at<int32_t>(0)]);
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 2;
	});
}
int32_t duel_announce_card(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.select_options.clear();
	auto paramcount = lua_gettop(L);
	if(lua_istable(L, 2)) {
		lua_table_iterate(L, 2, [&options = pduel->game_field->core.select_options, L] {
			options.push_back(lua_get<uint64_t>(L, -1));
		});
	} else if(paramcount <= 2) {
		uint32_t ttype = TYPE_MONSTER | TYPE_SPELL | TYPE_TRAP;
		if(paramcount == 2)
			ttype = lua_get<uint32_t>(L, 2);
		pduel->game_field->core.select_options.push_back(ttype);
		pduel->game_field->core.select_options.push_back(OPCODE_ISTYPE);
	} else {
		for(int32_t i = 2; i <= paramcount; ++i)
			pduel->game_field->core.select_options.push_back(lua_get<uint64_t>(L, i));
	}
	int32_t stack_size = 0;
	bool has_opcodes = false;
	for(auto& it : pduel->game_field->core.select_options) {
		if(it != OPCODE_ALLOW_ALIASES && it != OPCODE_ALLOW_TOKENS)
			has_opcodes = true;
		switch(it) {
		case OPCODE_ADD:
		case OPCODE_SUB:
		case OPCODE_MUL:
		case OPCODE_DIV:
		case OPCODE_AND:
		case OPCODE_OR:
		case OPCODE_BAND:
		case OPCODE_BOR:
		case OPCODE_BXOR:
		case OPCODE_LSHIFT:
		case OPCODE_RSHIFT:
			stack_size -= 1;
			break;
		case OPCODE_NEG:
		case OPCODE_NOT:
		case OPCODE_BNOT:
		case OPCODE_ISCODE:
		case OPCODE_ISSETCARD:
		case OPCODE_ISTYPE:
		case OPCODE_ISRACE:
		case OPCODE_ISATTRIBUTE:
		case OPCODE_ALLOW_ALIASES:
		case OPCODE_ALLOW_TOKENS:
			break;
		default:
			stack_size += 1;
			break;
		}
		if(stack_size <= 0)
			break;
	}
	if(stack_size != 1 && has_opcodes) {
		luaL_error(L, "Parameters are invalid.");
		unreachable();
	}
	if(!has_opcodes) {
		pduel->game_field->core.select_options.push_back(TYPE_MONSTER | TYPE_SPELL | TYPE_TRAP);
		pduel->game_field->core.select_options.push_back(OPCODE_ISTYPE);
	}
	pduel->game_field->add_process(PROCESSOR_ANNOUNCE_CARD, 0, 0, 0, playerid, 0);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_announce_type(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 1);
	pduel->game_field->core.select_options.clear();
	pduel->game_field->core.select_options.push_back(70);
	pduel->game_field->core.select_options.push_back(71);
	pduel->game_field->core.select_options.push_back(72);
	pduel->game_field->add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, playerid, 0);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		auto playerid = lua_get<uint8_t>(L, 1);
		auto message = pduel->new_message(MSG_HINT);
		message->write<uint8_t>(HINT_OPSELECTED);
		message->write<uint8_t>(playerid);
		message->write<uint64_t>(pduel->game_field->core.select_options[pduel->game_field->returns.at<int32_t>(0)]);
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_announce_number(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.select_options.clear();
	for(int32_t i = 2, tot = lua_gettop(L); i <= tot; ++i)
		pduel->game_field->core.select_options.push_back(lua_get<uint64_t>(L, i));
	pduel->game_field->add_process(PROCESSOR_ANNOUNCE_NUMBER, 0, 0, 0, playerid, 0);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->core.select_options[pduel->game_field->returns.at<int32_t>(0)]);
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 2;
	});
}
int32_t duel_announce_coin(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 1);
	pduel->game_field->core.select_options.clear();
	pduel->game_field->core.select_options.push_back(60);
	pduel->game_field->core.select_options.push_back(61);
	pduel->game_field->add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, playerid, 0);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		auto playerid = lua_get<uint8_t>(L, 1);
		auto message = pduel->new_message(MSG_HINT);
		message->write<uint8_t>(HINT_OPSELECTED);
		message->write<uint8_t>(playerid);
		message->write<uint64_t>(pduel->game_field->core.select_options[pduel->game_field->returns.at<int32_t>(0)]);
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_toss_coin(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto count = lua_get<uint8_t>(L, 2);
	if((playerid != 0 && playerid != 1) || count <= 0)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	if(count > 5)
		count = 5;
	pduel->game_field->add_process(PROCESSOR_TOSS_COIN, 0, pduel->game_field->core.reason_effect, 0, (pduel->game_field->core.reason_player << 16) + playerid, count);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		int32_t count = lua_get<uint8_t>(L, 2);
		for(int32_t i = 0; i < count; ++i)
			lua_pushinteger(L, pduel->game_field->core.coin_result[i]);
		return count;
	});
}
int32_t duel_toss_dice(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto count1 = lua_get<uint8_t>(L, 2);
	if((playerid != 0 && playerid != 1) || count1 <= 0)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	auto count2 = lua_get<uint8_t, 0>(L, 3);
	if(count1 > 5)
		count1 = 5;
	if(count2 > 5 - count1)
		count2 = 5 - count1;
	pduel->game_field->add_process(PROCESSOR_TOSS_DICE, 0, pduel->game_field->core.reason_effect, 0, (pduel->game_field->core.reason_player << 16) + playerid, count1 + (count2 << 16));
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		auto count1 = lua_get<uint8_t>(L, 2);
		auto count2 = lua_get<uint8_t, 0>(L, 3);
		for(int32_t i = 0; i < count1 + count2; ++i)
			lua_pushinteger(L, pduel->game_field->core.dice_result[i]);
		return count1 + count2;
	});
}
int32_t duel_rock_paper_scissors(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	uint8_t repeat = lua_get<bool, true>(L, 1);
	pduel->game_field->add_process(PROCESSOR_ROCK_PAPER_SCISSORS, 0, 0, 0, repeat, 0);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
int32_t duel_get_coin_result(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	for(const auto& coin : pduel->game_field->core.coin_result)
		lua_pushinteger(L, coin);
	return 5;
}
int32_t duel_get_dice_result(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	for(const auto& dice : pduel->game_field->core.dice_result)
		lua_pushinteger(L, dice);
	return 5;
}
int32_t duel_set_coin_result(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	for(int32_t i = 0; i < 5; ++i) {
		auto res = lua_get<uint8_t>(L, i + 1);
		if(res != 0 && res != 1)
			res = 0;
		pduel->game_field->core.coin_result[i] = res;
	}
	return 0;
}
int32_t duel_set_dice_result(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	for(int32_t i = 0; i < 5; ++i) {
		auto res = lua_get<uint8_t>(L, i + 1);
		pduel->game_field->core.dice_result[i] = res;
	}
	return 0;
}
int32_t duel_is_duel_type(lua_State* L) {
	check_param_count(L, 1);
	auto duel_type = lua_get<uint64_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->is_flag(duel_type));
	return 1;
}
int32_t duel_is_player_affected_by_effect(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushnil(L);
		return 1;
	}
	auto code = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	effect* peffect = pduel->game_field->is_player_affected_by_effect(playerid, code);
	interpreter::pushobject(L, peffect);
	return 1;
}
int32_t duel_get_player_effect(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushnil(L);
		return 1;
	}
	auto code = lua_get<uint32_t, 0>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	int32_t count = pduel->game_field->get_player_effect(playerid, code);
	if (count==0) {
		lua_pushnil(L);
		return 1;
	}
	return count;
}
int32_t duel_is_player_can_draw(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	auto count = lua_get<uint32_t, 0>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	if(count == 0)
		lua_pushboolean(L, pduel->game_field->is_player_can_draw(playerid));
	else
		lua_pushboolean(L, pduel->game_field->is_player_can_draw(playerid)
		                && (pduel->game_field->player[playerid].list_main.size() >= count));
	return 1;
}
int32_t duel_is_player_can_discard_deck(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto count = lua_get<uint32_t>(L, 2);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->is_player_can_discard_deck(playerid, count));
	return 1;
}
int32_t duel_is_player_can_discard_deck_as_cost(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto count = lua_get<uint32_t>(L, 2);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->is_player_can_discard_deck_as_cost(playerid, count));
	return 1;
}
int32_t duel_is_player_can_summon(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	if(lua_gettop(L) == 1)
		lua_pushboolean(L, pduel->game_field->is_player_can_action(playerid, EFFECT_CANNOT_SUMMON));
	else {
		check_param_count(L, 3);
		auto pcard = lua_get<card*, true>(L, 3);
		auto sumtype = lua_get<uint32_t>(L, 2);
		lua_pushboolean(L, pduel->game_field->is_player_can_summon(sumtype, playerid, pcard, playerid));
	}
	return 1;
}
int32_t duel_can_player_set_monster(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	if(lua_gettop(L) == 1)
		lua_pushboolean(L, pduel->game_field->is_player_can_action(playerid, EFFECT_CANNOT_MSET));
	else {
		check_param_count(L, 3);
		auto pcard = lua_get<card*, true>(L, 3);
		auto sumtype = lua_get<uint32_t>(L, 2);
		lua_pushboolean(L, pduel->game_field->is_player_can_mset(sumtype, playerid, pcard, playerid));
	}
	return 1;
}
int32_t duel_can_player_set_spell_trap(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	if(lua_gettop(L) == 1)
		lua_pushboolean(L, pduel->game_field->is_player_can_action(playerid, EFFECT_CANNOT_SSET));
	else {
		check_param_count(L, 2);
		auto pcard = lua_get<card*, true>(L, 2);
		lua_pushboolean(L, pduel->game_field->is_player_can_sset(playerid, pcard));
	}
	return 1;
}
int32_t duel_is_player_can_spsummon(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	if(lua_gettop(L) == 1)
		lua_pushboolean(L, pduel->game_field->is_player_can_spsummon(playerid));
	else {
		check_param_count(L, 5);
		auto pcard = lua_get<card*, true>(L, 5);
		auto sumtype = lua_get<uint32_t>(L, 2);
		auto sumpos = lua_get<uint8_t>(L, 3);
		auto toplayer = lua_get<uint8_t>(L, 4);
		lua_pushboolean(L, pduel->game_field->is_player_can_spsummon(pduel->game_field->core.reason_effect, sumtype, sumpos, playerid, toplayer, pcard));
	}
	return 1;
}
int32_t duel_is_player_can_flipsummon(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	if(lua_gettop(L) == 1)
		lua_pushboolean(L, pduel->game_field->is_player_can_action(playerid, EFFECT_CANNOT_FLIP_SUMMON));
	else {
		check_param_count(L, 2);
		auto pcard = lua_get<card*, true>(L, 2);
		lua_pushboolean(L, pduel->game_field->is_player_can_flipsummon(playerid, pcard));
	}
	return 1;
}
int32_t duel_is_player_can_spsummon_monster(lua_State* L) {
	check_param_count(L, 9);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	auto code = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	card_data dat = *pduel->read_card(code);
	dat.code = code;
	dat.alias = 0;
	if(!lua_isnoneornil(L, 3)) {
		if(lua_istable(L, 3)) {
			lua_table_iterate(L, 3, [&setcodes = dat.setcodes, &L] {
				setcodes.insert(lua_get<uint16_t>(L, -1));
			});
		} else
			dat.setcodes.insert(lua_get<uint16_t>(L, 3));
	}
	if(!lua_isnoneornil(L, 4))
		dat.type = lua_get<uint32_t>(L, 4);
	if(!lua_isnoneornil(L, 5))
		dat.attack = lua_get<int32_t>(L, 5);
	if(!lua_isnoneornil(L, 6))
		dat.defense = lua_get<int32_t>(L, 6);
	if(!lua_isnoneornil(L, 7))
		dat.level = lua_get<uint32_t>(L, 7);
	if(!lua_isnoneornil(L, 8))
		dat.race = lua_get<uint32_t>(L, 8);
	if(!lua_isnoneornil(L, 9))
		dat.attribute = lua_get<uint32_t>(L, 9);
	auto pos = lua_get<uint8_t, POS_FACEUP>(L, 10);
	auto toplayer = lua_get<uint8_t>(L, 11, playerid);
	auto sumtype = lua_get<uint32_t, 0>(L, 12);
	lua_pushboolean(L, pduel->game_field->is_player_can_spsummon_monster(playerid, toplayer, pos, sumtype, &dat));
	return 1;
}
int32_t duel_is_player_can_spsummon_count(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto count = lua_get<uint32_t>(L, 2);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->is_player_can_spsummon_count(playerid, count));
	return 1;
}
int32_t duel_is_player_can_release(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	if(lua_gettop(L) == 1)
		lua_pushboolean(L, pduel->game_field->is_player_can_action(playerid, EFFECT_CANNOT_RELEASE));
	else {
		check_param_count(L, 2);
		auto pcard = lua_get<card*, true>(L, 2);
		lua_pushboolean(L, pduel->game_field->is_player_can_release(playerid, pcard));
	}
	return 1;
}
int32_t duel_is_player_can_remove(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	if(lua_gettop(L) == 1)
		lua_pushboolean(L, pduel->game_field->is_player_can_action(playerid, EFFECT_CANNOT_REMOVE));
	else {
		check_param_count(L, 2);
		auto pcard = lua_get<card*, true>(L, 2);
		auto reason = lua_get<uint32_t, REASON_EFFECT>(L, 3);
		lua_pushboolean(L, pduel->game_field->is_player_can_remove(playerid, pcard, reason));
	}
	return 1;
}
int32_t duel_is_player_can_send_to_hand(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	if(lua_gettop(L) == 1)
		lua_pushboolean(L, pduel->game_field->is_player_can_action(playerid, EFFECT_CANNOT_TO_HAND));
	else {
		check_param_count(L, 2);
		auto pcard = lua_get<card*, true>(L, 2);
		lua_pushboolean(L, pduel->game_field->is_player_can_send_to_hand(playerid, pcard));
	}
	return 1;
}
int32_t duel_is_player_can_send_to_grave(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	if(lua_gettop(L) == 1)
		lua_pushboolean(L, pduel->game_field->is_player_can_action(playerid, EFFECT_CANNOT_TO_GRAVE));
	else {
		check_param_count(L, 2);
		auto pcard = lua_get<card*, true>(L, 2);
		lua_pushboolean(L, pduel->game_field->is_player_can_send_to_grave(playerid, pcard));
	}
	return 1;
}
int32_t duel_is_player_can_send_to_deck(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	if(lua_gettop(L) == 1)
		lua_pushboolean(L, pduel->game_field->is_player_can_action(playerid, EFFECT_CANNOT_TO_DECK));
	else {
		check_param_count(L, 2);
		auto pcard = lua_get<card*, true>(L, 2);
		lua_pushboolean(L, pduel->game_field->is_player_can_send_to_deck(playerid, pcard));
	}
	return 1;
}
int32_t duel_is_player_can_additional_summon(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->core.extra_summon[playerid] == 0);
	return 1;
}
inline int32_t is_player_can_procedure_summon_group(lua_State* L, uint32_t summon_type, uint32_t offset) {
	(void)offset;
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	const auto playerid = lua_get<uint8_t>(L, 1);
	effect_set eset;
	pduel->game_field->filter_field_effect(EFFECT_SPSUMMON_PROC_G, &eset);
	for(const auto& peff : eset) {
		if(peff->get_value() != summon_type)
			continue;
		card* pcard = peff->get_handler();
		if(pcard->current.controler != playerid && !peff->is_flag(EFFECT_FLAG_BOTH_SIDE))
			continue;
		auto& core = pduel->game_field->core;
		effect* oreason = core.reason_effect;
		uint8_t op = core.reason_player;
		core.reason_effect = peff;
		core.reason_player = pcard->current.controler;
		pduel->game_field->save_lp_cost();
		pduel->lua->add_param(peff, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(TRUE, PARAM_TYPE_BOOLEAN);
		pduel->lua->add_param(oreason, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(pduel->lua->check_condition(peff->condition, 5)) {
			pduel->game_field->restore_lp_cost();
			core.reason_effect = oreason;
			core.reason_player = op;
			lua_pushboolean(L, TRUE);
			return 1;
		}
		pduel->game_field->restore_lp_cost();
		core.reason_effect = oreason;
		core.reason_player = op;
	}
	lua_pushboolean(L, FALSE);
	return 1;
}
int32_t duel_is_player_can_pendulum_summon(lua_State* L) {
	return is_player_can_procedure_summon_group(L, SUMMON_TYPE_PENDULUM, 0);
}
int32_t duel_is_player_can_procedure_summon_group(lua_State* L) {
	check_param_count(L, 2);
	auto sumtype = lua_get<uint32_t>(L, 2);
	return is_player_can_procedure_summon_group(L, sumtype, 1);
}
int32_t duel_is_chain_negatable(lua_State* L) {
	check_param_count(L, 1);
	lua_pushboolean(L, 1);
	return 1;
}
int32_t duel_is_chain_disablable(lua_State* L) {
	check_param_count(L, 1);
	auto chaincount = lua_get<uint8_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	if(pduel->game_field->core.chain_solving) {
		lua_pushboolean(L, pduel->game_field->is_chain_disablable(chaincount));
		return 1;
	}
	lua_pushboolean(L, 1);
	return 1;
}
int32_t duel_check_chain_target(lua_State* L) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto chaincount = lua_get<uint8_t>(L, 1);
	auto pcard = lua_get<card*, true>(L, 2);
	lua_pushboolean(L, pduel->game_field->check_chain_target(chaincount, pcard));
	return 1;
}
int32_t duel_check_chain_uniqueness(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	if(pduel->game_field->core.current_chain.size() == 0) {
		lua_pushboolean(L, 1);
		return 1;
	}
	std::set<uint32_t> er;
	for(const auto& ch : pduel->game_field->core.current_chain)
		er.insert(ch.triggering_effect->get_handler()->get_code());
	lua_pushboolean(L, er.size() == pduel->game_field->core.current_chain.size());
	return 1;
}
int32_t duel_get_activity_count(lua_State* L) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	int32_t retct = lua_gettop(L) - 1;
	for(int32_t i = 0; i < retct; ++i) {
		auto activity_type = static_cast<ActivityType>(lua_get<uint8_t>(L, 2 + i));
		switch(activity_type) {
			case ACTIVITY_SUMMON:
				lua_pushinteger(L, pduel->game_field->core.summon_state_count[playerid]);
				break;
			case ACTIVITY_NORMALSUMMON:
				lua_pushinteger(L, pduel->game_field->core.normalsummon_state_count[playerid]);
				break;
			case ACTIVITY_SPSUMMON:
				lua_pushinteger(L, pduel->game_field->core.spsummon_state_count[playerid]);
				break;
			case ACTIVITY_FLIPSUMMON:
				lua_pushinteger(L, pduel->game_field->core.flipsummon_state_count[playerid]);
				break;
			case ACTIVITY_ATTACK:
				lua_pushinteger(L, pduel->game_field->core.attack_state_count[playerid]);
				break;
			case ACTIVITY_BATTLE_PHASE:
				lua_pushinteger(L, pduel->game_field->core.battle_phase_count[playerid]);
				break;
			default:
				lua_pushinteger(L, 0);
				break;
		}
	}
	return retct;
}
int32_t duel_check_phase_activity(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->core.phase_action);
	return 1;
}
int32_t duel_add_custom_activity_counter(lua_State* L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_FUNCTION, 3);
	auto counter_id = lua_get<uint32_t>(L, 1);
	auto activity_type = static_cast<ActivityType>(lua_get<uint8_t>(L, 2));
	int32_t counter_filter = interpreter::get_function_handle(L, 3);
	const auto pduel = lua_get<duel*>(L);
	auto& counter_map = [&, &core = pduel->game_field->core]()->processor::action_counter_t& {
		switch(activity_type) {
		case ACTIVITY_SUMMON:
			return core.summon_counter;
		case ACTIVITY_NORMALSUMMON:
			return core.normalsummon_counter;
		case ACTIVITY_SPSUMMON:
			return core.spsummon_counter;
		case ACTIVITY_FLIPSUMMON:
			return core.flipsummon_counter;
		case ACTIVITY_ATTACK:
			return core.attack_counter;
		case ACTIVITY_CHAIN:
			return core.chain_counter;
		default:
			luaL_error(L, "Passed invalid ACTIVITY counter.");
			unreachable();
		}
	}();
	if(counter_map.find(counter_id) != counter_map.end())
		return 0;
	counter_map.emplace(counter_id, std::make_pair(counter_filter, 0));
	return 0;
}
int32_t duel_get_custom_activity_count(lua_State* L) {
	check_param_count(L, 3);
	auto counter_id = lua_get<uint32_t>(L, 1);
	auto playerid = lua_get<uint8_t>(L, 2);
	auto activity_type = static_cast<ActivityType>(lua_get<uint8_t>(L, 3));
	const auto pduel = lua_get<duel*>(L);
	auto& counter_map = [&, &core = pduel->game_field->core]()->processor::action_counter_t& {
		switch(activity_type) {
		case ACTIVITY_SUMMON:
			return core.summon_counter;
		case ACTIVITY_NORMALSUMMON:
			return core.normalsummon_counter;
		case ACTIVITY_SPSUMMON:
			return core.spsummon_counter;
		case ACTIVITY_FLIPSUMMON:
			return core.flipsummon_counter;
		case ACTIVITY_ATTACK:
			return core.attack_counter;
		case ACTIVITY_CHAIN:
			return core.chain_counter;
		default:
			luaL_error(L, "Passed invalid ACTIVITY counter.");
			unreachable();
		}
	}();
	int32_t val = 0;
	auto it = counter_map.find(counter_id);
	if(it != counter_map.end())
		val = it->second.second;
	if(playerid == 0)
		lua_pushinteger(L, val & 0xffff);
	else
		lua_pushinteger(L, (val >> 16) & 0xffff);
	return 1;
}

int32_t duel_get_battled_count(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->core.battled_count[playerid]);
	return 1;
}
int32_t duel_is_able_to_enter_bp(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->is_able_to_enter_bp());
	return 1;
}
int32_t duel_get_random_number(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	int32_t min = 0;
	int32_t max = 1;
	if (lua_gettop(L) > 1) {
		min = lua_get<uint32_t>(L, 1);
		max = lua_get<uint32_t>(L, 2);
	} else
		max = lua_get<uint32_t>(L, 1);
	lua_pushinteger(L, pduel->get_next_integer(min, max));
	return 1;
}
int32_t duel_assume_reset(lua_State* L) {
	lua_get<duel*>(L)->restore_assumes();
	return 0;
}
int32_t duel_get_card_from_cardid(lua_State* L) {
	check_param_count(L, 1);
	auto id = lua_get<uint32_t>(L, 1);
	auto pduel = lua_get<duel*>(L);
	for(auto& pcard : pduel->cards) {
		if(pcard->cardid == id) {
			interpreter::pushobject(L, pcard);
			return 1;
		}
	}
	return 0;
}
int32_t duel_load_script(lua_State* L) {
	using SLS = duel::SCRIPT_LOAD_STATUS;
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	lua_getglobal(L, "tostring");
	lua_pushvalue(L, 1);
	lua_pcall(L, 1, 1, 0);
	auto string = lua_tostring_or_empty(L, -1);
	if(/*auto check_cache = */lua_get<bool, true>(L, 2)) {
		auto hash = [](const char* str)->uint32_t {
			uint32_t hash = 5381, c;
			while((c = *str++) != 0)
				hash = (((hash << 5) + hash) + c) & 0xffffffff; /* hash * 33 + c */
			return hash;
		}(string);
		auto& load_status = pduel->loaded_scripts[hash];
		if(load_status == SLS::LOADING) {
			luaL_error(L, "Recursive script loading detected.");
			unreachable();
		} else if(load_status != SLS::NOT_LOADED)
			lua_pushboolean(L, load_status == SLS::LOAD_SUCCEDED);
		else {
			load_status = SLS::LOADING;
			auto res = pduel->read_script(string);
			lua_pushboolean(L, res);
			load_status = res ? SLS::LOAD_SUCCEDED : SLS::LOAD_FAILED;
		}
		return 1;
	}
	lua_pushboolean(L, pduel->read_script(string));
	lua_getglobal(L, "edopro_exports");
	lua_pushnil(L);
	lua_setglobal(L, "edopro_exports");
	return 2;
}
int32_t duel_tag_swap(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if (playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->tag_swap(playerid);
	return 0;
}
int32_t duel_get_player_count(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->get_player_count(playerid));
	return 1;
}
int32_t duel_swap_deck_and_grave(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->swap_deck_and_grave(playerid);
	return 0;
}
int32_t duel_majestic_copy(lua_State* L) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 1);
	auto ccard = lua_get<card*, true>(L, 2);
	uint32_t resv = 0;
	uint16_t resc = 0;
	if(check_param(L, PARAM_TYPE_INT, 3, true)) {
		resv = lua_get<uint32_t>(L, 3);
		resc = lua_get<uint16_t, 1>(L, 4);
	} else {
		resv = RESET_EVENT + 0x1fe0000 + RESET_PHASE + PHASE_END + RESET_SELF_TURN + RESET_OPPO_TURN;
		resc = 0x1;
	}
	if(resv & (RESET_PHASE) && !(resv & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		resv |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	for(auto eit = ccard->single_effect.begin(); eit != ccard->field_effect.end(); ++eit) {
		if(eit == ccard->single_effect.end()) {
			eit = ccard->field_effect.begin();
			if(eit == ccard->field_effect.end())
				break;
		}
		effect* peffect = eit->second;
		if (!(peffect->type & 0x7c) && !peffect->is_flag(EFFECT_FLAG2_MAJESTIC_MUST_COPY)) continue;
		if(!peffect->is_flag(EFFECT_FLAG_INITIAL)) continue;
		effect* ceffect = peffect->clone(true);
		ceffect->owner = pcard;
		ceffect->flag[0] &= ~EFFECT_FLAG_INITIAL;
		ceffect->effect_owner = PLAYER_NONE;
		ceffect->reset_flag = resv;
		ceffect->reset_count = resc;
		ceffect->recharge();
		if(ceffect->type & EFFECT_TYPE_TRIGGER_F) {
			ceffect->type &= ~EFFECT_TYPE_TRIGGER_F;
			ceffect->type |= EFFECT_TYPE_TRIGGER_O;
			ceffect->flag[0] |= EFFECT_FLAG_DELAY;
		}
		if(ceffect->type & EFFECT_TYPE_QUICK_F) {
			ceffect->type &= ~EFFECT_TYPE_QUICK_F;
			ceffect->type |= EFFECT_TYPE_QUICK_O;
		}
		pcard->add_effect(ceffect);
	}
	return 0;
}
int32_t duel_get_starting_hand(lua_State* L) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->player[playerid].start_count);
	return 1;
}

static constexpr luaL_Reg duellib[] = {
	{ "EnableGlobalFlag", duel_enable_global_flag },
	{ "GetLP", duel_get_lp },
	{ "SetLP", duel_set_lp },
	{ "GetTurnPlayer", duel_get_turn_player },
	{ "GetTurnCount", duel_get_turn_count },
	{ "GetDrawCount", duel_get_draw_count },
	{ "RegisterEffect", duel_register_effect },
	{ "RegisterFlagEffect", duel_register_flag_effect },
	{ "GetFlagEffect", duel_get_flag_effect },
	{ "ResetFlagEffect", duel_reset_flag_effect },
	{ "SetFlagEffectLabel", duel_set_flag_effect_label },
	{ "GetFlagEffectLabel", duel_get_flag_effect_label },
	{ "Destroy", duel_destroy },
	{ "Remove", duel_remove },
	{ "SendtoGrave", duel_sendto_grave },
	{ "SendtoHand", duel_sendto_hand },
	{ "SendtoDeck", duel_sendto_deck },
	{ "SendtoExtraP", duel_sendto_extra },
	{ "Sendto", duel_sendto },
	{ "RemoveCards", duel_remove_cards },
	{ "GetOperatedGroup", duel_get_operated_group },
	{ "Summon", duel_summon },
	{ "SpecialSummonRule", duel_special_summon_rule },
	{ "SynchroSummon", duel_synchro_summon },
	{ "XyzSummon", duel_xyz_summon },
	{ "LinkSummon", duel_link_summon },
	{ "ProcedureSummon", duel_procedure_summon },
	{ "PendulumSummon", duel_pendulum_summon },
	{ "ProcedureSummonGroup", duel_procedure_summon_group },
	{ "MSet", duel_setm },
	{ "SSet", duel_sets },
	{ "CreateToken", duel_create_token },
	{ "SpecialSummon", duel_special_summon },
	{ "SpecialSummonStep", duel_special_summon_step },
	{ "SpecialSummonComplete", duel_special_summon_complete },
	{ "IsCanAddCounter", duel_is_can_add_counter },
	{ "RemoveCounter", duel_remove_counter },
	{ "IsCanRemoveCounter", duel_is_can_remove_counter },
	{ "GetCounter", duel_get_counter },
	{ "ChangePosition", duel_change_form },
	{ "Release", duel_release },
	{ "MoveToField", duel_move_to_field },
	{ "ReturnToField", duel_return_to_field },
	{ "MoveSequence", duel_move_sequence },
	{ "SwapSequence", duel_swap_sequence },
	{ "Activate", duel_activate_effect },
	{ "SetChainLimit", duel_set_chain_limit },
	{ "SetChainLimitTillChainEnd", duel_set_chain_limit_p },
	{ "GetChainMaterial", duel_get_chain_material },
	{ "ConfirmDecktop", duel_confirm_decktop },
	{ "ConfirmExtratop", duel_confirm_extratop },
	{ "ConfirmCards", duel_confirm_cards },
	{ "SortDecktop", duel_sort_decktop },
	{ "SortDeckbottom", duel_sort_deckbottom },
	{ "CheckEvent", duel_check_event },
	{ "RaiseEvent", duel_raise_event },
	{ "RaiseSingleEvent", duel_raise_single_event },
	{ "CheckTiming", duel_check_timing },
	{ "GetEnvironment", duel_get_environment },
	{ "IsEnvironment", duel_is_environment },
	{ "Win", duel_win },
	{ "Draw", duel_draw },
	{ "Damage", duel_damage },
	{ "Recover", duel_recover },
	{ "RDComplete", duel_rd_complete },
	{ "Equip", duel_equip },
	{ "EquipComplete", duel_equip_complete },
	{ "GetControl", duel_get_control },
	{ "SwapControl", duel_swap_control },
	{ "CheckLPCost", duel_check_lp_cost },
	{ "PayLPCost", duel_pay_lp_cost },
	{ "DiscardDeck", duel_discard_deck },
	{ "DiscardHand", duel_discard_hand },
	{ "DisableShuffleCheck", duel_disable_shuffle_check },
	{ "DisableSelfDestroyCheck", duel_disable_self_destroy_check },
	{ "ShuffleDeck", duel_shuffle_deck },
	{ "ShuffleExtra", duel_shuffle_extra },
	{ "ShuffleHand", duel_shuffle_hand },
	{ "ShuffleSetCard", duel_shuffle_setcard },
	{ "ChangeAttacker", duel_change_attacker },
	{ "ChangeAttackTarget", duel_change_attack_target },
	{ "AttackCostPaid", duel_attack_cost_paid },
	{ "ForceAttack", duel_force_attack },
	{ "CalculateDamage", duel_calculate_damage },
	{ "GetBattleDamage", duel_get_battle_damage },
	{ "ChangeBattleDamage", duel_change_battle_damage },
	{ "ChangeTargetCard", duel_change_target },
	{ "ChangeTargetPlayer", duel_change_target_player },
	{ "ChangeTargetParam", duel_change_target_param },
	{ "BreakEffect", duel_break_effect },
	{ "ChangeChainOperation", duel_change_effect },
	{ "NegateActivation", duel_negate_activate },
	{ "NegateEffect", duel_negate_effect },
	{ "NegateRelatedChain", duel_negate_related_chain },
	{ "NegateSummon", duel_disable_summon },
	{ "IncreaseSummonedCount", duel_increase_summon_count },
	{ "CheckSummonedCount", duel_check_summon_count },
	{ "GetLocationCount", duel_get_location_count },
	{ "GetMZoneCount", duel_get_mzone_count },
	{ "GetLocationCountFromEx", duel_get_location_count_fromex },
	{ "GetUsableMZoneCount", duel_get_usable_mzone_count },
	{ "GetLinkedGroup", duel_get_linked_group },
	{ "GetLinkedGroupCount", duel_get_linked_group_count },
	{ "GetLinkedZone", duel_get_linked_zone },
	{ "GetFreeLinkedZone", duel_get_free_linked_zone },
	{ "GetFieldCard", duel_get_field_card },
	{ "CheckLocation", duel_check_location },
	{ "GetCurrentChain", duel_get_current_chain },
	{ "GetChainInfo", duel_get_chain_info },
	{ "GetChainEvent", duel_get_chain_event },
	{ "GetFirstTarget", duel_get_first_target },
	{ "GetCurrentPhase", duel_get_current_phase },
	{ "SkipPhase", duel_skip_phase },
	{ "IsAttackCostPaid", duel_is_attack_cost_paid },
	{ "IsDamageCalculated", duel_is_damage_calculated },
	{ "GetAttacker", duel_get_attacker },
	{ "GetAttackTarget", duel_get_attack_target },
	{ "GetBattleMonster", duel_get_battle_monster },
	{ "NegateAttack", duel_disable_attack },
	{ "ChainAttack", duel_chain_attack },
	{ "Readjust", duel_readjust },
	{ "AdjustInstantly", duel_adjust_instantly },
	{ "GetFieldGroup", duel_get_field_group },
	{ "GetFieldGroupCount", duel_get_field_group_count },
	{ "GetDecktopGroup", duel_get_decktop_group },
	{ "GetExtraTopGroup", duel_get_extratop_group },
	{ "GetMatchingGroup", duel_get_matching_group },
	{ "GetMatchingGroupCount", duel_get_matching_count },
	{ "GetFirstMatchingCard", duel_get_first_matching_card },
	{ "IsExistingMatchingCard", duel_is_existing_matching_card },
	{ "SelectMatchingCard", duel_select_matching_cards },
	{ "SelectCardsFromCodes", duel_select_cards_code },
	{ "GetReleaseGroup", duel_get_release_group },
	{ "GetReleaseGroupCount", duel_get_release_group_count },
	{ "CheckReleaseGroup", duel_check_release_group },
	{ "SelectReleaseGroup", duel_select_release_group },
	{ "CheckReleaseGroupEx", duel_check_release_group_ex },
	{ "SelectReleaseGroupEx", duel_select_release_group_ex },
	{ "GetTributeGroup", duel_get_tribute_group },
	{ "GetTributeCount", duel_get_tribute_count },
	{ "CheckTribute", duel_check_tribute },
	{ "SelectTribute", duel_select_tribute },
	{ "GetTargetCount", duel_get_target_count },
	{ "IsExistingTarget", duel_is_existing_target },
	{ "SelectTarget", duel_select_target },
	{ "SelectFusionMaterial", duel_select_fusion_material },
	{ "SetFusionMaterial", duel_set_fusion_material },
	{ "GetRitualMaterial", duel_get_ritual_material },
	{ "ReleaseRitualMaterial", duel_release_ritual_material },
	{ "GetFusionMaterial", duel_get_fusion_material },
	{ "IsSummonCancelable", duel_is_summon_cancelable },
	{ "SetSelectedCard", duel_set_must_select_cards },
	{ "GrabSelectedCard", duel_grab_must_select_cards },
	{ "SetTargetCard", duel_set_target_card },
	{ "ClearTargetCard", duel_clear_target_card },
	{ "SetTargetPlayer", duel_set_target_player },
	{ "SetTargetParam", duel_set_target_param },
	{ "SetOperationInfo", duel_set_operation_info },
	{ "GetOperationInfo", duel_get_operation_info },
	{ "SetPossibleOperationInfo", duel_set_possible_operation_info },
	{ "GetPossibleOperationInfo", duel_get_possible_operation_info },
	{ "GetOperationCount", duel_get_operation_count },
	{ "ClearOperationInfo", duel_clear_operation_info },
	{ "Overlay", duel_overlay },
	{ "GetOverlayGroup", duel_get_overlay_group },
	{ "GetOverlayCount", duel_get_overlay_count },
	{ "CheckRemoveOverlayCard", duel_check_remove_overlay_card },
	{ "RemoveOverlayCard", duel_remove_overlay_card },
	{ "Hint", duel_hint },
	{ "HintSelection", duel_hint_selection },
	{ "SelectEffectYesNo", duel_select_effect_yesno },
	{ "SelectYesNo", duel_select_yesno },
	{ "SelectOption", duel_select_option },
	{ "SelectPosition", duel_select_position },
	{ "SelectDisableField", duel_select_disable_field },
	{ "SelectFieldZone", duel_select_field_zone },
	{ "AnnounceRace", duel_announce_race },
	{ "AnnounceAttribute", duel_announce_attribute },
	{ "AnnounceLevel", duel_announce_level },
	{ "AnnounceCard", duel_announce_card },
	{ "AnnounceType", duel_announce_type },
	{ "AnnounceNumber", duel_announce_number },
	{ "AnnounceCoin", duel_announce_coin },
	{ "TossCoin", duel_toss_coin },
	{ "TossDice", duel_toss_dice },
	{ "RockPaperScissors", duel_rock_paper_scissors },
	{ "GetCoinResult", duel_get_coin_result },
	{ "GetDiceResult", duel_get_dice_result },
	{ "SetCoinResult", duel_set_coin_result },
	{ "SetDiceResult", duel_set_dice_result },
	{ "IsDuelType", duel_is_duel_type },
	{ "IsPlayerAffectedByEffect", duel_is_player_affected_by_effect },
	{ "GetPlayerEffect", duel_get_player_effect },
	{ "IsPlayerCanDraw", duel_is_player_can_draw },
	{ "IsPlayerCanDiscardDeck", duel_is_player_can_discard_deck },
	{ "IsPlayerCanDiscardDeckAsCost", duel_is_player_can_discard_deck_as_cost },
	{ "IsPlayerCanSummon", duel_is_player_can_summon },
	{ "CanPlayerSetMonster", duel_can_player_set_monster },
	{ "CanPlayerSetSpellTrap", duel_can_player_set_spell_trap },
	{ "IsPlayerCanSpecialSummon", duel_is_player_can_spsummon },
	{ "IsPlayerCanFlipSummon", duel_is_player_can_flipsummon },
	{ "IsPlayerCanSpecialSummonMonster", duel_is_player_can_spsummon_monster },
	{ "IsPlayerCanSpecialSummonCount", duel_is_player_can_spsummon_count },
	{ "IsPlayerCanRelease", duel_is_player_can_release },
	{ "IsPlayerCanRemove", duel_is_player_can_remove },
	{ "IsPlayerCanSendtoHand", duel_is_player_can_send_to_hand },
	{ "IsPlayerCanSendtoGrave", duel_is_player_can_send_to_grave },
	{ "IsPlayerCanSendtoDeck", duel_is_player_can_send_to_deck },
	{ "IsPlayerCanAdditionalSummon", duel_is_player_can_additional_summon },
	{ "IsPlayerCanPendulumSummon", duel_is_player_can_pendulum_summon },
	{ "IsPlayerCanProcedureSummonGroup", duel_is_player_can_procedure_summon_group },
	{ "IsChainNegatable", duel_is_chain_negatable },
	{ "IsChainDisablable", duel_is_chain_disablable },
	{ "CheckChainTarget", duel_check_chain_target },
	{ "CheckChainUniqueness", duel_check_chain_uniqueness },
	{ "GetActivityCount", duel_get_activity_count },
	{ "CheckPhaseActivity", duel_check_phase_activity },
	{ "AddCustomActivityCounter", duel_add_custom_activity_counter },
	{ "GetCustomActivityCount", duel_get_custom_activity_count },
	{ "GetBattledCount", duel_get_battled_count },
	{ "IsAbleToEnterBP", duel_is_able_to_enter_bp },
	{ "TagSwap", duel_tag_swap },
	{ "GetPlayersCount", duel_get_player_count },
	{ "SwapDeckAndGrave", duel_swap_deck_and_grave },
	{ "MajesticCopy", duel_majestic_copy },
	{ "GetRandomNumber", duel_get_random_number },
	{ "AssumeReset", duel_assume_reset },
	{ "GetCardFromCardID", duel_get_card_from_cardid },
	{ "LoadScript", duel_load_script },
	{ "GetStartingHand", duel_get_starting_hand },
	{ NULL, NULL }
};
}

void scriptlib::push_duel_lib(lua_State* L) {
	luaL_newlib(L, duellib);
	lua_setglobal(L, "Duel");
}
