/*
 * playerop.cpp
 *
 *  Created on: 2010-12-22
 *      Author: Argon
 */

#include "field.h"
#include "duel.h"
#include "effect.h"
#include "card.h"

#include <algorithm>
#include <stack>
#include <bitset>

int32 field::select_battle_command(uint16 step, uint8 playerid) {
	if(step == 0) {
		auto message = pduel->new_message(MSG_SELECT_BATTLECMD);
		message->write<uint8>(playerid);
		//Activatable
		message->write<uint32>(core.select_chains.size());
		std::sort(core.select_chains.begin(), core.select_chains.end(), chain::chain_operation_sort);
		for(const auto& ch : core.select_chains) {
			effect* peffect = ch.triggering_effect;
			card* pcard = peffect->get_handler();
			message->write<uint32>(pcard->data.code);
			message->write<uint8>(pcard->current.controler);
			message->write<uint8>(pcard->current.location);
			message->write<uint32>(pcard->current.sequence);
			message->write<uint64>(peffect->description);
			message->write<uint8>(peffect->get_client_mode());
		}
		//Attackable
		message->write<uint32>(core.attackable_cards.size());
		for(auto& pcard : core.attackable_cards) {
			message->write<uint32>(pcard->data.code);
			message->write<uint8>(pcard->current.controler);
			message->write<uint8>(pcard->current.location);
			message->write<uint8>(pcard->current.sequence);
			message->write<uint8>(pcard->direct_attackable);
		}
		//M2, EP
		if(core.to_m2)
			message->write<uint8>(1);
		else
			message->write<uint8>(0);
		if(core.to_ep)
			message->write<uint8>(1);
		else
			message->write<uint8>(0);
		return FALSE;
	} else {
		uint32 t = returns.at<int32>(0) & 0xffff;
		uint32 s = returns.at<int32>(0) >> 16;
		if(t > 3
		   || (t == 0 && s >= core.select_chains.size())
		   || (t == 1 && s >= core.attackable_cards.size())
		   || (t == 2 && !core.to_m2)
		   || (t == 3 && !core.to_ep)) {
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		return TRUE;
	}
}
int32 field::select_idle_command(uint16 step, uint8 playerid) {
	if(step == 0) {
		auto message = pduel->new_message(MSG_SELECT_IDLECMD);
		message->write<uint8>(playerid);
		//idle summon
		message->write<uint32>(core.summonable_cards.size());
		for(auto& pcard : core.summonable_cards) {
			message->write<uint32>(pcard->data.code);
			message->write<uint8>(pcard->current.controler);
			message->write<uint8>(pcard->current.location);
			message->write<uint32>(pcard->current.sequence);
		}
		//idle spsummon
		message->write<uint32>(core.spsummonable_cards.size());
		for(auto& pcard : core.spsummonable_cards) {
			message->write<uint32>(pcard->data.code);
			message->write<uint8>(pcard->current.controler);
			message->write<uint8>(pcard->current.location);
			message->write<uint32>(pcard->current.sequence);
		}
		//idle pos change
		message->write<uint32>(core.repositionable_cards.size());
		for(auto& pcard : core.repositionable_cards) {
			message->write<uint32>(pcard->data.code);
			message->write<uint8>(pcard->current.controler);
			message->write<uint8>(pcard->current.location);
			message->write<uint8>(pcard->current.sequence);
		}
		//idle mset
		message->write<uint32>(core.msetable_cards.size());
		for(auto& pcard : core.msetable_cards) {
			message->write<uint32>(pcard->data.code);
			message->write<uint8>(pcard->current.controler);
			message->write<uint8>(pcard->current.location);
			message->write<uint32>(pcard->current.sequence);
		}
		//idle sset
		message->write<uint32>(core.ssetable_cards.size());
		for(auto& pcard : core.ssetable_cards) {
			message->write<uint32>(pcard->data.code);
			message->write<uint8>(pcard->current.controler);
			message->write<uint8>(pcard->current.location);
			message->write<uint32>(pcard->current.sequence);
		}
		//idle activate
		message->write<uint32>(core.select_chains.size());
		std::sort(core.select_chains.begin(), core.select_chains.end(), chain::chain_operation_sort);
		for(const auto& ch : core.select_chains) {
			effect* peffect = ch.triggering_effect;
			card* pcard = peffect->get_handler();
			message->write<uint32>(pcard->data.code);
			message->write<uint8>(pcard->current.controler);
			message->write<uint8>(pcard->current.location);
			message->write<uint32>(pcard->current.sequence);
			message->write<uint64>(peffect->description);
			message->write<uint8>(peffect->get_client_mode());
		}
		//To BP
		if(infos.phase == PHASE_MAIN1 && core.to_bp)
			message->write<uint8>(1);
		else
			message->write<uint8>(0);
		if(core.to_ep)
			message->write<uint8>(1);
		else
			message->write<uint8>(0);
		if(infos.can_shuffle && player[playerid].list_hand.size() > 1)
			message->write<uint8>(1);
		else
			message->write<uint8>(0);
		return FALSE;
	} else {
		uint32 t = returns.at<int32>(0) & 0xffff;
		uint32 s = returns.at<int32>(0) >> 16;
		if(t > 8
		   || (t == 0 && s >= core.summonable_cards.size())
		   || (t == 1 && s >= core.spsummonable_cards.size())
		   || (t == 2 && s >= core.repositionable_cards.size())
		   || (t == 3 && s >= core.msetable_cards.size())
		   || (t == 4 && s >= core.ssetable_cards.size())
		   || (t == 5 && s >= core.select_chains.size())
		   || (t == 6 && (infos.phase != PHASE_MAIN1 || !core.to_bp))
		   || (t == 7 && !core.to_ep)
		   || (t == 8 && !(infos.can_shuffle && player[playerid].list_hand.size() > 1))) {
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		return TRUE;
	}
}
int32 field::select_effect_yes_no(uint16 step, uint8 playerid, uint64 description, card* pcard) {
	if(step == 0) {
		if((playerid == 1) && is_flag(DUEL_SIMPLE_AI)) {
			returns.at<int32>(0) = 1;
			return TRUE;
		}
		auto message = pduel->new_message(MSG_SELECT_EFFECTYN);
		message->write<uint8>(playerid);
		message->write<uint32>(pcard->data.code);
		message->write(pcard->get_info_location());
		message->write<uint64>(description);
		returns.at<int32>(0) = -1;
		return FALSE;
	} else {
		if(returns.at<int32>(0) != 0 && returns.at<int32>(0) != 1) {
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		return TRUE;
	}
}
int32 field::select_yes_no(uint16 step, uint8 playerid, uint64 description) {
	if(step == 0) {
		if((playerid == 1) && is_flag(DUEL_SIMPLE_AI)) {
			returns.at<int32>(0) = 1;
			return TRUE;
		}
		auto message = pduel->new_message(MSG_SELECT_YESNO);
		message->write<uint8>(playerid);
		message->write<uint64>(description);
		returns.at<int32>(0) = -1;
		return FALSE;
	} else {
		if(returns.at<int32>(0) != 0 && returns.at<int32>(0) != 1) {
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		return TRUE;
	}
}
int32 field::select_option(uint16 step, uint8 playerid) {
	if(step == 0) {
		returns.at<int32>(0) = -1;
		if(core.select_options.size() == 0)
			return TRUE;
		if((playerid == 1) && is_flag(DUEL_SIMPLE_AI)) {
			returns.at<int32>(0) = 0;
			return TRUE;
		}
		auto message = pduel->new_message(MSG_SELECT_OPTION);
		message->write<uint8>(playerid);
		message->write<uint8>(static_cast<uint8>(core.select_options.size()));
		for(auto& option : core.select_options)
			message->write<uint64>(option);
		return FALSE;
	} else {
		if(returns.at<int32>(0) < 0 || returns.at<int32>(0) >= (int32)core.select_options.size()) {
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		return TRUE;
	}
}
bool field::parse_response_cards(uint8 cancelable) {
	int type = returns.at<int32>(0);
	if(type == -1) {
		if(cancelable) {
			return_cards.canceled = true;
			return true;
		}
		return false;
	}
	auto& list = return_cards.list;
	if(type == 3) {
		for(int32 i = 0; i < (int32)core.select_cards.size(); i++) {
			if(returns.bitGet(i + (sizeof(int) * 8)))
				list.push_back(core.select_cards[i]);
		}
	} else {
		uint32 size = returns.at<int32>(1);
		for(uint32 i = 0; i < size; ++i) {
			list.push_back(core.select_cards[
					(type == 0) ? returns.at<int32>(i + 2) :
					(type == 1) ? returns.at<int16>(i + 4) :
					returns.at<int8>(i + 8)
			]
			);
		}
	}
	std::sort(list.begin(), list.end());
	auto ip = std::unique(list.begin(), list.end());
	bool res = (ip == list.end());
	list.resize(std::distance(list.begin(), ip));
	return res;
}
int32 field::select_card(uint16 step, uint8 playerid, uint8 cancelable, uint8 min, uint8 max) {
	if(step == 0) {
		return_cards.clear();
		returns.clear();
		if(max == 0 || core.select_cards.empty())
			return TRUE;
		if(max > core.select_cards.size())
			max = static_cast<uint8>(core.select_cards.size());
		if(min > max)
			min = max;
		if((playerid == 1) && is_flag(DUEL_SIMPLE_AI)) {
			for(int32 i = 0; i < min; i++) {
				return_cards.list.push_back(core.select_cards[i]);
			}
			return TRUE;
		}
		core.units.begin()->arg2 = ((uint32)min) + (((uint32)max) << 16);
		auto message = pduel->new_message(MSG_SELECT_CARD);
		message->write<uint8>(playerid);
		message->write<uint8>(cancelable);
		message->write<uint32>(min);
		message->write<uint32>(max);
		message->write<uint32>((uint32)core.select_cards.size());
		std::sort(core.select_cards.begin(), core.select_cards.end(), card::card_operation_sort);
		for(auto& pcard : core.select_cards) {
			message->write<uint32>(pcard->data.code);
			message->write(pcard->get_info_location());
		}
		return FALSE;
	} else {
		if(!parse_response_cards(cancelable)) {
			return_cards.clear();
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		if(return_cards.canceled)
			return TRUE;
		if(return_cards.list.size() < min || return_cards.list.size() > max) {
			return_cards.clear();
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		return TRUE;
	}
}
int32 field::select_unselect_card(uint16 step, uint8 playerid, uint8 cancelable, uint8 min, uint8 max, uint8 finishable) {
	if (step == 0) {
		return_cards.clear();
		returns.clear();
		if (core.select_cards.empty() && core.unselect_cards.empty())
			return TRUE;
		if ((playerid == 1) && is_flag(DUEL_SIMPLE_AI)) {
			if(cancelable)
				returns.at<int8>(0) = -1;
			else
				returns.bitSet(1);
			return TRUE;
		}
		auto message = pduel->new_message(MSG_SELECT_UNSELECT_CARD);
		message->write<uint8>(playerid);
		message->write<uint8>(finishable);
		message->write<uint8>(cancelable);
		message->write<uint32>(min);
		message->write<uint32>(max);
		message->write<uint32>((uint32)core.select_cards.size());
		std::sort(core.select_cards.begin(), core.select_cards.end(), card::card_operation_sort);
		for (auto& pcard : core.select_cards) {
			message->write<uint32>(pcard->data.code);
			message->write(pcard->get_info_location());
		}
		message->write<uint32>((uint32)core.unselect_cards.size());
		for(auto& pcard : core.unselect_cards) {
			message->write<uint32>(pcard->data.code);
			message->write(pcard->get_info_location());
		}
		return FALSE;
	} else {
		if(returns.at<int32>(0) == -1) {
			if(cancelable || finishable) {
				return_cards.canceled = true;
				return TRUE;
			}
			return_cards.clear();
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		if(returns.at<int32>(0) == 0 || returns.at<int32>(0) > 1) {
			return_cards.clear();
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		int32 max = (int32)(core.select_cards.size() + core.unselect_cards.size());
		int retval = returns.at<int32>(1);
		if(retval < 0 || retval >= max){
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		return_cards.list.push_back((retval >= (int32)core.select_cards.size()) ? core.unselect_cards[retval - core.select_cards.size()] : core.select_cards[retval]);
		return TRUE;
	}
}
int32 field::select_chain(uint16 step, uint8 playerid, uint8 spe_count, uint8 forced) {
	if(step == 0) {
		returns.at<int32>(0) = -1;
		if((playerid == 1) && is_flag(DUEL_SIMPLE_AI)) {
			if(core.select_chains.size() == 0)
				returns.at<int32>(0) = -1;
			else if(forced)
				returns.at<int32>(0) = 0;
			else {
				bool act = true;
				for(const auto& ch : core.current_chain)
					if(ch.triggering_player == 1)
						act = false;
				if(act)
					returns.at<int32>(0) = 0;
				else
					returns.at<int32>(0) = -1;
			}
			return TRUE;
		}
		auto message = pduel->new_message(MSG_SELECT_CHAIN);
		message->write<uint8>(playerid);
		message->write<uint8>(spe_count);
		message->write<uint8>(forced);
		message->write<uint32>(pduel->game_field->core.hint_timing[playerid]);
		message->write<uint32>(pduel->game_field->core.hint_timing[1 - playerid]);
		std::sort(core.select_chains.begin(), core.select_chains.end(), chain::chain_operation_sort);
		message->write<uint32>(core.select_chains.size());
		for(const auto& ch : core.select_chains) {
			effect* peffect = ch.triggering_effect;
			card* pcard = peffect->get_handler();
			message->write<uint32>(pcard->data.code);
			message->write(pcard->get_info_location());
			message->write<uint64>(peffect->description);
			message->write<uint8>(peffect->get_client_mode());
		}
		return FALSE;
	} else {
		if(!forced && returns.at<int32>(0) == -1)
			return TRUE;
		if(returns.at<int32>(0) < 0 || returns.at<int32>(0) >= (int32)core.select_chains.size()) {
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		return TRUE;
	}
}
int32 field::select_place(uint16 step, uint8 playerid, uint32 flag, uint8 count) {
	if(step == 0) {
		if(count == 0)
			return TRUE;
		if((playerid == 1) && is_flag(DUEL_SIMPLE_AI)) {
			flag = ~flag;
			int32 filter;
			int32 pzone = 0;
			if(flag & 0x7f) {
				returns.at<int8>(0) = 1;
				returns.at<int8>(1) = LOCATION_MZONE;
				filter = flag & 0x7f;
			} else if(flag & 0x1f00) {
				returns.at<int8>(0) = 1;
				returns.at<int8>(1) = LOCATION_SZONE;
				filter = (flag >> 8) & 0x1f;
			} else if(flag & 0xc000) {
				returns.at<int8>(0) = 1;
				returns.at<int8>(1) = LOCATION_SZONE;
				filter = (flag >> 14) & 0x3;
				pzone = 1;
			} else if(flag & 0x7f0000) {
				returns.at<int8>(0) = 0;
				returns.at<int8>(1) = LOCATION_MZONE;
				filter = (flag >> 16) & 0x7f;
			} else if(flag & 0x1f000000) {
				returns.at<int8>(0) = 0;
				returns.at<int8>(1) = LOCATION_SZONE;
				filter = (flag >> 24) & 0x1f;
			} else {
				returns.at<int8>(0) = 0;
				returns.at<int8>(1) = LOCATION_SZONE;
				filter = (flag >> 30) & 0x3;
				pzone = 1;
			}
			if(!pzone) {
				if(filter & 0x40) returns.at<int8>(2) = 6;
				else if(filter & 0x20) returns.at<int8>(2) = 5;
				else if(filter & 0x4) returns.at<int8>(2) = 2;
				else if(filter & 0x2) returns.at<int8>(2) = 1;
				else if(filter & 0x8) returns.at<int8>(2) = 3;
				else if(filter & 0x1) returns.at<int8>(2) = 0;
				else if(filter & 0x10) returns.at<int8>(2) = 4;
			} else {
				if(filter & 0x1) returns.at<int8>(2) = 6;
				else if(filter & 0x2) returns.at<int8>(2) = 7;
			}
			return TRUE;
		}
		auto message = pduel->new_message((core.units.begin()->type == PROCESSOR_SELECT_PLACE) ? MSG_SELECT_PLACE : MSG_SELECT_DISFIELD);
		message->write<uint8>(playerid);
		message->write<uint8>(count);
		message->write<uint32>(flag);
		returns.at<int8>(0) = 0;
		return FALSE;
	} else {
		uint8 pt = 0;
		for(int8 i = 0; i < count; ++i) {
			uint8 p = returns.at<int8>(pt);
			uint8 l = returns.at<int8>(pt + 1);
			uint8 s = returns.at<int8>(pt + 2);
			if((p != 0 && p != 1)
					|| ((l != LOCATION_MZONE) && (l != LOCATION_SZONE))
					|| ((0x1u << s) & (flag >> (((p == playerid) ? 0 : 16) + ((l == LOCATION_MZONE) ? 0 : 8))))) {
				pduel->new_message(MSG_RETRY);
				return FALSE;
			}
			pt += 3;
		}
		return TRUE;
	}
}
int32 field::select_position(uint16 step, uint8 playerid, uint32 code, uint8 positions) {
	if(step == 0) {
		if(positions == 0) {
			returns.at<int32>(0) = POS_FACEUP_ATTACK;
			return TRUE;
		}
		positions &= 0xf;
		if(positions == 0x1 || positions == 0x2 || positions == 0x4 || positions == 0x8) {
			returns.at<int32>(0) = positions;
			return TRUE;
		}
		if((playerid == 1) && is_flag(DUEL_SIMPLE_AI)) {
			if(positions & 0x4)
				returns.at<int32>(0) = 0x4;
			else if(positions & 0x1)
				returns.at<int32>(0) = 0x1;
			else if(positions & 0x8)
				returns.at<int32>(0) = 0x8;
			else
				returns.at<int32>(0) = 0x2;
			return TRUE;
		}
		auto message = pduel->new_message(MSG_SELECT_POSITION);
		message->write<uint8>(playerid);
		message->write<uint32>(code);
		message->write<uint8>(positions);
		returns.at<int32>(0) = 0;
		return FALSE;
	} else {
		uint32 pos = returns.at<int32>(0);
		if((pos != 0x1 && pos != 0x2 && pos != 0x4 && pos != 0x8) || !(pos & positions)) {
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		return TRUE;
	}
}
int32 field::select_tribute(uint16 step, uint8 playerid, uint8 cancelable, uint8 min, uint8 max) {
	if(step == 0) {
		returns.clear();
		return_cards.clear();
		if(max == 0 || core.select_cards.empty())
			return TRUE;
		uint8 tm = 0;
		for(auto& pcard : core.select_cards)
			tm += pcard->release_param;
		if(max > 5)
			max = 5;
		if(max > tm)
			max = tm;
		if(min > max)
			min = max;
		core.units.begin()->arg2 = ((uint32)min) + (((uint32)max) << 16);
		auto message = pduel->new_message(MSG_SELECT_TRIBUTE);
		message->write<uint8>(playerid);
		message->write<uint8>(cancelable);
		message->write<uint32>(min);
		message->write<uint32>(max);
		message->write<uint32>((uint32)core.select_cards.size());
		std::sort(core.select_cards.begin(), core.select_cards.end(), card::card_operation_sort);
		for(auto& pcard : core.select_cards) {
			message->write<uint32>(pcard->data.code);
			message->write<uint8>(pcard->current.controler);
			message->write<uint8>(pcard->current.location);
			message->write<uint32>(pcard->current.sequence);
			message->write<uint8>(pcard->release_param);
		}
		return FALSE;
	} else {
		if(!parse_response_cards(cancelable)) {
			return_cards.clear();
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		if(return_cards.canceled)
			return TRUE;
		// UNUSED VARIABLES
		// int32 tot = (int32)return_cards.list.size();
		int32 tt = 0;
		for(auto& pcard : return_cards.list)
			tt += pcard->release_param;
		if (tt < min) {
			return_cards.clear();
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		return TRUE;
	}
}
int32 field::select_counter(uint16 step, uint8 playerid, uint16 countertype, uint16 count, uint8 s, uint8 o) {
	if(step == 0) {
		if(count == 0)
			return TRUE;
		uint8 avail = s;
		uint8 fp = playerid;
		uint32 total = 0;
		core.select_cards.clear();
		for(int p = 0; p < 2; ++p) {
			if(avail) {
				for(auto& pcard : player[fp].list_mzone) {
					if(pcard && pcard->get_counter(countertype)) {
						core.select_cards.push_back(pcard);
						total += pcard->get_counter(countertype);
					}
				}
				for(auto& pcard : player[fp].list_szone) {
					if(pcard && pcard->get_counter(countertype)) {
						core.select_cards.push_back(pcard);
						total += pcard->get_counter(countertype);
					}
				}
			}
			fp = 1 - fp;
			avail = o;
		}
		if(count > total) {
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		auto message = pduel->new_message(MSG_SELECT_COUNTER);
		message->write<uint8>(playerid);
		message->write<uint16>(countertype);
		message->write<uint16>(count);
		message->write<uint32>(core.select_cards.size());
		std::sort(core.select_cards.begin(), core.select_cards.end(), card::card_operation_sort);
		for(auto& pcard : core.select_cards) {
			message->write<uint32>(pcard->data.code);
			message->write<uint8>(pcard->current.controler);
			message->write<uint8>(pcard->current.location);
			message->write<uint8>(pcard->current.sequence);
			message->write<uint16>(pcard->get_counter(countertype));
		}
		return FALSE;
	} else {
		uint16 ct = 0;
		for(uint32 i = 0; i < core.select_cards.size(); ++i) {
			if(core.select_cards[i]->get_counter(countertype) < returns.at<int16>(i)) {
				pduel->new_message(MSG_RETRY);
				return FALSE;
			}
			ct += returns.at<int16>(i);
		}
		if(ct != count) {
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
	}
	return TRUE;
}
static int32 select_sum_check1(const std::vector<int32>& oparam, int32 size, int32 index, int32 acc) {
	if(acc == 0 || index == size)
		return FALSE;
	int32 o1 = oparam[index] & 0xffff;
	int32 o2 = oparam[index] >> 16;
	if(index == size - 1)
		return acc == o1 || acc == o2;
	return (acc > o1 && select_sum_check1(oparam, size, index + 1, acc - o1))
	       || (o2 > 0 && acc > o2 && select_sum_check1(oparam, size, index + 1, acc - o2));
}
int32 field::select_with_sum_limit(int16 step, uint8 playerid, int32 acc, int32 min, int32 max) {
	if(step == 0) {
		return_cards.clear();
		returns.clear();
		if(core.select_cards.empty())
			return TRUE;
		auto message = pduel->new_message(MSG_SELECT_SUM);
		message->write<uint8>(playerid);
		if(max)
			message->write<uint8>(0);
		else
			message->write<uint8>(1);
		if(max < min)
			max = min;
		message->write<uint32>(acc & 0xffff);
		message->write<uint32>(min);
		message->write<uint32>(max);
		message->write<uint32>(core.must_select_cards.size());
		for(auto& pcard : core.must_select_cards) {
			message->write<uint32>(pcard->data.code);
			message->write<uint8>(pcard->current.controler);
			message->write<uint8>(pcard->current.location);
			message->write<uint32>(pcard->current.sequence);
			message->write<uint32>(pcard->sum_param);
		}
		message->write<uint32>(core.select_cards.size());
		std::sort(core.select_cards.begin(), core.select_cards.end(), card::card_operation_sort);
		for(auto& pcard : core.select_cards) {
			message->write<uint32>(pcard->data.code);
			message->write<uint8>(pcard->current.controler);
			message->write<uint8>(pcard->current.location);
			message->write<uint32>(pcard->current.sequence);
			message->write<uint32>(pcard->sum_param);
		}
		return FALSE;
	} else {
		if(!parse_response_cards()) {
			return_cards.clear();
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		int32 tot = return_cards.list.size();
		if (max) {
			if(tot < min || tot > max) {
				return_cards.clear();
				pduel->new_message(MSG_RETRY);
				return FALSE;
			}
			int32 mcount = core.must_select_cards.size();
			std::vector<int32> oparam;
			for(auto& list : { &core.must_select_cards , &return_cards.list })
				for(auto& pcard : *list)
					oparam.push_back(pcard->sum_param);
			if(!select_sum_check1(oparam, tot + mcount, 0, acc)) {
				return_cards.clear();
				pduel->new_message(MSG_RETRY);
				return FALSE;
			}
			return TRUE;
		} else {
			// UNUSED VARIABLE
			// int32 mcount = core.must_select_cards.size();
			int32 sum = 0, mx = 0, mn = 0x7fffffff;
			for(auto& list : { &core.must_select_cards , &return_cards.list }) {
				for(auto& pcard : *list) {
					int32 op = pcard->sum_param;
					int32 o1 = op & 0xffff;
					int32 o2 = op >> 16;
					int32 ms = (o2 && o2 < o1) ? o2 : o1;
					sum += ms;
					mx += (o2 > o1) ? o2 : o1;
					if(ms < mn)
						mn = ms;
				}
			}
			if(mx < acc || sum - mn >= acc) {
				return_cards.clear();
				pduel->new_message(MSG_RETRY);
				return FALSE;
			}
			return TRUE;
		}
	}
	return TRUE;
}
int32 field::sort_card(int16 step, uint8 playerid, uint8 is_chain) {
	if(step == 0) {
		returns.clear();
		if((playerid == 1) && is_flag(DUEL_SIMPLE_AI)) {
			returns.at<int8>(0) = -1;
			return TRUE;
		}
		if(core.select_cards.empty())
			return TRUE;
		auto message = pduel->new_message((is_chain) ? MSG_SORT_CHAIN : MSG_SORT_CARD);
		message->write<uint8>(playerid);
		message->write<uint32>(core.select_cards.size());
		for(auto& pcard : core.select_cards) {
			message->write<uint32>(pcard->data.code);
			message->write<uint8>(pcard->current.controler);
			message->write<uint32>(pcard->current.location);
			message->write<uint32>(pcard->current.sequence);
		}
		return FALSE;
	} else {
		if(returns.at<int8>(0) == -1)
			return TRUE;
		byte c[64] = {};
		uint8 m = static_cast<uint8>(core.select_cards.size());
		for(uint8 i = 0; i < m; ++i) {
			int8 v = returns.at<int8>(i);
			if(v < 0 || v >= m || c[v]) {
				pduel->new_message(MSG_RETRY);
				return FALSE;
			}
			c[v] = 1;
		}
		return TRUE;
	}
	return TRUE;
}
int32 field::announce_race(int16 step, uint8 playerid, int32 count, int32 available) {
	if(step == 0) {
		int32 scount = 0;
		for(int32 ft = 0x1; ft != 0x2000000; ft <<= 1) {
			if(ft & available)
				scount++;
		}
		if(scount <= count) {
			count = scount;
			core.units.begin()->arg1 = (count << 16) + playerid;
		}
		auto message = pduel->new_message(MSG_ANNOUNCE_RACE);
		message->write<uint8>(playerid);
		message->write<uint8>(count);
		message->write<uint32>(available);
		return FALSE;
	} else {
		int32 rc = returns.at<int32>(0);
		int32 sel = 0;
		for(int32 ft = 0x1; ft != 0x2000000; ft <<= 1) {
			if(!(ft & rc)) continue;
			if(!(ft & available)) {
				pduel->new_message(MSG_RETRY);
				return FALSE;
			}
			sel++;
		}
		if(sel != count) {
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		auto message = pduel->new_message(MSG_HINT);
		message->write<uint8>(HINT_RACE);
		message->write<uint8>(playerid);
		message->write<uint64>(returns.at<int32>(0));
		return TRUE;
	}
	return TRUE;
}
int32 field::announce_attribute(int16 step, uint8 playerid, int32 count, int32 available) {
	if(step == 0) {
		int32 scount = 0;
		for(int32 ft = 0x1; ft != 0x80; ft <<= 1) {
			if(ft & available)
				scount++;
		}
		if(scount <= count) {
			count = scount;
			core.units.begin()->arg1 = (count << 16) + playerid;
		}
		auto message = pduel->new_message(MSG_ANNOUNCE_ATTRIB);
		message->write<uint8>(playerid);
		message->write<uint8>(count);
		message->write<uint32>(available);
		return FALSE;
	} else {
		int32 rc = returns.at<int32>(0);
		int32 sel = 0;
		for(int32 ft = 0x1; ft != 0x80; ft <<= 1) {
			if(!(ft & rc)) continue;
			if(!(ft & available)) {
				pduel->new_message(MSG_RETRY);
				return FALSE;
			}
			sel++;
		}
		if(sel != count) {
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		auto message = pduel->new_message(MSG_HINT);
		message->write<uint8>(HINT_ATTRIB);
		message->write<uint8>(playerid);
		message->write<uint64>(returns.at<int32>(0));
		return TRUE;
	}
	return TRUE;
}
#define BINARY_OP(opcode,op) case opcode: {\
								if (stack.size() >= 2) {\
									int32 rhs = stack.top();\
									stack.pop();\
									int32 lhs = stack.top();\
									stack.pop();\
									stack.push(lhs op rhs);\
								}\
								break;\
							}
#define UNARY_OP(opcode,op) case opcode: {\
								if (stack.size() >= 1) {\
									int32 val = stack.top();\
									stack.pop();\
									stack.push(op val);\
								}\
								break;\
							}
#define UNARY_OP_OP(opcode,val,op) UNARY_OP(opcode,cd->val op)
#define GET_OP(opcode,val) case opcode: {\
								stack.push(cd->val);\
								break;\
							}
static int32 is_declarable(const card_data* cd, const std::vector<uint64>& opcodes) {
	std::stack<int32> stack;
	for(auto& opcode : opcodes) {
		switch(opcode) {
		BINARY_OP(OPCODE_ADD, +);
		BINARY_OP(OPCODE_SUB, -);
		BINARY_OP(OPCODE_MUL, *);
		BINARY_OP(OPCODE_DIV, /);
		BINARY_OP(OPCODE_AND, &&);
		BINARY_OP(OPCODE_OR, ||);
		UNARY_OP(OPCODE_NEG, -);
		UNARY_OP(OPCODE_NOT, !);
		BINARY_OP(OPCODE_BAND, &);
		BINARY_OP(OPCODE_BOR, | );
		BINARY_OP(OPCODE_BXOR, ^);
		UNARY_OP(OPCODE_BNOT, ~);
		BINARY_OP(OPCODE_LSHIFT, <<);
		BINARY_OP(OPCODE_RSHIFT, >>);
		UNARY_OP_OP(OPCODE_ISCODE, code, == (uint32));
		UNARY_OP_OP(OPCODE_ISTYPE, type, &);
		UNARY_OP_OP(OPCODE_ISRACE, race, &);
		UNARY_OP_OP(OPCODE_ISATTRIBUTE, attribute, &);
		GET_OP(OPCODE_GETCODE, code);
		GET_OP(OPCODE_GETTYPE, type);
		GET_OP(OPCODE_GETRACE, race);
		GET_OP(OPCODE_GETATTRIBUTE, attribute);
		//GET_OP(OPCODE_GETSETCARD, setcode);
		case OPCODE_ISSETCARD: {
			if(stack.size() >= 1) {
				int32 set_code = stack.top();
				stack.pop();
				bool res = false;
				uint16 settype = set_code & 0xfff;
				uint16 setsubtype = set_code & 0xf000;
				for(auto& sc : cd->setcodes) {
					if((sc & 0xfff) == settype && (sc & 0xf000 & setsubtype) == setsubtype) {
						res = true;
						break;
					}
				}
				stack.push(res);
			}
			break;
		}
		default: {
			stack.push(opcode);
			break;
		}
		}
	}
	if(stack.size() != 1 || stack.top() == 0)
		return FALSE;
	return cd->code == CARD_MARINE_DOLPHIN || cd->code == CARD_TWINKLE_MOSS
		|| (!cd->alias && (cd->type & (TYPE_MONSTER + TYPE_TOKEN)) != (TYPE_MONSTER + TYPE_TOKEN));
}
#undef BINARY_OP
#undef UNARY_OP
#undef UNARY_OP_OP
#undef GET_OP
int32 field::announce_card(int16 step, uint8 playerid) {
	if(step == 0) {
		auto message = pduel->new_message(MSG_ANNOUNCE_CARD);
		message->write<uint8>(playerid);
		message->write<uint8>(static_cast<uint8>(core.select_options.size()));
		for(auto& option : core.select_options)
			message->write<uint64>(option);
		return FALSE;
	} else {
		int32 code = returns.at<int32>(0);
		auto data = pduel->read_card(code);
		if(!data->code || !is_declarable(data, core.select_options)) {
			/*auto message = */pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		auto message = pduel->new_message(MSG_HINT);
		message->write<uint8>(HINT_CODE);
		message->write<uint8>(playerid);
		message->write<uint64>(code);
		return TRUE;
	}
	return TRUE;
}
int32 field::announce_number(int16 step, uint8 playerid) {
	if(step == 0) {
		auto message = pduel->new_message(MSG_ANNOUNCE_NUMBER);
		message->write<uint8>(playerid);
		message->write<uint8>(static_cast<uint8>(core.select_options.size()));
		for(auto& option : core.select_options)
			message->write<uint64>(option);
		return FALSE;
	} else {
		int32 ret = returns.at<int32>(0);
		if(ret < 0 || ret >= (int32)core.select_options.size()) {
			pduel->new_message(MSG_RETRY);
			return FALSE;
		}
		auto message = pduel->new_message(MSG_HINT);
		message->write<uint8>(HINT_NUMBER);
		message->write<uint8>(playerid);
		message->write<uint64>(core.select_options[returns.at<int32>(0)]);
		return TRUE;
	}
}
