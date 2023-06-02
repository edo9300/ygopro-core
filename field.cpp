/*
 * field.cpp
 *
 *  Created on: 2010-7-21
 *      Author: Argon
 */

#include <algorithm>
#include "field.h"
#include "duel.h"
#include "card.h"
#include "group.h"
#include "effect.h"
#include "interpreter.h"

int32_t field::field_used_count[32] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5};

bool chain::chain_operation_sort(const chain& c1, const chain& c2) {
	return c1.triggering_effect->id < c2.triggering_effect->id;
}
void chain::set_triggering_state(card* pcard) {
	triggering_controler = pcard->current.controler;
	triggering_location = pcard->current.location;
	if(pcard->current.location & (LOCATION_MZONE | LOCATION_SZONE)) {
		if(pcard->current.is_location(LOCATION_FZONE))
			triggering_location |= LOCATION_FZONE;
		else if(pcard->current.is_location(LOCATION_PZONE))
			triggering_location |= LOCATION_PZONE;
		else if(pcard->current.is_location(LOCATION_STZONE))
			triggering_location |= LOCATION_STZONE;
		else if(pcard->current.is_location(LOCATION_EMZONE))
			triggering_location |= LOCATION_EMZONE;
		else if(pcard->current.is_location(LOCATION_MMZONE))
			triggering_location |= LOCATION_MMZONE;
	}
	triggering_sequence = pcard->current.sequence;
	triggering_position = pcard->current.position;
	triggering_state.code = pcard->get_code();
	triggering_state.code2 = pcard->get_another_code();
	triggering_state.level = pcard->get_level();
	triggering_state.rank = pcard->get_rank();
	triggering_state.attribute = pcard->get_attribute();
	triggering_state.race = pcard->get_race();
	triggering_state.attack = pcard->get_attack();
	triggering_state.defense = pcard->get_defense();
}
bool tevent::operator< (const tevent& v) const {
	return std::memcmp(this, &v, sizeof(tevent)) < 0;
}
field::field(duel* _pduel, const OCG_DuelOptions& options) :pduel(_pduel), player({ {options.team1, options.team2} }) {
	core.duel_options = options.flags;
	nil_event.event_code = 0;
	nil_event.event_cards = 0;
	nil_event.event_player = PLAYER_NONE;
	nil_event.event_value = 0;
	nil_event.reason = 0;
	nil_event.reason_effect = 0;
	nil_event.reason_player = PLAYER_NONE;
}
void field::reload_field_info() {
	auto message = pduel->new_message(MSG_RELOAD_FIELD);
	message->write<uint32_t>(core.duel_options);
	for(int32_t playerid = 0; playerid < 2; ++playerid) {
		message->write<uint32_t>(player[playerid].lp);
		for(auto& pcard : player[playerid].list_mzone) {
			if(pcard) {
				message->write<uint8_t>(1);
				message->write<uint8_t>(pcard->current.position);
				message->write<uint32_t>(pcard->xyz_materials.size());
			} else {
				message->write<uint8_t>(0);
			}
		}
		for(auto& pcard : player[playerid].list_szone) {
			if(pcard) {
				message->write<uint8_t>(1);
				message->write<uint8_t>(pcard->current.position);
				message->write<uint32_t>(pcard->xyz_materials.size());
			} else {
				message->write<uint8_t>(0);
			}
		}
		message->write<uint32_t>(player[playerid].list_main.size());
		message->write<uint32_t>(player[playerid].list_hand.size());
		message->write<uint32_t>(player[playerid].list_grave.size());
		message->write<uint32_t>(player[playerid].list_remove.size());
		message->write<uint32_t>(player[playerid].list_extra.size());
		message->write<uint32_t>(player[playerid].extra_p_count);
	}
	message->write<uint32_t>(core.current_chain.size());
	for(const auto& ch : core.current_chain) {
		effect* peffect = ch.triggering_effect;
		message->write<uint32_t>(peffect->get_handler()->data.code);
		message->write(peffect->get_handler()->get_info_location());
		message->write<uint8_t>(ch.triggering_controler);
		message->write<uint8_t>((uint8_t)ch.triggering_location);
		message->write<uint32_t>(ch.triggering_sequence);
		message->write<uint64_t>(peffect->description);
	}
}
// The core of moving cards, and Debug.AddCard() will call this function directly.
// check Fusion/S/X monster redirection by the rule, set fieldid_r
void field::add_card(uint8_t playerid, card* pcard, uint8_t location, uint8_t sequence, uint8_t pzone) {
	if (pcard->current.location != 0)
		return;
	if (!is_location_useable(playerid, location, sequence))
		return;
	if(pcard->is_extra_deck_monster() && (location & (LOCATION_HAND | LOCATION_DECK))) {
		location = LOCATION_EXTRA;
		pcard->sendto_param.position = POS_FACEDOWN_DEFENSE;
	}
	pcard->current.controler = playerid;
	pcard->current.location = location;
	switch (location) {
	case LOCATION_MZONE: {
		player[playerid].list_mzone[sequence] = pcard;
		pcard->current.sequence = sequence;
		break;
	}
	case LOCATION_SZONE: {
		player[playerid].list_szone[sequence] = pcard;
		pcard->current.sequence = sequence;
		break;
	}
	case LOCATION_DECK: {
		if (sequence == 0) {		//deck top
			player[playerid].list_main.push_back(pcard);
			pcard->current.sequence = static_cast<uint32_t>(player[playerid].list_main.size() - 1);
		} else if (sequence == 1) {		//deck bottom
			player[playerid].list_main.insert(player[playerid].list_main.begin(), pcard);
			reset_sequence(playerid, LOCATION_DECK);
		} else {		//deck top & shuffle
			player[playerid].list_main.push_back(pcard);
			pcard->current.sequence = static_cast<uint32_t>(player[playerid].list_main.size() - 1);
			if(!core.shuffle_check_disabled)
				core.shuffle_deck_check[playerid] = TRUE;
		}
		pcard->sendto_param.position = POS_FACEDOWN;
		break;
	}
	case LOCATION_HAND: {
		player[playerid].list_hand.push_back(pcard);
		pcard->current.sequence = static_cast<uint32_t>(player[playerid].list_hand.size() - 1);
		uint32_t pos = pcard->is_affected_by_effect(EFFECT_PUBLIC) ? POS_FACEUP : POS_FACEDOWN;
		pcard->sendto_param.position = pos;
		if(!(pcard->current.reason & REASON_DRAW) && !core.shuffle_check_disabled)
			core.shuffle_hand_check[playerid] = TRUE;
		break;
	}
	case LOCATION_GRAVE: {
		player[playerid].list_grave.push_back(pcard);
		pcard->current.sequence = static_cast<uint32_t>(player[playerid].list_grave.size() - 1);
		break;
	}
	case LOCATION_REMOVED: {
		player[playerid].list_remove.push_back(pcard);
		pcard->current.sequence = static_cast<uint32_t>(player[playerid].list_remove.size() - 1);
		break;
	}
	case LOCATION_EXTRA: {
		if(player[playerid].extra_p_count == 0 || ((pcard->data.type & TYPE_PENDULUM) && (pcard->sendto_param.position & POS_FACEUP)))
			player[playerid].list_extra.push_back(pcard);
		else
			player[playerid].list_extra.insert(player[playerid].list_extra.end() - player[playerid].extra_p_count, pcard);
		if((pcard->data.type & TYPE_PENDULUM) && (pcard->sendto_param.position & POS_FACEUP))
			++player[playerid].extra_p_count;
		reset_sequence(playerid, LOCATION_EXTRA);
		break;
	}
	}
	if(pzone)
		pcard->current.pzone = true;
	else
		pcard->current.pzone = false;
	pcard->apply_field_effect();
	pcard->fieldid = infos.field_id++;
	pcard->fieldid_r = pcard->fieldid;
	pcard->turnid = infos.turn_id;
	if(check_unique_onfield(pcard, pcard->current.controler, pcard->current.location))
		pcard->unique_fieldid = UINT_MAX;
	if (location == LOCATION_MZONE)
		player[playerid].used_location |= 1 << sequence;
	if (location == LOCATION_SZONE)
		player[playerid].used_location |= 256 << sequence;
}
void field::remove_card(card* pcard) {
	if (pcard->current.controler == PLAYER_NONE || pcard->current.location == 0)
		return;
	uint8_t playerid = pcard->current.controler;
	switch (pcard->current.location) {
	case LOCATION_MZONE:
		player[playerid].list_mzone[pcard->current.sequence] = 0;
		break;
	case LOCATION_SZONE:
		player[playerid].list_szone[pcard->current.sequence] = 0;
		break;
	case LOCATION_DECK:
		player[playerid].list_main.erase(player[playerid].list_main.begin() + pcard->current.sequence);
		reset_sequence(playerid, LOCATION_DECK);
		if(!core.shuffle_check_disabled)
			core.shuffle_deck_check[playerid] = TRUE;
		break;
	case LOCATION_HAND:
		player[playerid].list_hand.erase(player[playerid].list_hand.begin() + pcard->current.sequence);
		reset_sequence(playerid, LOCATION_HAND);
		break;
	case LOCATION_GRAVE:
		player[playerid].list_grave.erase(player[playerid].list_grave.begin() + pcard->current.sequence);
		reset_sequence(playerid, LOCATION_GRAVE);
		break;
	case LOCATION_REMOVED:
		player[playerid].list_remove.erase(player[playerid].list_remove.begin() + pcard->current.sequence);
		reset_sequence(playerid, LOCATION_REMOVED);
		break;
	case LOCATION_EXTRA:
		player[playerid].list_extra.erase(player[playerid].list_extra.begin() + pcard->current.sequence);
		reset_sequence(playerid, LOCATION_EXTRA);
		if((pcard->data.type & TYPE_PENDULUM) && (pcard->current.position & POS_FACEUP))
			--player[playerid].extra_p_count;
		break;
	}
	pcard->cancel_field_effect();
	if (pcard->current.location == LOCATION_MZONE)
		player[playerid].used_location &= ~(1 << pcard->current.sequence);
	if (pcard->current.location == LOCATION_SZONE)
		player[playerid].used_location &= ~(256 << pcard->current.sequence);
	if(core.current_chain.size() > 0)
		core.just_sent_cards.insert(pcard);
	pcard->previous.controler = pcard->current.controler;
	pcard->previous.location = pcard->current.location;
	pcard->previous.sequence = pcard->current.sequence;
	pcard->previous.position = pcard->current.position;
	pcard->previous.pzone = pcard->current.pzone;
	pcard->current.controler = PLAYER_NONE;
	pcard->current.location = 0;
	pcard->current.sequence = 0;
}
// moving cards:
// 1. draw()
// 2. discard_deck()
// 3. swap_control()
// 4. control_adjust()
// 5. move_card()
// check Fusion/S/X monster redirection by the rule
uint8_t field::move_card(uint8_t playerid, card* pcard, uint8_t location, uint8_t sequence, uint8_t pzone) {
	if (!is_location_useable(playerid, location, sequence))
		return FALSE;
	uint8_t preplayer = pcard->current.controler;
	uint8_t presequence = pcard->current.sequence;
	if(pcard->is_extra_deck_monster() && (location & (LOCATION_HAND | LOCATION_DECK))) {
		location = LOCATION_EXTRA;
		pcard->sendto_param.position = POS_FACEDOWN_DEFENSE;
	}
	if (pcard->current.location) {
		if (pcard->current.location == location && pcard->current.pzone == !!pzone) {
			if (pcard->current.location == LOCATION_DECK) {
				if(preplayer == playerid) {
					auto message = pduel->new_message(MSG_MOVE);
					message->write<uint32_t>(pcard->data.code);
					message->write(pcard->get_info_location());
					player[preplayer].list_main.erase(player[preplayer].list_main.begin() + pcard->current.sequence);
					if (sequence == 0) {		//deck top
						player[playerid].list_main.push_back(pcard);
					} else if (sequence == 1) {
						player[playerid].list_main.insert(player[playerid].list_main.begin(), pcard);
					} else {
						player[playerid].list_main.push_back(pcard);
						if(!core.shuffle_check_disabled)
							core.shuffle_deck_check[playerid] = TRUE;
					}
					reset_sequence(playerid, LOCATION_DECK);
					pcard->previous.controler = preplayer;
					pcard->current.controler = playerid;
					message->write(pcard->get_info_location());
					message->write<uint32_t>(pcard->current.reason);
					return TRUE;
				} else
					remove_card(pcard);
			} else if(location & LOCATION_ONFIELD) {
				if (playerid == preplayer && sequence == presequence)
					return FALSE;
				if(location == LOCATION_MZONE) {
					if(sequence >= player[playerid].list_mzone.size() || player[playerid].list_mzone[sequence])
						return FALSE;
				} else {
					if(sequence >= player[playerid].list_szone.size() || player[playerid].list_szone[sequence])
						return FALSE;
				}
				duel::duel_message* message = nullptr;
				if(preplayer == playerid) {
					message = pduel->new_message(MSG_MOVE);
					message->write<uint32_t>(pcard->data.code);
					message->write(pcard->get_info_location());
				}
				if(core.current_chain.size() > 0)
					core.just_sent_cards.insert(pcard);
				pcard->previous.controler = pcard->current.controler;
				pcard->previous.location = pcard->current.location;
				pcard->previous.sequence = pcard->current.sequence;
				pcard->previous.position = pcard->current.position;
				pcard->previous.pzone = pcard->current.pzone;
				if (location == LOCATION_MZONE) {
					player[preplayer].list_mzone[presequence] = 0;
					player[preplayer].used_location &= ~(1 << presequence);
					player[playerid].list_mzone[sequence] = pcard;
					player[playerid].used_location |= 1 << sequence;
					pcard->current.controler = playerid;
					pcard->current.sequence = sequence;
				} else {
					player[preplayer].list_szone[presequence] = 0;
					player[preplayer].used_location &= ~(256 << presequence);
					player[playerid].list_szone[sequence] = pcard;
					player[playerid].used_location |= 256 << sequence;
					pcard->current.controler = playerid;
					pcard->current.sequence = sequence;
				}
				if(message) {
					message->write(pcard->get_info_location());
					message->write<uint32_t>(pcard->current.reason);
				} else {
					pcard->fieldid = infos.field_id++;
					if(check_unique_onfield(pcard, pcard->current.controler, pcard->current.location))
						pcard->unique_fieldid = UINT_MAX;
				}
				return TRUE;
			} else if(location == LOCATION_HAND) {
				if(preplayer == playerid)
					return FALSE;
				remove_card(pcard);
			} else {
				if(location == LOCATION_GRAVE) {
					if(pcard->current.sequence == player[pcard->current.controler].list_grave.size() - 1)
						return FALSE;
					auto message = pduel->new_message(MSG_MOVE);
					message->write<uint32_t>(pcard->data.code);
					message->write(pcard->get_info_location());
					player[pcard->current.controler].list_grave.erase(player[pcard->current.controler].list_grave.begin() + pcard->current.sequence);
					player[pcard->current.controler].list_grave.push_back(pcard);
					reset_sequence(pcard->current.controler, LOCATION_GRAVE);
					message->write(pcard->get_info_location());
					message->write<uint32_t>(pcard->current.reason);
				} else if(location == LOCATION_REMOVED) {
					if(pcard->current.sequence == player[pcard->current.controler].list_remove.size() - 1)
						return FALSE;
					auto message = pduel->new_message(MSG_MOVE);
					message->write<uint32_t>(pcard->data.code);
					message->write(pcard->get_info_location());
					player[pcard->current.controler].list_remove.erase(player[pcard->current.controler].list_remove.begin() + pcard->current.sequence);
					player[pcard->current.controler].list_remove.push_back(pcard);
					reset_sequence(pcard->current.controler, LOCATION_REMOVED);
					message->write(pcard->get_info_location());
					message->write<uint32_t>(pcard->current.reason);
				} else {
					auto message = pduel->new_message(MSG_MOVE);
					message->write<uint32_t>(pcard->data.code);
					message->write(pcard->get_info_location());
					player[pcard->current.controler].list_extra.erase(player[pcard->current.controler].list_extra.begin() + pcard->current.sequence);
					player[pcard->current.controler].list_extra.push_back(pcard);
					reset_sequence(pcard->current.controler, LOCATION_EXTRA);
					message->write(pcard->get_info_location());
					message->write<uint32_t>(pcard->current.reason);
				}
				return TRUE;
			}
		} else {
			if((pcard->data.type & TYPE_PENDULUM) && (location == LOCATION_GRAVE)
			        && pcard->is_capable_send_to_extra(playerid)
			        && (((pcard->current.location == LOCATION_MZONE) && !pcard->is_status(STATUS_SUMMON_DISABLED))
			        || ((pcard->current.location == LOCATION_SZONE) && !pcard->is_status(STATUS_ACTIVATE_DISABLED)))) {
				location = LOCATION_EXTRA;
				pcard->sendto_param.position = POS_FACEUP_DEFENSE;
			}
			remove_card(pcard);
		}
	}
	add_card(playerid, pcard, location, sequence, pzone);
	return TRUE;
}
void field::swap_card(card* pcard1, card* pcard2, uint8_t new_sequence1, uint8_t new_sequence2) {
	uint8_t p1 = pcard1->current.controler, p2 = pcard2->current.controler;
	uint8_t l1 = pcard1->current.location, l2 = pcard2->current.location;
	uint8_t s1 = pcard1->current.sequence, s2 = pcard2->current.sequence;
	loc_info info1 = pcard1->get_info_location(), info2 = pcard2->get_info_location();
	if(!(l1 & LOCATION_ONFIELD) || !(l2 & LOCATION_ONFIELD))
		return;
	if((new_sequence1 != s1 && !is_location_useable(p1, l1, new_sequence1))
		|| (new_sequence2 != s2 && !is_location_useable(p2, l2, new_sequence2)))
		return;
	if(p1 == p2 && l1 == l2 && (new_sequence1 == s2 || new_sequence2 == s1))
		return;
	if(l1 == l2) {
		pcard1->previous.controler = p1;
		pcard1->previous.location = l1;
		pcard1->previous.sequence = s1;
		pcard1->previous.position = pcard1->current.position;
		pcard1->previous.pzone = pcard1->current.pzone;
		pcard1->current.controler = p2;
		pcard1->current.location = l2;
		pcard1->current.sequence = new_sequence2;
		pcard2->previous.controler = p2;
		pcard2->previous.location = l2;
		pcard2->previous.sequence = s2;
		pcard2->previous.position = pcard2->current.position;
		pcard2->previous.pzone = pcard2->current.pzone;
		pcard2->current.controler = p1;
		pcard2->current.location = l1;
		pcard2->current.sequence = new_sequence1;
		if(p1 != p2) {
			pcard1->fieldid = infos.field_id++;
			pcard2->fieldid = infos.field_id++;
			if(check_unique_onfield(pcard1, pcard1->current.controler, pcard1->current.location))
				pcard1->unique_fieldid = UINT_MAX;
			if(check_unique_onfield(pcard2, pcard2->current.controler, pcard2->current.location))
				pcard2->unique_fieldid = UINT_MAX;
		}
		if(l1 == LOCATION_MZONE) {
			player[p1].list_mzone[s1] = 0;
			player[p1].used_location &= ~(1 << s1);
			player[p2].list_mzone[s2] = 0;
			player[p2].used_location &= ~(1 << s2);
			player[p2].list_mzone[new_sequence2] = pcard1;
			player[p2].used_location |= 1 << new_sequence2;
			player[p1].list_mzone[new_sequence1] = pcard2;
			player[p1].used_location |= 1 << new_sequence1;
		} else if(l1 == LOCATION_SZONE) {
			player[p1].list_szone[s1] = 0;
			player[p1].used_location &= ~(256 << s1);
			player[p2].list_szone[s2] = 0;
			player[p2].used_location &= ~(256 << s2);
			player[p2].list_szone[new_sequence2] = pcard1;
			player[p2].used_location |= 256 << new_sequence2;
			player[p1].list_szone[new_sequence1] = pcard2;
			player[p1].used_location |= 256 << new_sequence1;
		}
	} else {
		remove_card(pcard1);
		remove_card(pcard2);
		add_card(p2, pcard1, l2, new_sequence2);
		add_card(p1, pcard2, l1, new_sequence1);
	}
	if(s1 == new_sequence1 && s2 == new_sequence2) {
		auto message = pduel->new_message(MSG_SWAP);
		message->write<uint32_t>(pcard1->data.code);
		message->write(info1);
		message->write<uint32_t>(pcard2->data.code);
		message->write(info2);
	} else if(s1 == new_sequence1) {
		auto message = pduel->new_message(MSG_MOVE);
		message->write<uint32_t>(pcard1->data.code);
		message->write(info1);
		message->write(pcard1->get_info_location());
		message->write<uint32_t>(0);
		message = pduel->new_message(MSG_MOVE);
		message->write<uint32_t>(pcard2->data.code);
		message->write(info2);
		message->write(pcard2->get_info_location());
		message->write<uint32_t>(0);
	} else {
		auto message = pduel->new_message(MSG_MOVE);
		message->write<uint32_t>(pcard2->data.code);
		message->write(info2);
		message->write(pcard2->get_info_location());
		message->write<uint32_t>(0);
		message = pduel->new_message(MSG_MOVE);
		message->write<uint32_t>(pcard1->data.code);
		message->write(info1);
		message->write(pcard1->get_info_location());
		message->write<uint32_t>(0);
	}
}
void field::swap_card(card* pcard1, card* pcard2) {
	return swap_card(pcard1, pcard2, pcard1->current.sequence, pcard2->current.sequence);
}
// add EFFECT_SET_CONTROL
void field::set_control(card* pcard, uint8_t playerid, uint16_t reset_phase, uint8_t reset_count) {
	if((core.remove_brainwashing && pcard->is_affected_by_effect(EFFECT_REMOVE_BRAINWASHING)) || std::get<uint8_t>(pcard->refresh_control_status()) == playerid)
		return;
	effect* peffect = pduel->new_effect();
	if(core.reason_effect)
		peffect->owner = core.reason_effect->get_handler();
	else
		peffect->owner = pcard;
	peffect->handler = pcard;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = EFFECT_SET_CONTROL;
	peffect->value = playerid;
	peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
	peffect->reset_flag = RESET_EVENT | 0xc6c0000;
	if(reset_count) {
		peffect->reset_flag |= RESET_PHASE | reset_phase;
		if(!(peffect->reset_flag & (RESET_SELF_TURN | RESET_OPPO_TURN)))
			peffect->reset_flag |= (RESET_SELF_TURN | RESET_OPPO_TURN);
		peffect->reset_count = reset_count;
	}
	pcard->add_effect(peffect);
	pcard->current.controler = playerid;
}

