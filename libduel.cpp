/*
 * libduel.cpp
 *
 *  Created on: 2010-5-6
 *      Author: Argon
 */

#include <algorithm>
#include <numeric>
#include "scriptlib.h"
#include "duel.h"
#include "field.h"
#include "card.h"
#include "effect.h"
#include "group.h"
#include "bit.h"

#define LUA_MODULE Duel
#include "function_array_helper.h"

namespace {

using namespace scriptlib;

LUA_FUNCTION(EnableGlobalFlag) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.global_flag |= lua_get<uint32_t>(L, 1);
	return 0;
}

LUA_FUNCTION(GetLP) {
	check_param_count(L, 1);
	auto p = lua_get<int8_t>(L, 1);
	if(p != 0 && p != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->player[p].lp);
	return 1;
}
LUA_FUNCTION(SetLP) {
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
LUA_FUNCTION(GetTurnPlayer) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->infos.turn_player);
	return 1;
}
LUA_FUNCTION(GetTurnCount) {
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
LUA_FUNCTION(GetDrawCount) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	lua_pushinteger(L, pduel->game_field->get_draw_count(playerid));
	return 1;
}
LUA_FUNCTION(RegisterEffect) {
	check_param_count(L, 2);
	auto peffect = lua_get<effect*, true>(L, 1);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_effect(peffect, playerid);
	return 0;
}
LUA_FUNCTION(RegisterFlagEffect) {
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
LUA_FUNCTION(GetFlagEffect) {
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
LUA_FUNCTION(ResetFlagEffect) {
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
LUA_FUNCTION(SetFlagEffectLabel) {
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
LUA_FUNCTION(GetFlagEffectLabel) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto code = (lua_get<uint32_t>(L, 2) & 0xfffffff) | 0x10000000;
	const auto pduel = lua_get<duel*>(L);
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, code, &eset);
	if(eset.empty()) {
		lua_pushnil(L);
		return 1;
	}
	luaL_checkstack(L, static_cast<int>(eset.size()), nullptr);
	for(auto& eff : eset)
		lua_pushinteger(L, eff->label.size() ? eff->label[0] : 0);
	return static_cast<int32_t>(eset.size());
}
LUA_FUNCTION(Destroy) {
	check_action_permission(L);
	check_param_count(L, 2);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto reason = lua_get<uint32_t>(L, 2);
	auto dest = lua_get<uint16_t, LOCATION_GRAVE>(L, 3);
	const auto reasonplayer = lua_get<uint8_t>(L, 4, pduel->game_field->core.reason_player);
	if(pcard)
		pduel->game_field->destroy(pcard, pduel->game_field->core.reason_effect, reason, reasonplayer, PLAYER_NONE, dest, 0);
	else
		pduel->game_field->destroy(pgroup->container, pduel->game_field->core.reason_effect, reason, reasonplayer, PLAYER_NONE, dest, 0);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(Remove) {
	check_action_permission(L);
	check_param_count(L, 3);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto pos = lua_get<uint8_t, 0>(L, 2);
	auto reason = lua_get<uint32_t>(L, 3);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 4);
	const auto reasonplayer = lua_get<uint8_t>(L, 5, pduel->game_field->core.reason_player);
	if (pcard)
		pduel->game_field->send_to(pcard, pduel->game_field->core.reason_effect, reason, reasonplayer, playerid, LOCATION_REMOVED, 0, pos);
	else
		pduel->game_field->send_to(pgroup->container, pduel->game_field->core.reason_effect, reason, reasonplayer, playerid, LOCATION_REMOVED, 0, pos);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(SendtoGrave) {
	check_action_permission(L);
	check_param_count(L, 2);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto reason = lua_get<uint32_t>(L, 2);
	auto playerid = lua_get<uint8_t, PLAYER_NONE>(L, 3);
	const auto reasonplayer = lua_get<uint8_t>(L, 4, pduel->game_field->core.reason_player);
	if (pcard)
		pduel->game_field->send_to(pcard, pduel->game_field->core.reason_effect, reason, reasonplayer, playerid, LOCATION_GRAVE, 0, POS_FACEUP);
	else
		pduel->game_field->send_to(pgroup->container, pduel->game_field->core.reason_effect, reason, reasonplayer, playerid, LOCATION_GRAVE, 0, POS_FACEUP);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(Summon) {
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
LUA_FUNCTION(SpecialSummonRule) {
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
LUA_FUNCTION(SynchroSummon) {
	return spsummon_rule(L, SUMMON_TYPE_SYNCHRO, 0);
}
LUA_FUNCTION(XyzSummon) {
	return spsummon_rule(L, SUMMON_TYPE_XYZ, 0);
}
LUA_FUNCTION(LinkSummon) {
	return spsummon_rule(L, SUMMON_TYPE_LINK, 0);
}
LUA_FUNCTION(ProcedureSummon) {
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
LUA_FUNCTION(PendulumSummon) {
	return spsummon_rule_group(L, SUMMON_TYPE_PENDULUM, 0);
}
LUA_FUNCTION(ProcedureSummonGroup) {
	check_param_count(L, 2);
	auto sumtype = lua_get<uint32_t>(L, 2);
	return spsummon_rule_group(L, sumtype, 1);
}
LUA_FUNCTION(MSet) {
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
LUA_FUNCTION(SSet) {
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
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(CreateToken) {
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
LUA_FUNCTION(SpecialSummon) {
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
		pduel->game_field->special_summon(card_set{ pcard }, sumtype, sumplayer, playerid, nocheck, nolimit, positions, zone);
	} else
		pduel->game_field->special_summon(pgroup->container, sumtype, sumplayer, playerid, nocheck, nolimit, positions, zone);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(SpecialSummonStep) {
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
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushboolean(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(SpecialSummonComplete) {
	check_action_permission(L);
	const auto pduel = lua_get<duel*>(L);
	auto& core = pduel->game_field->core;
	if(core.special_summoning.empty() && core.ss_tograve_set.empty()) {
		core.operated_set.clear();
		lua_pushinteger(L, 0);
		return 1;
	}
	pduel->game_field->special_summon_complete(core.reason_effect, core.reason_player);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(SendtoHand) {
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
	const auto reasonplayer = lua_get<uint8_t>(L, 4, pduel->game_field->core.reason_player);
	if(pcard)
		pduel->game_field->send_to(pcard, pduel->game_field->core.reason_effect, reason, reasonplayer, playerid, LOCATION_HAND, 0, POS_FACEUP);
	else
		pduel->game_field->send_to(pgroup->container, pduel->game_field->core.reason_effect, reason, reasonplayer, playerid, LOCATION_HAND, 0, POS_FACEUP);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(SendtoDeck) {
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
	const auto reasonplayer = lua_get<uint8_t>(L, 5, pduel->game_field->core.reason_player);
	uint16_t location = (sequence == -2) ? 0 : LOCATION_DECK;
	if(pcard)
		pduel->game_field->send_to(pcard, pduel->game_field->core.reason_effect, reason, reasonplayer, playerid, location, sequence, POS_FACEUP);
	else
		pduel->game_field->send_to(pgroup->container, pduel->game_field->core.reason_effect, reason, reasonplayer, playerid, location, sequence, POS_FACEUP);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(SendtoExtraP) {
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
	const auto reasonplayer = lua_get<uint8_t>(L, 4, pduel->game_field->core.reason_player);
	if(pcard)
		pduel->game_field->send_to(pcard, pduel->game_field->core.reason_effect, reason, reasonplayer, playerid, LOCATION_EXTRA, 0, POS_FACEUP);
	else
		pduel->game_field->send_to(pgroup->container, pduel->game_field->core.reason_effect, reason, reasonplayer, playerid, LOCATION_EXTRA, 0, POS_FACEUP);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(Sendto) {
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
	const auto reasonplayer = lua_get<uint8_t>(L, 7, pduel->game_field->core.reason_player);
	if(pcard)
		pduel->game_field->send_to(pcard, pduel->game_field->core.reason_effect, reason, reasonplayer, playerid, location, sequence, pos, TRUE);
	else
		pduel->game_field->send_to(pgroup->container, pduel->game_field->core.reason_effect, reason, reasonplayer, playerid, location, sequence, pos, TRUE);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(RemoveCards) {
	check_action_permission(L);
	check_param_count(L, 1);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	if(pcard) {
		auto message = pduel->new_message(MSG_REMOVE_CARDS);
		message->write<uint32_t>(1);
		message->write(pcard->get_info_location());
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
		pduel->game_field->remove_card(pcard);
	} else {
		auto message = pduel->new_message(MSG_REMOVE_CARDS);
		auto tot = pgroup->container.size();
		auto cur = std::min<size_t>(tot, 255);
		message->write<uint32_t>(cur);
		for(auto& card : pgroup->container) {
			message->write(card->get_info_location());
			--tot;
			--cur;
			if(cur == 0 && tot > 0) {
				message = pduel->new_message(MSG_REMOVE_CARDS);
				cur = std::min<size_t>(tot, 255);
				message->write<uint32_t>(cur);
			}
			card->enable_field_effect(false);
			card->cancel_field_effect();
		}
		for(auto& card : pgroup->container)
			pduel->game_field->remove_card(card);
	}
	return 0;
}
LUA_FUNCTION(GetOperatedGroup) {
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group(pduel->game_field->core.operated_set);
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(IsCanAddCounter) {
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
LUA_FUNCTION(RemoveCounter) {
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
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushboolean(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(IsCanRemoveCounter) {
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
LUA_FUNCTION(GetCounter) {
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
LUA_FUNCTION(ChangePosition) {
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
		pduel->game_field->change_position(card_set{ pcard }, pduel->game_field->core.reason_effect, pduel->game_field->core.reason_player, au, ad, du, dd, flag, TRUE);
	} else
		pduel->game_field->change_position(pgroup->container, pduel->game_field->core.reason_effect, pduel->game_field->core.reason_player, au, ad, du, dd, flag, TRUE);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(Release) {
	check_action_permission(L);
	check_param_count(L, 2);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	auto reason = lua_get<uint32_t>(L, 2);
	const auto reasonplayer = lua_get<uint8_t>(L, 3, pduel->game_field->core.reason_player);
	if(pcard)
		pduel->game_field->release(pcard, pduel->game_field->core.reason_effect, reason, reasonplayer);
	else
		pduel->game_field->release(pgroup->container, pduel->game_field->core.reason_effect, reason, reasonplayer);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(MoveToField) {
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
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushboolean(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(ReturnToField) {
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
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushboolean(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(MoveSequence) {
	check_action_permission(L);
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto pcard = lua_get<card*, true>(L, 1);
	auto seq = lua_get<uint8_t>(L, 2);
	uint16_t cur_loc = pcard->current.location;
	bool cur_pzone = pcard->current.pzone;
	auto location = lua_get<uint16_t>(L, 3, cur_loc);

	auto& field = *pduel->game_field;

	bool pzone = false;
	bool fzone = false;
	if(location == LOCATION_PZONE) {
		if(!field.is_flag(DUEL_PZONE))
			lua_error(L, "LOCATION_PZONE passed with pendulum zones disabled");
		pzone = true;
		location = LOCATION_SZONE;
	} else if(location == LOCATION_FZONE) {
		fzone = true;
		location = LOCATION_SZONE;
	}

	auto playerid = pcard->current.controler;

	if(location != cur_loc)
		lua_error(L, "The new location doesn't match the current one");

	if(pzone) {
		if(seq > 1)
			lua_error(L, "Invalid sequence");
		seq = field.get_pzone_index(seq, playerid);
	} else if(fzone) {
		if(seq != 0)
			lua_error(L, "Invalid sequence");
		seq = 5;
	} else if(location == LOCATION_MZONE && field.is_flag(DUEL_EMZONE)) {
		if(seq > 6)
			lua_error(L, "Invalid sequence");
	} else if ((location == LOCATION_MZONE || location == LOCATION_SZONE) && seq > 4)
		lua_error(L, "Invalid sequence");

	auto& core = field.core;
	int res = FALSE;
	if(pcard->is_affect_by_effect(core.reason_effect)) {
		const auto previous_loc_info = pcard->get_info_location();
		const auto previous_code = pcard->data.code;
		if((res = field.move_card(playerid, pcard, pcard->current.location, seq, pzone)) == TRUE) {
			if(cur_pzone != pzone) {
				auto message = pduel->new_message(MSG_MOVE);
				message->write<uint32_t>(previous_code);
				message->write(previous_loc_info);
				message->write(pcard->get_info_location());
				message->write<uint32_t>(pcard->current.reason);
			}
			field.raise_single_event(pcard, 0, EVENT_MOVE, core.reason_effect, 0, core.reason_player, playerid, 0);
			field.raise_event(pcard, EVENT_MOVE, core.reason_effect, 0, core.reason_player, playerid, 0);
			field.process_single_event();
			field.process_instant_event();
		}
	}
	lua_pushboolean(L, res);
	return 1;
}
LUA_FUNCTION(SwapSequence) {
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
LUA_FUNCTION(Activate) {
	check_action_permission(L);
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto peffect = lua_get<effect*, true>(L, 1);
	pduel->game_field->add_process(PROCESSOR_ACTIVATE_EFFECT, 0, peffect, 0, 0, 0);
	return 0;
}
LUA_FUNCTION(SetChainLimit) {
	check_param_count(L, 1);
	auto f = interpreter::get_function_handle(L, lua_get<function, true>(L, 1));
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.chain_limit.emplace_back(f, pduel->game_field->core.reason_player);
	return 0;
}
LUA_FUNCTION(SetChainLimitTillChainEnd) {
	check_param_count(L, 1);
	auto f = interpreter::get_function_handle(L, lua_get<function, true>(L, 1));
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.chain_limit_p.emplace_back(f, pduel->game_field->core.reason_player);
	return 0;
}
LUA_FUNCTION(GetChainMaterial) {
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
LUA_FUNCTION(ConfirmDecktop) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto count = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	const auto& player = pduel->game_field->player[playerid];
	const auto& list_main = player.list_main;
	if(count >= list_main.size())
		count = static_cast<uint32_t>(list_main.size());
	else if(list_main.size() > count) {
		if(pduel->game_field->core.global_flag & GLOBALFLAG_DECK_REVERSE_CHECK && pduel->game_field->core.deck_reversed) {
			card* pcard = *(list_main.rbegin() + count);
			auto message = pduel->new_message(MSG_DECK_TOP);
			message->write<uint8_t>(playerid);
			message->write<uint32_t>(count);
			message->write<uint32_t>(pcard->data.code);
			message->write<uint32_t>(pcard->current.position);
		}
	}
	auto message = pduel->new_message(MSG_CONFIRM_DECKTOP);
	message->write<uint8_t>(playerid);
	message->write<uint32_t>(count);
	for(auto cit = list_main.rbegin(), end = cit + count; cit != end; ++cit) {
		const auto& pcard = *cit;
		message->write<uint32_t>(pcard->data.code);
		message->write<uint8_t>(pcard->current.controler);
		message->write<uint8_t>(pcard->current.location);
		message->write<uint32_t>(pcard->current.sequence);
	}
	return lua_yield(L, 0);
}
LUA_FUNCTION(ConfirmExtratop) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto count = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	const auto& player = pduel->game_field->player[playerid];
	const auto& list_extra = player.list_extra;
	const uint32_t max_count = static_cast<uint32_t>(list_extra.size()) - player.extra_p_count;
	count = std::min(max_count, count);
	auto message = pduel->new_message(MSG_CONFIRM_EXTRATOP);
	message->write<uint8_t>(playerid);
	message->write<uint32_t>(count);
	for(auto cit = list_extra.rbegin() + player.extra_p_count, end = cit + count; cit != end; ++cit) {
		const auto& pcard = *cit;
		message->write<uint32_t>(pcard->data.code);
		message->write<uint8_t>(pcard->current.controler);
		message->write<uint8_t>(pcard->current.location);
		message->write<uint32_t>(pcard->current.sequence);
	}
	return lua_yield(L, 0);
}
LUA_FUNCTION(ConfirmCards) {
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
LUA_FUNCTION(SortDecktop) {
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
LUA_FUNCTION(SortDeckbottom) {
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
LUA_FUNCTION(CheckEvent) {
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
LUA_FUNCTION(RaiseEvent) {
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
LUA_FUNCTION(RaiseSingleEvent) {
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
LUA_FUNCTION(CheckTiming) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto tm = lua_get<uint32_t>(L, 1);
	lua_pushboolean(L, (pduel->game_field->core.hint_timing[0]&tm) || (pduel->game_field->core.hint_timing[1]&tm));
	return 1;
}
LUA_FUNCTION(GetEnvironment) {
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
LUA_FUNCTION(IsEnvironment) {
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
LUA_FUNCTION(Win) {
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
LUA_FUNCTION(Draw) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto count = lua_get<uint32_t>(L, 2);
	auto reason = lua_get<uint32_t>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->draw(pduel->game_field->core.reason_effect, reason, pduel->game_field->core.reason_player, playerid, count);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(Damage) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto amount = lua_get<int64_t>(L, 2);
	uint32_t actual_amount = 0;
	if(amount > 0)
		actual_amount = static_cast<uint32_t>(amount);
	auto reason = lua_get<uint32_t>(L, 3);
	bool is_step = lua_get<bool, false>(L, 4);
	const auto pduel = lua_get<duel*>(L);
	auto reason_player = lua_get<uint8_t>(L, 5, pduel->game_field->core.reason_player);
	if (reason_player > PLAYER_NONE)
		reason_player = pduel->game_field->core.reason_player;
	pduel->game_field->damage(pduel->game_field->core.reason_effect, reason, reason_player, 0, playerid, actual_amount, is_step);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<uint32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(Recover) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto amount = lua_get<int64_t>(L, 2);
	uint32_t actual_amount = 0;
	if(amount > 0)
		actual_amount = static_cast<uint32_t>(amount);
	auto reason = lua_get<uint32_t>(L, 3);
	bool is_step = lua_get<bool, false>(L, 4);
	const auto pduel = lua_get<duel*>(L);
	auto reason_player = lua_get<uint8_t>(L, 5, pduel->game_field->core.reason_player);
	if (reason_player > PLAYER_NONE)
		reason_player = pduel->game_field->core.reason_player;
	pduel->game_field->recover(pduel->game_field->core.reason_effect, reason, reason_player, playerid, actual_amount, is_step);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<uint32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(RDComplete) {
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.subunits.splice(pduel->game_field->core.subunits.end(), pduel->game_field->core.recover_damage_reserve);
	return lua_yield(L, 0);
}
LUA_FUNCTION(Equip) {
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
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushboolean(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(EquipComplete) {
	const auto pduel = lua_get<duel*>(L);
	const auto& field = pduel->game_field;
	auto& core = field->core;
	card_set etargets;
	for(auto& equip_card : core.equiping_cards) {
		if(equip_card->is_position(POS_FACEUP))
			equip_card->enable_field_effect(true);
		etargets.insert(equip_card->equiping_target);
	}
	if (etargets.size() == 0)
		return 0;
	field->adjust_instant();
	for(auto& equip_target : etargets) {
		field->raise_single_event(equip_target, &core.equiping_cards, EVENT_EQUIP,
		                          core.reason_effect, 0, core.reason_player, PLAYER_NONE, 0);
	}
	field->raise_event(&core.equiping_cards, EVENT_EQUIP,
	                               core.reason_effect, 0, core.reason_player, PLAYER_NONE, 0);
	core.hint_timing[0] |= TIMING_EQUIP;
	core.hint_timing[1] |= TIMING_EQUIP;
	field->process_single_event();
	field->process_instant_event();
	return lua_yield(L, 0);
}
LUA_FUNCTION(GetControl) {
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
		pduel->game_field->get_control(pgroup->container, pduel->game_field->core.reason_effect, chose_player, playerid, reset_phase, reset_count, zone);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(SwapControl) {
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
		lua_error(L, "Parameter %d should be \"Card\" or \"Group\".", 1);
	}
	if(obj1->lua_type == PARAM_TYPE_CARD) {
		pcard1 = static_cast<card*>(obj1);
		pcard2 = static_cast<card*>(obj2);
	} else if(obj1->lua_type == PARAM_TYPE_GROUP) {
		pgroup1 = static_cast<group*>(obj1);
		pgroup2 = static_cast<group*>(obj2);
	} else
		unreachable();
	const auto pduel = lua_get<duel*>(L);
	uint16_t reset_phase = lua_get<uint16_t, 0>(L, 3) & 0x3ff;
	auto reset_count = lua_get<uint8_t, 0>(L, 4);
	if(pcard1)
		pduel->game_field->swap_control(pduel->game_field->core.reason_effect, pduel->game_field->core.reason_player, pcard1, pcard2, reset_phase, reset_count);
	else
		pduel->game_field->swap_control(pduel->game_field->core.reason_effect, pduel->game_field->core.reason_player, pgroup1->container, pgroup2->container, reset_phase, reset_count);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushboolean(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(CheckLPCost) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto cost = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->check_lp_cost(playerid, cost));
	return 1;
}
LUA_FUNCTION(PayLPCost) {
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
LUA_FUNCTION(DiscardDeck) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto count = lua_get<uint16_t>(L, 2);
	auto reason = lua_get<uint32_t>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_DISCARD_DECK, 0, 0, 0, playerid + (count << 16), reason);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(DiscardHand) {
	check_action_permission(L);
	check_param_count(L, 5);
	const auto findex = lua_get<function>(L, 2);
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
	pduel->game_field->filter_matching_card(findex, playerid, LOCATION_HAND, 0, pgroup, pexception, pexgroup, extraargs);
	pduel->game_field->core.select_cards.assign(pgroup->container.begin(), pgroup->container.end());
	if(pduel->game_field->core.select_cards.size() == 0) {
		lua_pushinteger(L, 0);
		return 1;
	}
	pduel->game_field->add_process(PROCESSOR_DISCARD_HAND, 0, nullptr, nullptr, playerid, min + (max << 16), reason);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(DisableShuffleCheck) {
	const auto pduel = lua_get<duel*>(L);
	bool disable = lua_get<bool, true>(L, 1);
	pduel->game_field->core.shuffle_check_disabled = disable;
	return 0;
}
LUA_FUNCTION(DisableSelfDestroyCheck) {
	const auto pduel = lua_get<duel*>(L);
	bool disable = lua_get<bool, true>(L, 1);
	pduel->game_field->core.selfdes_disabled = disable;
	return 0;
}
LUA_FUNCTION(ShuffleDeck) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->shuffle(playerid, LOCATION_DECK);
	return lua_yield(L, 0);
}
LUA_FUNCTION(ShuffleExtra) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if (playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->shuffle(playerid, LOCATION_EXTRA);
	return lua_yield(L, 0);
}
LUA_FUNCTION(ShuffleHand) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->shuffle(playerid, LOCATION_HAND);
	return lua_yield(L, 0);
}
LUA_FUNCTION(ShuffleSetCard) {
	check_param_count(L, 1);
	auto pgroup = lua_get<group*, true>(L, 1);
	if(pgroup->container.size() <= 0)
		return 0;
	card* ms[7];
	uint8_t seq[7];
	auto it = pgroup->container.begin();
	uint8_t ct = 0;
	ms[ct] = *it;
	uint8_t loc = ms[ct]->current.location;
	if(loc != LOCATION_MZONE && loc != LOCATION_SZONE)
		return 0;
	uint8_t tp = ms[ct]->current.controler;
	if(tp > 1)
		return 0;
	if(ms[ct]->current.position & POS_FACEUP)
		return 0;
	if(ms[ct]->current.sequence > 4)
		return 0;
	seq[ct] = ms[ct]->current.sequence;
	++it;
	++ct;
	for(auto end = pgroup->container.end(); it != end; ++it, ++ct) {
		auto pcard = *it;
		const auto& current = pcard->current;
		if(current.location != loc)
			return 0;
		if(current.position & POS_FACEUP)
			return 0;
		if(current.sequence > 4)
			return 0;
		if(current.controler != tp)
			return 0;
		ms[ct] = pcard;
		seq[ct] = current.sequence;
	}
	const auto pduel = lua_get<duel*>(L);
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
	return lua_yield(L, 0);
}
LUA_FUNCTION(ChangeAttacker) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto new_attacker = lua_get<card*, true>(L, 1);
	auto& attacker = pduel->game_field->core.attacker;
	if(attacker == new_attacker)
		return 0;
	auto ignore_count = lua_get<bool, false>(L, 2);
	card* attack_target = pduel->game_field->core.attack_target;
	++attacker->announce_count;
	attacker->announced_cards.addcard(attack_target);
	pduel->game_field->attack_all_target_check();
	attacker = new_attacker;
	attacker->attack_controler = attacker->current.controler;
	pduel->game_field->core.pre_field[0] = attacker->fieldid_r;
	if(!ignore_count) {
		++attacker->attack_announce_count;
		if(pduel->game_field->infos.phase == PHASE_DAMAGE) {
			++attacker->attacked_count;
			attacker->attacked_cards.addcard(attack_target);
		}
	}
	return 0;
}
LUA_FUNCTION(ChangeAttackTarget) {
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
LUA_FUNCTION(AttackCostPaid) {
	const auto pduel = lua_get<duel*>(L);
	auto paid = lua_get<uint8_t, 1>(L, 1);
	pduel->game_field->core.attack_cost_paid = paid;
	return 0;
}
LUA_FUNCTION(ForceAttack) {
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
LUA_FUNCTION(CalculateDamage) {
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
LUA_FUNCTION(GetBattleDamage) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	lua_pushinteger(L, pduel->game_field->core.battle_damage[playerid]);
	return 1;
}
LUA_FUNCTION(ChangeBattleDamage) {
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
LUA_FUNCTION(ChangeTargetCard) {
	check_param_count(L, 2);
	auto count = lua_get<uint8_t>(L, 1);
	auto pgroup = lua_get<group*, true>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->change_target(count, pgroup);
	return 0;
}
LUA_FUNCTION(ChangeTargetPlayer) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto count = lua_get<uint8_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->change_target_player(count, playerid);
	return 0;
}
LUA_FUNCTION(ChangeTargetParam) {
	check_param_count(L, 2);
	auto count = lua_get<uint8_t>(L, 1);
	auto param = lua_get<int32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->change_target_param(count, param);
	return 0;
}
LUA_FUNCTION(BreakEffect) {
	check_action_permission(L);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->break_effect();
	pduel->game_field->raise_event((card*)0, EVENT_BREAK_EFFECT, 0, 0, PLAYER_NONE, PLAYER_NONE, 0);
	pduel->game_field->process_instant_event();
	return lua_yield(L, 0);
}
LUA_FUNCTION(ChangeChainOperation) {
	check_param_count(L, 2);
	auto f = interpreter::get_function_handle(L, lua_get<function, true>(L, 2));
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->change_chain_effect(lua_get<uint8_t>(L, 1), f);
	return 0;
}
LUA_FUNCTION(NegateActivation) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->negate_chain(lua_get<uint8_t>(L, 1)));
	return 1;
}
LUA_FUNCTION(NegateEffect) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->disable_chain(lua_get<uint8_t>(L, 1)));
	return 1;
}
LUA_FUNCTION(NegateRelatedChain) {
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
LUA_FUNCTION(NegateSummon) {
	check_param_count(L, 1);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	const auto pduel = lua_get<duel*>(L);
	uint8_t sumplayer = PLAYER_NONE;
	if(pcard) {
		sumplayer = pcard->summon.player;
		pcard->set_status(STATUS_SUMMONING, FALSE);
		pcard->set_status(STATUS_SUMMON_DISABLED, TRUE);
		if((pcard->summon.type & SUMMON_TYPE_PENDULUM) != SUMMON_TYPE_PENDULUM)
			pcard->set_status(STATUS_PROC_COMPLETE, FALSE);
	} else {
		for(auto& _pcard : pgroup->container) {
			sumplayer = _pcard->summon.player;
			_pcard->set_status(STATUS_SUMMONING, FALSE);
			_pcard->set_status(STATUS_SUMMON_DISABLED, TRUE);
			if((_pcard->summon.type & SUMMON_TYPE_PENDULUM) != SUMMON_TYPE_PENDULUM)
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
LUA_FUNCTION(IncreaseSummonedCount) {
	card* pcard = 0;
	effect* pextra = 0;
	if(lua_gettop(L) > 0)
		pcard = lua_get<card*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	uint8_t playerid = pduel->game_field->core.reason_player;
	if(pcard && (pextra = pcard->is_affected_by_effect(EFFECT_EXTRA_SUMMON_COUNT)) != nullptr)
		pextra->get_value(pcard);
	else
		++pduel->game_field->core.summon_count[playerid];
	return 0;
}
LUA_FUNCTION(CheckSummonedCount) {
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
LUA_FUNCTION(GetLocationCount) {
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
	lua_pushinteger(L, pduel->game_field->get_useable_count(nullptr, playerid, location, uplayer, reason, zone, &list));
	lua_pushinteger(L, list);
	return 2;
}
LUA_FUNCTION(GetMZoneCount) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	bool swapped = false;
	card* mcard = nullptr;
	group* mgroup = nullptr;
	const uint32_t default_loc = 0x1111 * pduel->game_field->is_flag(DUEL_3_COLUMNS_FIELD);
	uint32_t used_location[2] = { default_loc, default_loc };
	card_vector list_mzone[2];
	if(!lua_isnoneornil(L, 2)) {
		get_card_or_group(L, 2, mcard, mgroup);
		for(int32_t p = 0; p < 2; ++p) {
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
	lua_pushinteger(L, pduel->game_field->get_useable_count(nullptr, playerid, LOCATION_MZONE, uplayer, reason, zone, &list));
	lua_pushinteger(L, list);
	if(swapped) {
		pduel->game_field->player[0].used_location = used_location[0];
		pduel->game_field->player[1].used_location = used_location[1];
		pduel->game_field->player[0].list_mzone.swap(list_mzone[0]);
		pduel->game_field->player[1].list_mzone.swap(list_mzone[1]);
	}
	return 2;
}
LUA_FUNCTION(GetLocationCountFromEx) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	auto uplayer = lua_get<uint8_t>(L, 2, pduel->game_field->core.reason_player);
	bool swapped = false;
	card* mcard = nullptr;
	group* mgroup = nullptr;
	const uint32_t default_loc = 0x1111 * pduel->game_field->is_flag(DUEL_3_COLUMNS_FIELD);
	uint32_t used_location[2] = { default_loc, default_loc };
	card_vector list_mzone[2];
	if(!lua_isnoneornil(L, 3)) {
		get_card_or_group(L, 3, mcard, mgroup);
		for(int32_t p = 0; p < 2; ++p) {
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
LUA_FUNCTION(GetUsableMZoneCount) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	auto uplayer = lua_get<uint8_t>(L, 2, pduel->game_field->core.reason_player);
	uint32_t zone = 0xff;
	uint32_t flag1, flag2;
	int32_t ct1 = pduel->game_field->get_tofield_count(nullptr, playerid, LOCATION_MZONE, uplayer, LOCATION_REASON_TOFIELD, zone, &flag1);
	int32_t ct2 = pduel->game_field->get_spsummonable_count_fromex(nullptr, playerid, uplayer, zone, &flag2);
	int32_t ct3 = field::field_used_count[~(flag1 | flag2) & 0x1f];
	int32_t count = ct1 + ct2 - ct3;
	int32_t limit = pduel->game_field->get_mzone_limit(playerid, uplayer, LOCATION_REASON_TOFIELD);
	if(count > limit)
		count = limit;
	lua_pushinteger(L, count);
	return 1;
}
LUA_FUNCTION(GetLinkedGroup) {
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
LUA_FUNCTION(GetLinkedGroupCount) {
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
LUA_FUNCTION(GetLinkedZone) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->get_linked_zone(playerid));
	return 1;
}
LUA_FUNCTION(GetFreeLinkedZone) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->get_linked_zone(playerid, true));
	return 1;
}
LUA_FUNCTION(GetFieldCard) {
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
LUA_FUNCTION(CheckLocation) {
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
LUA_FUNCTION(GetCurrentChain) {
	const auto pduel = lua_get<duel*>(L);
	const auto real = lua_get<bool, false>(L, 1);
	const auto& core = pduel->game_field->core;
	lua_pushinteger(L, real ? core.real_chain_count : core.current_chain.size());
	return 1;
}
LUA_FUNCTION(GetChainInfo) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(lua_get<uint8_t>(L, 1));
	if(!ch)
		return 0;
	auto top = lua_gettop(L);
	uint32_t args = static_cast<uint32_t>(lua_istable(L, 2) ? lua_rawlen(L, 2) : top - 1);
	luaL_checkstack(L, args, nullptr);
	lua_iterate_table_or_stack(L, 2, top, [L, ch, pduel]() -> int {
		auto flag = lua_get<uint32_t>(L, -1);
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
			lua_pushinteger(L, ch->triggering_location & ~(LOCATION_STZONE | LOCATION_MMZONE | LOCATION_EMZONE));
			break;
		case CHAININFO_TRIGGERING_LOCATION_SYMBOLIC:
			lua_pushinteger(L, ch->triggering_location & ~(LOCATION_MZONE | LOCATION_SZONE));
			break;
		case CHAININFO_TRIGGERING_SEQUENCE:
			lua_pushinteger(L, ch->triggering_sequence);
			break;
		case CHAININFO_TRIGGERING_SEQUENCE_SYMBOLIC:
			if(pduel->game_field->is_flag(DUEL_3_COLUMNS_FIELD) && (ch->triggering_location & ~(LOCATION_STZONE | LOCATION_MMZONE)))
				lua_pushinteger(L, ch->triggering_sequence + 1);
			else if(ch->triggering_location & LOCATION_PZONE) {
				if(ch->triggering_sequence == pduel->game_field->get_pzone_index(0, ch->triggering_controler))
					lua_pushinteger(L, 0);
				else
					lua_pushinteger(L, 1);
			}
			else if(ch->triggering_location & LOCATION_FZONE) {
				lua_pushinteger(L, 0);
			}
			else if(ch->triggering_location & LOCATION_EMZONE) {
				lua_pushinteger(L, ch->triggering_sequence - 5);
			} else
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
			lua_error(L, "Passed invalid CHAININFO flag.");
		}
		return 1;
	});
	return args;
}
LUA_FUNCTION(GetChainEvent) {
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
LUA_FUNCTION(GetFirstTarget) {
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(0);
	if(!ch || !ch->target_cards || ch->target_cards->container.size() == 0)
		return 0;
	const auto& cset = ch->target_cards->container;
	luaL_checkstack(L, static_cast<int>(cset.size()), nullptr);
	for(auto& pcard : cset)
		interpreter::pushobject(L, pcard);
	return static_cast<int32_t>(cset.size());
}
LUA_FUNCTION(GetCurrentPhase) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->infos.phase);
	return 1;
}
LUA_FUNCTION(SkipPhase) {
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
LUA_FUNCTION(IsAttackCostPaid) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->core.attack_cost_paid);
	return 1;
}
LUA_FUNCTION(IsDamageCalculated) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->core.damage_calculated);
	return 1;
}
LUA_FUNCTION(GetAttacker) {
	const auto pduel = lua_get<duel*>(L);
	card* pcard = pduel->game_field->core.attacker;
	interpreter::pushobject(L, pcard);
	return 1;
}
LUA_FUNCTION(GetAttackTarget) {
	const auto pduel = lua_get<duel*>(L);
	card* pcard = pduel->game_field->core.attack_target;
	interpreter::pushobject(L, pcard);
	return 1;
}
LUA_FUNCTION(GetBattleMonster) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 1);
	card* attacker = pduel->game_field->core.attacker;
	card* defender = pduel->game_field->core.attack_target;
	for(int32_t i = 0; i < 2; ++i) {
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
LUA_FUNCTION(NegateAttack) {
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_ATTACK_DISABLE, 0, 0, 0, 0, 0);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushboolean(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(ChainAttack) {
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.chain_attack = TRUE;
	pduel->game_field->core.chain_attacker_id = pduel->game_field->core.attacker->fieldid;
	if(lua_gettop(L) > 0)
		pduel->game_field->core.chain_attack_target = lua_get<card*, true>(L, 1);
	return 0;
}
LUA_FUNCTION(Readjust) {
	const auto pduel = lua_get<duel*>(L);
	card* adjcard = pduel->game_field->core.reason_effect->get_handler();
	auto& readj_map = pduel->game_field->core.readjust_map[adjcard];
	++readj_map;
	if(readj_map > 3) {
		pduel->game_field->send_to(adjcard, 0, REASON_RULE, pduel->game_field->core.reason_player, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
		return lua_yield(L, 0);
	}
	pduel->game_field->core.re_adjust = TRUE;
	return 0;
}
LUA_FUNCTION(AdjustInstantly) {
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
LUA_FUNCTION(GetFieldGroup) {
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
LUA_FUNCTION(GetFieldGroupCount) {
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
LUA_FUNCTION(GetDecktopGroup) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto& main = pduel->game_field->player[playerid].list_main;
	const auto count = std::min<size_t>(lua_get<uint32_t>(L, 2), main.size());
	const auto offset = main.size() - count;
	group* pgroup = pduel->new_group(main.begin() + offset, main.end());
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(GetDeckbottomGroup) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto& main = pduel->game_field->player[playerid].list_main;
	const auto count = std::min<size_t>(lua_get<uint32_t>(L, 2), main.size());
	group* pgroup = pduel->new_group(main.begin(), main.begin() + count);
	interpreter::pushobject(L, pgroup);
	return 1;
}
/**
 * \brief Duel.GetExtraTopGroup
 * \param playerid, count
 * \return Group
 */
LUA_FUNCTION(GetExtraTopGroup) {
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
LUA_FUNCTION(GetMatchingGroup) {
	check_param_count(L, 5);
	const auto findex = lua_get<function>(L, 1);
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
	pduel->game_field->filter_matching_card(findex, self, location1, location2, pgroup, pexception, pexgroup, extraargs);
	interpreter::pushobject(L, pgroup);
	return 1;
}
/**
* \brief Duel.GetMatchingGroupCount
* \param filter_func, self, location1, location2, exception card, (extraargs...)
* \return Integer
*/
LUA_FUNCTION(GetMatchingGroupCount) {
	check_param_count(L, 5);
	const auto findex = lua_get<function>(L, 1);
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
	pduel->game_field->filter_matching_card(findex, self, location1, location2, pgroup, pexception, pexgroup, extraargs);
	lua_pushinteger(L, pgroup->container.size());
	return 1;
}
/**
* \brief Duel.GetFirstMatchingCard
* \param filter_func, self, location1, location2, exception card, (extraargs...)
* \return Card | nil
*/
LUA_FUNCTION(GetFirstMatchingCard) {
	check_param_count(L, 5);
	const auto findex = lua_get<function>(L, 1);
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
	pduel->game_field->filter_matching_card(findex, self, location1, location2, 0, pexception, pexgroup, extraargs, &pret);
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
LUA_FUNCTION(IsExistingMatchingCard) {
	check_param_count(L, 6);
	const auto findex = lua_get<function>(L, 1);
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
	lua_pushboolean(L, pduel->game_field->filter_matching_card(findex, self, location1, location2, 0, pexception, pexgroup, extraargs, 0, fcount));
	return 1;
}
/**
* \brief Duel.SelectMatchingCards
* \param playerid, filter_func, self, location1, location2, min, max, exception card, (extraargs...)
* \return Group
*/
LUA_FUNCTION(SelectMatchingCard) {
	check_action_permission(L);
	check_param_count(L, 8);
	const auto findex = lua_get<function>(L, 2);
	card* pexception = 0;
	group* pexgroup = 0;
	bool cancelable = false;
	uint8_t lastarg = 8;
	if(lua_isboolean(L, lastarg)) {
		check_param_count(L, 9);
		cancelable = lua_get<bool, false>(L, lastarg);
		++lastarg;
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
	pduel->game_field->filter_matching_card(findex, self, location1, location2, pgroup, pexception, pexgroup, extraargs);
	pduel->game_field->core.select_cards.assign(pgroup->container.begin(), pgroup->container.end());
	pduel->game_field->add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + (cancelable << 16), min + (max << 16));
	return lua_yieldk(L, 0, (lua_KContext)cancelable, push_return_cards);
}
LUA_FUNCTION(SelectCardsFromCodes) {
	check_action_permission(L);
	check_param_count(L, 6);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.select_cards_codes.clear();
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto min = lua_get<uint16_t>(L, 2);
	auto max = lua_get<uint16_t>(L, 3);
	bool cancelable = lua_get<bool>(L, 4);
	/*bool ret_index = */(void)lua_get<bool>(L, 5);
	lua_iterate_table_or_stack(L, 6, lua_gettop(L), [L, &select_codes = pduel->game_field->core.select_cards_codes]{
		select_codes.emplace_back(lua_get<uint32_t>(L, -1), static_cast<uint32_t>(select_codes.size() + 1));
	});
	pduel->game_field->add_process(PROCESSOR_SELECT_CARD_CODES, 0, 0, 0, playerid + (cancelable << 16), min + (max << 16));
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		const auto pduel = lua_get<duel*>(L);
		int ret = 1;
		const auto& ret_codes = pduel->game_field->return_card_codes;
		if(!ret_codes.canceled) {
			bool ret_index = lua_get<bool>(L, 5);
			luaL_checkstack(L, static_cast<int>(ret_codes.list.size() + (3 * ret_index) /* account for table creation */), nullptr);
			for(const auto& obj : ret_codes.list) {
				if(ret_index) {
					lua_newtable(L);
					lua_pushinteger(L, 1);
				}
				lua_pushinteger(L, obj.first);
				if(ret_index) {
					lua_settable(L, -3);
					lua_pushinteger(L, 2);
					lua_pushinteger(L, obj.second);
					lua_settable(L, -3);
				}
			}
			ret = (int)ret_codes.list.size();
		} else
			lua_pushnil(L);
		return ret;
	});
}
/**
* \brief Duel.GetReleaseGroup
* \param playerid
* \return Group
*/
LUA_FUNCTION(GetReleaseGroup) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	bool hand = lua_get<bool, false>(L, 2);
	bool oppo = lua_get<bool, false>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group();
	pduel->game_field->get_release_list(playerid, &pgroup->container, &pgroup->container, &pgroup->container, hand, 0, 0, 0, 0, oppo);
	interpreter::pushobject(L, pgroup);
	return 1;
}
/**
* \brief Duel.GetReleaseGroupCount
* \param playerid
* \return Integer
*/
LUA_FUNCTION(GetReleaseGroupCount) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	bool hand = lua_get<bool, false>(L, 2);
	bool oppo = lua_get<bool, false>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->get_release_list(playerid, 0, 0, 0, hand, 0, 0, 0, 0, oppo));
	return 1;
}
static int32_t check_release_group(lua_State* L, uint8_t use_hand) {
	check_param_count(L, 4);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto findex = lua_get<function>(L, 2);
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
		++lastarg;
		max = lua_get<uint16_t>(L, lastarg, min);
		++lastarg;
		check_field = lua_get<bool, false>(L, lastarg);
		++lastarg;
		to_check = lua_get<card*>(L, lastarg);
		++lastarg;
		toplayer = lua_get<uint8_t>(L, lastarg, playerid);
		++lastarg;
		zone = lua_get<uint32_t, 0xff>(L, lastarg);
		++lastarg;
		use_oppo = lua_get<bool, false>(L, lastarg);
		++lastarg;
	}
	if((pexception = lua_get<card*>(L, lastarg)) == nullptr)
		pexgroup = lua_get<group*>(L, lastarg);
	uint32_t extraargs = lua_gettop(L) - lastarg;
	int32_t result = pduel->game_field->check_release_list(playerid, min, max, use_hand, findex, extraargs, pexception, pexgroup, check_field, toplayer, zone, to_check, use_oppo);
	pduel->game_field->core.must_select_cards.clear();
	lua_pushboolean(L, result);
	return 1;

}
LUA_FUNCTION(CheckReleaseGroup) {
	return check_release_group(L, FALSE);
}
LUA_FUNCTION(CheckReleaseGroupEx) {
	return check_release_group(L, TRUE);
}
static int32_t select_release_group(lua_State* L, uint8_t use_hand) {
	check_action_permission(L);
	check_param_count(L, 5);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto findex = lua_get<function>(L, 2);
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
		++lastarg;
		cancelable = lua_get<bool, false>(L, lastarg);
		++lastarg;
		check_field = lua_get<bool, false>(L, lastarg);
		++lastarg;
		to_check = lua_get<card*>(L, lastarg);
		++lastarg;
		toplayer = lua_get<uint8_t>(L, lastarg, playerid);
		++lastarg;
		zone = lua_get<uint32_t, 0xff>(L, lastarg);
		++lastarg;
		use_oppo = lua_get<bool, false>(L, lastarg);
		++lastarg;
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
	pduel->game_field->get_release_list(playerid, &pduel->game_field->core.release_cards, &pduel->game_field->core.release_cards_ex, &pduel->game_field->core.release_cards_ex_oneof, use_hand, findex, extraargs, pexception, pexgroup, use_oppo);
	pduel->game_field->add_process(PROCESSOR_SELECT_RELEASE, 0, 0, (group*)to_check, playerid + (cancelable << 16), (max << 16) + min, check_field + (toplayer << 8) + (zone << 16));
	return lua_yieldk(L, 0, (lua_KContext)cancelable, push_return_cards);
}
LUA_FUNCTION(SelectReleaseGroup) {
	return select_release_group(L, false);
}
LUA_FUNCTION(SelectReleaseGroupEx) {
	return select_release_group(L, true);
}
/**
* \brief Duel.GetTributeGroup
* \param targetcard
* \return Group
*/
LUA_FUNCTION(GetTributeGroup) {
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
LUA_FUNCTION(GetTributeCount) {
	check_param_count(L, 1);
	auto target = lua_get<card*, true>(L, 1);
	group* mg = 0;
	if(!lua_isnoneornil(L, 2))
		mg = lua_get<group*, true>(L, 2);
	bool ex = lua_get<bool, false>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->get_summon_release_list(target, nullptr, nullptr, nullptr, mg, ex));
	return 1;
}
LUA_FUNCTION(CheckTribute) {
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
LUA_FUNCTION(SelectTribute) {
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
LUA_FUNCTION(GetTargetCount) {
	check_param_count(L, 5);
	const auto findex = lua_get<function>(L, 1);
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
	pduel->game_field->filter_matching_card(findex, self, location1, location2, pgroup, pexception, pexgroup, extraargs, 0, 0, TRUE);
	lua_pushinteger(L, pgroup->container.size());
	return 1;
}
/**
* \brief Duel.IsExistingTarget
* \param filter_func, self, location1, location2, count, exception card, (extraargs...)
* \return boolean
*/
LUA_FUNCTION(IsExistingTarget) {
	check_param_count(L, 6);
	const auto findex = lua_get<function>(L, 1);
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
	lua_pushboolean(L, pduel->game_field->filter_matching_card(findex, self, location1, location2, 0, pexception, pexgroup, extraargs, 0, count, TRUE));
	return 1;
}
/**
* \brief Duel.SelectTarget
* \param playerid, filter_func, self, location1, location2, min, max, exception card, (extraargs...)
* \return Group
*/
LUA_FUNCTION(SelectTarget) {
	check_action_permission(L);
	check_param_count(L, 8);
	const auto findex = lua_get<function>(L, 2);
	card* pexception = 0;
	group* pexgroup = 0;
	bool cancelable = false;
	uint8_t lastarg = 8;
	if(lua_isboolean(L, lastarg)) {
		check_param_count(L, 9);
		cancelable = lua_get<bool, false>(L, lastarg);
		++lastarg;
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
	pduel->game_field->filter_matching_card(findex, self, location1, location2, pgroup, pexception, pexgroup, extraargs, 0, 0, TRUE);
	pduel->game_field->core.select_cards.assign(pgroup->container.begin(), pgroup->container.end());
	pduel->game_field->add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + (cancelable << 16), min + (max << 16));
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		const auto pduel = lua_get<duel*>(L);
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
LUA_FUNCTION(SelectFusionMaterial) {
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
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		const auto pduel = lua_get<duel*>(L);
		group* pgroup = pduel->new_group(pduel->game_field->core.fusion_materials);
		interpreter::pushobject(L, pgroup);
		return 1;
	});
}
LUA_FUNCTION(SetFusionMaterial) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pgroup = lua_get<group*, true>(L, 1);
	pduel->game_field->core.fusion_materials = pgroup->container;
	return 0;
}
LUA_FUNCTION(GetRitualMaterial) {
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
LUA_FUNCTION(ReleaseRitualMaterial) {
	check_action_permission(L);
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pgroup = lua_get<group*, true>(L, 1);
	pduel->game_field->ritual_release(pgroup->container);
	return lua_yield(L, 0);
}
LUA_FUNCTION(GetFusionMaterial) {
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
LUA_FUNCTION(IsSummonCancelable) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->core.summon_cancelable);
	return 1;
}
LUA_FUNCTION(SetSelectedCard) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 1, pcard, pgroup);
	if(pcard)
		pduel->game_field->core.must_select_cards = { pcard };
	else
		pduel->game_field->core.must_select_cards.assign(pgroup->container.begin(), pgroup->container.end());
	return 0;
}
LUA_FUNCTION(GrabSelectedCard) {
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group(pduel->game_field->core.must_select_cards);
	pduel->game_field->core.must_select_cards.clear();
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(SetTargetCard) {
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
LUA_FUNCTION(ClearTargetCard) {
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(0);
	if(ch && ch->target_cards)
		ch->target_cards->container.clear();
	return 0;
}
LUA_FUNCTION(SetTargetPlayer) {
	check_action_permission(L);
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(0);
	if(ch)
		ch->target_player = playerid;
	return 0;
}
LUA_FUNCTION(SetTargetParam) {
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
LUA_FUNCTION(SetOperationInfo) {
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
		if(cate == 0x200 && playerid == PLAYER_ALL && opt.op_cards->container.size() != 2)
			lua_error(L, "Called Duel.SetOperationInfo with CATEGORY_SPECIAL_SUMMON and PLAYER_ALL but the group size wasn't exactly 2.");
		opt.op_cards->is_readonly = TRUE;
	}
	auto omit = ch->opinfos.find(cate);
	if(omit != ch->opinfos.end() && omit->second.op_cards)
		pduel->delete_group(omit->second.op_cards);
	ch->opinfos[cate] = std::move(opt);
	return 0;
}
LUA_FUNCTION(GetOperationInfo) {
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
LUA_FUNCTION(SetPossibleOperationInfo) {
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
LUA_FUNCTION(GetPossibleOperationInfo) {
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
LUA_FUNCTION(GetOperationCount) {
	check_param_count(L, 1);
	auto ct = lua_get<uint32_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	chain* ch = pduel->game_field->get_chain(ct);
	if(!ch)
		return 0;
	lua_pushinteger(L, ch->opinfos.size());
	return 1;
}
LUA_FUNCTION(ClearOperationInfo) {
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
LUA_FUNCTION(Overlay) {
	check_action_permission(L);
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto target = lua_get<card*, true>(L, 1);
	if(target->overlay_target != nullptr)
		lua_error(L, "Attempt to overlay materials to a card that is an overlay material.");
	card* pcard = nullptr;
	group* pgroup = nullptr;
	get_card_or_group(L, 2, pcard, pgroup);
	bool send_materials_to_grave = lua_get<bool, false>(L, 3);
	if(pcard) {
		if(pcard == target)
			lua_error(L, "Attempt to overlay a card with itself.");
		pduel->game_field->xyz_overlay(target, pcard, send_materials_to_grave);
	} else {
		if(pgroup->has_card(target))
			lua_error(L, "Attempt to overlay a card with itself.");
		pduel->game_field->xyz_overlay(target, pgroup->container, send_materials_to_grave);
	}
	return lua_yield(L, 0);
}
LUA_FUNCTION(GetOverlayGroup) {
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
LUA_FUNCTION(GetOverlayCount) {
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
LUA_FUNCTION(CheckRemoveOverlayCard) {
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
LUA_FUNCTION(RemoveOverlayCard) {
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
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(Hint) {
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
LUA_FUNCTION(HintSelection) {
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
		for(auto& _pcard : pgroup->container)
			message->write(_pcard->get_info_location());
	}
	return 0;
}
LUA_FUNCTION(SelectEffectYesNo) {
	check_action_permission(L);
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto desc = lua_get<uint64_t, 95>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_SELECT_EFFECTYN, 0, 0, (group*)pcard, playerid, desc);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushboolean(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(SelectYesNo) {
	check_action_permission(L);
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto desc = lua_get<uint64_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, playerid, desc);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushboolean(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(SelectOption) {
	check_action_permission(L);
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.select_options.clear();
	lua_iterate_table_or_stack(L, 2 + lua_isboolean(L, 2), lua_gettop(L), [L, &select_options = pduel->game_field->core.select_options] {
		select_options.push_back(lua_get<uint64_t>(L, -1));
	});
	pduel->game_field->add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, playerid, 0);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		const auto pduel = lua_get<duel*>(L);
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
LUA_FUNCTION(SelectPosition) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto pcard = lua_get<card*, true>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto positions = lua_get<uint8_t>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_SELECT_POSITION, 0, 0, 0, playerid + (positions << 16), pcard->data.code);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(SelectDisableField) {
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
		ct1 = pduel->game_field->get_useable_count(nullptr, playerid, LOCATION_MZONE, PLAYER_NONE, 0, 0xff, &plist);
		if (all_field) {
			plist &= ~0x60;
			if (!pduel->game_field->is_location_useable(playerid, LOCATION_MZONE, 5))
				plist |= 0x20;
			else
				++ct1;
			if (!pduel->game_field->is_location_useable(playerid, LOCATION_MZONE, 6))
				plist |= 0x40;
			else
				++ct1;
		}
		flag = (flag & 0xffffff00) | plist;
	}
	if(location1 & LOCATION_SZONE) {
		ct2 = pduel->game_field->get_useable_count(nullptr, playerid, LOCATION_SZONE, PLAYER_NONE, 0, 0xff, &plist);
		if (all_field) {
			plist &= ~0xe0;
			if (!pduel->game_field->is_location_useable(playerid, LOCATION_SZONE, 5))
				plist |= 0x20;
			else
				++ct2;
			if (!pduel->game_field->is_location_useable(playerid, LOCATION_SZONE, 6))
				plist |= 0x40;
			else
				++ct2;
			if (!pduel->game_field->is_location_useable(playerid, LOCATION_SZONE, 7))
				plist |= 0x80;
			else
				++ct2;
		}
		flag = (flag & 0xffff00ff) | (plist << 8);
	}
	if(location2 & LOCATION_MZONE) {
		ct3 = pduel->game_field->get_useable_count(nullptr, 1 - playerid, LOCATION_MZONE, PLAYER_NONE, 0, 0xff, &plist);
		if (all_field) {
			plist &= ~0x60;
			if (!pduel->game_field->is_location_useable(1 - playerid, LOCATION_MZONE, 5))
				plist |= 0x20;
			else
				++ct3;
			if (!pduel->game_field->is_location_useable(1 - playerid, LOCATION_MZONE, 6))
				plist |= 0x40;
			else
				++ct3;
		}
		flag = (flag & 0xff00ffff) | (plist << 16);
	}
	if(location2 & LOCATION_SZONE) {
		ct4 = pduel->game_field->get_useable_count(nullptr, 1 - playerid, LOCATION_SZONE, PLAYER_NONE, 0, 0xff, &plist);
		if (all_field) {
			plist &= ~0xe0;
			if (!pduel->game_field->is_location_useable(1 - playerid, LOCATION_SZONE, 5))
				plist |= 0x20;
			else
				++ct4;
			if (!pduel->game_field->is_location_useable(1 - playerid, LOCATION_SZONE, 6))
				plist |= 0x40;
			else
				++ct4;
			if (!pduel->game_field->is_location_useable(1 - playerid, LOCATION_SZONE, 7))
				plist |= 0x80;
			else
				++ct4;
		}
		flag = (flag & 0xffffff) | (plist << 24);
	}
	flag |= filter | ((all_field) ? 0x800080 : 0xe0e0e0e0);
	if(count > ct1 + ct2 + ct3 + ct4)
		count = ct1 + ct2 + ct3 + ct4;
	if(count == 0)
		return 0;
	pduel->game_field->add_process(PROCESSOR_SELECT_DISFIELD, 0, 0, 0, playerid, flag, count);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		const auto pduel = lua_get<duel*>(L);
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
LUA_FUNCTION(SelectFieldZone) {
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
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		const auto pduel = lua_get<duel*>(L);
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
LUA_FUNCTION(AnnounceRace) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto available = lua_get<uint64_t>(L, 3);
	if(bit::has_invalid_bits(available, RACE_ALL))
		lua_error(L, "Passed an invalid race.");
	auto count = std::min(lua_get<uint8_t>(L, 2), bit::popcnt(available));
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_ANNOUNCE_RACE, 0, 0, 0, playerid + (count << 16), available);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<uint64_t>(0));
		return 1;
	});
}
LUA_FUNCTION(AnnounceAttribute) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto available = lua_get<uint32_t>(L, 3);
	if(bit::has_invalid_bits(available, ATTRIBUTE_ALL))
		lua_error(L, "Passed an invalid attribute.");
	auto count = std::min(lua_get<uint8_t>(L, 2), bit::popcnt(available));
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_ANNOUNCE_ATTRIB, 0, 0, 0, playerid + (count << 16), available);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(AnnounceNumberRange) {
	check_action_permission(L);
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto min = lua_get<uint32_t, 1>(L, 2);
	auto max = lua_get<uint32_t, 12>(L, 3);
	if(min > max)
		std::swap(min, max);
	const auto pduel = lua_get<duel*>(L);
	std::set<uint32_t> excluded;
	lua_iterate_table_or_stack(L, 4, lua_gettop(L), [L, &excluded, min, max] {
		if(!lua_isnil(L, -1)) {
			auto val = lua_get<uint32_t>(L, -1);
			if(val >= min && val <= max)
				excluded.insert(val);
		}
	});
	auto& select_options = pduel->game_field->core.select_options;
	if(excluded.empty()) {
		select_options.resize((max - min) + 1);
		std::iota(select_options.begin(), select_options.end(), min);
	} else {
		select_options.clear();
		auto it = excluded.cbegin();
		const auto cend = excluded.cend();
		uint32_t i = min;
		for(; i <= max; ++i) {
			if(it == cend)
				break;
			if(*it == i) {
				++it;
				continue;
			}
			select_options.push_back(i);
		}
		for(; i <= max; ++i)
			select_options.push_back(i);
	}
	if(select_options.empty())
		return 0;
	pduel->game_field->add_process(PROCESSOR_ANNOUNCE_NUMBER, 0, 0, 0, playerid, 0);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		const auto pduel = lua_get<duel*>(L);
		lua_pushinteger(L, pduel->game_field->core.select_options[pduel->game_field->returns.at<int32_t>(0)]);
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 2;
	});
}
LUA_FUNCTION_ALIAS(AnnounceLevel);
LUA_FUNCTION(AnnounceCard) {
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
	if(stack_size != 1 && has_opcodes)
		lua_error(L, "Parameters are invalid.");
	if(!has_opcodes) {
		pduel->game_field->core.select_options.push_back(TYPE_MONSTER | TYPE_SPELL | TYPE_TRAP);
		pduel->game_field->core.select_options.push_back(OPCODE_ISTYPE);
	}
	pduel->game_field->add_process(PROCESSOR_ANNOUNCE_CARD, 0, 0, 0, playerid, 0);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(AnnounceType) {
	check_action_permission(L);
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 1);
	pduel->game_field->core.select_options.clear();
	pduel->game_field->core.select_options.push_back(70);
	pduel->game_field->core.select_options.push_back(71);
	pduel->game_field->core.select_options.push_back(72);
	pduel->game_field->add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, playerid, 0);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		const auto pduel = lua_get<duel*>(L);
		auto playerid = lua_get<uint8_t>(L, 1);
		auto message = pduel->new_message(MSG_HINT);
		message->write<uint8_t>(HINT_OPSELECTED);
		message->write<uint8_t>(playerid);
		message->write<uint64_t>(pduel->game_field->core.select_options[pduel->game_field->returns.at<int32_t>(0)]);
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(AnnounceNumber) {
	check_action_permission(L);
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->core.select_options.clear();
	lua_iterate_table_or_stack(L, 2, lua_gettop(L), [L, &select_options = pduel->game_field->core.select_options] {
		select_options.push_back(lua_get<uint64_t>(L, -1));
	});
	pduel->game_field->add_process(PROCESSOR_ANNOUNCE_NUMBER, 0, 0, 0, playerid, 0);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		const auto pduel = lua_get<duel*>(L);
		lua_pushinteger(L, pduel->game_field->core.select_options[pduel->game_field->returns.at<int32_t>(0)]);
		lua_pushinteger(L, pduel->game_field->returns.at<int32_t>(0));
		return 2;
	});
}
LUA_FUNCTION(AnnounceCoin) {
	check_action_permission(L);
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto desc = lua_get<uint64_t, 552>(L, 2);
	auto message = pduel->new_message(MSG_HINT);
	message->write<uint8_t>(HINT_SELECTMSG);
	message->write<uint8_t>(playerid);
	message->write<uint64_t>(desc);
	pduel->game_field->core.select_options.clear();
	pduel->game_field->core.select_options.push_back(60);
	pduel->game_field->core.select_options.push_back(61);
	pduel->game_field->add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, playerid, 0);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		const auto pduel = lua_get<duel*>(L);
		if(/*bool sel_hint = */lua_get<bool, true>(L, 2)) {
			auto playerid = lua_get<uint8_t>(L, 1);
			auto message = pduel->new_message(MSG_HINT);
			message->write<uint8_t>(HINT_OPSELECTED);
			message->write<uint8_t>(playerid);
			message->write<uint64_t>(pduel->game_field->core.select_options[pduel->game_field->returns.at<int32_t>(0)]);
		}
		lua_pushinteger(L, 1 - pduel->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(TossCoin) {
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
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) -> int {
		const auto pduel = lua_get<duel*>(L);
		const auto& coin_results = pduel->game_field->core.coin_results;
		luaL_checkstack(L, static_cast<int>(coin_results.size()), nullptr);
		for(const auto& result : coin_results)
			lua_pushinteger(L, static_cast<int8_t>(result));
		return static_cast<int32_t>(coin_results.size());
	});
}
LUA_FUNCTION(TossDice) {
	check_action_permission(L);
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	auto count1 = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto count2 = lua_get<uint8_t, 0>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->add_process(PROCESSOR_TOSS_DICE, 0, pduel->game_field->core.reason_effect, 0, (pduel->game_field->core.reason_player << 16) + playerid, count1 + (count2 << 16));
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) -> int {
		const auto pduel = lua_get<duel*>(L);
		const auto& dice_results = pduel->game_field->core.dice_results;
		luaL_checkstack(L, static_cast<int>(dice_results.size()), nullptr);
		for(const auto& result : dice_results)
			lua_pushinteger(L, result);
		return static_cast<int32_t>(dice_results.size());
	});
}
LUA_FUNCTION(RockPaperScissors) {
	const auto pduel = lua_get<duel*>(L);
	uint8_t repeat = lua_get<bool, true>(L, 1);
	pduel->game_field->add_process(PROCESSOR_ROCK_PAPER_SCISSORS, 0, 0, 0, repeat, 0);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		lua_pushinteger(L, lua_get<duel*>(L)->game_field->returns.at<int32_t>(0));
		return 1;
	});
}
LUA_FUNCTION(GetCoinResult) {
	const auto& coin_results = lua_get<duel*>(L)->game_field->core.coin_results;
	luaL_checkstack(L, static_cast<int>(coin_results.size()), nullptr);
	for(const auto& coin : coin_results)
		lua_pushinteger(L, static_cast<int>(coin));
	return static_cast<int32_t>(coin_results.size());
}
LUA_FUNCTION(GetDiceResult) {
	const auto& dice_results = lua_get<duel*>(L)->game_field->core.dice_results;
	luaL_checkstack(L, static_cast<int>(dice_results.size()), nullptr);
	for(const auto& dice : dice_results)
		lua_pushinteger(L, dice);
	return static_cast<int32_t>(dice_results.size());
}
LUA_FUNCTION(SetCoinResult) {
	auto& coin_results = lua_get<duel*>(L)->game_field->core.coin_results;
	if(lua_gettop(L) != (int)coin_results.size())
		lua_error(L, "The number of coin results passed didn't match the expected amount of %d. (Passed %d)", coin_results.size(), lua_gettop(L));
	for(size_t i = 0; i < coin_results.size(); ++i)
		coin_results[i] = static_cast<bool>(lua_get<uint8_t>(L, static_cast<int>(i + 1)));
	return 0;
}
LUA_FUNCTION(SetDiceResult) {
	auto& dice_results = lua_get<duel*>(L)->game_field->core.dice_results;
	if(lua_gettop(L) != (int)dice_results.size())
		lua_error(L, "The number of dice results passed didn't match the expected amount of %d. (Passed %d)", dice_results.size(), lua_gettop(L));
	for(size_t i = 0; i < dice_results.size(); ++i)
		dice_results[i] = lua_get<uint8_t>(L, static_cast<int>(i + 1));
	return 0;
}
LUA_FUNCTION(IsDuelType) {
	check_param_count(L, 1);
	auto duel_type = lua_get<uint64_t>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->is_flag(duel_type));
	return 1;
}
LUA_FUNCTION(GetDuelType) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->core.duel_options);
	return 1;
}
LUA_FUNCTION(IsPlayerAffectedByEffect) {
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
LUA_FUNCTION(GetPlayerEffect) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushnil(L);
		return 1;
	}
	auto code = lua_get<uint32_t, 0>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	effect_set eset;
	pduel->game_field->get_player_effect(playerid, code, &eset);
	if(eset.empty()) {
		lua_pushnil(L);
		return 1;
	}
	luaL_checkstack(L, static_cast<int>(eset.size()), nullptr);
	for(const auto& peff : eset)
		interpreter::pushobject(L, peff);
	return static_cast<int32_t>(eset.size());
}
LUA_FUNCTION(IsPlayerCanDraw) {
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
LUA_FUNCTION(IsPlayerCanDiscardDeck) {
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
LUA_FUNCTION(IsPlayerCanDiscardDeckAsCost) {
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
LUA_FUNCTION(IsPlayerCanSummon) {
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
LUA_FUNCTION(CanPlayerSetMonster) {
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
LUA_FUNCTION(CanPlayerSetSpellTrap) {
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
LUA_FUNCTION(IsPlayerCanSpecialSummon) {
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
LUA_FUNCTION(IsPlayerCanFlipSummon) {
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
LUA_FUNCTION(IsPlayerCanSpecialSummonMonster) {
	check_param_count(L, 9);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	auto code = lua_get<uint32_t>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	card_data dat = pduel->read_card(code);
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
		dat.race = lua_get<uint64_t>(L, 8);
	if(!lua_isnoneornil(L, 9))
		dat.attribute = lua_get<uint32_t>(L, 9);
	auto pos = lua_get<uint8_t, POS_FACEUP>(L, 10);
	auto toplayer = lua_get<uint8_t>(L, 11, playerid);
	auto sumtype = lua_get<uint32_t, 0>(L, 12);
	lua_pushboolean(L, pduel->game_field->is_player_can_spsummon_monster(playerid, toplayer, pos, sumtype, &dat));
	return 1;
}
LUA_FUNCTION(IsPlayerCanSpecialSummonCount) {
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
LUA_FUNCTION(IsPlayerCanRelease) {
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
LUA_FUNCTION(IsPlayerCanRemove) {
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
LUA_FUNCTION(IsPlayerCanSendtoHand) {
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
LUA_FUNCTION(IsPlayerCanSendtoGrave) {
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
LUA_FUNCTION(IsPlayerCanSendtoDeck) {
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
LUA_FUNCTION(IsPlayerCanAdditionalSummon) {
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
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		pduel->lua->add_param<PARAM_TYPE_BOOLEAN>(TRUE);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(oreason);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
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
LUA_FUNCTION(IsPlayerCanPendulumSummon) {
	return is_player_can_procedure_summon_group(L, SUMMON_TYPE_PENDULUM, 0);
}
LUA_FUNCTION(IsPlayerCanProcedureSummonGroup) {
	check_param_count(L, 2);
	auto sumtype = lua_get<uint32_t>(L, 2);
	return is_player_can_procedure_summon_group(L, sumtype, 1);
}
LUA_FUNCTION(IsChainNegatable) {
	check_param_count(L, 1);
	lua_pushboolean(L, 1);
	return 1;
}
LUA_FUNCTION(IsChainDisablable) {
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
LUA_FUNCTION(IsChainSolving) {
	lua_pushboolean(L, lua_get<duel*>(L)->game_field->core.chain_solving);
	return 1;
}
LUA_FUNCTION(CheckChainTarget) {
	check_param_count(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto chaincount = lua_get<uint8_t>(L, 1);
	auto pcard = lua_get<card*, true>(L, 2);
	lua_pushboolean(L, pduel->game_field->check_chain_target(chaincount, pcard));
	return 1;
}
LUA_FUNCTION(CheckChainUniqueness) {
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
LUA_FUNCTION(GetActivityCount) {
	check_param_count(L, 2);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	auto top = lua_gettop(L);
	int32_t retct = static_cast<int32_t>(lua_istable(L, 2) ? lua_rawlen(L, 2) : top - 1);
	luaL_checkstack(L, retct, nullptr);
	lua_iterate_table_or_stack(L, 2, top, [L, &core = pduel->game_field->core, playerid]() -> int {
		auto activity_type = static_cast<ActivityType>(lua_get<uint8_t>(L, -1));
		switch(activity_type) {
		case ACTIVITY_SUMMON:
			lua_pushinteger(L, core.summon_state_count[playerid]);
			break;
		case ACTIVITY_NORMALSUMMON:
			lua_pushinteger(L, core.normalsummon_state_count[playerid]);
			break;
		case ACTIVITY_SPSUMMON:
			lua_pushinteger(L, core.spsummon_state_count[playerid]);
			break;
		case ACTIVITY_FLIPSUMMON:
			lua_pushinteger(L, core.flipsummon_state_count[playerid]);
			break;
		case ACTIVITY_ATTACK:
			lua_pushinteger(L, core.attack_state_count[playerid]);
			break;
		case ACTIVITY_BATTLE_PHASE:
			lua_pushinteger(L, core.battle_phase_count[playerid]);
			break;
		default:
			lua_error(L, "Passed invalid ACTIVITY flag.");
			break;
		}
		return 1;
	});
	return retct;
}
LUA_FUNCTION(CheckPhaseActivity) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->core.phase_action);
	return 1;
}
LUA_FUNCTION(AddCustomActivityCounter) {
	check_param_count(L, 3);
	const auto findex = lua_get<function, true>(L, 3);
	auto counter_id = lua_get<uint32_t>(L, 1);
	auto activity_type = static_cast<ActivityType>(lua_get<uint8_t>(L, 2));
	if(activity_type == ACTIVITY_BATTLE_PHASE || activity_type > ACTIVITY_CHAIN)
		lua_error(L, "Passed invalid ACTIVITY counter.");
	int32_t counter_filter = interpreter::get_function_handle(L, findex);
	const auto pduel = lua_get<duel*>(L);
	auto& counter_map = pduel->game_field->core.get_counter_map(activity_type);
	if(counter_map.find(counter_id) != counter_map.end())
		return 0;
	counter_map.emplace(counter_id, processor::action_value_t{ counter_filter, { 0, 0 } });
	return 0;
}
LUA_FUNCTION(GetCustomActivityCount) {
	check_param_count(L, 3);
	auto counter_id = lua_get<uint32_t>(L, 1);
	auto playerid = lua_get<uint8_t>(L, 2);
	auto activity_type = static_cast<ActivityType>(lua_get<uint8_t>(L, 3));
	if(activity_type == ACTIVITY_BATTLE_PHASE || activity_type > ACTIVITY_CHAIN || activity_type == 0)
		lua_error(L, "Passed invalid ACTIVITY counter.");
	const auto pduel = lua_get<duel*>(L);
	auto& counter_map = pduel->game_field->core.get_counter_map(activity_type);
	int32_t val = 0;
	auto it = counter_map.find(counter_id);
	if(it != counter_map.end())
		val = it->second.player_amount[playerid];
	lua_pushinteger(L, val);
	return 1;
}

LUA_FUNCTION(GetBattledCount) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->core.battled_count[playerid]);
	return 1;
}
LUA_FUNCTION(IsAbleToEnterBP) {
	const auto pduel = lua_get<duel*>(L);
	lua_pushboolean(L, pduel->game_field->is_able_to_enter_bp());
	return 1;
}
LUA_FUNCTION(GetRandomNumber) {
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
LUA_FUNCTION(AssumeReset) {
	lua_get<duel*>(L)->restore_assumes();
	return 0;
}
LUA_FUNCTION(GetCardFromCardID) {
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
LUA_FUNCTION(LoadScript) {
	using SLS = duel::SCRIPT_LOAD_STATUS;
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	const auto* string = lua_tolstring(L, 1, nullptr);
	if(!string)
		lua_error(L, "Parameter 1 should be \"String\".");
	if(/*auto check_cache = */lua_get<bool, true>(L, 2)) {
		auto hash = [](const char* str)->uint32_t {
			uint32_t hash = 5381, c;
			while((c = *str++) != 0)
				hash = (((hash << 5) + hash) + c) & 0xffffffff; /* hash * 33 + c */
			return hash;
		}(string);
		auto& load_status = pduel->loaded_scripts[hash];
		if(load_status == SLS::LOADING)
			lua_error(L, "Recursive script loading detected.");
		else if(load_status != SLS::NOT_LOADED)
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
LUA_FUNCTION(TagSwap) {
	check_action_permission(L);
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if (playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->tag_swap(playerid);
	return lua_yield(L, 0);
}
LUA_FUNCTION(GetPlayersCount) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->get_player_count(playerid));
	return 1;
}
LUA_FUNCTION(SwapDeckAndGrave) {
	check_action_permission(L);
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	pduel->game_field->swap_deck_and_grave(playerid);
	return 0;
}
LUA_FUNCTION(MajesticCopy) {
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
LUA_FUNCTION(GetStartingHand) {
	check_param_count(L, 1);
	auto playerid = lua_get<uint8_t>(L, 1);
	if(playerid != 0 && playerid != 1)
		return 0;
	const auto pduel = lua_get<duel*>(L);
	lua_pushinteger(L, pduel->game_field->player[playerid].start_count);
	return 1;
}
#define INFO_FUNC_FROM_CODE(lua_name,attr) \
LUA_FUNCTION(GetCard ##lua_name ##FromCode) { \
	check_param_count(L, 1); \
	auto code = lua_get<uint32_t>(L, 1); \
	const auto& data = lua_get<duel*>(L)->read_card(code); \
	if(data.code == 0) \
		return 0; \
	lua_pushinteger(L, data.attr); \
	return 1; \
}
INFO_FUNC_FROM_CODE(Alias, alias)
INFO_FUNC_FROM_CODE(Type, type)
INFO_FUNC_FROM_CODE(Level, level)
INFO_FUNC_FROM_CODE(Attribute, attribute)
INFO_FUNC_FROM_CODE(Race, race)
INFO_FUNC_FROM_CODE(Attack, attack)
INFO_FUNC_FROM_CODE(Defense, defense)
INFO_FUNC_FROM_CODE(Rscale, rscale)
INFO_FUNC_FROM_CODE(Lscale, lscale)
INFO_FUNC_FROM_CODE(LinkMarker, link_marker)
#undef INFO_FUNC_FROM_CODE

LUA_FUNCTION(GetCardSetcodeFromCode) {
	check_param_count(L, 1);
	auto code = lua_get<uint32_t>(L, 1);
	const auto& data = lua_get<duel*>(L)->read_card(code);
	if(data.code == 0)
		return 0;
	luaL_checkstack(L, static_cast<int>(data.setcodes.size()), nullptr);
	for(auto& setcode : data.setcodes)
		lua_pushinteger(L, setcode);
	return static_cast<int32_t>(data.setcodes.size());
}
}

void scriptlib::push_duel_lib(lua_State* L) {
	static constexpr auto duellib = GET_LUA_FUNCTIONS_ARRAY();
	static_assert(duellib.back().name == nullptr, "");
	lua_createtable(L, 0, static_cast<int>(duellib.size() - 1));
	luaL_setfuncs(L, duellib.data(), 0);
	lua_setglobal(L, "Duel");
}