card* field::get_field_card(uint32_t playerid, uint32_t location, uint32_t sequence) {
	switch(location) {
	case LOCATION_MZONE: {
		if(sequence < player[playerid].list_mzone.size())
			return player[playerid].list_mzone[sequence];
		return nullptr;
	}
	case LOCATION_MMZONE: {
		if(is_flag(DUEL_3_COLUMNS_FIELD)) {
			if(sequence < 3)
				return player[playerid].list_mzone[sequence + 1];
		} else if(sequence < 5)
			return player[playerid].list_mzone[sequence];
		return nullptr;
	}
	case LOCATION_EMZONE: {
		if(sequence < 2)
			return player[playerid].list_mzone[sequence + 5];
		return nullptr;
	}
	case LOCATION_SZONE: {
		if(sequence < player[playerid].list_szone.size())
			return player[playerid].list_szone[sequence];
		return nullptr;
	}
	case LOCATION_STZONE: {
		if(is_flag(DUEL_3_COLUMNS_FIELD)) {
			if(sequence < 3)
				return player[playerid].list_szone[sequence + 1];
		} else if(sequence < 5)
			return player[playerid].list_szone[sequence];
		return nullptr;
	}
	case LOCATION_FZONE: {
		if(sequence == 0)
			return player[playerid].list_szone[5];
		return nullptr;
	}
	case LOCATION_PZONE: {
		if(sequence < 2) {
			card* pcard = player[playerid].list_szone[get_pzone_index(sequence, playerid)];
			return pcard && pcard->current.pzone ? pcard : 0;
		}
		return nullptr;
	}
	case LOCATION_DECK: {
		if(sequence < player[playerid].list_main.size())
			return player[playerid].list_main[sequence];
		return nullptr;
	}
	case LOCATION_HAND: {
		if(sequence < player[playerid].list_hand.size())
			return player[playerid].list_hand[sequence];
		return nullptr;
	}
	case LOCATION_GRAVE: {
		if(sequence < player[playerid].list_grave.size())
			return player[playerid].list_grave[sequence];
		return nullptr;
	}
	case LOCATION_REMOVED: {
		if(sequence < player[playerid].list_remove.size())
			return player[playerid].list_remove[sequence];
		return nullptr;
	}
	case LOCATION_EXTRA: {
		if(sequence < player[playerid].list_extra.size())
			return player[playerid].list_extra[sequence];
		return nullptr;
	}
	}
	return 0;
}
// return: the given slot in LOCATION_MZONE or all LOCATION_SZONE is available or not
int32_t field::is_location_useable(uint32_t playerid, uint32_t location, uint32_t sequence) {
	uint32_t flag = player[playerid].disabled_location | player[playerid].used_location;
	if(location == LOCATION_EMZONE) {
		sequence += 5;
		location = LOCATION_MZONE;
	}
	if(location == LOCATION_MMZONE) {
		location = LOCATION_MZONE;
		sequence += 1 * is_flag(DUEL_3_COLUMNS_FIELD);
	}
	if(location == LOCATION_STZONE) {
		location = LOCATION_SZONE;
		sequence += 1 * is_flag(DUEL_3_COLUMNS_FIELD);
	}
	if (location == LOCATION_MZONE) {
		if(flag & (0x1u << sequence))
			return FALSE;
		if(sequence >= 5) {
			uint32_t oppo = player[1 - playerid].disabled_location | player[1 - playerid].used_location;
			if(oppo & (0x1u << (11 - sequence)))
				return FALSE;
		}
	} else if (location == LOCATION_SZONE) {
		if(flag & (0x100u << sequence))
			return FALSE;
	} else if (location == LOCATION_FZONE) {
		if(flag & (0x100u << (5 + sequence)))
			return FALSE;
	} else if (location == LOCATION_PZONE) {
		if(!is_flag(DUEL_PZONE))
			return FALSE;
		if(flag & (0x100u << get_pzone_index(sequence, playerid)))
			return FALSE;
	}
	return TRUE;
}
// uplayer: request player, PLAYER_NONE means ignoring EFFECT_MAX_MZONE, EFFECT_MAX_SZONE
// list: store local flag in list
// return: usable count of LOCATION_MZONE or real LOCATION_SZONE of playerid requested by uplayer (may be negative)
int32_t field::get_useable_count(card* pcard, uint8_t playerid, uint8_t location, uint8_t uplayer, uint32_t reason, uint32_t zone, uint32_t* list) {
	if(location == LOCATION_MZONE && pcard && pcard->current.location == LOCATION_EXTRA)
		return get_useable_count_fromex(pcard, playerid, uplayer, zone, list);
	else
		return get_useable_count_other(pcard, playerid, location, uplayer, reason, zone, list);
}
int32_t field::get_useable_count_fromex(card* pcard, uint8_t playerid, uint8_t uplayer, uint32_t zone, uint32_t* list) {
	bool use_temp_card = false;
	if(!pcard) {
		use_temp_card = true;
		pcard = temp_card;
		pcard->current.location = LOCATION_EXTRA;
	}
	int useable_count = 0;
	if(is_flag(DUEL_EMZONE))
		useable_count = get_useable_count_fromex_rule4(pcard, playerid, uplayer, zone, list);
	else
		useable_count = get_useable_count_other(pcard, playerid, LOCATION_MZONE, uplayer, LOCATION_REASON_TOFIELD, zone, list);
	if(use_temp_card)
		pcard->current.location = 0;
	return useable_count;
}
int32_t field::get_spsummonable_count(card* pcard, uint8_t playerid, uint32_t zone, uint32_t* list) {
	if(pcard->current.location == LOCATION_EXTRA)
		return get_spsummonable_count_fromex(pcard, playerid, playerid, zone, list);
	else
		return get_tofield_count(pcard, playerid, LOCATION_MZONE, playerid, LOCATION_REASON_TOFIELD, zone, list);
}
int32_t field::get_spsummonable_count_fromex(card* pcard, uint8_t playerid, uint8_t uplayer, uint32_t zone, uint32_t* list) {
	bool use_temp_card = false;
	if(!pcard) {
		use_temp_card = true;
		pcard = temp_card;
		pcard->current.location = LOCATION_EXTRA;
	}
	int spsummonable_count = 0;
	if(is_flag(DUEL_EMZONE))
		spsummonable_count = get_spsummonable_count_fromex_rule4(pcard, playerid, uplayer, zone, list);
	else
		spsummonable_count = get_tofield_count(pcard, playerid, LOCATION_MZONE, uplayer, LOCATION_REASON_TOFIELD, zone, list);
	if(use_temp_card)
		pcard->current.location = 0;
	return spsummonable_count;
}
int32_t field::get_useable_count_other(card* pcard, uint8_t playerid, uint8_t location, uint8_t uplayer, uint32_t reason, uint32_t zone, uint32_t* list) {
	int32_t count = get_tofield_count(pcard, playerid, location, uplayer, reason, zone, list);
	int32_t limit;
	if(location == LOCATION_MZONE)
		limit = get_mzone_limit(playerid, uplayer, reason);
	else
		limit = get_szone_limit(playerid, uplayer, reason);
	if(count > limit)
		count = limit;
	return count;
}
int32_t field::get_tofield_count(card* pcard, uint8_t playerid, uint8_t location, uint32_t uplayer, uint32_t reason, uint32_t zone, uint32_t* list) {
	if (location != LOCATION_MZONE && location != LOCATION_SZONE)
		return 0;
	uint32_t flag = player[playerid].disabled_location | player[playerid].used_location;
	if(location == LOCATION_MZONE) {
		flag |= ~get_forced_zones(pcard, playerid, location, uplayer, reason);
		flag = (flag | ~zone) & 0x1f;
	} else
		flag = ((flag >> 8) | ~zone) & 0x1f;
	int32_t count = 5 - field_used_count[flag];
	if(location == LOCATION_MZONE)
		flag |= (1u << 5) | (1u << 6);
	if(list)
		*list = flag;
	return count;
}
int32_t field::get_useable_count_fromex_rule4(card* pcard, uint8_t playerid, uint8_t uplayer, uint32_t zone, uint32_t* list) {
	int32_t count = get_spsummonable_count_fromex_rule4(pcard, playerid, uplayer, zone, list);
	int32_t limit = get_mzone_limit(playerid, uplayer, LOCATION_REASON_TOFIELD);
	if(count > limit)
		count = limit;
	return count;
}
int32_t field::get_spsummonable_count_fromex_rule4(card* pcard, uint8_t playerid, uint8_t uplayer, uint32_t zone, uint32_t* list) {
	uint32_t flag = player[playerid].disabled_location | player[playerid].used_location;
	flag |= ~get_forced_zones(pcard, playerid, LOCATION_MZONE, uplayer, LOCATION_REASON_TOFIELD);
	if(player[playerid].list_mzone[5] && is_location_useable(playerid, LOCATION_MZONE, 6)
		&& check_extra_link(playerid, pcard, 6)) {
		flag |= 1u << 5;
	} else if(player[playerid].list_mzone[6] && is_location_useable(playerid, LOCATION_MZONE, 5)
		&& check_extra_link(playerid, pcard, 5)) {
		flag |= 1u << 6;
	} else if(player[playerid].list_mzone[5] || player[playerid].list_mzone[6]) {
		flag |= (1u << 5) | (1u << 6);
	} else {
		if(!is_location_useable(playerid, LOCATION_MZONE, 5))
			flag |= 1u << 5;
		if(!is_location_useable(playerid, LOCATION_MZONE, 6))
			flag |= 1u << 6;
	}
	uint32_t rule_zone = get_rule_zone_fromex(playerid, pcard);
	flag = flag | ~zone | ~rule_zone;
	if(list)
		*list = flag & 0x7f;
	int32_t count = 5 - field_used_count[flag & 0x1f];
	if(~flag & ((1u << 5) | (1u << 6)))
		++count;
	return count;
}
int32_t field::get_mzone_limit(uint8_t playerid, uint8_t uplayer, uint32_t reason) {
	uint32_t used_flag = player[playerid].used_location;
	used_flag = used_flag & 0x1f;
	int32_t max = 5;
	int32_t used_count = field_used_count[used_flag];
	if(is_flag(DUEL_EMZONE)) {
		max = 7;
		if(player[playerid].list_mzone[5])
			++used_count;
		if(player[playerid].list_mzone[6])
			++used_count;
	}
	effect_set eset;
	if(uplayer < 2)
		filter_player_effect(playerid, EFFECT_MAX_MZONE, &eset);
	for(const auto& peff : eset) {
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		pduel->lua->add_param<PARAM_TYPE_INT>(uplayer);
		pduel->lua->add_param<PARAM_TYPE_INT>(reason);
		int32_t v = peff->get_value(3);
		if(max > v)
			max = v;
	}
	int32_t limit = max - used_count;
	return limit;
}
int32_t field::get_szone_limit(uint8_t playerid, uint8_t uplayer, uint32_t reason) {
	uint32_t used_flag = player[playerid].used_location;
	used_flag = (used_flag >> 8) & 0x1f;
	effect_set eset;
	if(uplayer < 2)
		filter_player_effect(playerid, EFFECT_MAX_SZONE, &eset);
	int32_t max = 5;
	for(const auto& peff : eset) {
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		pduel->lua->add_param<PARAM_TYPE_INT>(uplayer);
		pduel->lua->add_param<PARAM_TYPE_INT>(reason);
		int32_t v = peff->get_value(3);
		if(max > v)
			max = v;
	}
	int32_t limit = max - field_used_count[used_flag];
	return limit;
}
int32_t field::get_forced_zones(card* pcard, uint8_t playerid, uint8_t location, uint32_t uplayer, uint32_t reason) {
	if(location != LOCATION_MZONE)
		return 0xff;
	effect_set eset;
	uint32_t res = 0xff7fff7f;
	if(uplayer < 2)
		filter_player_effect(uplayer, EFFECT_MUST_USE_MZONE, &eset);
	if(pcard)
		pcard->filter_effect(EFFECT_MUST_USE_MZONE, &eset);
	for(const auto& peff : eset) {
		if(peff->is_flag(EFFECT_FLAG_COUNT_LIMIT) && peff->count_limit == 0)
			continue;
		if(peff->operation) {
			pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff, true);
			pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
			pduel->lua->add_param<PARAM_TYPE_INT>(uplayer);
			pduel->lua->add_param<PARAM_TYPE_INT>(reason);
			if(!pduel->lua->check_condition(peff->operation, 4))
				continue;
		}
		if(peff->is_flag(EFFECT_FLAG_PLAYER_TARGET)) {
			pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
			pduel->lua->add_param<PARAM_TYPE_INT>(uplayer);
			pduel->lua->add_param<PARAM_TYPE_INT>(reason);
			res &= peff->get_value(3);
		} else {
			pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
			pduel->lua->add_param<PARAM_TYPE_INT>(uplayer);
			pduel->lua->add_param<PARAM_TYPE_INT>(reason);
			res &= peff->get_value(pcard, 3);
		}
	}
	if(uplayer == playerid || uplayer > 1)
		res &= 0xff;
	else
		(res >>= 16 )&= 0xff;
	return res;
}
uint32_t field::get_rule_zone_fromex(int32_t playerid, card* pcard) {
	if(is_flag(DUEL_EMZONE)) {
		if(is_flag(DUEL_FSX_MMZONE) && pcard && pcard->is_position(POS_FACEDOWN) && (pcard->data.type & (TYPE_FUSION | TYPE_SYNCHRO | TYPE_XYZ)))
			return 0x7f;
		else
			return get_linked_zone(playerid) | (1u << 5) | (1u << 6);
	} else {
		return 0x1f;
	}
}
uint32_t field::get_linked_zone(int32_t playerid, bool free, bool actually_linked) {
	uint32_t zones = 0;
	for(auto& pcard : player[playerid].list_mzone) {
		if(pcard)
			zones |= pcard->get_linked_zone(free);
	}
	for(auto& pcard : player[playerid].list_szone) {
		if(pcard)
			zones |= pcard->get_linked_zone(free);
	}
	uint32_t oppzones = 0;
	for(auto& pcard : player[1 - playerid].list_mzone) {
		if(pcard)
			oppzones |= pcard->get_linked_zone(free);
	}
	for(auto& pcard : player[1 - playerid].list_szone) {
		if(pcard)
			oppzones |= pcard->get_linked_zone(free);
	}
	zones |= ((oppzones & 0xffff) << 16) | (oppzones >> 16);
	if(!actually_linked) {
		effect_set eset;
		uint32_t value;
		filter_field_effect(EFFECT_BECOME_LINKED_ZONE, &eset);
		for(const auto& peff : eset) {
			value = peff->get_value();
			if(value) {
				zones |= (value >> (16 * playerid)) & 0x7f;
			}
		}
	}
	return zones;
}
void field::get_linked_cards(uint8_t self, uint8_t location1, uint8_t location2, card_set* cset) {
	get_cards_in_zone(cset, get_linked_zone(self), self, location1);
	get_cards_in_zone(cset, get_linked_zone(1 - self), 1 - self, location2);
}
int32_t field::check_extra_link(int32_t playerid, card* pcard, int32_t sequence) {
	if(!pcard)
		return FALSE;
	if(player[playerid].list_mzone[sequence])
		return FALSE;
	uint8_t cur_controler = pcard->current.controler;
	uint8_t cur_location = pcard->current.location;
	uint8_t cur_sequence = pcard->current.sequence;
	uint8_t cur_position = pcard->current.position;
	player[playerid].list_mzone[sequence] = pcard;
	pcard->current.controler = playerid;
	pcard->current.location = LOCATION_MZONE;
	pcard->current.sequence = sequence;
	pcard->current.position = POS_FACEUP_ATTACK;
	int32_t ret = pcard->is_extra_link_state();
	player[playerid].list_mzone[sequence] = 0;
	pcard->current.controler = cur_controler;
	pcard->current.location = cur_location;
	pcard->current.sequence = cur_sequence;
	pcard->current.position = cur_position;
	return ret;
}
void field::get_cards_in_zone(card_set* cset, uint32_t zone, int32_t playerid, int32_t location) const {
	if(!(location & LOCATION_ONFIELD))
		return;
	if(location & LOCATION_MZONE) {
		uint32_t icheck = 0x1;
		for(auto& pcard : player[playerid].list_mzone) {
			if(zone & icheck) {
				if(pcard)
					cset->insert(pcard);
			}
			icheck <<= 1;
		}
	}
	if(location & LOCATION_SZONE) {
		uint32_t icheck = 0x1 << 8;
		for(auto& pcard : player[playerid].list_szone) {
			if(zone & icheck) {
				if(pcard)
					cset->insert(pcard);
			}
			icheck <<= 1;
		}
	}
}
void field::shuffle(uint8_t playerid, uint8_t location) {
	if(!(location & (LOCATION_HAND | LOCATION_DECK | LOCATION_EXTRA)))
		return;
	card_vector& to_shuffle = (location == LOCATION_HAND) ? player[playerid].list_hand : (location == LOCATION_DECK) ? player[playerid].list_main : player[playerid].list_extra;
	if(to_shuffle.size() == 0)
		return;
	if(location == LOCATION_HAND) {
		bool shuffle = false;
		for(auto& pcard : to_shuffle)
			if(!pcard->is_position(POS_FACEUP))
				shuffle = true;
		if(!shuffle) {
			core.shuffle_hand_check[playerid] = FALSE;
			return;
		}
	}
	if(location == LOCATION_HAND || !is_flag(DUEL_PSEUDO_SHUFFLE)) {
		int32_t upper_bound = static_cast<int32_t>(to_shuffle.size());
		if(location == LOCATION_EXTRA)
			upper_bound -= player[playerid].extra_p_count;
		if(upper_bound > 1) {
			for(int32_t i = 0; i < upper_bound - 1; ++i) {
				auto r = pduel->get_next_integer(i, upper_bound - 1);
				auto* t = to_shuffle[i];
				to_shuffle[i] = to_shuffle[r];
				to_shuffle[r] = t;
			}
			reset_sequence(playerid, location);
		}
	}
	if(location == LOCATION_HAND || location == LOCATION_EXTRA) {
		auto message = pduel->new_message((location == LOCATION_HAND) ? MSG_SHUFFLE_HAND : MSG_SHUFFLE_EXTRA);
		message->write<uint8_t>(playerid);
		message->write<uint32_t>(to_shuffle.size());
		for(auto& pcard : to_shuffle)
			message->write<uint32_t>(pcard->data.code);
		if(location == LOCATION_HAND) {
			core.shuffle_hand_check[playerid] = FALSE;
			for(auto& pcard : to_shuffle) {
				for(auto& i : pcard->indexer) {
					effect* peffect = i.first;
					if(peffect->is_flag(EFFECT_FLAG_CLIENT_HINT) && !peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET)) {
						auto _message = pduel->new_message(MSG_CARD_HINT);
						_message->write(pcard->get_info_location());
						_message->write<uint8_t>(CHINT_DESC_ADD);
						_message->write<uint64_t>(peffect->description);
					}
				}
			}
		}
	} else {
		auto message = pduel->new_message(MSG_SHUFFLE_DECK);
		message->write<uint8_t>(playerid);
		core.shuffle_deck_check[playerid] = FALSE;
		if(core.global_flag & GLOBALFLAG_DECK_REVERSE_CHECK) {
			card* ptop = to_shuffle.back();
			if(core.deck_reversed || (ptop->current.position == POS_FACEUP_DEFENSE)) {
				message = pduel->new_message(MSG_DECK_TOP);
				message->write<uint8_t>(playerid);
				message->write<uint32_t>(0);
				message->write<uint32_t>(ptop->data.code);
				message->write<uint32_t>(ptop->current.position);
			}
		}
	}
}
void field::reset_sequence(uint8_t playerid, uint8_t location) {
	if(location & (LOCATION_ONFIELD))
		return;
	uint32_t i = 0;
	switch(location) {
	case LOCATION_DECK:
		for(auto& pcard : player[playerid].list_main)
			pcard->current.sequence = i++;
		break;
	case LOCATION_HAND:
		for(auto& pcard : player[playerid].list_hand)
			pcard->current.sequence = i++;
		break;
	case LOCATION_EXTRA:
		for(auto& pcard : player[playerid].list_extra)
			pcard->current.sequence = i++;
		break;
	case LOCATION_GRAVE:
		for(auto& pcard : player[playerid].list_grave)
			pcard->current.sequence = i++;
		break;
	case LOCATION_REMOVED:
		for(auto& pcard : player[playerid].list_remove)
			pcard->current.sequence = i++;
		break;
	}
}
void field::swap_deck_and_grave(uint8_t playerid) {
	auto& cur_player = player[playerid];
	for(auto& pcard : cur_player.list_grave) {
		if(core.current_chain.size() > 0)
			core.just_sent_cards.insert(pcard);
		pcard->previous.location = LOCATION_GRAVE;
		pcard->previous.sequence = pcard->current.sequence;
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
	}
	for(auto& pcard : cur_player.list_main) {
		if(core.current_chain.size() > 0)
			core.just_sent_cards.insert(pcard);
		pcard->previous.location = LOCATION_DECK;
		pcard->previous.sequence = pcard->current.sequence;
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
	}
	cur_player.list_grave.swap(cur_player.list_main);
	auto message = pduel->new_message(MSG_SWAP_GRAVE_DECK);
	message->write<uint8_t>(playerid);
	card_vector ex;
	ProgressiveBuffer buff;
	buff.data.reserve((cur_player.list_main.size() / sizeof(uint8_t)) + 1);
	int i = 0;
	for(auto clit = cur_player.list_main.begin(); clit != cur_player.list_main.end(); ++i) {
		if((*clit)->is_extra_deck_monster()) {
			buff.bitToggle(i, true);
			ex.push_back(*clit);
			clit = cur_player.list_main.erase(clit);
		} else {
			buff.bitToggle(i, false);
			++clit;
		}
	}
	for(auto& pcard : cur_player.list_grave) {
		pcard->current.position = POS_FACEUP;
		pcard->current.location = LOCATION_GRAVE;
		pcard->current.reason = REASON_EFFECT;
		pcard->current.reason_effect = core.reason_effect;
		pcard->current.reason_player = core.reason_player;
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
		pcard->reset(RESET_TOGRAVE, RESET_EVENT);
	}
	for(auto& pcard : cur_player.list_main) {
		pcard->current.position = POS_FACEDOWN_DEFENSE;
		pcard->current.location = LOCATION_DECK;
		pcard->current.reason = REASON_EFFECT;
		pcard->current.reason_effect = core.reason_effect;
		pcard->current.reason_player = core.reason_player;
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
		pcard->reset(RESET_TODECK, RESET_EVENT);
		raise_single_event(pcard, 0, EVENT_LEAVE_GRAVE, pduel->game_field->core.reason_effect, 0, pduel->game_field->core.reason_player, 0, 0);
	}
	for(auto& pcard : ex) {
		pcard->current.position = POS_FACEDOWN_DEFENSE;
		pcard->current.location = LOCATION_EXTRA;
		pcard->current.reason = REASON_EFFECT;
		pcard->current.reason_effect = core.reason_effect;
		pcard->current.reason_player = core.reason_player;
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
		pcard->reset(RESET_TODECK, RESET_EVENT);
		raise_single_event(pcard, 0, EVENT_LEAVE_GRAVE, pduel->game_field->core.reason_effect, 0, pduel->game_field->core.reason_player, 0, 0);
	}
	message->write<uint32_t>(cur_player.list_extra.size() - cur_player.extra_p_count);
	cur_player.list_extra.insert(cur_player.list_extra.end() - cur_player.extra_p_count, ex.begin(), ex.end());
	reset_sequence(playerid, LOCATION_GRAVE);
	reset_sequence(playerid, LOCATION_EXTRA);
	message->write<uint32_t>(buff.data.size());
	message->write(buff.data.data(), buff.data.size());
	shuffle(playerid, LOCATION_DECK);
	if(cur_player.list_main.size() || ex.size()) {
		card_set cset(ex.begin(), ex.end());
		cset.insert(cur_player.list_main.begin(), cur_player.list_main.end());
		raise_event(&cset, EVENT_LEAVE_GRAVE, pduel->game_field->core.reason_effect, 0, pduel->game_field->core.reason_player, 0, 0);
		process_single_event();
		process_instant_event();
	}
}
void field::reverse_deck(uint8_t playerid) {
	auto count = player[playerid].list_main.size();
	if(count == 0)
		return;
	for(size_t i = 0; i < count / 2; ++i) {
		card* tmp = player[playerid].list_main[i];
		tmp->current.sequence = static_cast<uint32_t>(count - 1 - i);
		player[playerid].list_main[count - 1 - i]->current.sequence = static_cast<uint32_t>(i);
		player[playerid].list_main[i] = player[playerid].list_main[count - 1 - i];
		player[playerid].list_main[count - 1 - i] = tmp;
	}
}
int field::get_player_count(uint8_t playerid) {
	return static_cast<int>(player[playerid].extra_lists_main.size() + 1);
}
void field::tag_swap(uint8_t playerid) {
	if(player[playerid].extra_lists_main.empty())
		return;

	//main
	for(auto& pcard : player[playerid].list_main) {
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
	}
	std::swap(player[playerid].list_main, player[playerid].extra_lists_main[player[playerid].tag_index]);
	for(auto& pcard : player[playerid].list_main) {
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
	}
	//hand
	for(auto& pcard : player[playerid].list_hand) {
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
	}
	std::swap(player[playerid].list_hand, player[playerid].extra_lists_hand[player[playerid].tag_index]);
	for(auto& pcard : player[playerid].list_hand) {
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
	}
	//extra
	for(auto& pcard : player[playerid].list_extra) {
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
	}
	std::swap(player[playerid].list_extra, player[playerid].extra_lists_extra[player[playerid].tag_index]);
	std::swap(player[playerid].extra_p_count, player[playerid].extra_extra_p_count[player[playerid].tag_index]);
	for(auto& pcard : player[playerid].list_extra) {
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
	}
	auto message = pduel->new_message(MSG_TAG_SWAP);
	message->write<uint8_t>(playerid);
	message->write<uint32_t>(player[playerid].list_main.size());
	message->write<uint32_t>(player[playerid].list_extra.size());
	message->write<uint32_t>(player[playerid].extra_p_count);
	message->write<uint32_t>(player[playerid].list_hand.size());
	if(core.deck_reversed && player[playerid].list_main.size())
		message->write<uint32_t>(player[playerid].list_main.back()->data.code);
	else
		message->write<uint32_t>(0);
	for(auto& pcard : player[playerid].list_hand) {
		message->write<uint32_t>(pcard->data.code);
		message->write<uint32_t>(pcard->current.position);
	}
	for(auto& pcard : player[playerid].list_extra) {
		message->write<uint32_t>(pcard->data.code);
		message->write<uint32_t>(pcard->current.position);
	}
	player[playerid].tag_index = (player[playerid].tag_index + 1) % player[playerid].extra_lists_main.size();
}
bool field::relay_check(uint8_t playerid) {
	if (player[playerid].exchanges >= player[playerid].extra_lists_main.size())
		return false;
	++player[playerid].exchanges;
	core.force_turn_end = true;
	player[playerid].recharge = true;
	return true;
}
void field::next_player(uint8_t playerid) {
	//main
	for(auto& pcard : player[playerid].list_main) {
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
	}
	std::swap(player[playerid].list_main, player[playerid].extra_lists_main[player[playerid].exchanges - 1]);
	for(auto& pcard : player[playerid].list_main) {
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
	}
	//hand
	for(auto& pcard : player[playerid].list_hand) {
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
	}
	std::swap(player[playerid].list_hand, player[playerid].extra_lists_hand[player[playerid].exchanges - 1]);
	for(auto& pcard : player[playerid].list_hand) {
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
	}
	//extra
	for(auto& pcard : player[playerid].list_extra) {
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
	}
	std::swap(player[playerid].list_extra, player[playerid].extra_lists_extra[player[playerid].exchanges - 1]);
	std::swap(player[playerid].extra_p_count, player[playerid].extra_extra_p_count[player[playerid].exchanges - 1]);
	for(auto& pcard : player[playerid].list_extra) {
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
	}
	auto message = pduel->new_message(MSG_TAG_SWAP);
	message->write<uint8_t>(playerid);
	message->write<uint32_t>(player[playerid].list_main.size());
	message->write<uint32_t>(player[playerid].list_extra.size());
	message->write<uint32_t>(player[playerid].extra_p_count);
	message->write<uint32_t>(player[playerid].list_hand.size());
	if (core.deck_reversed && player[playerid].list_main.size())
		message->write<uint32_t>(player[playerid].list_main.back()->data.code);
	else
		message->write<uint32_t>(0);
	for(auto& pcard : player[playerid].list_hand) {
		message->write<uint32_t>(pcard->data.code);
		message->write<uint32_t>(pcard->current.position);
	}
	for(auto& pcard : player[playerid].list_extra) {
		message->write<uint32_t>(pcard->data.code);
		message->write<uint32_t>(pcard->current.position);
	}
	player[playerid].lp = player[playerid].start_lp;
	message = pduel->new_message(MSG_LPUPDATE);
	message->write<uint8_t>(playerid);
	message->write<uint32_t>(player[playerid].start_lp);
	player[playerid].recharge = false;
}
bool field::is_flag(uint64_t flag) const {
	return (core.duel_options & flag) == flag;
}
bool field::has_separate_pzone(uint8_t p) const {
	(void)p;
	return is_flag(DUEL_SEPARATE_PZONE);
}
uint32_t field::get_pzone_zones_flag() const {
	uint32_t flag = 0;
	if(!has_separate_pzone(0))
		flag |= 0xC000;
	if(!has_separate_pzone(1))
		flag |= 0xC000 << 16;
	return flag;
}
uint8_t field::get_pzone_index(uint8_t seq, uint8_t p) const {
	if(seq > 1)
		return 0;
	if(has_separate_pzone(p))        // 6 and 7
		return seq + 6;
	if(is_flag(DUEL_3_COLUMNS_FIELD))// 1 and 3
		return seq * 2 + 1;
	return seq * 4;					 // 0 and 4
}
void field::add_effect(effect* peffect, uint8_t owner_player) {
	if (!peffect->handler) {
		peffect->flag[0] |= EFFECT_FLAG_FIELD_ONLY;
		peffect->handler = peffect->owner;
		peffect->effect_owner = owner_player;
		peffect->id = infos.field_id++;
	}
	peffect->card_type = peffect->owner->data.type;
	effect_container::iterator it;
	if (!(peffect->type & EFFECT_TYPE_ACTIONS)) {
		it = effects.aura_effect.emplace(peffect->code, peffect);
		if(peffect->code == EFFECT_SPSUMMON_COUNT_LIMIT)
			effects.spsummon_count_eff.insert(peffect);
		if(peffect->type & EFFECT_TYPE_GRANT)
			effects.grant_effect.emplace(peffect, field_effect::gain_effects());
	} else {
		if (peffect->type & EFFECT_TYPE_IGNITION)
			it = effects.ignition_effect.emplace(peffect->code, peffect);
		else if (peffect->type & EFFECT_TYPE_TRIGGER_O && peffect->type & EFFECT_TYPE_FIELD)
			it = effects.trigger_o_effect.emplace(peffect->code, peffect);
		else if (peffect->type & EFFECT_TYPE_TRIGGER_F && peffect->type & EFFECT_TYPE_FIELD)
			it = effects.trigger_f_effect.emplace(peffect->code, peffect);
		else if (peffect->type & EFFECT_TYPE_QUICK_O)
			it = effects.quick_o_effect.emplace(peffect->code, peffect);
		else if (peffect->type & EFFECT_TYPE_QUICK_F)
			it = effects.quick_f_effect.emplace(peffect->code, peffect);
		else if (peffect->type & EFFECT_TYPE_ACTIVATE)
			it = effects.activate_effect.emplace(peffect->code, peffect);
		else if (peffect->type & EFFECT_TYPE_CONTINUOUS)
			it = effects.continuous_effect.emplace(peffect->code, peffect);
	}
	effects.indexer.emplace(peffect, it);
	if(peffect->is_flag(EFFECT_FLAG_FIELD_ONLY)) {
		if(peffect->is_disable_related())
			update_disable_check_list(peffect);
		if(peffect->is_flag(EFFECT_FLAG_OATH))
			effects.oath.emplace(peffect, core.reason_effect);
		if(peffect->reset_flag & RESET_PHASE)
			effects.pheff.insert(peffect);
		if(peffect->reset_flag & RESET_CHAIN)
			effects.cheff.insert(peffect);
		if(peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT))
			effects.rechargeable.insert(peffect);
		if(peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET) && peffect->is_flag(EFFECT_FLAG_CLIENT_HINT)) {
			bool target_player[2] = {false, false};
			if(peffect->s_range)
				target_player[owner_player] = true;
			if(peffect->o_range)
				target_player[1 - owner_player] = true;
			if(target_player[0]) {
				auto message = pduel->new_message(MSG_PLAYER_HINT);
				message->write<uint8_t>(0);
				message->write<uint8_t>(PHINT_DESC_ADD);
				message->write<uint64_t>(peffect->description);
			}
			if(target_player[1]) {
				auto message = pduel->new_message(MSG_PLAYER_HINT);
				message->write<uint8_t>(1);
				message->write<uint8_t>(PHINT_DESC_ADD);
				message->write<uint64_t>(peffect->description);
			}
		}
	}
}
void field::remove_effect(effect* peffect) {
	auto eit = effects.indexer.find(peffect);
	if (eit == effects.indexer.end())
		return;
	auto it = eit->second;
	if (!(peffect->type & EFFECT_TYPE_ACTIONS)) {
		effects.aura_effect.erase(it);
		if(peffect->code == EFFECT_SPSUMMON_COUNT_LIMIT)
			effects.spsummon_count_eff.erase(peffect);
		if(peffect->type & EFFECT_TYPE_GRANT)
			erase_grant_effect(peffect);
	} else {
		if (peffect->type & EFFECT_TYPE_IGNITION)
			effects.ignition_effect.erase(it);
		else if (peffect->type & EFFECT_TYPE_TRIGGER_O)
			effects.trigger_o_effect.erase(it);
		else if (peffect->type & EFFECT_TYPE_TRIGGER_F)
			effects.trigger_f_effect.erase(it);
		else if (peffect->type & EFFECT_TYPE_QUICK_O)
			effects.quick_o_effect.erase(it);
		else if (peffect->type & EFFECT_TYPE_QUICK_F)
			effects.quick_f_effect.erase(it);
		else if (peffect->type & EFFECT_TYPE_ACTIVATE)
			effects.activate_effect.erase(it);
		else if (peffect->type & EFFECT_TYPE_CONTINUOUS)
			effects.continuous_effect.erase(it);
	}
	effects.indexer.erase(peffect);
	if(peffect->is_flag(EFFECT_FLAG_FIELD_ONLY)) {
		if(peffect->is_disable_related())
			update_disable_check_list(peffect);
		if(peffect->is_flag(EFFECT_FLAG_OATH))
			effects.oath.erase(peffect);
		if(peffect->reset_flag & RESET_PHASE)
			effects.pheff.erase(peffect);
		if(peffect->reset_flag & RESET_CHAIN)
			effects.cheff.erase(peffect);
		if(peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT))
			effects.rechargeable.erase(peffect);
		core.reseted_effects.insert(peffect);
		if(peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET) && peffect->is_flag(EFFECT_FLAG_CLIENT_HINT)) {
			bool target_player[2] = {false, false};
			if(peffect->s_range)
				target_player[peffect->effect_owner] = true;
			if(peffect->o_range)
				target_player[1 - peffect->effect_owner] = true;
			if(target_player[0]) {
				auto message = pduel->new_message(MSG_PLAYER_HINT);
				message->write<uint8_t>(0);
				message->write<uint8_t>(PHINT_DESC_REMOVE);
				message->write<uint64_t>(peffect->description);
			}
			if(target_player[1]) {
				auto message = pduel->new_message(MSG_PLAYER_HINT);
				message->write<uint8_t>(1);
				message->write<uint8_t>(PHINT_DESC_REMOVE);
				message->write<uint64_t>(peffect->description);
			}
		}
	}
}
void field::remove_oath_effect(effect* reason_effect) {
	for(auto oeit = effects.oath.begin(); oeit != effects.oath.end();) {
		auto rm = oeit++;
		if(rm->second == reason_effect) {
			effect* peffect = rm->first;
			effects.oath.erase(rm);
			if(peffect->is_flag(EFFECT_FLAG_FIELD_ONLY))
				remove_effect(peffect);
			else
				peffect->handler->remove_effect(peffect);
		}
	}
}
void field::release_oath_relation(effect* reason_effect) {
	for(auto& oeit : effects.oath)
		if(oeit.second == reason_effect)
			oeit.second = 0;
}
void field::reset_phase(uint32_t phase) {
	for(auto eit = effects.pheff.begin(); eit != effects.pheff.end();) {
		auto rm = eit++;
		if((*rm)->reset(phase, RESET_PHASE)) {
			if((*rm)->is_flag(EFFECT_FLAG_FIELD_ONLY))
				remove_effect((*rm));
			else
				(*rm)->handler->remove_effect((*rm));
		}
	}
}
void field::reset_chain() {
	for(const auto& ch_lim_p : core.chain_limit_p)
		luaL_unref(pduel->lua->lua_state, LUA_REGISTRYINDEX, ch_lim_p.function);
	core.chain_limit_p.clear();
	core.effect_count_code_chain.clear();
	for(auto eit = effects.cheff.begin(); eit != effects.cheff.end();) {
		auto rm = eit++;
		if((*rm)->is_flag(EFFECT_FLAG_FIELD_ONLY))
			remove_effect((*rm));
		else
			(*rm)->handler->remove_effect((*rm));
	}
}
processor::effect_count_map& field::get_count_map(uint8_t flag) {
	if(flag & EFFECT_COUNT_CODE_DUEL)
		return core.effect_count_code_duel;
	if(flag & EFFECT_COUNT_CODE_CHAIN)
		return core.effect_count_code_chain;
	return core.effect_count_code;
}
static inline uint64_t generate_count_map_key(uint32_t code, uint8_t flag, uint8_t hopt_index, uint8_t playerid) {
	return static_cast<uint64_t>(code) << 32 | (hopt_index << 16 | flag << 8 | playerid);
}
void field::add_effect_code(uint32_t code, uint8_t flag, uint8_t hopt_index, uint8_t playerid) {
	auto& count_map = get_count_map(flag);
	const auto key = generate_count_map_key(code, hopt_index, flag, playerid);
	++count_map[key];
}
uint32_t field::get_effect_code(uint32_t code, uint8_t flag, uint8_t hopt_index, uint8_t playerid) {
	const auto& count_map = get_count_map(flag);
	const auto key = generate_count_map_key(code, hopt_index, flag, playerid);
	auto iter = count_map.find(key);
	if(iter == count_map.end())
		return 0;
	return iter->second;
}
void field::dec_effect_code(uint32_t code, uint8_t flag, uint8_t hopt_index, uint8_t playerid) {
	auto& count_map = get_count_map(flag);
	const auto key = generate_count_map_key(code, hopt_index, flag, playerid);
	auto iter = count_map.find(key);
	if(iter == count_map.end())
		return;
	--iter->second;
}
void field::filter_field_effect(uint32_t code, effect_set* eset, uint8_t sort) {
	auto rg = effects.aura_effect.equal_range(code);
	for (; rg.first != rg.second; ) {
		effect* peffect = rg.first->second;
		++rg.first;
		if (peffect->is_available())
			eset->push_back(peffect);
	}
	if(sort)
		std::sort(eset->begin(), eset->end(), effect_sort_id);
}
void field::filter_affected_cards(effect* peffect, card_set* cset) {
	if((peffect->type & EFFECT_TYPE_ACTIONS) || !(peffect->type & EFFECT_TYPE_FIELD)
		|| peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET | EFFECT_FLAG_SPSUM_PARAM))
		return;
	uint8_t self = peffect->get_handler_player();
	if(self == PLAYER_NONE)
		return;
	std::vector<card_vector*> cvec;
	uint16_t range = peffect->s_range;
	for(uint32_t p = 0; p < 2; ++p) {
		if(range & LOCATION_MZONE)
			cvec.push_back(&player[self].list_mzone);
		if(range & LOCATION_SZONE)
			cvec.push_back(&player[self].list_szone);
		if(range & LOCATION_GRAVE)
			cvec.push_back(&player[self].list_grave);
		if(range & LOCATION_REMOVED)
			cvec.push_back(&player[self].list_remove);
		if(range & LOCATION_HAND)
			cvec.push_back(&player[self].list_hand);
		if(range & LOCATION_DECK)
			cvec.push_back(&player[self].list_main);
		if(range & LOCATION_EXTRA)
			cvec.push_back(&player[self].list_extra);
		range = peffect->o_range;
		self = 1 - self;
	}
	for(auto& cvit : cvec) {
		for(auto& pcard : *cvit) {
			if(pcard && peffect->is_target(pcard))
				cset->insert(pcard);
		}
	}
}
void field::filter_inrange_cards(effect* peffect, card_set* cset) {
	if(peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET | EFFECT_FLAG_SPSUM_PARAM))
		return;
	uint8_t self = peffect->get_handler_player();
	if(self == PLAYER_NONE)
		return;
	uint16_t range = peffect->s_range;
	std::vector<card_vector*> cvec;
	for(uint32_t p = 0; p < 2; ++p) {
		if(range & LOCATION_MZONE)
			cvec.push_back(&player[self].list_mzone);
		if(range & LOCATION_SZONE)
			cvec.push_back(&player[self].list_szone);
		if(range & LOCATION_GRAVE)
			cvec.push_back(&player[self].list_grave);
		if(range & LOCATION_REMOVED)
			cvec.push_back(&player[self].list_remove);
		if(range & LOCATION_HAND)
			cvec.push_back(&player[self].list_hand);
		if(range & LOCATION_DECK)
			cvec.push_back(&player[self].list_main);
		if(range & LOCATION_EXTRA)
			cvec.push_back(&player[self].list_extra);
		range = peffect->o_range;
		self = 1 - self;
	}
	for(auto& cvit : cvec) {
		for(auto& pcard : *cvit) {
			if(pcard && peffect->is_fit_target_function(pcard))
				cset->insert(pcard);
		}
	}
}
void field::filter_player_effect(uint8_t playerid, uint32_t code, effect_set* eset, uint8_t sort) {
	auto rg = effects.aura_effect.equal_range(code);
	for (; rg.first != rg.second; ++rg.first) {
		effect* peffect = rg.first->second;
		if (peffect->is_target_player(playerid) && peffect->is_available())
			eset->push_back(peffect);
	}
	if(sort)
		std::sort(eset->begin(), eset->end(), effect_sort_id);
}
int32_t field::filter_matching_card(int32_t findex, uint8_t self, uint32_t location1, uint32_t location2, group* pgroup, card* pexception, group* pexgroup, uint32_t extraargs, card** pret, int32_t fcount, int32_t is_target) {
	if(self != 0 && self != 1)
		return FALSE;
	int32_t count = 0;
	auto checkc = [&](auto* pcard, bool(*extrafil)(card* pcard)=nullptr)->bool {
		if(pcard && (!extrafil || extrafil(pcard))
		   && pcard != pexception && !(pexgroup && pexgroup->has_card(pcard))
		   && (!findex || pduel->lua->check_matching(pcard, findex, extraargs))
		   && (!is_target || pcard->is_capable_be_effect_target(core.reason_effect, core.reason_player))) {
			if(pret) {
				*pret = pcard;
				return true;
			}
			++count;
			if(fcount && count >= fcount)
				return true;
			if(pgroup)
				pgroup->container.insert(pcard);
		}
		return false;
	};
	auto mzonechk = [&checkc](auto pcard)->bool {
		return checkc(pcard, [](auto pcard)->bool {return !pcard->get_status(STATUS_SUMMONING | STATUS_SUMMON_DISABLED | STATUS_SPSUMMON_STEP); });
	};
	auto szonechk = [&checkc](auto pcard)->bool {
		return checkc(pcard, [](auto pcard)->bool {return !pcard->is_status(STATUS_ACTIVATE_DISABLED); });
	};
	auto pzonechk = [&checkc](auto pcard)->bool {
		return checkc(pcard, [](auto pcard)->bool {return pcard->current.pzone && !pcard->is_status(STATUS_ACTIVATE_DISABLED); });
	};
	auto check_list = [](const auto& list, auto func)->bool {
		return std::find_if(list.begin(), list.end(), func) != list.end();
	};
	for(uint32_t p = 0, location = location1; p < 2; ++p, location = location2, self = 1 - self) {
		if(location & LOCATION_MZONE) {
			if(check_list(player[self].list_mzone, mzonechk))
				return TRUE;
		} else {
			if(location & LOCATION_MMZONE) {
				const auto mzonebegin = player[self].list_mzone.cbegin();
				const auto mzoneend = mzonebegin + 5;
				if(std::find_if(mzonebegin, mzoneend, mzonechk) != mzoneend)
					return TRUE;
			}
			if(location & LOCATION_EMZONE) {
				auto mzonebegin = player[self].list_mzone.cbegin() + 5;
				auto mzoneend = mzonebegin + 2;
				if(is_flag(DUEL_3_COLUMNS_FIELD)) {
					++mzonebegin;
					--mzoneend;
				}
				if(std::find_if(mzonebegin, mzoneend, mzonechk) != mzoneend)
					return TRUE;
			}
		}
		if(location & LOCATION_SZONE) {
			if(check_list(player[self].list_szone, szonechk))
				return TRUE;
		} else {
			if(location & LOCATION_STZONE) {
				auto szonebegin = player[self].list_szone.cbegin();
				auto szoneend = szonebegin + 5;
				if(is_flag(DUEL_3_COLUMNS_FIELD)) {
					++szonebegin;
					--szoneend;
				}
				if(std::find_if(szonebegin, szoneend, szonechk) != szoneend)
					return TRUE;
			}
			if((location & LOCATION_FZONE) && szonechk(player[self].list_szone[5]))
				return TRUE;
			if((location & LOCATION_PZONE) && (pzonechk(player[self].list_szone[get_pzone_index(0, self)]) || pzonechk(player[self].list_szone[get_pzone_index(1, self)])))
				return TRUE;
		}
		if((location & LOCATION_DECK) && check_list(player[self].list_main, checkc))
			return TRUE;
		if((location & LOCATION_EXTRA) && check_list(player[self].list_extra, checkc))
			return TRUE;
		if((location & LOCATION_HAND) && check_list(player[self].list_hand, checkc))
			return TRUE;
		if((location & LOCATION_GRAVE) && check_list(player[self].list_grave, checkc))
			return TRUE;
		if((location & LOCATION_REMOVED) && check_list(player[self].list_remove, checkc))
			return TRUE;
	}
	return FALSE;
}
// Duel.GetFieldGroup(), Duel.GetFieldGroupCount()
int32_t field::filter_field_card(uint8_t self, uint32_t location1, uint32_t location2, group* pgroup) {
	if(self != 0 && self != 1)
		return 0;
	uint32_t location = location1;
	size_t count = 0;
	for(uint32_t p = 0; p < 2; ++p, location = location2, self = 1 - self) {
		if(location & LOCATION_MZONE) {
			for(auto& pcard : player[self].list_mzone) {
				if(pcard && !pcard->get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP)) {
					if(pgroup)
						pgroup->container.insert(pcard);
					++count;
				}
			}
		} else {
			if(location & LOCATION_MMZONE) {
				auto mzonebegin = player[self].list_mzone.cbegin();
				auto mzoneend = mzonebegin + 5;
				if(is_flag(DUEL_3_COLUMNS_FIELD)) {
					++mzonebegin;
					--mzoneend;
				}
				for(; mzonebegin != mzoneend; ++mzonebegin) {
					auto* pcard = *mzonebegin;
					if(pcard) {
						if(pgroup)
							pgroup->container.insert(pcard);
						++count;
					}
				}
			}
			if(location & LOCATION_EMZONE) {
				for(auto it = player[self].list_mzone.cbegin() + 5, end = it + 2; it != end; ++it) {
					auto* pcard = *it;
					if(pcard) {
						if(pgroup)
							pgroup->container.insert(pcard);
						++count;
					}
				}
			}
		}
		if(location & LOCATION_SZONE) {
			for(auto& pcard : player[self].list_szone) {
				if(pcard) {
					if(pgroup)
						pgroup->container.insert(pcard);
					++count;
				}
			}
		} else {
			if(location & LOCATION_STZONE) {
				auto szonebegin = player[self].list_szone.cbegin();
				auto szoneend = szonebegin + 5;
				if(is_flag(DUEL_3_COLUMNS_FIELD)) {
					++szonebegin;
					--szoneend;
				}
				for(; szonebegin != szoneend; ++szonebegin) {
					auto* pcard = *szonebegin;
					if(pcard) {
						if(pgroup)
							pgroup->container.insert(pcard);
						++count;
					}
				}
			}
			if(location & LOCATION_FZONE) {
				card* pcard = player[self].list_szone[5];
				if(pcard) {
					if(pgroup)
						pgroup->container.insert(pcard);
					++count;
				}
			}
			if(location & LOCATION_PZONE) {
				for(int32_t i = 0; i < 2; ++i) {
					card* pcard = player[self].list_szone[get_pzone_index(i, self)];
					if(pcard && pcard->current.pzone) {
						if(pgroup)
							pgroup->container.insert(pcard);
						++count;
					}
				}
			}
		}
		if(location & LOCATION_HAND) {
			if(pgroup)
				pgroup->container.insert(player[self].list_hand.begin(), player[self].list_hand.end());
			count += player[self].list_hand.size();
		}
		if(location & LOCATION_DECK) {
			if(pgroup)
				pgroup->container.insert(player[self].list_main.rbegin(), player[self].list_main.rend());
			count += player[self].list_main.size();
		}
		if(location & LOCATION_EXTRA) {
			if(pgroup)
				pgroup->container.insert(player[self].list_extra.rbegin(), player[self].list_extra.rend());
			count += player[self].list_extra.size();
		}
		if(location & LOCATION_GRAVE) {
			if(pgroup)
				pgroup->container.insert(player[self].list_grave.rbegin(), player[self].list_grave.rend());
			count += player[self].list_grave.size();
		}
		if(location & LOCATION_REMOVED) {
			if(pgroup)
				pgroup->container.insert(player[self].list_remove.rbegin(), player[self].list_remove.rend());
			count += player[self].list_remove.size();
		}
	}
	return static_cast<int32_t>(count);
}
effect* field::is_player_affected_by_effect(uint8_t playerid, uint32_t code) {
	auto rg = effects.aura_effect.equal_range(code);
	for (; rg.first != rg.second; ++rg.first) {
		effect* peffect = rg.first->second;
		if (peffect->is_target_player(playerid) && peffect->is_available())
			return peffect;
	}
	return 0;
}
void field::get_player_effect(uint8_t playerid, uint32_t code, effect_set* eset) {
	for (auto rg = effects.aura_effect.begin(); rg != effects.aura_effect.end(); ++rg) {
		effect* peffect = rg->second;
		if((code == 0 || peffect->code == code) && peffect->is_target_player(playerid) && peffect->is_available())
			eset->push_back(peffect);
	}
}
int32_t field::get_release_list(uint8_t playerid, card_set* release_list, card_set* ex_list, card_set* ex_list_oneof, int32_t use_hand, int32_t fun, int32_t exarg, card* exc, group* exg, uint8_t use_oppo) {
	uint32_t rcount = 0;
	for(auto& pcard : player[playerid].list_mzone) {
		if(pcard && pcard != exc && !(exg && exg->has_card(pcard)) && pcard->is_releasable_by_nonsummon(playerid)
		        && (!fun || pduel->lua->check_matching(pcard, fun, exarg))) {
			if(release_list)
				release_list->insert(pcard);
			pcard->release_param = 1;
			++rcount;
		}
	}
	if(use_hand) {
		for(auto& pcard : player[playerid].list_hand) {
			if(pcard && pcard != exc && !(exg && exg->has_card(pcard)) && pcard->is_releasable_by_nonsummon(playerid)
			        && (!fun || pduel->lua->check_matching(pcard, fun, exarg))) {
				if(release_list)
					release_list->insert(pcard);
				pcard->release_param = 1;
				++rcount;
			}
		}
	}
	int32_t ex_oneof_max = 0;
	if(use_oppo) {
		for(auto& pcard : player[1 - playerid].list_mzone) {
			if(pcard && pcard != exc && !(exg && exg->has_card(pcard)) && (pcard->is_position(POS_FACEUP) || !fun)
			   && pcard->is_releasable_by_nonsummon(playerid) && (!fun || pduel->lua->check_matching(pcard, fun, exarg))) {
				if(release_list)
					release_list->insert(pcard);
				pcard->release_param = 1;
				++rcount;
			}
		}
	} else {
		for(auto& pcard : player[1 - playerid].list_mzone) {
			if(pcard && pcard != exc && !(exg && exg->has_card(pcard)) && (pcard->is_position(POS_FACEUP) || !fun)
			   && pcard->is_releasable_by_nonsummon(playerid) && (!fun || pduel->lua->check_matching(pcard, fun, exarg))) {
				pcard->release_param = 1;
				if(pcard->is_affected_by_effect(EFFECT_EXTRA_RELEASE)) {
					if(ex_list)
						ex_list->insert(pcard);
					++rcount;
				} else {
					effect* peffect = pcard->is_affected_by_effect(EFFECT_EXTRA_RELEASE_NONSUM);
					if(!peffect || (peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT) && peffect->count_limit == 0))
						continue;
					pduel->lua->add_param<PARAM_TYPE_EFFECT>(core.reason_effect);
					pduel->lua->add_param<PARAM_TYPE_INT>(REASON_COST);
					pduel->lua->add_param<PARAM_TYPE_INT>(core.reason_player);
					if(!peffect->check_value_condition(3))
						continue;
					if(ex_list_oneof)
						ex_list_oneof->insert(pcard);
					ex_oneof_max = 1;
				}
			}
		}
	}
	return rcount + ex_oneof_max;
}
int32_t field::check_release_list(uint8_t playerid, int32_t min, int32_t /*max*/, int32_t use_hand, int32_t fun, int32_t exarg, card* exc, group* exg, uint8_t check_field, uint8_t to_player, uint8_t zone, card* to_check, uint8_t use_oppo) {
	card_set relcard;
	//card_set relcard_must;
	card_set relcard_oneof;
	bool has_to_choose_one = false;
	card_set must_choose_one;
	int32_t rcount = get_release_list(playerid, &relcard, &relcard, &relcard_oneof, use_hand, fun, exarg, exc, exg, use_oppo);
	if(check_field) {
		int32_t ct = 0;
		zone &= (0x1f & get_forced_zones(to_check, playerid, LOCATION_MZONE, to_player, LOCATION_REASON_TOFIELD));
		ct = get_useable_count(to_check, playerid, LOCATION_MZONE, to_player, LOCATION_REASON_TOFIELD, zone);
		if(ct < min) {
			has_to_choose_one = true;
			for(auto& pcard : relcard) {
				if((pcard->current.location == LOCATION_MZONE && pcard->current.controler == playerid && ((zone >> pcard->current.sequence) & 1)))
					must_choose_one.insert(pcard);
			}
		}
	}
	if(has_to_choose_one && must_choose_one.empty())
		return FALSE;
	bool has_oneof = false;
	for(auto& pcard : core.must_select_cards) {
		auto it = relcard.find(pcard);
		if(it != relcard.end())
			relcard.erase(it);
		else if(!has_oneof && relcard_oneof.find(pcard) != relcard_oneof.end())
			has_oneof = true;
		else
			return FALSE;
	}
	rcount = (int32_t)relcard.size();// +relcard_must.size();
	if(!has_oneof && !relcard_oneof.empty())
		++rcount;
	return (rcount >= min);
}
// return: the max release count of mg or all monsters on field
int32_t field::get_summon_release_list(card* target, card_set* release_list, card_set* ex_list, card_set* ex_list_oneof, group* mg, uint32_t ex, uint32_t releasable, uint32_t pos) {
	uint8_t p = target->current.controler;
	card_set ex_tribute;
	effect_set eset;
	target->filter_effect(EFFECT_ADD_EXTRA_TRIBUTE, &eset);
	for(const auto& peff : eset) {
		if(peff->get_value() & pos)
			filter_inrange_cards(peff, &ex_tribute);
	}
	uint32_t rcount = 0;
	for(auto& pcard : player[p].list_mzone) {
		if(pcard && ((releasable >> pcard->current.sequence) & 1) && pcard->is_releasable_by_summon(p, target)) {
			if(mg && !mg->has_card(pcard))
				continue;
			if(release_list)
				release_list->insert(pcard);
			if(pcard->is_affected_by_effect(EFFECT_TRIPLE_TRIBUTE, target))
				pcard->release_param = 3;
			else if (pcard->is_affected_by_effect(EFFECT_DOUBLE_TRIBUTE, target))
				pcard->release_param = 2;
			else
				pcard->release_param = 1;
			rcount += pcard->release_param;
		}
	}
	uint32_t ex_oneof_max = 0;
	for(auto& pcard : player[1 - p].list_mzone) {
		if(!pcard || !((releasable >> (pcard->current.sequence + 16)) & 1) || !pcard->is_releasable_by_summon(p, target))
			continue;
		if (mg && !mg->has_card(pcard))
			continue;
		if(pcard->is_affected_by_effect(EFFECT_TRIPLE_TRIBUTE, target))
			pcard->release_param = 3;
		else if (pcard->is_affected_by_effect(EFFECT_DOUBLE_TRIBUTE, target))
			pcard->release_param = 2;
		else
			pcard->release_param = 1;
		if(pcard->is_affected_by_effect(EFFECT_EXTRA_RELEASE)) {
			if(ex_list)
				ex_list->insert(pcard);
			rcount += pcard->release_param;
		} else if(ex || ex_tribute.find(pcard) != ex_tribute.end()) {
			if(release_list)
				release_list->insert(pcard);
			rcount += pcard->release_param;
		} else {
			effect* peffect = pcard->is_affected_by_effect(EFFECT_EXTRA_RELEASE_SUM);
			if(!peffect || (peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT) && peffect->count_limit == 0))
				continue;
			if(ex_list_oneof)
				ex_list_oneof->insert(pcard);
			if(ex_oneof_max < pcard->release_param)
				ex_oneof_max = pcard->release_param;
		}
	}
	for(auto& pcard : ex_tribute) {
		if(pcard->current.location == LOCATION_MZONE || !pcard->is_releasable_by_summon(p, target))
			continue;
		if(release_list)
			release_list->insert(pcard);
		if(pcard->is_affected_by_effect(EFFECT_TRIPLE_TRIBUTE, target))
			pcard->release_param = 3;
		else if (pcard->is_affected_by_effect(EFFECT_DOUBLE_TRIBUTE, target))
			pcard->release_param = 2;
		else
			pcard->release_param = 1;
		rcount += pcard->release_param;
	}
	return rcount + ex_oneof_max;
}
int32_t field::get_summon_count_limit(uint8_t playerid) {
	if(is_flag(DUEL_UNLIMITED_SUMMONS)) {
		return INT32_MAX - 100;
	}
	effect_set eset;
	filter_player_effect(playerid, EFFECT_SET_SUMMON_COUNT_LIMIT, &eset);
	int32_t count = 1;
	for(const auto& peff : eset) {
		int32_t c = peff->get_value();
		if(c > count)
			count = c;
	}
	return count;
}
int32_t field::get_draw_count(uint8_t playerid) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_DRAW_COUNT, &eset);
	int32_t count = player[playerid].draw_count;
	for(const auto& peffect : eset) {
		int32_t c = peffect->get_value();
		if(c == 0)
			return 0;
		if(c > count)
			count = c;
	}
	return count;
}
void field::get_ritual_material(uint8_t playerid, effect* peffect, card_set* material, bool check_level) {
	auto mzonecheck = [&](card* pcard) {
		return  (!check_level || pcard->get_level() > 0) && pcard->is_affect_by_effect(peffect)
			&& pcard->is_releasable_by_effect(playerid, peffect);
	};
	for(auto& pcard : player[playerid].list_mzone) {
		if(pcard && mzonecheck(pcard) && pcard->is_releasable_by_nonsummon(playerid))
			material->insert(pcard);
	}
	for(auto& pcard : player[1 - playerid].list_mzone) {
		if(pcard && pcard->is_position(POS_FACEUP) && mzonecheck(pcard) && pcard->is_releasable_by_nonsummon(playerid) && pcard->is_affected_by_effect(EFFECT_EXTRA_RELEASE))
			material->insert(pcard);
	}
	for(auto& pcard : player[playerid].list_hand)
		if((pcard->data.type & TYPE_MONSTER) && pcard->is_releasable_by_nonsummon(playerid))
			material->insert(pcard);
	for(auto& pcard : player[playerid].list_grave)
		if((pcard->data.type & TYPE_MONSTER) && pcard->is_affected_by_effect(EFFECT_EXTRA_RITUAL_MATERIAL) && pcard->is_removeable(playerid, POS_FACEUP, REASON_EFFECT))
			material->insert(pcard);
	for(auto& pcard : player[playerid].list_main)
		if(pcard->is_affected_by_effect(EFFECT_EXTRA_RITUAL_MATERIAL)
		   && (!check_level || pcard->get_level() > 0))
			material->insert(pcard);
	for(auto& pcard : player[playerid].list_extra)
		if(pcard->is_affected_by_effect(EFFECT_EXTRA_RITUAL_MATERIAL)
		   && (!check_level || pcard->get_level() > 0))
			material->insert(pcard);
	for(auto& pcard : player[playerid].list_mzone) {
		if(!pcard)
			continue;
		for(auto& ppcard : pcard->xyz_materials)
			if(ppcard->is_affected_by_effect(EFFECT_EXTRA_RITUAL_MATERIAL))
				material->insert(ppcard);
	}
}
void field::get_fusion_material(uint8_t playerid, card_set* material) {
	for(auto& pcard : player[playerid].list_mzone) {
		if(pcard)
			material->insert(pcard);
	}
	for(auto& pcard : player[playerid].list_szone) {
		if(pcard && pcard->is_affected_by_effect(EFFECT_EXTRA_FUSION_MATERIAL))
			material->insert(pcard);
	}
	for(auto& pcard : player[playerid].list_hand)
		if(pcard->data.type & TYPE_MONSTER || pcard->is_affected_by_effect(EFFECT_EXTRA_FUSION_MATERIAL))
			material->insert(pcard);
	for(auto& pcard : player[playerid].list_grave)
		if(pcard->is_affected_by_effect(EFFECT_EXTRA_FUSION_MATERIAL))
			material->insert(pcard);
}
void field::ritual_release(const card_set& material) {
	card_set rel, rem, tograve;
	for(auto& pcard : material) {
		if(pcard->current.location == LOCATION_GRAVE)
			rem.insert(pcard);
		else if((pcard->current.location & (LOCATION_OVERLAY | LOCATION_EXTRA | LOCATION_DECK)) != 0)
			tograve.insert(pcard);
		else
			rel.insert(pcard);
	}
	send_to(std::move(tograve), core.reason_effect, REASON_RITUAL + REASON_EFFECT + REASON_MATERIAL, core.reason_player, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
	release(std::move(rel), core.reason_effect, REASON_RITUAL + REASON_EFFECT + REASON_MATERIAL, core.reason_player);
	send_to(std::move(rem), core.reason_effect, REASON_RITUAL + REASON_EFFECT + REASON_MATERIAL, core.reason_player, PLAYER_NONE, LOCATION_REMOVED, 0, POS_FACEUP);
}
void field::get_overlay_group(uint8_t playerid, uint8_t self, uint8_t oppo, card_set* pset, group* pgroup) {
	if(pgroup) {
		for(auto& pcard : pgroup->container) {
			if(pcard && !pcard->get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP))
				pset->insert(pcard->xyz_materials.begin(), pcard->xyz_materials.end());
		}
	} else {
		for(int i = 0; i < 2; ++i) {
			if((i == playerid && self) || (i == (1 - playerid) && oppo)) {
				for(auto& pcard : player[i].list_mzone) {
					if(pcard && !pcard->get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP))
						pset->insert(pcard->xyz_materials.begin(), pcard->xyz_materials.end());
				}
			}
		}
	}
}
int32_t field::get_overlay_count(uint8_t playerid, uint8_t self, uint8_t oppo, group* pgroup) {
	size_t count = 0;
	if(pgroup) {
		for(auto& pcard : pgroup->container) {
			if(pcard && !pcard->get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP))
				count += pcard->xyz_materials.size();
		}
		return static_cast<int32_t>(count);
	}
	for(int i = 0; i < 2; ++i) {
		if((i == playerid && self) || (i == (1 - playerid) && oppo)) {
			for(auto& pcard : player[i].list_mzone) {
				if(pcard && !pcard->get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP))
					count += pcard->xyz_materials.size();
			}
		}
	}
	return static_cast<int32_t>(count);
}
// put all cards in the target of peffect into effects.disable_check_set
void field::update_disable_check_list(effect* peffect) {
	card_set cset;
	filter_affected_cards(peffect, &cset);
	for (auto& pcard : cset)
		add_to_disable_check_list(pcard);
}
void field::add_to_disable_check_list(card* pcard) {
	effects.disable_check_set.insert(pcard);
}
void field::adjust_disable_check_list() {
	for(const auto& pcard : effects.disable_check_set) {
		if(!pcard->is_status(STATUS_TO_ENABLE | STATUS_TO_DISABLE)) { // prevent loop
			int32_t pre_disable = pcard->get_status(STATUS_DISABLED | STATUS_FORBIDDEN);
			pcard->refresh_disable_status();
			int32_t new_disable = pcard->get_status(STATUS_DISABLED | STATUS_FORBIDDEN);
			if(pre_disable != new_disable && pcard->is_status(STATUS_EFFECT_ENABLED)) {
				pcard->filter_disable_related_cards(); // change effects.disable_check_list
				if(pre_disable)
					pcard->set_status(STATUS_TO_ENABLE, TRUE);
				else
					pcard->set_status(STATUS_TO_DISABLE, TRUE);
			}
		}
		if(pcard->is_status(STATUS_DISABLED) && pcard->is_status(STATUS_TO_DISABLE) && !pcard->is_status(STATUS_TO_ENABLE))
			pcard->reset(RESET_DISABLE, RESET_EVENT);
		pcard->set_status(STATUS_TO_ENABLE | STATUS_TO_DISABLE, FALSE);
	}
	effects.disable_check_set.clear();
}
// adjust SetUniqueOnField(), EFFECT_SELF_DESTROY, EFFECT_SELF_TOGRAVE
void field::adjust_self_destroy_set() {
	if(core.selfdes_disabled || !core.unique_destroy_set.empty() || !core.self_destroy_set.empty() || !core.self_tograve_set.empty())
		return;
	int32_t p = infos.turn_player;
	for(int32_t p1 = 0; p1 < 2; ++p1) {
		std::vector<card*> uniq_set;
		for(auto& ucard : core.unique_cards[p]) {
			if(ucard->is_position(POS_FACEUP) && ucard->get_status(STATUS_EFFECT_ENABLED)
					&& !ucard->get_status(STATUS_DISABLED | STATUS_FORBIDDEN)) {
				card_set cset;
				ucard->get_unique_target(&cset, p);
				if(cset.size() == 0)
					ucard->unique_fieldid = 0;
				else if(cset.size() == 1) {
					auto cit = cset.begin();
					ucard->unique_fieldid = (*cit)->fieldid;
				} else
					uniq_set.push_back(ucard);
			}
		}
		std::sort(uniq_set.begin(), uniq_set.end(), [](card* lhs, card* rhs) { return lhs->fieldid < rhs->fieldid; });
		for(auto& pcard : uniq_set) {
			add_process(PROCESSOR_SELF_DESTROY, 0, 0, 0, p, 0, 0, 0, pcard);
			core.unique_destroy_set.insert(pcard);
		}
		p = 1 - p;
	}
	card_set cset;
	for(p = 0; p < 2; ++p) {
		for(auto& pcard : player[p].list_mzone) {
			if(pcard && pcard->is_position(POS_FACEUP) && !pcard->is_status(STATUS_BATTLE_DESTROYED))
				cset.insert(pcard);
		}
		for(auto& pcard : player[p].list_szone) {
			if(pcard && pcard->is_position(POS_FACEUP))
				cset.insert(pcard);
		}
	}
	for(auto& pcard : cset) {
		effect* peffect = pcard->is_affected_by_effect(EFFECT_SELF_DESTROY);
		if(peffect) {
			core.self_destroy_set.insert(pcard);
		}
	}
	if(core.global_flag & GLOBALFLAG_SELF_TOGRAVE) {
		for(auto& pcard : cset) {
			effect* peffect = pcard->is_affected_by_effect(EFFECT_SELF_TOGRAVE);
			if(peffect) {
				core.self_tograve_set.insert(pcard);
			}
		}
	}
	if(!core.self_destroy_set.empty())
		add_process(PROCESSOR_SELF_DESTROY, 10, 0, 0, 0, 0);
	if(!core.self_tograve_set.empty())
		add_process(PROCESSOR_SELF_DESTROY, 20, 0, 0, 0, 0);
}
void field::erase_grant_effect(effect* peffect) {
	auto eit = effects.grant_effect.find(peffect);
	for(auto& it : eit->second)
		it.first->remove_effect(it.second);
	effects.grant_effect.erase(eit);
}
int32_t field::adjust_grant_effect() {
	auto CheckValidLabel = [this](int32_t lref) {
		if(!lref)
			return false;
		const auto L = pduel->lua->current_state;
		bool res = false;
		lua_rawgeti(L, LUA_REGISTRYINDEX, lref);
		if(auto obj = lua_touserdata(L, -1)) {
			res = (*static_cast<lua_obj**>(obj))->lua_type == PARAM_TYPE_EFFECT;
		}
		lua_pop(L, 1);
		return res;
	};
	int32_t adjusted = FALSE;
	for(auto& eit : effects.grant_effect.sorted) {
		effect* peffect = eit.first;
		if(!CheckValidLabel(peffect->label_object))
			continue;
		card_set cset;
		if(peffect->is_available())
			filter_affected_cards(peffect, &cset);
		card_set add_set;
		for(auto& pcard : cset) {
			if(pcard->is_affect_by_effect(peffect) && !eit.second->count(pcard))
				add_set.insert(pcard);
		}
		card_set remove_set;
		for(auto& cit : *eit.second) {
			card* pcard = cit.first;
			if(!pcard->is_affect_by_effect(peffect) || !cset.count(pcard))
				remove_set.insert(pcard);
		}
		for(auto& pcard : add_set) {
			effect* geffect = (effect*)peffect->get_label_object();
			effect* ceffect = geffect->clone();
			ceffect->owner = pcard;
			pcard->add_effect(ceffect);
			eit.second->emplace(pcard, ceffect);
		}
		for(auto& pcard : remove_set) {
			auto it = eit.second->find(pcard);
			pcard->remove_effect(it->second);
			eit.second->erase(it);
		}
		if(!add_set.empty() || !remove_set.empty())
			adjusted = TRUE;
	}
	return adjusted;
}
void field::add_unique_card(card* pcard) {
	uint8_t con = pcard->current.controler;
	if(pcard->unique_pos[0])
		core.unique_cards[con].insert(pcard);
	if(pcard->unique_pos[1])
		core.unique_cards[1 - con].insert(pcard);
	pcard->unique_fieldid = 0;
}
void field::remove_unique_card(card* pcard) {
	uint8_t con = pcard->current.controler;
	if(con == PLAYER_NONE)
		return;
	if(pcard->unique_pos[0])
		core.unique_cards[con].erase(pcard);
	if(pcard->unique_pos[1])
		core.unique_cards[1 - con].erase(pcard);
}
// return: pcard->unique_effect or 0
effect* field::check_unique_onfield(card* pcard, uint8_t controler, uint8_t location, card* icard) {
	for(auto& ucard : core.unique_cards[controler]) {
		if((ucard != pcard) && (ucard != icard) && ucard->is_position(POS_FACEUP) && ucard->get_status(STATUS_EFFECT_ENABLED)
			&& !ucard->get_status(STATUS_DISABLED | STATUS_FORBIDDEN)
			&& ucard->unique_fieldid && ucard->check_unique_code(pcard) && (ucard->unique_location & location))
			return ucard->unique_effect;
	}
	if(!pcard->unique_code || !(pcard->unique_location & location) || pcard->get_status(STATUS_DISABLED | STATUS_FORBIDDEN))
		return 0;
	card_set cset;
	pcard->get_unique_target(&cset, controler, icard);
	if(pcard->check_unique_code(pcard))
		cset.insert(pcard);
	if(cset.size() >= 2)
		return pcard->unique_effect;
	return 0;
}
int32_t field::check_spsummon_once(card* pcard, uint8_t playerid) {
	if(pcard->spsummon_code == 0)
		return TRUE;
	auto iter = core.spsummon_once_map[playerid].find(pcard->spsummon_code);
	return (iter == core.spsummon_once_map[playerid].end()) || (iter->second == 0);
}
// increase the binary custom counter 1~5
void field::check_card_counter(card* pcard, ActivityType counter_type, int32_t playerid) {
	for(auto& iter : core.get_counter_map(counter_type)) {
		auto& info = iter.second;
		auto& player_counter = info.player_amount[playerid];
		if(player_counter != 0)
			continue;
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		if(!pduel->lua->check_condition(info.check_function, 1))
			++player_counter;
	}
}
void field::check_card_counter(group* pgroup, ActivityType counter_type, int32_t playerid) {
	for(auto& iter : core.get_counter_map(counter_type)) {
		auto& info = iter.second;
		auto& player_counter = info.player_amount[playerid];
		if(player_counter != 0)
			continue;
		for(auto& pcard : pgroup->container) {
			pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
			if(!pduel->lua->check_condition(info.check_function, 1)) {
				++player_counter;
				break;
			}
		}
	}
}

chain::applied_chain_counter_t* field::check_chain_counter(effect* peffect, int32_t playerid, int32_t chainid) {
	chain::applied_chain_counter_t* ret = nullptr;
	for(auto& iter : core.chain_counter) {
		auto& info = iter.second;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peffect);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		pduel->lua->add_param<PARAM_TYPE_INT>(chainid);
		if(!pduel->lua->check_condition(info.check_function, 3)) {
			if(ret == nullptr)
				ret = new chain::applied_chain_counter_t;
			++info.player_amount[playerid];
			ret->push_back(iter.first);
		}
	}
	return ret;
}
void field::restore_chain_counter(uint8_t playerid, const chain::applied_chain_counter_t& counters) {
	for(auto& id : counters)
		--core.chain_counter[id].player_amount[playerid];
}
void field::set_spsummon_counter(uint8_t playerid, bool add, bool chain) {
	if(is_flag(DUEL_CANNOT_SUMMON_OATH_OLD)) {
		if(add) {
			++core.spsummon_state_count[playerid];
			if(chain)
				++core.spsummon_state_count_rst[playerid];
		} else {
			if(chain) {
				core.spsummon_state_count[playerid] -= core.spsummon_state_count_rst[playerid];
				core.spsummon_state_count_rst[playerid] = 0;
			} else
				--core.spsummon_state_count[playerid];
		}
	} else
		++core.spsummon_state_count[playerid];
	if(core.global_flag & GLOBALFLAG_SPSUMMON_COUNT) {
		for(auto& peffect : effects.spsummon_count_eff) {
			card* pcard = peffect->get_handler();
			if(is_flag(DUEL_CANNOT_SUMMON_OATH_OLD)) {
				if(add) {
					if(peffect->is_available()) {
						if(((playerid == pcard->current.controler) && peffect->s_range) || ((playerid != pcard->current.controler) && peffect->o_range)) {
							++pcard->spsummon_counter[playerid];
							if(chain)
								++pcard->spsummon_counter_rst[playerid];
						}
					}
				} else {
					pcard->spsummon_counter[playerid] -= pcard->spsummon_counter_rst[playerid];
					pcard->spsummon_counter_rst[playerid] = 0;
				}
			} else {
				if(peffect->is_available()) {
					if(((playerid == pcard->current.controler) && peffect->s_range) || ((playerid != pcard->current.controler) && peffect->o_range)) {
						++pcard->spsummon_counter[playerid];
					}
				}
			}
		}
	}
}
int32_t field::check_spsummon_counter(uint8_t playerid, uint8_t ct) {
	if(core.global_flag & GLOBALFLAG_SPSUMMON_COUNT) {
		for(auto& peffect : effects.spsummon_count_eff) {
			card* pcard = peffect->get_handler();
			uint16_t val = (uint16_t)peffect->value;
			if(peffect->is_available()) {
				if(pcard->spsummon_counter[playerid] + ct > val)
					return FALSE;
			}
		}
	}
	return TRUE;
}
int32_t field::check_lp_cost(uint8_t playerid, uint32_t lp) {
	effect_set eset;
	int32_t val = lp;
	filter_player_effect(playerid, EFFECT_LPCOST_CHANGE, &eset);
	for(const auto& peff : eset) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(core.reason_effect);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		pduel->lua->add_param<PARAM_TYPE_INT>(val);
		val = peff->get_value(3);
	}
	if(val <= 0)
		return TRUE;
	tevent e;
	e.event_cards = 0;
	e.event_player = playerid;
	e.event_value = lp;
	e.reason = 0;
	e.reason_effect = core.reason_effect;
	e.reason_player = playerid;
	if(effect_replace_check(EFFECT_LPCOST_REPLACE, e))
		return TRUE;
	//cost[playerid].amount += val;
	if(val <= player[playerid].lp)
		return TRUE;
	return FALSE;
}
/*
void field::save_lp_cost() {
	for(uint8_t playerid = 0; playerid < 2; ++playerid) {
		if(cost[playerid].count < 8)
			cost[playerid].lpstack[cost[playerid].count] = cost[playerid].amount;
		++cost[playerid].count;
	}
}
void field::restore_lp_cost() {
	for(uint8_t playerid = 0; playerid < 2; ++playerid) {
		--cost[playerid].count;
		if(cost[playerid].count < 8)
			cost[playerid].amount = cost[playerid].lpstack[cost[playerid].count];
	}
}
*/
uint32_t field::get_field_counter(uint8_t playerid, uint8_t self, uint8_t oppo, uint16_t countertype) {
	uint8_t c = self;
	uint32_t count = 0;
	for(int32_t p = 0; p < 2; ++p, playerid = 1 - playerid, c = oppo) {
		if(c) {
			for(auto& pcard : player[playerid].list_mzone) {
				if(pcard)
					count += pcard->get_counter(countertype);
			}
			for(auto& pcard : player[playerid].list_szone) {
				if(pcard)
					count += pcard->get_counter(countertype);
			}
		}
	}
	return count;
}
int32_t field::effect_replace_check(uint32_t code, const tevent& e) {
	auto pr = effects.continuous_effect.equal_range(code);
	for(auto eit = pr.first; eit != pr.second;) {
		effect* peffect = eit->second;
		++eit;
		if(peffect->is_activateable(peffect->get_handler_player(), e))
			return TRUE;
	}
	return FALSE;
}
int32_t field::get_attack_target(card* pcard, card_vector* v, uint8_t chain_attack, bool select_target) {
	pcard->direct_attackable = 0;
	uint8_t p = pcard->current.controler;
	card_vector auto_attack, only_attack, must_attack, attack_tg;
	for(auto& atarget : player[1 - p].list_mzone) {
		if(atarget) {
			if(atarget->is_affected_by_effect(EFFECT_ONLY_BE_ATTACKED))
				auto_attack.push_back(atarget);
			if(pcard->is_affected_by_effect(EFFECT_ONLY_ATTACK_MONSTER, atarget))
				only_attack.push_back(atarget);
			if(pcard->is_affected_by_effect(EFFECT_MUST_ATTACK_MONSTER, atarget))
				must_attack.push_back(atarget);
		}
	}
	card_vector* pv = nullptr;
	int32_t atype = 0;
	if(auto_attack.size()) {
		atype = 1;
		if(auto_attack.size() != 1)
			return atype;
		pv = &auto_attack;
	} else if(pcard->is_affected_by_effect(EFFECT_ONLY_ATTACK_MONSTER)) {
		atype = 2;
		if(only_attack.size() != 1)
			return atype;
		pv = &only_attack;
	} else if(pcard->is_affected_by_effect(EFFECT_MUST_ATTACK_MONSTER)) {
		atype = 3;
		if(must_attack.empty())
			return atype;
		pv = &must_attack;
	} else {
		atype = 4;
		for(auto& atarget : player[1 - p].list_mzone)
			if(atarget != core.attacker)
				attack_tg.push_back(atarget);
		if(is_player_affected_by_effect(p, EFFECT_SELF_ATTACK) && (!pcard->is_affected_by_effect(EFFECT_ATTACK_ALL) || !attack_tg.size())) {
			for(auto& atarget : player[p].list_mzone)
				if (atarget != core.attacker)
					attack_tg.push_back(atarget);
		}
		pv = &attack_tg;
	}
	int32_t extra_count = 0;
	effect_set eset;
	pcard->filter_effect(EFFECT_EXTRA_ATTACK, &eset);
	for(const auto& peff : eset) {
		int32_t val = peff->get_value(pcard);
		if(val > extra_count)
			extra_count = val;
	}
	int32_t extra_count_m = 0;
	eset.clear();
	pcard->filter_effect(EFFECT_EXTRA_ATTACK_MONSTER, &eset);
	for(const auto& peff : eset) {
		int32_t val = peff->get_value(pcard);
		if(val > extra_count_m)
			extra_count_m = val;
	}
	if(!chain_attack && pcard->announce_count > extra_count
		&& (!extra_count_m || pcard->announce_count > extra_count_m || pcard->announced_cards.findcard(0))) {
		effect* peffect;
		if((peffect = pcard->is_affected_by_effect(EFFECT_ATTACK_ALL)) != nullptr && pcard->attack_all_target) {
			for(auto& atarget : *pv) {
				if(!atarget)
					continue;
				pduel->lua->add_param<PARAM_TYPE_CARD>(atarget);
				if(!peffect->check_value_condition(1))
					continue;
				if(pcard->announced_cards.findcard(atarget) >= (uint32_t)peffect->get_value(atarget))
					continue;
				if(atype >= 2 && atarget->is_affected_by_effect(EFFECT_IGNORE_BATTLE_TARGET, pcard))
					continue;
				if(select_target && (atype == 2 || atype == 4)) {
					if(atarget->is_affected_by_effect(EFFECT_CANNOT_BE_BATTLE_TARGET, pcard))
						continue;
					if(pcard->is_affected_by_effect(EFFECT_CANNOT_SELECT_BATTLE_TARGET, atarget))
						continue;
				}
				v->push_back(atarget);
			}
		}
		return atype;
	}
	//chain attack or announce count check passed
	uint32_t mcount = 0;
	for(auto& atarget : *pv) {
		if(!atarget)
			continue;
		if(atype >= 2 && atarget->is_affected_by_effect(EFFECT_IGNORE_BATTLE_TARGET, pcard))
			continue;
		if(atarget->current.controler != p)
			++mcount;
		if(chain_attack && core.chain_attack_target && atarget != core.chain_attack_target)
			continue;
		if(select_target && (atype == 2 || atype == 4)) {
			if(atarget->is_affected_by_effect(EFFECT_CANNOT_BE_BATTLE_TARGET, pcard))
				continue;
			if(pcard->is_affected_by_effect(EFFECT_CANNOT_SELECT_BATTLE_TARGET, atarget))
				continue;
		}
		v->push_back(atarget);
	}
	if(atype <= 3)
		return atype;
	if((mcount == 0 || pcard->is_affected_by_effect(EFFECT_DIRECT_ATTACK) || core.attack_player)
			&& !pcard->is_affected_by_effect(EFFECT_CANNOT_DIRECT_ATTACK)
			&& !(extra_count_m && pcard->announce_count > extra_count)
			&& !(chain_attack && core.chain_attack_target))
		pcard->direct_attackable = 1;
	return atype;
}
bool field::confirm_attack_target() {
	card_vector cv;
	get_attack_target(core.attacker, &cv, core.chain_attack, false);
	return (core.attack_target && std::find(cv.begin(), cv.end(), core.attack_target) != cv.end())
		|| (!core.attack_target && core.attacker->direct_attackable);
}
// update the validity for EFFECT_ATTACK_ALL (approximate solution)
void field::attack_all_target_check() {
	if(!core.attacker)
		return;
	if(!core.attack_target) {
		core.attacker->attack_all_target = FALSE;
		return;
	}
	effect* peffect = core.attacker->is_affected_by_effect(EFFECT_ATTACK_ALL);
	if(!peffect)
		return;
	if(!peffect->get_value(core.attack_target))
		core.attacker->attack_all_target = FALSE;
}
int32_t field::check_tribute(card* pcard, int32_t min, int32_t max, group* mg, uint8_t toplayer, uint32_t zone, uint32_t releasable, uint32_t pos) {
	int32_t ex = FALSE;
	uint32_t sumplayer = pcard->current.controler;
	if(toplayer == 1 - sumplayer)
		ex = TRUE;
	card_set release_list, ex_list;
	int32_t m = get_summon_release_list(pcard, &release_list, &ex_list, 0, mg, ex, releasable, pos);
	if(max > m)
		max = m;
	if(min > max)
		return FALSE;
	zone &= (0x1f & get_forced_zones(pcard, toplayer, LOCATION_MZONE, sumplayer, LOCATION_REASON_TOFIELD));
	int32_t s = 0;
	int32_t ct = get_tofield_count(pcard, toplayer, LOCATION_MZONE, sumplayer, LOCATION_REASON_TOFIELD, zone);
	if(ct <= 0 && max <= 0)
		return FALSE;
	for(auto& _pcard : (static_cast<int>(ex_list.size()) >= min) ? ex_list : release_list) {
		if(_pcard->current.location == LOCATION_MZONE && _pcard->current.controler == toplayer) {
			++s;
			if((zone >> _pcard->current.sequence) & 1)
				++ct;
		}
	}
	if(ct <= 0)
		return FALSE;
	max -= (int32_t)ex_list.size();
	int32_t fcount = get_mzone_limit(toplayer, sumplayer, LOCATION_REASON_TOFIELD);
	if(s < -fcount + 1)
		return FALSE;
	if(max < 0)
		max = 0;
	if(max < -fcount + 1)
		return FALSE;
	return TRUE;
}
int32_t field::check_with_sum_limit(const card_vector& mats, int32_t acc, int32_t index, int32_t count, int32_t min, int32_t max, int32_t* should_continue) {
	if(count > max)
		return FALSE;
	while(index < (int32_t)mats.size()) {
		int32_t op1 = mats[index]->sum_param & 0xffff;
		int32_t op2 = (mats[index]->sum_param >> 16) & 0xffff;
		if((op1 == acc || op2 == acc) && count >= min)
			return TRUE;
		++index;
		if(acc > op1 && check_with_sum_limit(mats, acc - op1, index, count + 1, min, max, should_continue))
			return TRUE;
		if(op2 && acc > op2 && check_with_sum_limit(mats, acc - op2, index, count + 1, min, max, should_continue))
			return TRUE;
	}
	if(count + 1 < min && should_continue)
		*should_continue = FALSE;
	return FALSE;
}
int32_t field::check_with_sum_limit_m(const card_vector& mats, int32_t acc, int32_t index, int32_t min, int32_t max, int32_t must_count, int32_t* should_continue) {
	if(acc == 0)
		return index == must_count && 0 >= min && 0 <= max;
	if(index == must_count)
		return check_with_sum_limit(mats, acc, index, 1, min, max, should_continue);
	if(index >= (int32_t)mats.size())
		return FALSE;
	int32_t op1 = mats[index]->sum_param & 0xffff;
	int32_t op2 = (mats[index]->sum_param >> 16) & 0xffff;
	if(acc >= op1 && check_with_sum_limit_m(mats, acc - op1, index + 1, min, max, must_count, should_continue))
		return TRUE;
	if(op2 && acc >= op2 && check_with_sum_limit_m(mats, acc - op2, index + 1, min, max, must_count, should_continue))
		return TRUE;
	return FALSE;
}
int32_t field::check_with_sum_greater_limit(const card_vector& mats, int32_t acc, int32_t index, int32_t opmin, int32_t* should_continue) {
	while(index < (int32_t)mats.size()) {
		int32_t op1 = mats[index]->sum_param & 0xffff;
		int32_t op2 = (mats[index]->sum_param >> 16) & 0xffff;
		if((acc <= op1 && acc + opmin > op1) || (op2 && acc <= op2 && acc + opmin > op2))
			return TRUE;
		++index;
		if(check_with_sum_greater_limit(mats, acc - op1, index, std::min(opmin, op1), should_continue))
			return TRUE;
		if(op2 && check_with_sum_greater_limit(mats, acc - op2, index, std::min(opmin, op2), should_continue))
			return TRUE;
	}
	return FALSE;
}
int32_t field::check_with_sum_greater_limit_m(const card_vector& mats, int32_t acc, int32_t index, int32_t opmin, int32_t must_count, int32_t* should_continue) {
	if(acc <= 0)
		return index == must_count && acc + opmin > 0;
	if(index == must_count)
		return check_with_sum_greater_limit(mats, acc, index, opmin, should_continue);
	if(index >= (int32_t)mats.size())
		return FALSE;
	int32_t op1 = mats[index]->sum_param & 0xffff;
	int32_t op2 = (mats[index]->sum_param >> 16) & 0xffff;
	if(check_with_sum_greater_limit_m(mats, acc - op1, index + 1, std::min(opmin, op1), must_count, should_continue))
		return TRUE;
	if(op2 && check_with_sum_greater_limit_m(mats, acc - op2, index + 1, std::min(opmin, op2), must_count, should_continue))
		return TRUE;
	if(should_continue && index < must_count && (acc - op1 <= 0 && (!op2 || acc - op2 <= 0)))
		*should_continue = FALSE;
	return FALSE;
}
int32_t field::is_player_can_draw(uint8_t playerid) {
	return !is_player_affected_by_effect(playerid, EFFECT_CANNOT_DRAW);
}
int32_t field::is_player_can_discard_deck(uint8_t playerid, int32_t count) {
	if(player[playerid].list_main.size() < (uint32_t)count)
		return FALSE;
	return !is_player_affected_by_effect(playerid, EFFECT_CANNOT_DISCARD_DECK);
}
int32_t field::is_player_can_discard_deck_as_cost(uint8_t playerid, int32_t count) {
	if(player[playerid].list_main.size() < (uint32_t)count)
		return FALSE;
	if(is_player_affected_by_effect(playerid, EFFECT_CANNOT_DISCARD_DECK))
		return FALSE;
	card* topcard = player[playerid].list_main.back();
	if((count == 1) && topcard->is_position(POS_FACEUP))
		return topcard->is_capable_cost_to_grave(playerid);
	bool cant_remove_s = !is_player_can_action(playerid, EFFECT_CANNOT_REMOVE);
	bool cant_remove_o = !is_player_can_action(1 - playerid, EFFECT_CANNOT_REMOVE);
	effect_set eset;
	filter_field_effect(EFFECT_TO_GRAVE_REDIRECT, &eset);
	for(const auto& peff : eset) {
		uint32_t redirect = peff->get_value();
		uint8_t p = peff->get_handler_player();
		if(redirect & LOCATION_REMOVED) {
			if(topcard->is_affected_by_effect(EFFECT_CANNOT_REMOVE))
				continue;
			if((cant_remove_s && (p == playerid)) || (cant_remove_o && (p != playerid)))
				continue;
		}
		if((p == playerid && peff->s_range & LOCATION_DECK) || (p != playerid && peff->o_range & LOCATION_DECK))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_discard_hand(uint8_t playerid, card* pcard, effect* peffect, uint32_t reason) {
	if(pcard->current.location != LOCATION_HAND)
		return FALSE;
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_DISCARD_HAND, &eset);
	for(const auto& peff : eset) {
		if(!peff->target)
			return FALSE;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peffect);
		pduel->lua->add_param<PARAM_TYPE_INT>(reason);
		if (pduel->lua->check_condition(peff->target, 4))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_action(uint8_t playerid, uint32_t actionlimit) {
	effect_set eset;
	filter_player_effect(playerid, actionlimit, &eset);
	for(const auto& peff : eset) {
		if(!peff->target)
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_summon(uint32_t sumtype, uint8_t playerid, card* pcard, uint8_t toplayer) {
	effect_set eset;
	sumtype |= SUMMON_TYPE_NORMAL;
	filter_player_effect(playerid, EFFECT_CANNOT_SUMMON, &eset);
	for(const auto& peff : eset) {
		if(!peff->target)
			return FALSE;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		pduel->lua->add_param<PARAM_TYPE_INT>(sumtype);
		pduel->lua->add_param<PARAM_TYPE_INT>(POS_FACEUP);
		pduel->lua->add_param<PARAM_TYPE_INT>(toplayer);
		if(pduel->lua->check_condition(peff->target, 6))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_mset(uint32_t sumtype, uint8_t playerid, card* pcard, uint8_t toplayer) {
	effect_set eset;
	sumtype |= SUMMON_TYPE_NORMAL;
	filter_player_effect(playerid, EFFECT_CANNOT_MSET, &eset);
	for(const auto& peff : eset) {
		if(!peff->target)
			return FALSE;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		pduel->lua->add_param<PARAM_TYPE_INT>(sumtype);
		pduel->lua->add_param<PARAM_TYPE_INT>(toplayer);
		pduel->lua->add_param<PARAM_TYPE_INT>(POS_FACEDOWN);
		if (pduel->lua->check_condition(peff->target, 6))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_sset(uint8_t playerid, card* pcard) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_SSET, &eset);
	for(const auto& peff : eset) {
		if(!peff->target)
			return FALSE;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		if (pduel->lua->check_condition(peff->target, 3))
			return FALSE;
	}
	return TRUE;
}
// check player-effect EFFECT_CANNOT_SPECIAL_SUMMON without target
int32_t field::is_player_can_spsummon(uint8_t playerid) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_SPECIAL_SUMMON, &eset);
	for(auto& eff : eset) {
		if(!eff->target)
			return FALSE;
	}
	return is_player_can_spsummon_count(playerid, 1);
}
int32_t field::is_player_can_spsummon(effect* peffect, uint32_t sumtype, uint8_t sumpos, uint8_t playerid, uint8_t toplayer, card* pcard) {
	if(pcard->is_affected_by_effect(EFFECT_CANNOT_SPECIAL_SUMMON))
		return FALSE;
	if(pcard->is_status(STATUS_FORBIDDEN))
		return FALSE;
	if((pcard->data.type & TYPE_TOKEN) && (pcard->current.location & LOCATION_ONFIELD))
		return FALSE;
	if((pcard->data.type & TYPE_LINK) && (pcard->data.type & TYPE_MONSTER))
		sumpos &= POS_FACEUP_ATTACK;
	if(sumpos == 0)
		return FALSE;
	sumtype |= SUMMON_TYPE_SPECIAL;
	save_lp_cost();
	if(!pcard->check_cost_condition(EFFECT_SPSUMMON_COST, playerid, sumtype)) {
		restore_lp_cost();
		return FALSE;
	}
	restore_lp_cost();
	if(sumpos & POS_FACEDOWN && is_player_affected_by_effect(playerid, EFFECT_DEVINE_LIGHT))
		sumpos = (sumpos & POS_FACEUP) | ((sumpos & POS_FACEDOWN) >> 1);
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_SPECIAL_SUMMON, &eset);
	for(auto& eff : eset) {
		if(!eff->target)
			return FALSE;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(eff);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		pduel->lua->add_param<PARAM_TYPE_INT>(sumtype);
		pduel->lua->add_param<PARAM_TYPE_INT>(sumpos);
		pduel->lua->add_param<PARAM_TYPE_INT>(toplayer);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peffect);
		if(pduel->lua->check_condition(eff->target, 7))
			return FALSE;
	}
	eset.clear();
	filter_player_effect(playerid, EFFECT_FORCE_SPSUMMON_POSITION, &eset);
	for(auto& eff : eset) {
		if(eff->target) {
			pduel->lua->add_param<PARAM_TYPE_EFFECT>(eff);
			pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
			pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
			pduel->lua->add_param<PARAM_TYPE_INT>(sumtype);
			pduel->lua->add_param<PARAM_TYPE_INT>(sumpos);
			pduel->lua->add_param<PARAM_TYPE_INT>(toplayer);
			pduel->lua->add_param<PARAM_TYPE_EFFECT>(peffect);
			if(!pduel->lua->check_condition(eff->target, 7))
				continue;
		}
		sumpos &= eff->get_value();
		if(sumpos == 0)
			return FALSE;
	}
	if(!check_spsummon_once(pcard, playerid))
		return FALSE;
	if(!check_spsummon_counter(playerid))
		return FALSE;
	return TRUE;
}
int32_t field::is_player_can_flipsummon(uint8_t playerid, card* pcard) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_FLIP_SUMMON, &eset);
	for(const auto& peff : eset) {
		if(!peff->target)
			return FALSE;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		if (pduel->lua->check_condition(peff->target, 3))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_spsummon_monster(uint8_t playerid, uint8_t toplayer, uint8_t sumpos, uint32_t sumtype, card_data* pdata) {
	temp_card->data = *pdata;
	int32_t result = is_player_can_spsummon(core.reason_effect, sumtype, sumpos, playerid, toplayer, temp_card);
	temp_card->data = {};
	return result;
}
int32_t field::is_player_can_release(uint8_t playerid, card* pcard) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_RELEASE, &eset);
	for(const auto& peff : eset) {
		if(!peff->target)
			return FALSE;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		if (pduel->lua->check_condition(peff->target, 3))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_spsummon_count(uint8_t playerid, uint32_t count) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_LEFT_SPSUMMON_COUNT, &eset);
	for(const auto& peff : eset) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(core.reason_effect);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		int32_t v = peff->get_value(2);
		if(v < (int32_t)count)
			return FALSE;
	}
	return check_spsummon_counter(playerid, count);
}
int32_t field::is_player_can_place_counter(uint8_t playerid, card* pcard, uint16_t countertype, uint16_t count) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_PLACE_COUNTER, &eset);
	for(const auto& peff : eset) {
		if(!peff->target)
			return FALSE;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		pduel->lua->add_param<PARAM_TYPE_INT>(countertype);
		pduel->lua->add_param<PARAM_TYPE_INT>(count);
		if (pduel->lua->check_condition(peff->target, 5))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_remove_counter(uint8_t playerid, card* pcard, uint8_t self, uint8_t oppo, uint16_t countertype, uint16_t count, uint32_t reason) {
	if((pcard && pcard->get_counter(countertype) >= count) || (!pcard && get_field_counter(playerid, self, oppo, countertype) >= count))
		return TRUE;
	auto pr = effects.continuous_effect.equal_range(EFFECT_RCOUNTER_REPLACE + countertype);
	tevent e;
	e.event_cards = 0;
	e.event_player = playerid;
	e.event_value = count;
	e.reason = reason;
	e.reason_effect = core.reason_effect;
	e.reason_player = playerid;
	for(auto eit = pr.first; eit != pr.second;) {
		effect* peffect = eit->second;
		++eit;
		if(peffect->is_activateable(peffect->get_handler_player(), e))
			return TRUE;
	}
	return FALSE;
}
int32_t field::is_player_can_remove_overlay_card(uint8_t playerid, group* pgroup, uint8_t self, uint8_t oppo, uint16_t min, uint32_t reason) {
	if(get_overlay_count(playerid, self, oppo, pgroup) >= min)
		return TRUE;
	auto pr = effects.continuous_effect.equal_range(EFFECT_OVERLAY_REMOVE_REPLACE);
	tevent e;
	e.event_cards = 0;
	e.event_player = playerid;
	e.event_value = min;
	e.reason = reason;
	e.reason_effect = core.reason_effect;
	e.reason_player = playerid;
	for(auto eit = pr.first; eit != pr.second;) {
		effect* peffect = eit->second;
		++eit;
		if(peffect->is_activateable(peffect->get_handler_player(), e))
			return TRUE;
	}
	return FALSE;
}
int32_t field::is_player_can_send_to_grave(uint8_t playerid, card* pcard) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_TO_GRAVE, &eset);
	for(const auto& peff : eset) {
		if(!peff->target)
			return FALSE;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		if (pduel->lua->check_condition(peff->target, 3))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_send_to_hand(uint8_t playerid, card* pcard) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_TO_HAND, &eset);
	for(const auto& peff : eset) {
		if(!peff->target)
			return FALSE;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(core.reason_effect);
		if(pduel->lua->check_condition(peff->target, 4))
			return FALSE;
	}
	if(pcard->is_extra_deck_monster() && !is_player_can_send_to_deck(playerid, pcard))
		return FALSE;
	return TRUE;
}
int32_t field::is_player_can_send_to_deck(uint8_t playerid, card* pcard) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_TO_DECK, &eset);
	for(const auto& peff : eset) {
		if(!peff->target)
			return FALSE;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		if(pduel->lua->check_condition(peff->target, 3))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_remove(uint8_t playerid, card* pcard, uint32_t reason) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_REMOVE, &eset);
	for(const auto& peff : eset) {
		if(!peff->target)
			return FALSE;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		pduel->lua->add_param<PARAM_TYPE_INT>(reason);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(core.reason_effect);
		if(pduel->lua->check_condition(peff->target, 5))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_chain_negatable(uint8_t chaincount) {
	effect_set eset;
	if(chaincount > core.current_chain.size())
		return FALSE;
	effect* peffect;
	if(chaincount == 0)
		peffect = core.current_chain.back().triggering_effect;
	else
		peffect = core.current_chain[chaincount - 1].triggering_effect;
	if(peffect->is_flag(EFFECT_FLAG_CANNOT_INACTIVATE))
		return FALSE;
	filter_field_effect(EFFECT_CANNOT_INACTIVATE, &eset);
	for(const auto& peff : eset) {
		pduel->lua->add_param<PARAM_TYPE_INT>(chaincount);
		if(peff->check_value_condition(1))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_chain_disablable(uint8_t chaincount) {
	effect_set eset;
	if(chaincount > core.current_chain.size())
		return FALSE;
	effect* peffect;
	if(chaincount == 0)
		peffect = core.current_chain.back().triggering_effect;
	else
		peffect = core.current_chain[chaincount - 1].triggering_effect;
	if(!peffect->get_handler()->get_status(STATUS_FORBIDDEN)) {
		if(peffect->is_flag(EFFECT_FLAG_CANNOT_DISABLE))
			return FALSE;
		filter_field_effect(EFFECT_CANNOT_DISEFFECT, &eset);
		for(const auto& peff : eset) {
			pduel->lua->add_param<PARAM_TYPE_INT>(chaincount);
			if(peff->check_value_condition(1))
				return FALSE;
		}
	}
	return TRUE;
}
int32_t field::is_chain_disabled(uint8_t chaincount) {
	if(chaincount > core.current_chain.size())
		return FALSE;
	chain* pchain;
	if(chaincount == 0)
		pchain = &core.current_chain.back();
	else
		pchain = &core.current_chain[chaincount - 1];
	if(pchain->flag & CHAIN_DISABLE_EFFECT)
		return TRUE;
	card* pcard = pchain->triggering_effect->get_handler();
	auto rg = pcard->single_effect.equal_range(EFFECT_DISABLE_CHAIN);
	for(; rg.first != rg.second; ++rg.first) {
		effect* peffect = rg.first->second;
		if(peffect->get_value() == pchain->chain_id) {
			peffect->reset_flag |= RESET_CHAIN;
			return TRUE;
		}
	}
	return FALSE;
}
int32_t field::check_chain_target(uint8_t chaincount, card* pcard) {
	if(chaincount > core.current_chain.size())
		return FALSE;
	chain* pchain;
	if(chaincount == 0)
		pchain = &core.current_chain.back();
	else
		pchain = &core.current_chain[chaincount - 1];
	effect* peffect = pchain->triggering_effect;
	uint8_t tp = pchain->triggering_player;
	if(!peffect->is_flag(EFFECT_FLAG_CARD_TARGET) || !peffect->target)
		return FALSE;
	pduel->lua->add_param<PARAM_TYPE_EFFECT>(peffect);
	pduel->lua->add_param<PARAM_TYPE_INT>(tp);
	pduel->lua->add_param<PARAM_TYPE_GROUP>(pchain->evt.event_cards );
	pduel->lua->add_param<PARAM_TYPE_INT>(pchain->evt.event_player);
	pduel->lua->add_param<PARAM_TYPE_INT>(pchain->evt.event_value);
	pduel->lua->add_param<PARAM_TYPE_EFFECT>(pchain->evt.reason_effect );
	pduel->lua->add_param<PARAM_TYPE_INT>(pchain->evt.reason);
	pduel->lua->add_param<PARAM_TYPE_INT>(pchain->evt.reason_player);
	pduel->lua->add_param<PARAM_TYPE_INT>(0);
	pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
	return pduel->lua->check_condition(peffect->target, 10);
}
chain* field::get_chain(uint8_t chaincount) {
	if(chaincount == 0 && core.continuous_chain.size() && (core.reason_effect != nullptr && core.reason_effect->type & EFFECT_TYPE_CONTINUOUS))
		return &core.continuous_chain.back();
	if(chaincount == 0 || chaincount > core.current_chain.size()) {
		chaincount = (uint8_t)core.current_chain.size();
		if(chaincount == 0)
			return nullptr;
	}
	return &core.current_chain[chaincount - 1];
}
int32_t field::get_cteffect(effect* peffect, int32_t playerid, int32_t store) {
	card* phandler = peffect->get_handler();
	if(phandler->data.type != (TYPE_TRAP | TYPE_CONTINUOUS))
		return FALSE;
	if(!(peffect->type & EFFECT_TYPE_ACTIVATE))
		return FALSE;
	if(peffect->code != EVENT_FREE_CHAIN)
		return FALSE;
	if(peffect->cost || peffect->target || peffect->operation)
		return FALSE;
	if(store) {
		core.select_chains.clear();
		core.select_options.clear();
	}
	for(auto& efit : phandler->field_effect) {
		effect* feffect = efit.second;
		if(!(feffect->type & (EFFECT_TYPE_TRIGGER_F | EFFECT_TYPE_TRIGGER_O | EFFECT_TYPE_QUICK_O)))
			continue;
		if(!feffect->in_range(phandler))
			continue;
		uint32_t code = efit.first;
		if(code == EVENT_FREE_CHAIN || code == EVENT_PHASE + (uint32_t)infos.phase) {
			nil_event.event_code = code;
			if(get_cteffect_evt(feffect, playerid, nil_event, store) && !store)
				return TRUE;
		} else {
			for(const auto& ev : core.point_event) {
				if(code != ev.event_code)
					continue;
				if(get_cteffect_evt(feffect, playerid, ev, store) && !store)
					return TRUE;
			}
			for(const auto& ev : core.instant_event) {
				if(code != ev.event_code)
					continue;
				if(get_cteffect_evt(feffect, playerid, ev, store) && !store)
					return TRUE;
			}
		}
	}
	return (store && !core.select_chains.empty()) ? TRUE : FALSE;
}
int32_t field::get_cteffect_evt(effect* feffect, int32_t playerid, const tevent& e, int32_t store) {
	if(!feffect->is_activateable(playerid, e, FALSE, FALSE, FALSE, FALSE, TRUE))
		return FALSE;
	if(store) {
		core.select_chains.emplace_back();
		chain& newchain = core.select_chains.back();
		newchain.evt = e;
		newchain.triggering_effect = feffect;
		core.select_options.push_back(feffect->description);
	}
	return TRUE;
}
int32_t field::check_cteffect_hint(effect* peffect, uint8_t playerid) {
	card* phandler = peffect->get_handler();
	if(phandler->data.type != (TYPE_TRAP | TYPE_CONTINUOUS))
		return FALSE;
	if(!(peffect->type & EFFECT_TYPE_ACTIVATE))
		return FALSE;
	if(peffect->code != EVENT_FREE_CHAIN)
		return FALSE;
	if(peffect->cost || peffect->target || peffect->operation)
		return FALSE;
	for(auto& efit : phandler->field_effect) {
		effect* feffect = efit.second;
		if(!(feffect->type & (EFFECT_TYPE_TRIGGER_F | EFFECT_TYPE_TRIGGER_O | EFFECT_TYPE_QUICK_O)))
			continue;
		if(!feffect->in_range(phandler))
			continue;
		uint32_t code = efit.first;
		if(code == EVENT_FREE_CHAIN || code == (uint32_t)(EVENT_PHASE | infos.phase)) {
			nil_event.event_code = code;
			if(get_cteffect_evt(feffect, playerid, nil_event, FALSE)
				&& (code != EVENT_FREE_CHAIN || check_hint_timing(feffect)))
				return TRUE;
		} else {
			for(const auto& ev : core.point_event) {
				if(code != ev.event_code)
					continue;
				if(get_cteffect_evt(feffect, playerid, ev, FALSE))
					return TRUE;
			}
			for(const auto& ev : core.instant_event) {
				if(code != ev.event_code)
					continue;
				if(get_cteffect_evt(feffect, playerid, ev, FALSE))
					return TRUE;
			}
		}
	}
	return FALSE;
}
int32_t field::check_nonpublic_trigger(chain& ch) {
	effect* peffect = ch.triggering_effect;
	card* phandler = peffect->get_handler();
	if(!peffect->is_flag(EFFECT_FLAG_FIELD_ONLY)
		&& (((peffect->type & EFFECT_TYPE_SINGLE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)
			&& phandler->is_has_relation(ch) && (ch.triggering_location & (LOCATION_HAND | LOCATION_DECK)))
			|| (peffect->range & (LOCATION_HAND | LOCATION_DECK)))) {
		ch.flag |= CHAIN_HAND_TRIGGER;
		core.new_ochain_h.push_back(ch);
		//TCG segoc
		if(!is_flag(DUEL_TCG_SEGOC_NONPUBLIC) && ((ch.triggering_location == LOCATION_HAND && phandler->is_position(POS_FACEDOWN))
			|| ch.triggering_location == LOCATION_DECK
			|| (peffect->range && !peffect->in_range(ch))))
			return FALSE;
	}
	return TRUE;
}
int32_t field::check_trigger_effect(const chain& ch) const {
	effect* peffect = ch.triggering_effect;
	card* phandler = peffect->get_handler();
	if(pduel->game_field->is_flag(DUEL_TRIGGER_ONLY_IN_LOCATION))
		return phandler->is_has_relation(ch);
	if((peffect->type & EFFECT_TYPE_FIELD) && !phandler->is_has_relation(ch))
		return FALSE;
	if(peffect->code == EVENT_FLIP && infos.phase == PHASE_DAMAGE)
		return TRUE;
	if(pduel->game_field->is_flag(DUEL_TRIGGER_WHEN_PRIVATE_KNOWLEDGE))
		return TRUE;
	if((phandler->current.location & LOCATION_DECK) && !(ch.triggering_location == LOCATION_DECK))
		return FALSE;
	if((ch.triggering_location & (LOCATION_DECK | LOCATION_HAND | LOCATION_EXTRA))
		&& (ch.triggering_position & POS_FACEDOWN))
		return TRUE;
	if(!(phandler->current.location & (LOCATION_DECK | LOCATION_HAND | LOCATION_EXTRA))
		|| phandler->is_position(POS_FACEUP))
		return TRUE;
	return FALSE;
}
int32_t field::check_spself_from_hand_trigger(const chain& ch) const {
	effect* peffect = ch.triggering_effect;
	uint8_t tp = ch.triggering_player;
	if((peffect->status & EFFECT_STATUS_SUMMON_SELF) && (ch.flag & CHAIN_HAND_TRIGGER)) {
		return std::none_of(core.current_chain.begin(), core.current_chain.end(), [tp](const chain& ch) {
			return ch.triggering_player == tp
				&& (ch.triggering_effect->status & EFFECT_STATUS_SUMMON_SELF) && (ch.flag & CHAIN_HAND_TRIGGER);
		});
	}
	return TRUE;
}
int32_t field::is_able_to_enter_bp() {
	return (is_flag(DUEL_ATTACK_FIRST_TURN) || infos.turn_id != 1 || is_player_affected_by_effect(infos.turn_player, EFFECT_BP_FIRST_TURN))
	        && infos.phase < PHASE_BATTLE_START
	        && !is_player_affected_by_effect(infos.turn_player, EFFECT_CANNOT_BP);
}
