/*
 * card.cpp
 *
 *  Created on: 2010-5-7
 *      Author: Argon
 */
#include "card.h"
#include "field.h"
#include "effect.h"
#include "duel.h"
#include "group.h"
#include "interpreter.h"
#include <algorithm>
#include <iterator>

bool card_sort::operator()(const card* c1, const card* c2) const {
	return c1->cardid < c2->cardid;
}
bool card_state::is_location(int32_t loc) const {
	if((loc & LOCATION_FZONE) && location == LOCATION_SZONE && sequence == 5)
		return true;
	if((loc & LOCATION_PZONE) && location == LOCATION_SZONE && pzone)
		return true;
	if(location & loc)
		return true;
	return false;
}
void card_state::set0xff() {
	code = 0xffffffff;
	code2 = 0xffffffff;
	type = 0xffffffff;
	level = 0xffffffff;
	rank = 0xffffffff;
	link = 0xffffffff;
	link_marker = 0xffffffff;
	lscale = 0xffffffff;
	rscale = 0xffffffff;
	attribute = 0xffffffff;
	race = 0xffffffff;
	attack = 0xffffffff;
	defense = 0xffffffff;
	base_attack = 0xffffffff;
	base_defense = 0xffffffff;
	controler = 0xff;
	location = 0xff;
	sequence = 0xffffffff;
	position = 0xffffffff;
	reason = 0xffffffff;
	reason_player = 0xff;
}
bool card::card_operation_sort(card* c1, card* c2) {
	duel* pduel = c1->pduel;
	int32_t cp1 = c1->overlay_target ? c1->overlay_target->current.controler : c1->current.controler;
	int32_t cp2 = c2->overlay_target ? c2->overlay_target->current.controler : c2->current.controler;
	if(cp1 != cp2) {
		if(cp1 == PLAYER_NONE || cp2 == PLAYER_NONE)
			return cp1 < cp2;
		if(pduel->game_field->infos.turn_player == 0)
			return cp1 < cp2;
		else
			return cp1 > cp2;
	}
	if(c1->current.location != c2->current.location)
		return c1->current.location < c2->current.location;
	if(c1->current.location & LOCATION_OVERLAY) {
		if(c1->overlay_target->current.sequence != c2->overlay_target->current.sequence)
			return c1->overlay_target->current.sequence < c2->overlay_target->current.sequence;
		else return c1->current.sequence < c2->current.sequence;
	} else {
		if(c1->current.location & (LOCATION_DECK | LOCATION_EXTRA | LOCATION_GRAVE | LOCATION_REMOVED))
			return c1->current.sequence > c2->current.sequence;
		else
			return c1->current.sequence < c2->current.sequence;
	}
}
void card::attacker_map::addcard(card* pcard) {
	uint32_t fid = pcard ? pcard->fieldid_r : 0;
	auto pr = emplace(fid, std::make_pair(pcard, 0));
	pr.first->second.second++;
}
uint32_t card::attacker_map::findcard(card* pcard) {
	uint32_t fid = pcard ? pcard->fieldid_r : 0;
	auto it = find(fid);
	if(it == end())
		return 0;
	else
		return it->second.second;
}
card::card(duel* pd) : lua_obj_helper(pd) {
	temp.set0xff();
	current.controler = PLAYER_NONE;
}
template<typename T>
void insert_value(std::vector<uint8_t>& vec, const T& _val) {
	T val = _val;
	const auto vec_size = vec.size();
	const auto val_size = sizeof(T);
	vec.resize(vec_size + val_size);
	std::memcpy(&vec[vec_size], &val, val_size);
}

#define CHECK_AND_INSERT_T(query, value, type)if(query_flag & query) {\
insert_value<uint16_t>(pduel->query_buffer, sizeof(uint32_t) + sizeof(type));\
insert_value<uint32_t>(pduel->query_buffer, query);\
insert_value<type>(pduel->query_buffer, value);\
}
#define CHECK_AND_INSERT(query, value)CHECK_AND_INSERT_T(query, value, uint32_t)

void card::get_infos(int32_t query_flag) {
	CHECK_AND_INSERT(QUERY_CODE, data.code);
	CHECK_AND_INSERT(QUERY_POSITION, get_info_location().position);
	CHECK_AND_INSERT(QUERY_ALIAS, get_code());
	CHECK_AND_INSERT(QUERY_TYPE, get_type());
	CHECK_AND_INSERT(QUERY_LEVEL, get_level());
	CHECK_AND_INSERT(QUERY_RANK, get_rank());
	CHECK_AND_INSERT(QUERY_ATTRIBUTE, get_attribute());
	CHECK_AND_INSERT(QUERY_RACE, get_race());
	CHECK_AND_INSERT(QUERY_ATTACK, get_attack());
	CHECK_AND_INSERT(QUERY_DEFENSE, get_defense());
	CHECK_AND_INSERT(QUERY_BASE_ATTACK, get_base_attack());
	CHECK_AND_INSERT(QUERY_BASE_DEFENSE, get_base_defense());
	CHECK_AND_INSERT(QUERY_REASON, current.reason);
	CHECK_AND_INSERT(QUERY_COVER, cover);
	if(query_flag & QUERY_REASON_CARD) {
		insert_value<uint16_t>(pduel->query_buffer, sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint64_t));
		insert_value<uint32_t>(pduel->query_buffer, QUERY_REASON_CARD);
		if(current.reason_card) {
			loc_info info = current.reason_card->get_info_location();
			insert_value<uint8_t>(pduel->query_buffer, info.controler);
			insert_value<uint8_t>(pduel->query_buffer, info.location);
			insert_value<uint32_t>(pduel->query_buffer, info.sequence);
			insert_value<uint32_t>(pduel->query_buffer, info.position);
		} else {
			insert_value<uint16_t>(pduel->query_buffer, 0);
			insert_value<uint64_t>(pduel->query_buffer, 0);
		}
	}
	if(query_flag & QUERY_EQUIP_CARD) {
		insert_value<uint16_t>(pduel->query_buffer, sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint64_t));
		insert_value<uint32_t>(pduel->query_buffer, QUERY_EQUIP_CARD);
		if(equiping_target) {
			loc_info info = equiping_target->get_info_location();
			insert_value<uint8_t>(pduel->query_buffer, info.controler);
			insert_value<uint8_t>(pduel->query_buffer, info.location);
			insert_value<uint32_t>(pduel->query_buffer, info.sequence);
			insert_value<uint32_t>(pduel->query_buffer, info.position);
		} else {
			insert_value<uint16_t>(pduel->query_buffer, 0);
			insert_value<uint64_t>(pduel->query_buffer, 0);
		}
	}
	if(query_flag & QUERY_TARGET_CARD) {
		insert_value<uint16_t>(pduel->query_buffer, sizeof(uint32_t) + sizeof(uint32_t) + static_cast<uint16_t>(effect_target_cards.size()) * (sizeof(uint16_t) + sizeof(uint64_t)));
		insert_value<uint32_t>(pduel->query_buffer, QUERY_TARGET_CARD);
		insert_value<uint32_t>(pduel->query_buffer, effect_target_cards.size());
		for(auto& pcard : effect_target_cards) {
			loc_info info = pcard->get_info_location();
			insert_value<uint8_t>(pduel->query_buffer, info.controler);
			insert_value<uint8_t>(pduel->query_buffer, info.location);
			insert_value<uint32_t>(pduel->query_buffer, info.sequence);
			insert_value<uint32_t>(pduel->query_buffer, info.position);
		}
	}
	if(query_flag & QUERY_OVERLAY_CARD) {
		insert_value<uint16_t>(pduel->query_buffer, sizeof(uint32_t) + sizeof(uint32_t) + static_cast<uint16_t>(xyz_materials.size()) * sizeof(uint32_t));
		insert_value<uint32_t>(pduel->query_buffer, QUERY_OVERLAY_CARD);
		insert_value<uint32_t>(pduel->query_buffer, xyz_materials.size());
		for(auto& xcard : xyz_materials)
			insert_value<uint32_t>(pduel->query_buffer, xcard->data.code);
	}
	if(query_flag & QUERY_COUNTERS) {
		insert_value<uint16_t>(pduel->query_buffer, sizeof(uint32_t) + sizeof(uint32_t) + static_cast<uint16_t>(counters.size()) * sizeof(uint32_t));
		insert_value<uint32_t>(pduel->query_buffer, QUERY_COUNTERS);
		insert_value<uint32_t>(pduel->query_buffer, counters.size());
		for(auto& cmit : counters)
			insert_value<uint32_t>(pduel->query_buffer, cmit.first + ((cmit.second[0] + cmit.second[1]) << 16));
	}
	CHECK_AND_INSERT_T(QUERY_OWNER, owner, uint8_t);
	CHECK_AND_INSERT(QUERY_STATUS, status);
	CHECK_AND_INSERT_T(QUERY_IS_PUBLIC, (is_position(POS_FACEUP) || is_related_to_chains() || (current.location == LOCATION_HAND && is_affected_by_effect(EFFECT_PUBLIC))) ? 1 : 0, uint8_t);
	CHECK_AND_INSERT(QUERY_LSCALE, get_lscale());
	CHECK_AND_INSERT(QUERY_RSCALE, get_rscale());
	if(query_flag & QUERY_LINK) {
		insert_value<uint16_t>(pduel->query_buffer, sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t));
		insert_value<uint32_t>(pduel->query_buffer, QUERY_LINK);
		insert_value<uint32_t>(pduel->query_buffer, get_link());
		insert_value<uint32_t>(pduel->query_buffer, get_link_marker());
	}
	CHECK_AND_INSERT_T(QUERY_IS_HIDDEN, is_affected_by_effect(EFFECT_DARKNESS_HIDE) ? 1 : 0, uint8_t);
	insert_value<uint16_t>(pduel->query_buffer, sizeof(uint32_t));
	insert_value<uint32_t>(pduel->query_buffer, QUERY_END);
}
int32_t card::is_related_to_chains() {
	if(!is_status(STATUS_CHAINING))
		return FALSE;
	for(const auto& chain : pduel->game_field->core.current_chain){
		if(chain.triggering_effect->get_handler() == this && is_has_relation(chain.triggering_effect)) {
			return TRUE;
		}
	}
	return FALSE;
}
#undef CHECK_AND_INSERT_T
#undef CHECK_AND_INSERT
loc_info card::get_info_location() {
	if(overlay_target) {
		return { overlay_target->current.controler, (uint8_t)((overlay_target->current.location | LOCATION_OVERLAY) & 0xff), overlay_target->current.sequence, current.sequence };
	} else {
		return { current.controler, current.location , current.sequence, current.position };
	}
}
// mapping of double-name cards
uint32_t card::second_code(uint32_t code){
	switch(code){
		case CARD_MARINE_DOLPHIN:
			return 17955766u;
		case CARD_TWINKLE_MOSS:
			return 17732278u;
		default:
			return 0;
	}
}
// return: the current card name
// for double-name card, it returns printed name
uint32_t card::get_code() {
	auto search = assume.find(ASSUME_CODE);
	if(search != assume.end())
		return search->second;
	if (temp.code != 0xffffffff)
		return temp.code;
	effect_set effects;
	uint32_t code = data.code;
	temp.code = data.code;
	filter_effect(EFFECT_CHANGE_CODE, &effects);
	if (effects.size())
		code = effects.back()->get_value(this);
	temp.code = 0xffffffff;
	if (code == data.code) {
		effects.clear();
		filter_effect(EFFECT_ADD_CODE, &effects);
		effect* addcode = nullptr;
		for(auto it = effects.rbegin(); it != effects.rend(); it++) {
			if(!(*it)->operation) {
				addcode = *it;
				break;
			}
		}
		if(data.alias && !addcode)
			code = data.alias;
	} else {
		const auto& dat = pduel->read_card(code);
		if (dat.alias && !second_code(code))
			code = dat.alias;
	}
	return code;
}
// return: the current second card name
// for double-name cards, it returns the name in description
uint32_t card::get_another_code() {
	uint32_t code = get_code();
	if(code != data.code){
		return second_code(code);
	}
	effect_set eset;
	filter_effect(EFFECT_ADD_CODE, &eset);
	effect* addcode = nullptr;
	for(auto it = eset.rbegin(); it != eset.rend(); it++) {
		if(!(*it)->operation) {
			addcode = *it;
			break;
		}
	}
	if(!addcode)
		return 0;
	uint32_t otcode = addcode->get_value(this);
	if(get_code() != otcode)
	if(code != otcode)
		return otcode;
	return 0;
}
uint32_t card::get_summon_code(card* scard, uint64_t sumtype, uint8_t playerid) {
	std::set<uint32_t> codes;
	effect_set eset;
	bool changed = false;
	filter_effect(EFFECT_ADD_CODE, &eset, FALSE);
	filter_effect(EFFECT_REMOVE_CODE, &eset, FALSE);
	filter_effect(EFFECT_CHANGE_CODE, &eset);
	for(const auto& peffect : eset) {
		if (!peffect->operation)
			continue;
		pduel->lua->add_param(scard, PARAM_TYPE_CARD);
		pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if (!pduel->lua->check_condition(peffect->operation, 3))
			continue;
		if (peffect->code == EFFECT_ADD_CODE)
			codes.insert(peffect->get_value(this));
		else if (peffect->code == EFFECT_REMOVE_CODE) {
			auto cit = codes.find(peffect->get_value(this));
			if (cit != codes.end())
				codes.erase(cit);
		} else {
			codes.clear();
			codes.insert(peffect->get_value(this));
			changed = true;
		}
	}
	if (!changed) {
		codes.insert(get_code());
		uint32_t code = get_another_code();
		if (code)
			codes.insert(code);
	}
	for(uint32_t code : codes)
		lua_pushinteger(pduel->lua->current_state, code);
	return codes.size();
}
int32_t card::is_set_card(uint32_t set_code) {
	uint32_t code = get_code();
	uint32_t settype = set_code & 0xfff;
	uint32_t setsubtype = set_code & 0xf000;
	for(auto& setcode : (code != data.code) ? pduel->read_card(code).setcodes : data.setcodes) {
		if ((setcode & 0xfffu) == settype && (setcode & 0xf000u & setsubtype) == setsubtype)
			return TRUE;
	}
	//add set code
	effect_set eset;
	filter_effect(EFFECT_ADD_SETCODE, &eset);
	for(const auto& peffect : eset) {
		uint32_t value = peffect->get_value(this);
		if ((value & 0xfff) == settype && (value & 0xf000 & setsubtype) == setsubtype)
			return TRUE;
	}
	//another code
	uint32_t code2 = get_another_code();
	if (code2 == 0)
		return FALSE;
	for(auto& setcode : pduel->read_card(code2).setcodes) {
		if((setcode & 0xfffu) == settype && (setcode & 0xf000u & setsubtype) == setsubtype)
			return TRUE;
	}
	return FALSE;
}
int32_t card::is_origin_set_card(uint32_t set_code) {
	uint32_t settype = set_code & 0xfff;
	uint32_t setsubtype = set_code & 0xf000;
	for (auto& setcode : data.setcodes) {
		if((setcode & 0xfffu) == settype && (setcode & 0xf000u & setsubtype) == setsubtype)
			return TRUE;
	}
	return FALSE;
}
int32_t card::is_pre_set_card(uint32_t set_code) {
	uint32_t code = previous.code;
	uint32_t settype = set_code & 0xfff;
	uint32_t setsubtype = set_code & 0xf000;
	for(auto& setcode : (code != data.code) ? pduel->read_card(code).setcodes : data.setcodes) {
		if ((setcode & 0xfffu) == settype && (setcode & 0xf000u & setsubtype) == setsubtype)
			return TRUE;
	}
	//add set code
	for(auto& setcode : previous.setcodes) {
		if((setcode & 0xfffu) == settype && (setcode & 0xf000u & setsubtype) == setsubtype)
			return TRUE;
	}
	//another code
	if(previous.code2 == 0)
		return FALSE;
	for(auto& setcode : pduel->read_card(previous.code2).setcodes) {
		if((setcode & 0xfffu) == settype && (setcode & 0xf000u & setsubtype) == setsubtype)
			return TRUE;
	}
	return FALSE;
}
int32_t card::is_sumon_set_card(uint32_t set_code, card* scard, uint64_t sumtype, uint8_t playerid) {
	uint32_t settype = set_code & 0xfff;
	uint32_t setsubtype = set_code & 0xf000;
	effect_set eset;
	std::set<uint32_t> codes;
	bool changed = false;
	filter_effect(EFFECT_ADD_CODE, &eset, FALSE);
	filter_effect(EFFECT_REMOVE_CODE, &eset, FALSE);
	filter_effect(EFFECT_CHANGE_CODE, &eset);
	for(const auto& peffect : eset) {
		if (!peffect->operation)
			continue;
		pduel->lua->add_param(scard, PARAM_TYPE_CARD);
		pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if (!pduel->lua->check_condition(peffect->operation, 3))
			continue;
		if (peffect->code== EFFECT_ADD_CODE)
			codes.insert(peffect->get_value(this));
		else if (peffect->code == EFFECT_REMOVE_CODE) {
			auto cit = codes.find(peffect->get_value(this));
			if (cit != codes.end())
				codes.erase(cit);
		} else {
			codes.clear();
			codes.insert(peffect->get_value(this));
			changed = true;
		}
	}
	std::set<uint16_t> setcodes;
	for (uint32_t code : codes) {
		const auto& sets = pduel->read_card(code).setcodes;
		if(sets.size())
			setcodes.insert(sets.begin(), sets.end());
	}
	eset.clear();
	filter_effect(EFFECT_ADD_SETCODE, &eset, FALSE);
	filter_effect(EFFECT_CHANGE_SETCODE, &eset);
	for(const auto& peffect : eset) {
		if (!peffect->operation)
			continue;
		pduel->lua->add_param(scard, PARAM_TYPE_CARD);
		pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if (!pduel->lua->check_condition(peffect->operation, 3))
			continue;
		uint32_t setcode = peffect->get_value(this);
		if (peffect->code == EFFECT_CHANGE_SETCODE) {
			setcodes.clear();
			changed = true;
		}
		setcodes.insert(setcode & 0xffff);
	}
	if (!changed && is_set_card(set_code))
		return TRUE;
	for (uint16_t setcode : setcodes)
		if ((setcode & 0xfffu) == settype && (setcode & 0xf000u & setsubtype) == setsubtype)
			return TRUE;
	return FALSE;
}
uint32_t card::get_set_card() {
	uint32_t count = 0;
	uint32_t code = get_code();
	for(auto& setcode : (code != data.code) ? pduel->read_card(code).setcodes : data.setcodes) {
		count++;
		lua_pushinteger(pduel->lua->current_state, setcode);
	}
	//add set code
	effect_set eset;
	filter_effect(EFFECT_ADD_SETCODE, &eset);
	for(auto& eff : eset) {
		uint32_t value = eff->get_value(this);
		for(; value > 0; count++, value = value >> 16)
			lua_pushinteger(pduel->lua->current_state, value & 0xffff);
	}
	//another code
	uint32_t code2 = get_another_code();
	if (code2 != 0) {
		for(auto& setcode : pduel->read_card(code2).setcodes) {
			count++;
			lua_pushinteger(pduel->lua->current_state, setcode);
		}
	}
	return count;
}
std::set<uint16_t> card::get_origin_set_card() {
	return data.setcodes;
}
uint32_t card::get_pre_set_card() {
	uint32_t count = 0;
	uint32_t code = previous.code;
	for(auto& setcode : (code != data.code) ? pduel->read_card(code).setcodes : data.setcodes) {
		count++;
		lua_pushinteger(pduel->lua->current_state, setcode);
	}
	//add set code
	for(auto& setcode : previous.setcodes) {
		count++;
		lua_pushinteger(pduel->lua->current_state, setcode);
	}
	//another code
	if (previous.code2 != 0) {
		for(auto& setcode : pduel->read_card(previous.code2).setcodes) {
			count++;
			lua_pushinteger(pduel->lua->current_state, setcode);
		}
	}
	return count;
}
uint32_t card::get_summon_set_card(card* scard, uint64_t sumtype, uint8_t playerid) {
	effect_set eset;
	std::set<uint32_t> codes;
	bool changed = false;
	filter_effect(EFFECT_ADD_CODE, &eset, FALSE);
	filter_effect(EFFECT_REMOVE_CODE, &eset, FALSE);
	filter_effect(EFFECT_CHANGE_CODE, &eset);
	for(const auto& peffect : eset) {
		if (!peffect->operation)
			continue;
		pduel->lua->add_param(scard, PARAM_TYPE_CARD);
		pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if (!pduel->lua->check_condition(peffect->operation, 3))
			continue;
		if (peffect->code == EFFECT_ADD_CODE)
			codes.insert(peffect->get_value(this));
		else if (peffect->code == EFFECT_REMOVE_CODE) {
			auto cit = codes.find(peffect->get_value(this));
			if (cit != codes.end())
				codes.erase(cit);
		} else {
			codes.clear();
			codes.insert(peffect->get_value(this));
			changed = true;
		}
	}
	std::set<uint16_t> setcodes;
	for (uint32_t code : codes) {
		const auto& sets = pduel->read_card(code).setcodes;
		if(sets.size())
			setcodes.insert(sets.begin(), sets.end());
	}
	eset.clear();
	filter_effect(EFFECT_ADD_SETCODE, &eset, FALSE);
	filter_effect(EFFECT_CHANGE_SETCODE, &eset);
	for(const auto& peffect : eset) {
		if (!peffect->operation)
			continue;
		pduel->lua->add_param(scard, PARAM_TYPE_CARD);
		pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if (!pduel->lua->check_condition(peffect->operation, 3))
			continue;
		uint32_t setcode = peffect->get_value(this);
		if (peffect->code == EFFECT_CHANGE_SETCODE) {
			setcodes.clear();
			changed = true;
		}
		setcodes.insert(setcode & 0xffff);
	}
	for (uint16_t setcode : setcodes)
		lua_pushinteger(pduel->lua->current_state, setcode);
	uint32_t count = setcodes.size();
	if(!changed)
		count += get_set_card();
	return count;
}
uint32_t card::get_type(card* scard, uint64_t sumtype, uint8_t playerid) {
	auto search = assume.find(ASSUME_TYPE);
	if(search != assume.end())
		return search->second;
	if(!(current.location & (LOCATION_ONFIELD | LOCATION_HAND | LOCATION_GRAVE)))
		return data.type;
	if(current.is_location(LOCATION_PZONE) && !sumtype)
		return TYPE_PENDULUM + TYPE_SPELL;
	if (temp.type != 0xffffffff)
		return temp.type;
	effect_set effects;
	int32_t type = data.type;
	int32_t alttype = 0;
	if(sumtype && current.location == LOCATION_SZONE)
		alttype = data.type;
	bool changed = false;
	temp.type = data.type;
	filter_effect(EFFECT_ADD_TYPE, &effects, FALSE);
	filter_effect(EFFECT_REMOVE_TYPE, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_TYPE, &effects);
	for(const auto& peffect : effects) {
		if (peffect->operation && !sumtype)
			continue;
		if (peffect->operation) {
			pduel->lua->add_param(scard, PARAM_TYPE_CARD);
			pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			if (!pduel->lua->check_condition(peffect->operation, 3))
				continue;
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			if (peffect->code == EFFECT_ADD_TYPE)
				alttype |= peffect->get_value(this,1);
			else if (peffect->code == EFFECT_REMOVE_TYPE)
				alttype &= ~(peffect->get_value(this,1));
			else {
				alttype = peffect->get_value(this, 1);
				changed = true;
			}
		} else {
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			if (peffect->code == EFFECT_ADD_TYPE)
				type |= peffect->get_value(this,1);
			else if (peffect->code == EFFECT_REMOVE_TYPE)
				type &= ~(peffect->get_value(this,1));
			else
				type = peffect->get_value(this,1);
			temp.type = type;
		}
	}
	type |= alttype;
	if (changed)
		type = alttype;
	temp.type = 0xffffffff;
	return type;
}
// Atk and def are sepcial cases since text atk/def ? are involved.
// Asuumption: we can only change the atk/def of cards in LOCATION_MZONE.
int32_t card::get_base_attack() {
	if(!(data.type & TYPE_MONSTER) && !(get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_PRE_MONSTER))
		return 0;
	if (current.location != LOCATION_MZONE || get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP))
		return data.attack;
	if (temp.base_attack != -1)
		return temp.base_attack;
	int32_t batk = data.attack;
	if(batk < 0)
		batk = 0;
	int32_t bdef = data.defense;
	if(bdef < 0)
		bdef = 0;
	temp.base_attack = batk;
	effect_set eset;
	int32_t swap = 0;
	if(!(data.type & TYPE_LINK)) {
		filter_effect(EFFECT_SWAP_BASE_AD, &eset, FALSE);
		swap = eset.size();
	}
	filter_effect(EFFECT_SET_BASE_ATTACK, &eset, FALSE);
	if(swap)
		filter_effect(EFFECT_SET_BASE_DEFENSE, &eset, FALSE);
	std::sort(eset.begin(), eset.end(), effect_sort_id);
	// calculate continuous effects of this first
	for(effect_set::size_type i = 0; i < eset.size();) {
		if(!(eset[i]->type & EFFECT_TYPE_SINGLE) || eset[i]->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			switch(eset[i]->code) {
			case EFFECT_SET_BASE_ATTACK:
				batk = eset[i]->get_value(this);
				if(batk < 0)
					batk = 0;
				temp.base_attack = batk;
				eset.erase(eset.begin() + i);
				continue;
			case EFFECT_SET_BASE_DEFENSE:
				bdef = eset[i]->get_value(this);
				if(bdef < 0)
					bdef = 0;
				eset.erase(eset.begin() + i);
				continue;
			}
		}
		++i;
	}
	for(const auto& peffect : eset) {
		switch(peffect->code) {
		case EFFECT_SET_BASE_ATTACK:
			batk = peffect->get_value(this);
			if(batk < 0)
				batk = 0;
			break;
		case EFFECT_SET_BASE_DEFENSE:
			bdef = peffect->get_value(this);
			if(bdef < 0)
				bdef = 0;
			break;
		case EFFECT_SWAP_BASE_AD:
			std::swap(batk, bdef);
			break;
		}
		temp.base_attack = batk;
	}
	temp.base_attack = -1;
	return batk;
}
int32_t card::get_attack() {
	auto search = assume.find(ASSUME_ATTACK);
	if(search != assume.end())
		return search->second;
	if(!(data.type & TYPE_MONSTER) && !(get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_PRE_MONSTER))
		return 0;
	if (current.location != LOCATION_MZONE || get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP))
		return data.attack;
	if (temp.attack != -1)
		return temp.attack;
	int32_t batk = data.attack;
	if(batk < 0)
		batk = 0;
	int32_t bdef = data.defense;
	if(bdef < 0)
		bdef = 0;
	temp.base_attack = batk;
	temp.attack = batk;
	int32_t atk = -1;
	int32_t up_atk = 0, upc_atk = 0;
	int32_t swap_final = FALSE;
	effect_set eset;
	filter_effect(EFFECT_UPDATE_ATTACK, &eset, FALSE);
	filter_effect(EFFECT_SET_ATTACK, &eset, FALSE);
	filter_effect(EFFECT_SET_ATTACK_FINAL, &eset, FALSE);
	filter_effect(EFFECT_SWAP_ATTACK_FINAL, &eset, FALSE);
	filter_effect(EFFECT_SET_BASE_ATTACK, &eset, FALSE);
	if(!(data.type & TYPE_LINK)) {
		filter_effect(EFFECT_SWAP_AD, &eset, FALSE);
		filter_effect(EFFECT_SWAP_BASE_AD, &eset, FALSE);
		filter_effect(EFFECT_SET_BASE_DEFENSE, &eset, FALSE);
	}
	std::sort(eset.begin(), eset.end(), effect_sort_id);
	int32_t rev = FALSE;
	if(is_affected_by_effect(EFFECT_REVERSE_UPDATE))
		rev = TRUE;
	effect_set effects_atk, effects_atk_r;
	for(effect_set::size_type i = 0; i < eset.size();) {
		if(!(eset[i]->type & EFFECT_TYPE_SINGLE) || eset[i]->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			switch(eset[i]->code) {
			case EFFECT_SET_BASE_ATTACK:
				batk = eset[i]->get_value(this);
				if(batk < 0)
					batk = 0;
				temp.base_attack = batk;
				eset.erase(eset.begin() + i);
				continue;
			case EFFECT_SET_BASE_DEFENSE:
				bdef = eset[i]->get_value(this);
				if(bdef < 0)
					bdef = 0;
				eset.erase(eset.begin() + i);
				continue;
			}
		}
		++i;
	}
	temp.attack = batk;
	for(const auto& peffect : eset) {
		switch(peffect->code) {
		case EFFECT_UPDATE_ATTACK:
			if((peffect->type & EFFECT_TYPE_SINGLE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE))
				up_atk += peffect->get_value(this);
			else
				upc_atk += peffect->get_value(this);
			break;
		case EFFECT_SET_ATTACK:
			atk = peffect->get_value(this);
			if(!(peffect->type & EFFECT_TYPE_SINGLE))
				up_atk = 0;
			break;
		case EFFECT_SET_ATTACK_FINAL:
			if((peffect->type & EFFECT_TYPE_SINGLE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
				atk = peffect->get_value(this);
				up_atk = 0;
				upc_atk = 0;
			} else {
				if(!peffect->is_flag(EFFECT_FLAG_DELAY))
					effects_atk.push_back(peffect);
				else
					effects_atk_r.push_back(peffect);
			}
			break;
		case EFFECT_SET_BASE_ATTACK:
			batk = peffect->get_value(this);
			if(batk < 0)
				batk = 0;
			atk = -1;
			break;
		case EFFECT_SWAP_ATTACK_FINAL:
			atk = peffect->get_value(this);
			up_atk = 0;
			upc_atk = 0;
			break;
		case EFFECT_SET_BASE_DEFENSE:
			bdef = peffect->get_value(this);
			if(bdef < 0)
				bdef = 0;
			break;
		case EFFECT_SWAP_AD:
			swap_final = !swap_final;
			break;
		case EFFECT_SWAP_BASE_AD:
			std::swap(batk, bdef);
			break;
		}
		temp.base_attack = batk;
		if(!rev) {
			temp.attack = ((atk < 0) ? batk : atk) + up_atk + upc_atk;
		} else {
			temp.attack = ((atk < 0) ? batk : atk) - up_atk - upc_atk;
		}
		if(temp.attack < 0)
			temp.attack = 0;
	}
	for(const auto& peffect : effects_atk)
		temp.attack = peffect->get_value(this);
	if(temp.defense == -1) {
		if(swap_final) {
			temp.attack = get_defense();
		}
		for(const auto& peffect : effects_atk_r) {
			temp.attack = peffect->get_value(this);
			if(peffect->is_flag(EFFECT_FLAG_REPEAT))
				temp.attack = peffect->get_value(this);
		}
	}
	atk = temp.attack;
	if(atk < 0)
		atk = 0;
	temp.base_attack = -1;
	temp.attack = -1;
	return atk;
}
int32_t card::get_base_defense() {
	if(data.type & TYPE_LINK)
		return 0;
	if(!(data.type & TYPE_MONSTER) && !(get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_PRE_MONSTER))
		return 0;
	if (current.location != LOCATION_MZONE || get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP))
		return data.defense;
	if (temp.base_defense != -1)
		return temp.base_defense;
	int32_t batk = data.attack;
	if(batk < 0)
		batk = 0;
	int32_t bdef = data.defense;
	if(bdef < 0)
		bdef = 0;
	temp.base_defense = bdef;
	effect_set eset;
	filter_effect(EFFECT_SWAP_BASE_AD, &eset, FALSE);
	int32_t swap = eset.size();
	filter_effect(EFFECT_SET_BASE_DEFENSE, &eset, FALSE);
	if(swap)
		filter_effect(EFFECT_SET_BASE_ATTACK, &eset, FALSE);
	std::sort(eset.begin(), eset.end(), effect_sort_id);
	for(effect_set::size_type i = 0; i < eset.size();) {
		if(!(eset[i]->type & EFFECT_TYPE_SINGLE) || eset[i]->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			switch(eset[i]->code) {
			case EFFECT_SET_BASE_ATTACK:
				batk = eset[i]->get_value(this);
				if(batk < 0)
					batk = 0;
				eset.erase(eset.begin() + i);
				continue;
			case EFFECT_SET_BASE_DEFENSE:
				bdef = eset[i]->get_value(this);
				if(bdef < 0)
					bdef = 0;
				temp.base_defense = bdef;
				eset.erase(eset.begin() + i);
				continue;
			}
		}
		++i;
	}
	for(const auto& peffect : eset) {
		switch(peffect->code) {
		case EFFECT_SET_BASE_ATTACK:
			batk = peffect->get_value(this);
			if(batk < 0)
				batk = 0;
			break;
		case EFFECT_SET_BASE_DEFENSE:
			bdef = peffect->get_value(this);
			if(bdef < 0)
				bdef = 0;
			break;
		case EFFECT_SWAP_BASE_AD:
			std::swap(batk, bdef);
			break;
		}
		temp.base_defense = bdef;
	}
	temp.base_defense = -1;
	return bdef;
}
int32_t card::get_defense() {
	if(data.type & TYPE_LINK)
		return 0;
	auto search = assume.find(ASSUME_DEFENSE);
	if(search != assume.end())
		return search->second;
	if(!(data.type & TYPE_MONSTER) && !(get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_PRE_MONSTER))
		return 0;
	if (current.location != LOCATION_MZONE || get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP))
		return data.defense;
	if (temp.defense != -1)
		return temp.defense;
	int32_t batk = data.attack;
	if(batk < 0)
		batk = 0;
	int32_t bdef = data.defense;
	if(bdef < 0)
		bdef = 0;
	temp.base_defense = bdef;
	temp.defense = bdef;
	int32_t def = -1;
	int32_t up_def = 0, upc_def = 0;
	int32_t swap_final = FALSE;
	effect_set eset;
	filter_effect(EFFECT_SWAP_AD, &eset, FALSE);
	filter_effect(EFFECT_UPDATE_DEFENSE, &eset, FALSE);
	filter_effect(EFFECT_SET_DEFENSE, &eset, FALSE);
	filter_effect(EFFECT_SET_DEFENSE_FINAL, &eset, FALSE);
	filter_effect(EFFECT_SWAP_DEFENSE_FINAL, &eset, FALSE);
	filter_effect(EFFECT_SWAP_BASE_AD, &eset, FALSE);
	filter_effect(EFFECT_SET_BASE_ATTACK, &eset, FALSE);
	filter_effect(EFFECT_SET_BASE_DEFENSE, &eset, FALSE);
	std::sort(eset.begin(), eset.end(), effect_sort_id);
	int32_t rev = FALSE;
	if(is_affected_by_effect(EFFECT_REVERSE_UPDATE))
		rev = TRUE;
	effect_set effects_def, effects_def_r;
	for(effect_set::size_type i = 0; i < eset.size();) {
		if(!(eset[i]->type & EFFECT_TYPE_SINGLE) || eset[i]->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			switch(eset[i]->code) {
			case EFFECT_SET_BASE_ATTACK:
				batk = eset[i]->get_value(this);
				if(batk < 0)
					batk = 0;
				eset.erase(eset.begin() + i);
				continue;
			case EFFECT_SET_BASE_DEFENSE:
				bdef = eset[i]->get_value(this);
				if(bdef < 0)
					bdef = 0;
				temp.base_defense = bdef;
				eset.erase(eset.begin() + i);
				continue;
			}
		}
		++i;
	}
	temp.defense = bdef;
	for(const auto& peffect : eset) {
		switch(peffect->code) {
		case EFFECT_UPDATE_DEFENSE:
			if((peffect->type & EFFECT_TYPE_SINGLE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE))
				up_def += peffect->get_value(this);
			else
				upc_def += peffect->get_value(this);
			break;
		case EFFECT_SET_DEFENSE:
			def = peffect->get_value(this);
			if(!(peffect->type & EFFECT_TYPE_SINGLE))
				up_def = 0;
			break;
		case EFFECT_SET_DEFENSE_FINAL:
			if((peffect->type & EFFECT_TYPE_SINGLE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
				def = peffect->get_value(this);
				up_def = 0;
				upc_def = 0;
			} else {
				if(!peffect->is_flag(EFFECT_FLAG_DELAY))
					effects_def.push_back(peffect);
				else
					effects_def_r.push_back(peffect);
			}
			break;
		case EFFECT_SET_BASE_DEFENSE:
			bdef = peffect->get_value(this);
			if(bdef < 0)
				bdef = 0;
			def = -1;
			break;
		case EFFECT_SWAP_DEFENSE_FINAL:
			def = peffect->get_value(this);
			up_def = 0;
			upc_def = 0;
			break;
		case EFFECT_SET_BASE_ATTACK:
			batk = peffect->get_value(this);
			if(batk < 0)
				batk = 0;
			break;
		case EFFECT_SWAP_AD:
			swap_final = !swap_final;
			break;
		case EFFECT_SWAP_BASE_AD:
			std::swap(batk, bdef);
			break;
		}
		temp.base_defense = bdef;
		if(!rev) {
			temp.defense = ((def < 0) ? bdef : def) + up_def + upc_def;
		} else {
			temp.defense = ((def < 0) ? bdef : def) - up_def - upc_def;
		}
		if(temp.defense < 0)
			temp.defense = 0;
	}
	for(const auto& peffect : effects_def)
		temp.defense = peffect->get_value(this);
	if(temp.attack == -1) {
		if(swap_final) {
			temp.defense = get_attack();
		}
		for(const auto& peffect : effects_def_r) {
			temp.defense = peffect->get_value(this);
			if(peffect->is_flag(EFFECT_FLAG_REPEAT))
				temp.defense = peffect->get_value(this);
		}
	}
	def = temp.defense;
	if(def < 0)
		def = 0;
	temp.base_defense = -1;
	temp.defense = -1;
	return def;
}
// Level/Attribute/Race is available for:
// 1. cards with original type TYPE_MONSTER or
// 2. cards with current type TYPE_MONSTER or
// 3. cards with EFFECT_PRE_MONSTER
uint32_t card::get_level() {
	if(((data.type & TYPE_XYZ) && !(is_affected_by_effect(EFFECT_RANK_LEVEL) || is_affected_by_effect(EFFECT_RANK_LEVEL_S)))
		|| (data.type & TYPE_LINK) || (status & STATUS_NO_LEVEL)
		|| (!(data.type & TYPE_MONSTER) && !(get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_PRE_MONSTER)))
		return 0;
	auto search = assume.find(ASSUME_LEVEL);
	if(search != assume.end())
		return search->second;
	if (temp.level != 0xffffffff)
		return temp.level;
	effect_set effects;
	int32_t level = data.level;
	temp.level = level;
	int32_t up = 0, upc = 0;
	if (is_affected_by_effect(EFFECT_RANK_LEVEL_S)|| is_affected_by_effect(EFFECT_LEVEL_RANK_S)) {
		filter_effect(EFFECT_UPDATE_RANK, &effects, FALSE);
		filter_effect(EFFECT_UPDATE_LEVEL, &effects, FALSE);
		filter_effect(EFFECT_CHANGE_RANK, &effects, FALSE);
		filter_effect(EFFECT_CHANGE_LEVEL, &effects, FALSE);
		filter_effect(EFFECT_CHANGE_RANK_FINAL, &effects, FALSE);
		filter_effect(EFFECT_CHANGE_LEVEL_FINAL, &effects);
	}
	else {
		filter_effect(EFFECT_UPDATE_LEVEL, &effects, FALSE);
		filter_effect(EFFECT_CHANGE_LEVEL, &effects, FALSE);
		filter_effect(EFFECT_CHANGE_LEVEL_FINAL, &effects);
	}
	for(const auto& peffect : effects) {
		switch (peffect->code) {
		case EFFECT_UPDATE_RANK:
		case EFFECT_UPDATE_LEVEL:
			if ((peffect->type & EFFECT_TYPE_SINGLE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE))
				up += peffect->get_value(this);
			else
				upc += peffect->get_value(this);
			break;
		case EFFECT_CHANGE_RANK:
		case EFFECT_CHANGE_LEVEL:
			level  = peffect->get_value(this);
			up = 0;
			break;
		case EFFECT_CHANGE_RANK_FINAL:
		case EFFECT_CHANGE_LEVEL_FINAL:
			level = peffect->get_value(this);
			up = 0;
			upc = 0;
			break;
		}
		temp.level = level + up + upc;
	}
	level += up + upc;
	if(level < 1 && (get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_ALLOW_NEGATIVE))
		level = 1;
	temp.level = 0xffffffff;
	return level;
}
uint32_t card::get_rank() {
	if(((!(data.type & TYPE_XYZ) || (status & STATUS_NO_LEVEL)) && !(is_affected_by_effect(EFFECT_LEVEL_RANK) || is_affected_by_effect(EFFECT_LEVEL_RANK_S))) 
	|| (data.type & TYPE_LINK))
		return 0;
	auto search = assume.find(ASSUME_RANK);
	if(search != assume.end())
		return search->second;
	if(!(current.location & LOCATION_MZONE))
		return data.level;
	if (temp.level != 0xffffffff)
		return temp.level;
	effect_set effects;
	int32_t rank = data.level;
	temp.level = rank;
	int32_t up = 0, upc = 0;
	if (is_affected_by_effect(EFFECT_RANK_LEVEL_S) || is_affected_by_effect(EFFECT_LEVEL_RANK_S)) {
		filter_effect(EFFECT_UPDATE_RANK, &effects, FALSE);
		filter_effect(EFFECT_UPDATE_LEVEL, &effects, FALSE);
		filter_effect(EFFECT_CHANGE_RANK, &effects, FALSE);
		filter_effect(EFFECT_CHANGE_LEVEL, &effects, FALSE);
		filter_effect(EFFECT_CHANGE_RANK_FINAL, &effects, FALSE);
		filter_effect(EFFECT_CHANGE_LEVEL_FINAL, &effects);
	}
	else {
		filter_effect(EFFECT_UPDATE_RANK, &effects, FALSE);
		filter_effect(EFFECT_CHANGE_RANK, &effects, FALSE);
		filter_effect(EFFECT_CHANGE_RANK_FINAL, &effects);
	}
	for(const auto& peffect : effects) {
		switch (peffect->code) {
		case EFFECT_UPDATE_RANK:
		case EFFECT_UPDATE_LEVEL:
			if ((peffect->type & EFFECT_TYPE_SINGLE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE))
				up += peffect->get_value(this);
			else
				upc += peffect->get_value(this);
			break;
		case EFFECT_CHANGE_RANK:
		case EFFECT_CHANGE_LEVEL:
			rank = peffect->get_value(this);
			up = 0;
			break;
		case EFFECT_CHANGE_RANK_FINAL:
		case EFFECT_CHANGE_LEVEL_FINAL:
			rank = peffect->get_value(this);
			up = 0;
			upc = 0;
			break;
		}
		temp.level = rank + up + upc;
	}
	rank += up + upc;
	if(rank < 1 && (get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_ALLOW_NEGATIVE))
		rank = 1;
	temp.level = 0xffffffff;
	return rank;
}
uint32_t card::get_link() {
	if(!(data.type & TYPE_LINK) || (status & STATUS_NO_LEVEL))
		return 0;
	auto search = assume.find(ASSUME_LINK);
	if(search != assume.end())
		return search->second;
	if(!(current.location & LOCATION_MZONE))
		return data.level;
	if (temp.level != 0xffffffff)
		return temp.level;
	effect_set effects;
	int32_t link = data.level;
	temp.level = link;
	int32_t up = 0, upc = 0;
	filter_effect(EFFECT_UPDATE_LINK, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_LINK, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_LINK_FINAL, &effects);
	for(const auto& peffect : effects) {
		switch (peffect->code) {
		case EFFECT_UPDATE_LINK:
			if ((peffect->type & EFFECT_TYPE_SINGLE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE))
				up += peffect->get_value(this);
			else
				upc += peffect->get_value(this);
			break;
			case EFFECT_CHANGE_LINK:
			link = peffect->get_value(this);
			up = 0;
			break;
			case EFFECT_CHANGE_LINK_FINAL:
			link = peffect->get_value(this);
			up = 0;
			upc = 0;
			break;
		}
		temp.level = link + up + upc;
	}
	link += up + upc;
	if(link < 1 && (get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_ALLOW_NEGATIVE))
		link = 1;
	temp.level = 0xffffffff;
	return link;		
}
uint32_t card::get_synchro_level(card* pcard) {
	if(((data.type & TYPE_XYZ) || ((status & STATUS_NO_LEVEL) && !(is_affected_by_effect(EFFECT_RANK_LEVEL) || is_affected_by_effect(EFFECT_RANK_LEVEL_S))))
	|| (data.type & TYPE_LINK))
		return 0;
	uint32_t lev;
	effect_set eset;
	filter_effect(EFFECT_SYNCHRO_LEVEL, &eset);
	if(eset.size())
		lev = eset[0]->get_value(pcard);
	else
		lev = get_level();
	return lev;
}
uint32_t card::get_ritual_level(card* pcard) {
	if(((data.type & TYPE_XYZ) || ((status & STATUS_NO_LEVEL) && !(is_affected_by_effect(EFFECT_RANK_LEVEL) || is_affected_by_effect(EFFECT_RANK_LEVEL_S))))
	|| (data.type & TYPE_LINK))
		return 0;
	uint32_t lev;
	effect_set eset;
	filter_effect(EFFECT_RITUAL_LEVEL, &eset);
	if(eset.size())
		lev = eset[0]->get_value(pcard);
	else
		lev = get_level();
	return lev;
}
uint32_t card::check_xyz_level(card* pcard, uint32_t lv) {
	if(status & STATUS_NO_LEVEL)
		return FALSE;
	effect_set eset;
	filter_effect(EFFECT_XYZ_LEVEL, &eset);
	for(auto& eff : eset) {
		std::vector<int32_t> res;
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		eff->get_value(2, &res);
		if(res.size() == 1) {
			uint32_t lev = res[0];
			if((lev & 0xfff) == lv || ((lev >> 16) & 0xfff) == lv)
				return TRUE;
		} else {
			if(std::find(res.begin(), res.end(), (int32_t)lv) != res.end())
				return TRUE;
		}
	}
	return get_level() == lv;
}
// see get_level()
uint32_t card::get_attribute(card* scard, uint64_t sumtype, uint8_t playerid) {
	auto search = assume.find(ASSUME_ATTRIBUTE);
	if(search != assume.end())
		return search->second;
	if(!(data.type & TYPE_MONSTER) && !(get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_PRE_MONSTER))
		return 0;
	if (temp.attribute != 0xffffffff)
		return temp.attribute;
	effect_set effects;
	int32_t attribute = data.attribute, altattribute = 0;
	bool changed = false;
	temp.attribute = data.attribute;
	filter_effect(EFFECT_ADD_ATTRIBUTE, &effects, FALSE);
	filter_effect(EFFECT_REMOVE_ATTRIBUTE, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_ATTRIBUTE, &effects);
	for(const auto& peffect : effects) {
		if (peffect->operation && !sumtype)
			continue;
		if (peffect->operation) {
			pduel->lua->add_param(scard, PARAM_TYPE_CARD);
			pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			if (!pduel->lua->check_condition(peffect->operation, 3))
				continue;
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			if (peffect->code == EFFECT_ADD_ATTRIBUTE)
				altattribute |= peffect->get_value(this, 1);
			else if (peffect->code == EFFECT_REMOVE_ATTRIBUTE)
				altattribute &= ~(peffect->get_value(this, 1));
			else if (peffect->code == EFFECT_CHANGE_ATTRIBUTE) {
				altattribute = peffect->get_value(this, 1);
				changed = true;
			}
		} else {
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			if (peffect->code == EFFECT_ADD_ATTRIBUTE)
				attribute |= peffect->get_value(this, 1);
			else if (peffect->code == EFFECT_REMOVE_ATTRIBUTE)
				attribute &= ~(peffect->get_value(this, 1));
			else if (peffect->code == EFFECT_CHANGE_ATTRIBUTE)
				attribute = peffect->get_value(this, 1);
			temp.attribute = attribute;
		}
	}
	attribute |= altattribute;
	if (changed)
		attribute = altattribute;
	temp.attribute = 0xffffffff;
	return attribute;
}
// see get_level()
uint32_t card::get_race(card* scard, uint64_t sumtype, uint8_t playerid) {
	auto search = assume.find(ASSUME_RACE);
	if(search != assume.end())
		return search->second;
	if(!(data.type & TYPE_MONSTER) && !(get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_PRE_MONSTER) && !sumtype)
		return 0;
	if (temp.race != 0xffffffff)
		return temp.race;
	effect_set effects;
	int32_t race = data.race, altrace = 0;
	bool changed = false;
	temp.race = data.race;
	filter_effect(EFFECT_ADD_RACE, &effects, FALSE);
	filter_effect(EFFECT_REMOVE_RACE, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_RACE, &effects);
	for(const auto& peffect : effects) {
		if (peffect->operation && !sumtype)
			continue;
		if (peffect->operation) {
			pduel->lua->add_param(scard, PARAM_TYPE_CARD);
			pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			if (!pduel->lua->check_condition(peffect->operation, 3))
				continue;
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			if (peffect->code == EFFECT_ADD_RACE)
				altrace |= peffect->get_value(this,1);
			else if (peffect->code == EFFECT_REMOVE_RACE)
				altrace &= ~(peffect->get_value(this,1));
			else if (peffect->code == EFFECT_CHANGE_RACE) {
				altrace = peffect->get_value(this, 1);
				changed = true;
			}
		} else {
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			if (peffect->code == EFFECT_ADD_RACE)
				race |= peffect->get_value(this,1);
			else if (peffect->code == EFFECT_REMOVE_RACE)
				race &= ~(peffect->get_value(this, 1));
			else if (peffect->code == EFFECT_CHANGE_RACE)
				race = peffect->get_value(this, 1);
			temp.race = race;
		}
	}
	race |= altrace;
	if (changed)
		race = altrace;
	temp.race = 0xffffffff;
	return race;
}
uint32_t card::get_lscale() {
	if(!current.is_location(LOCATION_PZONE))
		return data.lscale;
	if (temp.lscale != 0xffffffff)
		return temp.lscale;
	effect_set effects;
	int32_t lscale = data.lscale;
	temp.lscale = data.lscale;
	int32_t up = 0, upc = 0;
	filter_effect(EFFECT_UPDATE_LSCALE, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_LSCALE, &effects);
	for(const auto& peffect : effects) {
		if (peffect->code == EFFECT_UPDATE_LSCALE) {
			if ((peffect->type & EFFECT_TYPE_SINGLE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE))
				up += peffect->get_value(this);
			else
				upc += peffect->get_value(this);
		} else {
			lscale = peffect->get_value(this);
			up = 0;
		}
		temp.lscale = lscale;
	}
	lscale += up + upc;
	temp.lscale = 0xffffffff;
	return lscale;
}
uint32_t card::get_rscale() {
	if(!current.is_location(LOCATION_PZONE))
		return data.rscale;
	if (temp.rscale != 0xffffffff)
		return temp.rscale;
	effect_set effects;
	int32_t rscale = data.rscale;
	temp.rscale = data.rscale;
	int32_t up = 0, upc = 0;
	filter_effect(EFFECT_UPDATE_RSCALE, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_RSCALE, &effects);
	for(const auto& peffect : effects) {
		if (peffect->code == EFFECT_UPDATE_RSCALE) {
			if ((peffect->type & EFFECT_TYPE_SINGLE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE))
				up += peffect->get_value(this);
			else
				upc += peffect->get_value(this);
		} else {
			rscale = peffect->get_value(this);
			up = 0;
		}
		temp.rscale = rscale;
	}
	rscale += up + upc;
	temp.rscale = 0xffffffff;
	return rscale;
}
uint32_t card::get_link_marker() {
	auto rotate = [](uint32_t& marker) {
		marker=(((marker & LINK_MARKER_BOTTOM_LEFT) ? LINK_MARKER_BOTTOM_RIGHT : 0) |
			((marker & LINK_MARKER_BOTTOM) ? LINK_MARKER_RIGHT : 0) |
			((marker & LINK_MARKER_BOTTOM_RIGHT) ? LINK_MARKER_TOP_RIGHT : 0) |
			((marker & LINK_MARKER_RIGHT) ? LINK_MARKER_TOP : 0) |
			((marker & LINK_MARKER_TOP_RIGHT) ? LINK_MARKER_TOP_LEFT : 0) |
			((marker & LINK_MARKER_TOP) ? LINK_MARKER_LEFT : 0) |
			((marker & LINK_MARKER_TOP_LEFT) ? LINK_MARKER_BOTTOM_LEFT : 0) |
			((marker & LINK_MARKER_LEFT) ? LINK_MARKER_BOTTOM : 0));
	};
	if((current.position & POS_FACEDOWN) && (current.is_location(LOCATION_ONFIELD)))
		return 0;
	auto search = assume.find(ASSUME_LINKMARKER);
	if(search != assume.end())
		return search->second;
	if(!(get_type() & TYPE_LINK))
		return 0;
	if (temp.link_marker != 0xffffffff)
		return temp.link_marker;
	effect_set effects;
	uint32_t link_marker = data.link_marker;
	temp.link_marker = data.link_marker;
	filter_effect(EFFECT_ADD_LINKMARKER, &effects, FALSE);
	filter_effect(EFFECT_REMOVE_LINKMARKER, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_LINKMARKER, &effects);
	for(const auto& peffect : effects) {
		if (peffect->code == EFFECT_ADD_LINKMARKER)
			link_marker |= peffect->get_value(this);
		else if (peffect->code == EFFECT_REMOVE_LINKMARKER)
			link_marker &= ~(peffect->get_value(this));
		else if (peffect->code == EFFECT_CHANGE_LINKMARKER)
			link_marker = peffect->get_value(this);
		temp.link_marker = link_marker;
	}
	temp.link_marker = 0xffffffff;
	if((current.position & POS_ATTACK) == 0 && current.is_location(LOCATION_ONFIELD))
		rotate(link_marker);
	return link_marker;
}
int32_t card::is_link_marker(uint32_t dir, uint32_t marker) {
	if(marker)
		return (int32_t)(marker & dir);
	else
		return (int32_t)(get_link_marker() & dir);
}
uint32_t card::get_linked_zone(bool free) {
	if(!(get_type() & TYPE_LINK) || !(current.location & LOCATION_ONFIELD) || get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP))
		return 0;
	int32_t zones = 0;
	int32_t s = current.sequence;
	int32_t location = current.location;
	auto marker = get_link_marker();
	if(location == LOCATION_MZONE) {
		if(s > 0 && s <= 4 && is_link_marker(LINK_MARKER_LEFT, marker))
			zones |= 1u << (s - 1);
		if(s <= 3 && is_link_marker(LINK_MARKER_RIGHT, marker))
			zones |= 1u << (s + 1);
		if(s < 5) {
			if(s > 0 && is_link_marker(LINK_MARKER_BOTTOM_LEFT, marker))
				zones |= 1u << (s + 7);
			if(s < 4 && is_link_marker(LINK_MARKER_BOTTOM_RIGHT, marker))
				zones |= 1u << (s + 9);
			if(is_link_marker(LINK_MARKER_BOTTOM, marker))
				zones |= 1u << (s + 8);
		}
		if(pduel->game_field->is_flag(DUEL_EMZONE)) {
			if((s == 0 && is_link_marker(LINK_MARKER_TOP_RIGHT, marker))
				|| (s == 1 && is_link_marker(LINK_MARKER_TOP, marker))
				|| (s == 2 && is_link_marker(LINK_MARKER_TOP_LEFT, marker)))
				zones |= (1u << 5) | (1u << (16 + 6));
			if((s == 2 && is_link_marker(LINK_MARKER_TOP_RIGHT, marker))
				|| (s == 3 && is_link_marker(LINK_MARKER_TOP, marker))
				|| (s == 4 && is_link_marker(LINK_MARKER_TOP_LEFT, marker)))
				zones |= (1u << 6) | (1u << (16 + 5));
			if(s == 5) {
				if(is_link_marker(LINK_MARKER_BOTTOM_LEFT, marker))
					zones |= 1u << 0;
				if(is_link_marker(LINK_MARKER_BOTTOM, marker))
					zones |= 1u << 1;
				if(is_link_marker(LINK_MARKER_BOTTOM_RIGHT, marker))
					zones |= 1u << 2;
				if(is_link_marker(LINK_MARKER_TOP_LEFT, marker))
					zones |= 1u << (16 + 4);
				if(is_link_marker(LINK_MARKER_TOP, marker))
					zones |= 1u << (16 + 3);
				if(is_link_marker(LINK_MARKER_TOP_RIGHT, marker))
					zones |= 1u << (16 + 2);
			}
			if(s == 6) {
				if(is_link_marker(LINK_MARKER_BOTTOM_LEFT, marker))
					zones |= 1u << 2;
				if(is_link_marker(LINK_MARKER_BOTTOM, marker))
					zones |= 1u << 3;
				if(is_link_marker(LINK_MARKER_BOTTOM_RIGHT, marker))
					zones |= 1u << 4;
				if(is_link_marker(LINK_MARKER_TOP_LEFT, marker))
					zones |= 1u << (16 + 2);
				if(is_link_marker(LINK_MARKER_TOP, marker))
					zones |= 1u << (16 + 1);
				if(is_link_marker(LINK_MARKER_TOP_RIGHT, marker))
					zones |= 1u << (16 + 0);
			}
		}
	} else if(location == LOCATION_SZONE) {
		if(s > 4)
			return 0;
		if(s > 0) {
			if(is_link_marker(LINK_MARKER_LEFT, marker))
				zones |= 1u << (s + 7);
			if(is_link_marker(LINK_MARKER_TOP_LEFT, marker))
				zones |= 1u << (s - 1);
		}
		if(s < 4) {
			if(is_link_marker(LINK_MARKER_RIGHT, marker))
				zones |= 1u << (s + 9);
			if(is_link_marker(LINK_MARKER_TOP_RIGHT, marker))
				zones |= 1u << (s + 1);
		}
		if(is_link_marker(LINK_MARKER_TOP, marker))
			zones |= 1u << s;
	}
	if(free) {
		for(int i = 0; i < 8; i++) {
			if(!pduel->game_field->is_location_useable(current.controler, LOCATION_MZONE, i)) {
				zones &= ~(1u << i);
			}
		}
		for(int i = 0; i < 8; i++) {
			if(!pduel->game_field->is_location_useable(current.controler, LOCATION_SZONE, i)) {
				zones &= ~(1u << (i + 8));
			}
		}
		for(int i = 0; i < 8; i++) {
			if(!pduel->game_field->is_location_useable(1 - current.controler, LOCATION_MZONE, i + 16)) {
				zones &= ~(1u << (i + 16));
			}
		}
		for(int i = 0; i < 8; i++) {
			if(!pduel->game_field->is_location_useable(1 - current.controler, LOCATION_SZONE, i + 16)) {
				zones &= ~(1u << (i + 24));
			}
		}
	}
	return zones;
}
void card::get_linked_cards(card_set* cset, uint32_t zones) {
	cset->clear();
	if(!(data.type & TYPE_LINK) || !(current.location & LOCATION_ONFIELD) || get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP))
		return;
	int32_t p = current.controler;
	uint32_t linked_zone = (zones) ? zones : get_linked_zone();
	pduel->game_field->get_cards_in_zone(cset, linked_zone, p, LOCATION_ONFIELD);
	pduel->game_field->get_cards_in_zone(cset, linked_zone >> 16, 1 - p, LOCATION_ONFIELD);
	for(auto it = cset->begin(); it != cset->end();) {
		if((*it)->current.location == LOCATION_SZONE && !((*it)->get_type() & TYPE_LINK)) {
			it = cset->erase(it);
			continue;
		}
		it++;
	}
}
uint32_t card::get_mutual_linked_zone() {
	uint32_t linked_zone = get_linked_zone();
	if(!linked_zone)
		return 0;
	int32_t zones = 0;
	card_set cset;
	get_linked_cards(&cset, linked_zone);
	for(auto& pcard : cset) {
		auto zone = pcard->get_linked_zone();
		uint32_t is_szone = pcard->current.location == LOCATION_SZONE ? 8 : 0;
		uint32_t is_player = (current.controler == pcard->current.controler) ? 0 : 16;
		if(is_mutual_linked(pcard, linked_zone, zone)) {
			zones |= 1 << (pcard->current.sequence + is_szone + is_player);
		}
	}
	return zones;
}
void card::get_mutual_linked_cards(card_set* cset) {
	cset->clear();
	if(!(data.type & TYPE_LINK) || !(current.location & LOCATION_ONFIELD))
		return;
	int32_t p = current.controler;
	uint32_t mutual_linked_zone = get_mutual_linked_zone();
	pduel->game_field->get_cards_in_zone(cset, mutual_linked_zone, p, LOCATION_ONFIELD);
	pduel->game_field->get_cards_in_zone(cset, mutual_linked_zone >> 16, 1 - p, LOCATION_ONFIELD);
}
int32_t card::is_link_state() {
	if(!(current.location & LOCATION_ONFIELD))
		return FALSE;
	card_set cset;
	get_linked_cards(&cset);
	if(cset.size())
		return TRUE;
	int32_t p = current.controler;
	uint32_t is_szone = current.location == LOCATION_SZONE ? 8 : 0;
	uint32_t linked_zone = pduel->game_field->get_linked_zone(p, false, true);
	if((linked_zone >> (current.sequence + is_szone)) & 1)
		return TRUE;
	return FALSE;
}
int32_t card::is_mutual_linked(card* pcard, uint32_t zones1, uint32_t zones2) {
	int32_t ret = FALSE;
	if(!zones1)
		zones1 = get_linked_zone();
	uint32_t is_szone = pcard->current.location == LOCATION_SZONE ? 8 : 0;
	uint32_t is_player = (current.controler == pcard->current.controler) ? 0 : 16;
	if(zones1 & (1 << (pcard->current.sequence + is_szone + is_player))) {
		zones2 = zones2 ? zones2 : pcard->get_linked_zone();
		is_szone = current.location == LOCATION_SZONE ? 8 : 0;
		is_player = (current.controler == pcard->current.controler) ? 0 : 16;
		ret = zones2 & (1 << (current.sequence + is_szone + is_player));
	}
	return ret;
}
int32_t card::is_extra_link_state() {
	if(current.location != LOCATION_MZONE)
		return FALSE;
	uint32_t checked = (current.location == LOCATION_MZONE) ? (1u << current.sequence) : (1u << (current.sequence + 8));
	uint32_t linked_zone = get_mutual_linked_zone();
	const auto& list_mzone0 = pduel->game_field->player[current.controler].list_mzone;
	const auto& list_szone0 = pduel->game_field->player[current.controler].list_szone;
	const auto& list_mzone1 = pduel->game_field->player[1 - current.controler].list_mzone;
	const auto& list_szone1 = pduel->game_field->player[1 - current.controler].list_szone;
	while(true) {
		if(((linked_zone >> 5) | (linked_zone >> (16 + 6))) & ((linked_zone >> 6) | (linked_zone >> (16 + 5))) & 1)
			return TRUE;
		int32_t checking = (int32_t)(linked_zone & ~checked);
		if(!checking)
			return FALSE;
		int32_t rightmost = checking & (-checking);
		checked |= (uint32_t)rightmost;
		if(rightmost < 0x10000) {
			for(int32_t i = 0; i < 16; ++i) {
				if(rightmost & 1) {
					card* pcard = (i < 8) ? list_mzone0[i] : list_szone0[i - 8];
					linked_zone |= pcard->get_mutual_linked_zone();
					break;
				}
				rightmost >>= 1;
			}
		} else {
			rightmost >>= 16;
			for(int32_t i = 0; i < 16; ++i) {
				if(rightmost & 1) {
					card* pcard = (i < 8) ? list_mzone1[i] : list_szone1[i - 8];
					uint32_t zone = pcard->get_mutual_linked_zone();
					linked_zone |= ((zone & 0xffff) << 16) | (zone >> 16);
					break;
				}
				rightmost >>= 1;
			}
		}
	}
	return FALSE;
}
int32_t card::is_position(int32_t pos) {
	return current.position & pos;
}
void card::set_status(uint32_t _status, int32_t enabled) {
	if (enabled)
		this->status |= _status;
	else
		this->status &= ~_status;
}
// match at least 1 status
int32_t card::get_status(uint32_t _status) {
	return this->status & _status;
}
// match all status
int32_t card::is_status(uint32_t _status) {
	if ((this->status & _status) == _status)
		return TRUE;
	return FALSE;
}
uint32_t card::get_column_zone(int32_t loc1, int32_t left, int32_t right) {
	int32_t zones = 0;
	int32_t loc2 = current.location;
	int32_t seq = current.sequence;
	if(!(loc1 & LOCATION_ONFIELD) || !(loc2 & LOCATION_ONFIELD) || left < 0 || right < 0)
		return 0;
	if(loc2 & LOCATION_SZONE && seq >= 5)
		return 0;
	if (loc2 & LOCATION_MZONE && (seq == 5 || seq == 6))
		seq = (seq == 5) ? 1 : 3;
	int32_t seq1 = seq - left < 0 ? 0 : seq - left;
	int32_t seq2 = seq + right > 4 ? 4 : seq + right;
	auto chkextra = [seq = current.sequence, mzone = (current.location == LOCATION_MZONE)](uint8_t s)->bool { 
		return !mzone || seq < 5 || seq != s;
	};
	if (loc1 & LOCATION_MZONE) {
		for (int32_t s = seq1; s <= seq2; s++) {
			zones |= (1u << s) | (1u << (16 + (4 - s)));
			if (pduel->game_field->is_flag(DUEL_EMZONE)) {
				if (s == 1 && chkextra(5))
					zones |= (1u << 5) | (1u << (16 + 6));
				else if (s == 3 && chkextra(6))
					zones |= (1u << 6) | (1u << (16 + 5));
			}
		}
	}
	if (loc1 & LOCATION_SZONE) {
		for (int32_t s = seq1; s <= seq2; s++)
			zones |= (1u << (8 + s)) | (1u << (16 + 8 + (4 - s)));
	}
	zones &= ~((1 << current.sequence) << ((loc2 == LOCATION_SZONE) ? 8 : 0));
	return zones;
}
void card::get_column_cards(card_set* cset, int32_t left, int32_t right) {
	cset->clear();
	if(!(current.location & LOCATION_ONFIELD))
		return;
	int32_t p = current.controler;
	uint32_t column_zone = get_column_zone(LOCATION_ONFIELD, left, right);
	pduel->game_field->get_cards_in_zone(cset, column_zone, p, LOCATION_ONFIELD);
	pduel->game_field->get_cards_in_zone(cset, column_zone >> 16, 1 - p, LOCATION_ONFIELD);
}
int32_t card::is_all_column() {
	if(!(current.location & LOCATION_ONFIELD))
		return FALSE;
	card_set cset;
	get_column_cards(&cset, 0, 0);
	uint32_t full = 3;
	if(pduel->game_field->is_flag(DUEL_EMZONE) && (current.sequence == 1 || current.sequence == 3))
		full++;
	if(cset.size() == full)
		return TRUE;
	return FALSE;
}
void card::equip(card* target, uint32_t send_msg) {
	if (equiping_target)
		return;
	target->equiping_cards.insert(this);
	equiping_target = target;
	for (auto& it : equip_effect) {
		if (it.second->is_disable_related())
			pduel->game_field->add_to_disable_check_list(equiping_target);
	}
	if(send_msg) {
		auto message = pduel->new_message(MSG_EQUIP);
		message->write(get_info_location());
		message->write(target->get_info_location());
	}
	return;
}
void card::unequip() {
	if (!equiping_target)
		return;
	for (auto it = equip_effect.begin(); it != equip_effect.end(); ++it) {
		if (it->second->is_disable_related())
			pduel->game_field->add_to_disable_check_list(equiping_target);
	}
	equiping_target->equiping_cards.erase(this);
	pre_equip_target = equiping_target;
	equiping_target = 0;
	return;
}
int32_t card::get_union_count() {
	int32_t count = 0;
	for(auto& pcard : equiping_cards) {
		if((pcard->data.type & TYPE_UNION) && pcard->is_affected_by_effect(EFFECT_UNION_STATUS))
			count++;
	}
	return count;
}
int32_t card::get_old_union_count() {
	int32_t count = 0;
	for(auto& pcard : equiping_cards) {
		if((pcard->data.type & TYPE_UNION) && pcard->is_affected_by_effect(EFFECT_OLDUNION_STATUS))
			count++;
	}
	return count;
}
void card::xyz_overlay(const card_set& materials) {
	if(materials.size() == 0)
		return;
	card_set des, from_grave;
	card_vector cv;
	cv.reserve(materials.size());
	std::copy_if(materials.begin(), materials.end(), std::back_inserter(cv), [this](const card* pcard) {return pcard->overlay_target != this; });
	{
		const auto prev_size = cv.size();
		for(auto& pcard : materials) {
			if(pcard->xyz_materials.size())
				cv.insert(cv.begin(), pcard->xyz_materials.begin(), pcard->xyz_materials.end());
		}
		const auto cur_size = cv.size();
		if(cur_size - prev_size >= 2)
			std::sort(cv.begin(), cv.begin() + ((cur_size - prev_size) - 1), card::card_operation_sort);
		if(prev_size >= 2)
			std::sort(cv.begin() + (cur_size - prev_size), cv.end(), card::card_operation_sort);
	}
	const auto& player = pduel->game_field->player;
	duel::duel_message* decktop[2] = { nullptr, nullptr };
	const size_t s[2] = { player[0].list_main.size(), player[1].list_main.size() };
	if(pduel->game_field->core.global_flag & GLOBALFLAG_DECK_REVERSE_CHECK) {
		const auto m_end = materials.end();
		if(s[0] > 0 && materials.find(player[0].list_main.back()) != m_end) {
			decktop[0] = pduel->new_message(MSG_DECK_TOP);
			decktop[0]->write<uint8_t>(0);
		}
		if(s[1] > 0 && materials.find(player[1].list_main.back()) != m_end) {
			decktop[1] = pduel->new_message(MSG_DECK_TOP);
			decktop[1]->write<uint8_t>(1);
		}
	}
	for(auto& pcard : cv) {
		pcard->current.reason = REASON_XYZ + REASON_MATERIAL;
		pcard->reset(RESET_LEAVE + RESET_OVERLAY, RESET_EVENT);
		if(pcard->unique_code)
			pduel->game_field->remove_unique_card(pcard);
		if(pcard->equiping_target)
			pcard->unequip();
		for(auto cit = pcard->equiping_cards.begin(); cit != pcard->equiping_cards.end();) {
			card* equipc = *cit++;
			des.insert(equipc);
			equipc->unequip();
		}
		pcard->clear_card_target();
		auto message = pduel->new_message(MSG_MOVE);
		message->write<uint32_t>(pcard->data.code);
		message->write(pcard->get_info_location());
		if(pcard->overlay_target) {
			pcard->overlay_target->xyz_remove(pcard);
		} else {
			pcard->enable_field_effect(false);
			pduel->game_field->remove_card(pcard);
			pduel->game_field->add_to_disable_check_list(pcard);
			if(pcard->previous.location == LOCATION_GRAVE) {
				from_grave.insert(pcard);
				pduel->game_field->raise_single_event(pcard, 0, EVENT_LEAVE_GRAVE, pduel->game_field->core.reason_effect, 0, pduel->game_field->core.reason_player, 0, 0);
			}
		}
		xyz_add(pcard);
		message->write(pcard->get_info_location());
		message->write<uint32_t>(pcard->current.reason);
	}
	auto writetopcard = [rev=pduel->game_field->core.deck_reversed, &decktop, &player, &s](int playerid) {
		if(!decktop[playerid])
			return;
		auto& msg = decktop[playerid];
		const auto& list = player[playerid].list_main;
		if(list.empty() || (!rev && list.back()->current.position != POS_FACEUP_DEFENSE))
			msg->data.clear();
		else {
			auto& prevcount = s[playerid];
			const auto* ptop = list.back();
			msg->write<uint32_t>(prevcount - list.size());
			msg->write<uint32_t>(ptop->data.code);
			msg->write<uint32_t>(ptop->current.position);
		}
	};
	writetopcard(0);
	writetopcard(1);
	if(from_grave.size()) {
		pduel->game_field->raise_event(&from_grave, EVENT_LEAVE_GRAVE, pduel->game_field->core.reason_effect, 0, pduel->game_field->core.reason_player, 0, 0);
		pduel->game_field->process_single_event();
		pduel->game_field->process_instant_event();
	}
	if(des.size())
		pduel->game_field->destroy(&des, 0, REASON_LOST_TARGET + REASON_RULE, PLAYER_NONE);
	else
		pduel->game_field->adjust_instant();
}
void card::xyz_add(card* mat) {
	if(mat->current.location != 0)
		return;
	xyz_materials.push_back(mat);
	mat->overlay_target = this;
	mat->current.controler = PLAYER_NONE;
	mat->current.location = LOCATION_OVERLAY;
	mat->current.sequence = xyz_materials.size() - 1;
	for(auto& eit : mat->xmaterial_effect) {
		effect* peffect = eit.second;
		if(peffect->type & EFFECT_TYPE_FIELD)
			pduel->game_field->add_effect(peffect);
	}
}
void card::xyz_remove(card* mat) {
	if(mat->overlay_target != this)
		return;
	xyz_materials.erase(xyz_materials.begin() + mat->current.sequence);
	if(pduel->game_field->core.current_chain.size() > 0)
		pduel->game_field->core.just_sent_cards.insert(mat);
	mat->previous.controler = mat->current.controler;
	mat->previous.location = mat->current.location;
	mat->previous.sequence = mat->current.sequence;
	mat->previous.pzone = mat->current.pzone;
	mat->current.controler = PLAYER_NONE;
	mat->current.location = 0;
	mat->current.sequence = 0;
	mat->overlay_target = 0;
	for(auto clit = xyz_materials.begin(); clit != xyz_materials.end(); ++clit)
		(*clit)->current.sequence = clit - xyz_materials.begin();
	for(auto eit = mat->xmaterial_effect.begin(); eit != mat->xmaterial_effect.end(); ++eit) {
		effect* peffect = eit->second;
		if(peffect->type & EFFECT_TYPE_FIELD)
			pduel->game_field->remove_effect(peffect);
	}
}
void card::apply_field_effect() {
	if (current.controler == PLAYER_NONE)
		return;
	for (auto& it : field_effect) {
		if (it.second->in_range(this)
				|| ((it.second->range & LOCATION_HAND) && (it.second->type & EFFECT_TYPE_TRIGGER_O) && !(it.second->code & EVENT_PHASE))) {
			pduel->game_field->add_effect(it.second);
		}
	}
	if(unique_code && (current.location & unique_location))
		pduel->game_field->add_unique_card(this);
	spsummon_counter[0] = spsummon_counter[1] = 0;
	spsummon_counter_rst[0] = spsummon_counter_rst[1] = 0;
}
void card::cancel_field_effect() {
	if (current.controler == PLAYER_NONE)
		return;
	for (auto& it : field_effect) {
		if (it.second->in_range(this)
				|| ((it.second->range & LOCATION_HAND) && (it.second->type & EFFECT_TYPE_TRIGGER_O) && !(it.second->code & EVENT_PHASE))) {
			pduel->game_field->remove_effect(it.second);
		}
	}
	if(unique_code && (current.location & unique_location))
		pduel->game_field->remove_unique_card(this);
}
// STATUS_EFFECT_ENABLED: the card is ready to use
// false: before moving, summoning, chaining
// true: ready
void card::enable_field_effect(bool enabled) {
	if (current.location == 0)
		return;
	if ((enabled && get_status(STATUS_EFFECT_ENABLED)) || (!enabled && !get_status(STATUS_EFFECT_ENABLED)))
		return;
	refresh_disable_status();
	if (enabled) {
		set_status(STATUS_EFFECT_ENABLED, TRUE);
		for (auto& it : single_effect) {
			if (it.second->is_flag(EFFECT_FLAG_SINGLE_RANGE) && it.second->in_range(this))
				it.second->id = pduel->game_field->infos.field_id++;
		}
		for (auto& it : field_effect) {
			if (it.second->in_range(this))
				it.second->id = pduel->game_field->infos.field_id++;
		}
		if(current.location == LOCATION_SZONE) {
			for (auto& it : equip_effect)
				it.second->id = pduel->game_field->infos.field_id++;
		}
		for (auto& it : target_effect) {
			if (it.second->in_range(this))
				it.second->id = pduel->game_field->infos.field_id++;
		}
		if (get_status(STATUS_DISABLED))
			reset(RESET_DISABLE, RESET_EVENT);
	} else
		set_status(STATUS_EFFECT_ENABLED, FALSE);
	if (get_status(STATUS_DISABLED | STATUS_FORBIDDEN))
		return;
	filter_disable_related_cards();
}
int32_t card::add_effect(effect* peffect) {
	if (get_status(STATUS_COPYING_EFFECT) && peffect->is_flag(EFFECT_FLAG_UNCOPYABLE)) {
		pduel->uncopy.insert(peffect);
		return 0;
	}
	if (indexer.find(peffect) != indexer.end())
		return 0;
	card_set check_target = { this };
	effect_container::iterator eit;
	if (peffect->type & EFFECT_TYPE_SINGLE) {
		if((peffect->code == EFFECT_SET_ATTACK || peffect->code == EFFECT_SET_BASE_ATTACK) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			for(auto it = single_effect.begin(); it != single_effect.end();) {
				auto rm = it++;
				if((rm->second->code == EFFECT_SET_ATTACK || rm->second->code == EFFECT_SET_ATTACK_FINAL || rm->second->code == EFFECT_SET_BASE_ATTACK)
						&& !rm->second->is_flag(EFFECT_FLAG_SINGLE_RANGE))
					remove_effect(rm->second);
			}
		}
		if(peffect->code == EFFECT_SET_ATTACK_FINAL && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			for(auto it = single_effect.begin(); it != single_effect.end();) {
				auto rm = it++;
				if((rm->second->code == EFFECT_UPDATE_ATTACK || rm->second->code == EFFECT_SET_ATTACK
								|| rm->second->code == EFFECT_SET_ATTACK_FINAL) && !rm->second->is_flag(EFFECT_FLAG_SINGLE_RANGE))
					remove_effect(rm->second);
			}
		}
		if((peffect->code == EFFECT_SET_DEFENSE || peffect->code == EFFECT_SET_BASE_DEFENSE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			for(auto it = single_effect.begin(); it != single_effect.end();) {
				auto rm = it++;
				if((rm->second->code == EFFECT_SET_DEFENSE || rm->second->code == EFFECT_SET_DEFENSE_FINAL || rm->second->code == EFFECT_SET_BASE_DEFENSE)
								&& !rm->second->is_flag(EFFECT_FLAG_SINGLE_RANGE))
					remove_effect(rm->second);
			}
		}
		if(peffect->code == EFFECT_SET_DEFENSE_FINAL && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			for(auto it = single_effect.begin(); it != single_effect.end();) {
				auto rm = it++;
				if((rm->second->code == EFFECT_UPDATE_DEFENSE || rm->second->code == EFFECT_SET_DEFENSE
								|| rm->second->code == EFFECT_SET_DEFENSE_FINAL) && !rm->second->is_flag(EFFECT_FLAG_SINGLE_RANGE))
					remove_effect(rm->second);
			}
		}
		eit = single_effect.emplace(peffect->code, peffect);
	} else if (peffect->type & EFFECT_TYPE_EQUIP) {
		eit = equip_effect.emplace(peffect->code, peffect);
		if (equiping_target)
			check_target = { equiping_target };
		else
			check_target.clear();
	} else if(peffect->type & EFFECT_TYPE_TARGET) {
		eit = target_effect.emplace(peffect->code, peffect);
		if(!effect_target_cards.empty())
			check_target = effect_target_cards;
		else
			check_target.clear();
	} else if (peffect->type & EFFECT_TYPE_XMATERIAL) {
		eit = xmaterial_effect.emplace(peffect->code, peffect);
		if (overlay_target)
			check_target = { overlay_target };
		else
			check_target.clear();
	} else if (peffect->type & EFFECT_TYPE_FIELD) {
		eit = field_effect.emplace(peffect->code, peffect);
	} else
		return 0;
	peffect->id = pduel->game_field->infos.field_id++;
	peffect->card_type = data.type;
	if(get_status(STATUS_INITIALIZING))
		peffect->flag[0] |= EFFECT_FLAG_INITIAL;
	if (get_status(STATUS_COPYING_EFFECT)) {
		peffect->copy_id = pduel->game_field->infos.copy_id;
		peffect->reset_flag |= pduel->game_field->core.copy_reset;
		peffect->reset_count = pduel->game_field->core.copy_reset_count;
	}
	effect* reason_effect = pduel->game_field->core.reason_effect;
	if(peffect->is_flag(EFFECT_FLAG_COPY_INHERIT) && reason_effect && reason_effect->copy_id) {
		peffect->copy_id = reason_effect->copy_id;
		peffect->reset_flag |= reason_effect->reset_flag;
		if(peffect->reset_count > reason_effect->reset_count)
			peffect->reset_count = reason_effect->reset_count;
	}
	indexer.emplace(peffect, eit);
	peffect->handler = this;
	if((peffect->type & EFFECT_TYPE_FIELD)) {
		if(peffect->in_range(this)
			|| (current.controler != PLAYER_NONE && ((peffect->range & LOCATION_HAND) && (peffect->type & EFFECT_TYPE_TRIGGER_O) && !(peffect->code & EVENT_PHASE))))
			pduel->game_field->add_effect(peffect);
	}
	if (current.controler != PLAYER_NONE && !check_target.empty()) {
		if(peffect->is_disable_related())
			for(auto& target : check_target)
				pduel->game_field->add_to_disable_check_list(target);
	}
	if(peffect->is_flag(EFFECT_FLAG_OATH)) {
		pduel->game_field->effects.oath.emplace(peffect, reason_effect);
	}
	if(peffect->reset_flag & RESET_PHASE) {
		pduel->game_field->effects.pheff.insert(peffect);
		if(peffect->reset_count == 0)
			peffect->reset_count += 1;
	}
	if(peffect->reset_flag & RESET_CHAIN)
		pduel->game_field->effects.cheff.insert(peffect);
	if(peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT))
		pduel->game_field->effects.rechargeable.insert(peffect);
	if(peffect->is_flag(EFFECT_FLAG_CLIENT_HINT)) {
		auto message = pduel->new_message(MSG_CARD_HINT);
		message->write(get_info_location());
		message->write<uint8_t>(CHINT_DESC_ADD);
		message->write<uint64_t>(peffect->description);
	}
	if(peffect->type & EFFECT_TYPE_SINGLE && peffect->code == EFFECT_UPDATE_LEVEL && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
		int32_t val = peffect->get_value(this);
		if(val > 0) {
			pduel->game_field->raise_single_event(this, 0, EVENT_LEVEL_UP, peffect, 0, 0, 0, val);
			pduel->game_field->process_single_event();
		}
	}
	return peffect->id;
}
void card::remove_effect(effect* peffect) {
	auto it = indexer.find(peffect);
	if (it == indexer.end())
		return;
	remove_effect(peffect, it->second);
}
void card::remove_effect(effect* peffect, effect_container::iterator it) {
	card_set check_target = { this };
	if (peffect->type & EFFECT_TYPE_SINGLE) {
		single_effect.erase(it);
	} else if (peffect->type & EFFECT_TYPE_EQUIP) {
		equip_effect.erase(it);
		if (equiping_target)
			check_target = { equiping_target };
		else
			check_target.clear();
	} else if(peffect->type & EFFECT_TYPE_TARGET) {
		target_effect.erase(it);
		if(!effect_target_cards.empty())
			check_target = effect_target_cards;
		else
			check_target.clear();
	} else if (peffect->type & EFFECT_TYPE_XMATERIAL) {
		xmaterial_effect.erase(it);
		if (overlay_target)
			check_target = { overlay_target };
		else
			check_target.clear();
	} else if (peffect->type & EFFECT_TYPE_FIELD) {
		check_target.clear();
		if (peffect->is_available() && peffect->is_disable_related()) {
			pduel->game_field->update_disable_check_list(peffect);
		}
		field_effect.erase(it);
		if(peffect->in_range(this)
			|| (current.controler != PLAYER_NONE && ((peffect->range & LOCATION_HAND) && (peffect->type & EFFECT_TYPE_TRIGGER_O) && !(peffect->code & EVENT_PHASE))))
			pduel->game_field->remove_effect(peffect);
	}
	if ((current.controler != PLAYER_NONE) && !get_status(STATUS_DISABLED | STATUS_FORBIDDEN) && !check_target.empty()) {
		if (peffect->is_disable_related())
			for(auto& target : check_target)
				pduel->game_field->add_to_disable_check_list(target);
	}
	if (peffect->is_flag(EFFECT_FLAG_INITIAL) && peffect->copy_id && is_status(STATUS_EFFECT_REPLACED)) {
		set_status(STATUS_EFFECT_REPLACED, FALSE);
		if (!(data.type & TYPE_NORMAL) || (data.type & TYPE_PENDULUM)) {
			set_status(STATUS_INITIALIZING, TRUE);
			pduel->lua->add_param(this, PARAM_TYPE_CARD);
			pduel->lua->call_card_function(this, "initial_effect", 1, 0);
			set_status(STATUS_INITIALIZING, FALSE);
		}
	}
	indexer.erase(peffect);
	if(peffect->is_flag(EFFECT_FLAG_OATH))
		pduel->game_field->effects.oath.erase(peffect);
	if(peffect->reset_flag & RESET_PHASE)
		pduel->game_field->effects.pheff.erase(peffect);
	if(peffect->reset_flag & RESET_CHAIN)
		pduel->game_field->effects.cheff.erase(peffect);
	if(peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT))
		pduel->game_field->effects.rechargeable.erase(peffect);
	if(((peffect->code & 0xf0000) == EFFECT_COUNTER_PERMIT) && (peffect->type & EFFECT_TYPE_SINGLE)) {
		auto cmit = counters.find(peffect->code & 0xffff);
		if(cmit != counters.end()) {
			auto message = pduel->new_message(MSG_REMOVE_COUNTER);
			message->write<uint16_t>(cmit->first);
			message->write<uint8_t>(current.controler);
			message->write<uint8_t>(current.location);
			message->write<uint8_t>(current.sequence);
			message->write<uint16_t>(cmit->second[0] + cmit->second[1]);
			counters.erase(cmit);
		}
	}
	if(peffect->is_flag(EFFECT_FLAG_CLIENT_HINT)) {
		auto message = pduel->new_message(MSG_CARD_HINT);
		message->write(get_info_location());
		message->write<uint8_t>(CHINT_DESC_REMOVE);
		message->write<uint64_t>(peffect->description);
	}
	if(peffect->code == EFFECT_UNIQUE_CHECK) {
		pduel->game_field->remove_unique_card(this);
		unique_pos[0] = unique_pos[1] = 0;
		unique_code = 0;
	}
	pduel->game_field->core.reseted_effects.insert(peffect);
}
int32_t card::copy_effect(uint32_t code, uint32_t reset, uint32_t count) {
	if(pduel->read_card(code).type & TYPE_NORMAL)
		return -1;
	set_status(STATUS_COPYING_EFFECT, TRUE);
	uint32_t cr = pduel->game_field->core.copy_reset;
	uint8_t crc = pduel->game_field->core.copy_reset_count;
	pduel->game_field->core.copy_reset = reset;
	pduel->game_field->core.copy_reset_count = count;
	pduel->lua->add_param(this, PARAM_TYPE_CARD);
	pduel->lua->call_code_function(code, "initial_effect", 1, 0);
	pduel->game_field->infos.copy_id++;
	set_status(STATUS_COPYING_EFFECT, FALSE);
	pduel->game_field->core.copy_reset = cr;
	pduel->game_field->core.copy_reset_count = crc;
	for(auto& peffect : pduel->uncopy)
		pduel->delete_effect(peffect);
	pduel->uncopy.clear();
	if((data.type & TYPE_MONSTER) && !(data.type & TYPE_EFFECT)) {
		effect* peffect = pduel->new_effect();
		if(pduel->game_field->core.reason_effect)
			peffect->owner = pduel->game_field->core.reason_effect->get_handler();
		else
			peffect->owner = this;
		peffect->handler = this;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_ADD_TYPE;
		peffect->value = TYPE_EFFECT;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = reset;
		peffect->reset_count = count;
		this->add_effect(peffect);
	}
	return pduel->game_field->infos.copy_id - 1;
}
int32_t card::replace_effect(uint32_t code, uint32_t reset, uint32_t count, bool recreating) {
	if(pduel->read_card(code).type & TYPE_NORMAL)
		return -1;
	if(is_status(STATUS_EFFECT_REPLACED))
		set_status(STATUS_EFFECT_REPLACED, FALSE);
	for(auto i = indexer.begin(); i != indexer.end();) {
		auto rm = i++;
		effect* peffect = rm->first;
		auto it = rm->second;
		if (peffect->is_flag(EFFECT_FLAG_INITIAL | EFFECT_FLAG_COPY_INHERIT))
			remove_effect(peffect, it);
	}
	uint32_t cr = pduel->game_field->core.copy_reset;
	uint8_t crc = pduel->game_field->core.copy_reset_count;
	pduel->game_field->core.copy_reset = reset;
	pduel->game_field->core.copy_reset_count = count;
	set_status(STATUS_INITIALIZING, TRUE);
	if(!recreating)
		set_status(STATUS_COPYING_EFFECT, TRUE);
	pduel->lua->load_card_script(code);
	pduel->lua->add_param(this, PARAM_TYPE_CARD);
	pduel->lua->call_code_function(code, "initial_effect", 1, 0);
	set_status(STATUS_INITIALIZING | STATUS_COPYING_EFFECT, FALSE);
	pduel->game_field->infos.copy_id++;
	pduel->game_field->core.copy_reset = cr;
	pduel->game_field->core.copy_reset_count = crc;
	set_status(STATUS_EFFECT_REPLACED, TRUE);
	for(auto& peffect : pduel->uncopy)
		pduel->delete_effect(peffect);
	pduel->uncopy.clear();
	if((data.type & TYPE_MONSTER) && !(data.type & TYPE_EFFECT)) {
		effect* peffect = pduel->new_effect();
		if(pduel->game_field->core.reason_effect)
			peffect->owner = pduel->game_field->core.reason_effect->get_handler();
		else
			peffect->owner = this;
		peffect->handler = this;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_ADD_TYPE;
		peffect->value = TYPE_EFFECT;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = reset;
		peffect->reset_count = count;
		this->add_effect(peffect);
	}
	return pduel->game_field->infos.copy_id - 1;
}
void card::reset(uint32_t id, uint32_t reset_type) {
	if (reset_type != RESET_EVENT && reset_type != RESET_PHASE && reset_type != RESET_CODE && reset_type != RESET_COPY && reset_type != RESET_CARD)
		return;
	if (reset_type == RESET_EVENT) {
		for (auto rit = relations.begin(); rit != relations.end();) {
			auto rrm = rit++;
			if (rrm->second & 0xffff0000 & id)
				relations.erase(rrm);
		}
		if(id & (RESET_TODECK | RESET_TOHAND | RESET_TOGRAVE | RESET_REMOVE | RESET_TEMP_REMOVE
			| RESET_OVERLAY | RESET_MSCHANGE))
			clear_relate_effect();
		if(id & (RESET_TODECK | RESET_TOHAND | RESET_TOGRAVE | RESET_REMOVE | RESET_TEMP_REMOVE
			| RESET_OVERLAY | RESET_MSCHANGE | RESET_LEAVE | RESET_TOFIELD)) {
			indestructable_effects.clear();
			announced_cards.clear();
			attacked_cards.clear();
			attack_announce_count = 0;
			announce_count = 0;
			attacked_count = 0;
			attack_all_target = TRUE;
		}
		if(id & (RESET_TODECK | RESET_TOHAND | RESET_TOGRAVE | RESET_REMOVE | RESET_TEMP_REMOVE
			| RESET_OVERLAY | RESET_MSCHANGE | RESET_LEAVE | RESET_TOFIELD | RESET_TURN_SET)) {
			battled_cards.clear();
			reset_effect_count();
			auto pr = field_effect.equal_range(EFFECT_DISABLE_FIELD);
			for(; pr.first != pr.second; ++pr.first)
				pr.first->second->value = 0;
		}
		if(id & (RESET_TODECK | RESET_TOHAND | RESET_TOGRAVE | RESET_REMOVE | RESET_TEMP_REMOVE
			| RESET_OVERLAY | RESET_MSCHANGE | RESET_TOFIELD  | RESET_TURN_SET)) {
			counters.clear();
		}
		if(id & (RESET_TODECK | RESET_TOHAND | RESET_TOGRAVE | RESET_REMOVE | RESET_TEMP_REMOVE
			| RESET_LEAVE | RESET_TOFIELD | RESET_TURN_SET | RESET_CONTROL)) {
			auto pr = field_effect.equal_range(EFFECT_USE_EXTRA_MZONE);
			for(; pr.first != pr.second; ++pr.first)
				pr.first->second->value = pr.first->second->value & 0xffff;
			pr = field_effect.equal_range(EFFECT_USE_EXTRA_SZONE);
			for(; pr.first != pr.second; ++pr.first)
				pr.first->second->value = pr.first->second->value & 0xffff;
		}
		if(id & RESET_TOFIELD) {
			pre_equip_target = 0;
		}
		if(id & RESET_DISABLE) {
			for(auto cmit = counters.begin(); cmit != counters.end();) {
				auto rm = cmit++;
				if(rm->second[1] > 0) {
					auto message = pduel->new_message(MSG_REMOVE_COUNTER);
					message->write<uint16_t>(rm->first);
					message->write<uint8_t>(current.controler);
					message->write<uint8_t>(current.location);
					message->write<uint8_t>(current.sequence);
					message->write<uint16_t>(rm->second[1]);
					rm->second[1] = 0;
					if(rm->second[0] == 0)
						counters.erase(rm);
				}
			}
		}
		if(id & RESET_TURN_SET) {
			effect* peffect = std::get<effect*>(refresh_control_status());
			if(peffect && (!(peffect->type & EFFECT_TYPE_SINGLE) || peffect->condition)) {
				effect* new_effect = pduel->new_effect();
				new_effect->id = peffect->id;
				new_effect->owner = this;
				new_effect->handler = this;
				new_effect->type = EFFECT_TYPE_SINGLE;
				new_effect->code = EFFECT_SET_CONTROL;
				new_effect->value = current.controler;
				new_effect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
				new_effect->reset_flag = RESET_EVENT | 0xec0000;
				this->add_effect(new_effect);
			}
		}
	}
	for (auto i = indexer.begin(); i != indexer.end();) {
		auto rm = i++;
		effect* peffect = rm->first;
		auto it = rm->second;
		if (peffect->reset(id, reset_type))
			remove_effect(peffect, it);
	}
}
void card::reset_effect_count() {
	for (auto& i : indexer) {
		effect* peffect = i.first;
		if (peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT))
			peffect->recharge();
	}
}
// refresh STATUS_DISABLED based on EFFECT_DISABLE and EFFECT_CANNOT_DISABLE
// refresh STATUS_FORBIDDEN based on EFFECT_FORBIDDEN
void card::refresh_disable_status() {
	filter_immune_effect();
	// forbidden
	if (is_affected_by_effect(EFFECT_FORBIDDEN))
		set_status(STATUS_FORBIDDEN, TRUE);
	else
		set_status(STATUS_FORBIDDEN, FALSE);
	// disabled
	if (!is_affected_by_effect(EFFECT_CANNOT_DISABLE) && is_affected_by_effect(EFFECT_DISABLE))
		set_status(STATUS_DISABLED, TRUE);
	else
		set_status(STATUS_DISABLED, FALSE);
}
std::tuple<uint8_t, effect*> card::refresh_control_status() {
	uint8_t final = owner;
	effect* ceffect = nullptr;
	uint32_t last_id = 0;
	if(pduel->game_field->core.remove_brainwashing && is_affected_by_effect(EFFECT_REMOVE_BRAINWASHING))
		last_id = pduel->game_field->core.last_control_changed_id;
	effect_set eset;
	filter_effect(EFFECT_SET_CONTROL, &eset);
	if(eset.size()) {
		effect* peffect = eset.back();
		if(peffect->id >= last_id) {
			final = (uint8_t)peffect->get_value(this);
			ceffect = peffect;
		}
	}
	return std::make_tuple(final, ceffect);
}
void card::count_turn(uint16_t ct) {
	turn_counter = ct;
	auto message = pduel->new_message(MSG_CARD_HINT);
	message->write(get_info_location());
	message->write<uint8_t>(CHINT_TURN);
	message->write<uint64_t>(ct);
}
void card::create_relation(card* target, uint32_t reset) {
	if (relations.find(target) != relations.end())
		return;
	relations[target] = reset;
}
int32_t card::is_has_relation(card* target) {
	if (relations.find(target) != relations.end())
		return TRUE;
	return FALSE;
}
void card::release_relation(card* target) {
	if (relations.find(target) == relations.end())
		return;
	relations.erase(target);
}
void card::create_relation(const chain& ch) {
	relate_effect.emplace(ch.triggering_effect, ch.chain_id);
}
int32_t card::is_has_relation(const chain& ch) {
	if (relate_effect.find(std::make_pair(ch.triggering_effect, ch.chain_id)) != relate_effect.end())
		return TRUE;
	return FALSE;
}
void card::release_relation(const chain& ch) {
	relate_effect.erase(std::make_pair(ch.triggering_effect, ch.chain_id));
}
void card::clear_relate_effect() {
	relate_effect.clear();
}
void card::create_relation(effect* peffect) {
	for(auto it = pduel->game_field->core.current_chain.rbegin(); it != pduel->game_field->core.current_chain.rend(); ++it) {
		if(it->triggering_effect == peffect) {
			create_relation(*it);
			return;
		}
	}
	relate_effect.emplace(peffect, (uint16_t)0);
}
int32_t card::is_has_relation(effect* peffect) {
	for(auto& it : relate_effect) {
		if(it.first == peffect)
			return TRUE;
	}
	return FALSE;
}
void card::release_relation(effect* peffect) {
	for(auto it = pduel->game_field->core.current_chain.rbegin(); it != pduel->game_field->core.current_chain.rend(); ++it) {
		if(it->triggering_effect == peffect) {
			release_relation(*it);
			return;
		}
	}
	relate_effect.erase(std::make_pair(peffect, (uint16_t)0));
}
int32_t card::leave_field_redirect(uint32_t /*reason*/) {
	effect_set es;
	uint32_t redirect;
	uint32_t redirects = 0;
	if(data.type & TYPE_TOKEN)
		return 0;
	filter_effect(EFFECT_LEAVE_FIELD_REDIRECT, &es);
	for(const auto& peff : es) {
		redirect = peff->get_value(this, 0);
		if((redirect & LOCATION_HAND) && !is_affected_by_effect(EFFECT_CANNOT_TO_HAND) && pduel->game_field->is_player_can_send_to_hand(peff->get_handler_player(), this))
			redirects |= redirect;
		else if((redirect & LOCATION_DECK) && !is_affected_by_effect(EFFECT_CANNOT_TO_DECK) && pduel->game_field->is_player_can_send_to_deck(peff->get_handler_player(), this))
			redirects |= redirect;
		else if((redirect & LOCATION_REMOVED) && !is_affected_by_effect(EFFECT_CANNOT_REMOVE) && pduel->game_field->is_player_can_remove(peff->get_handler_player(), this, REASON_EFFECT))
			redirects |= redirect;
	}
	if(redirects & LOCATION_REMOVED)
		return LOCATION_REMOVED;
	// the ruling for the priority of the following redirects can't be confirmed for now
	if(redirects & LOCATION_DECK) {
		if((redirects & LOCATION_DECKBOT) == LOCATION_DECKBOT)
			return LOCATION_DECKBOT;
		if((redirects & LOCATION_DECKSHF) == LOCATION_DECKSHF)
			return LOCATION_DECKSHF;
		return LOCATION_DECK;
	}
	if(redirects & LOCATION_HAND)
		return LOCATION_HAND;
	return 0;
}
int32_t card::destination_redirect(uint8_t destination, uint32_t /*reason*/) {
	effect_set es;
	uint32_t redirect;
	if(data.type & TYPE_TOKEN)
		return 0;
	if(destination == LOCATION_HAND)
		filter_effect(EFFECT_TO_HAND_REDIRECT, &es);
	else if(destination == LOCATION_DECK)
		filter_effect(EFFECT_TO_DECK_REDIRECT, &es);
	else if(destination == LOCATION_GRAVE)
		filter_effect(EFFECT_TO_GRAVE_REDIRECT, &es);
	else if(destination == LOCATION_REMOVED)
		filter_effect(EFFECT_REMOVE_REDIRECT, &es);
	else
		return 0;
	for(const auto& peff : es) {
		redirect = peff->get_value(this, 0);
		if((redirect & LOCATION_HAND) && !is_affected_by_effect(EFFECT_CANNOT_TO_HAND) && pduel->game_field->is_player_can_send_to_hand(peff->get_handler_player(), this))
			return redirect;
		if((redirect & LOCATION_DECK) && !is_affected_by_effect(EFFECT_CANNOT_TO_DECK) && pduel->game_field->is_player_can_send_to_deck(peff->get_handler_player(), this))
			return redirect;
		if((redirect & LOCATION_REMOVED) && !is_affected_by_effect(EFFECT_CANNOT_REMOVE) && pduel->game_field->is_player_can_remove(peff->get_handler_player(), this, REASON_EFFECT))
			return redirect;
		if((redirect & LOCATION_GRAVE) && !is_affected_by_effect(EFFECT_CANNOT_TO_GRAVE) && pduel->game_field->is_player_can_send_to_grave(peff->get_handler_player(), this))
			return redirect;
	}
	return 0;
}
// cmit->second[0]: permanent
// cmit->second[1]: reset while negated
int32_t card::add_counter(uint8_t playerid, uint16_t countertype, uint16_t count, uint8_t singly) {
	if(!is_can_add_counter(playerid, countertype, count, singly, 0))
		return FALSE;
	uint16_t cttype = countertype & ~COUNTER_NEED_ENABLE;
	auto pr = counters.emplace(cttype, counter_map::mapped_type());
	auto cmit = pr.first;
	if(pr.second) {
		cmit->second[0] = 0;
		cmit->second[1] = 0;
	}
	uint16_t pcount = count;
	if(singly) {
		effect_set eset;
		uint16_t limit = 0;
		filter_effect(EFFECT_COUNTER_LIMIT + cttype, &eset);
		for(const auto& peffect : eset)
			limit = peffect->get_value();
		if(limit) {
			uint16_t mcount = limit - get_counter(cttype);
			if(pcount > mcount)
				pcount = mcount;
		}
	}
	if((countertype & COUNTER_WITHOUT_PERMIT) && !(countertype & COUNTER_NEED_ENABLE))
		cmit->second[0] += pcount;
	else
		cmit->second[1] += pcount;
	auto message = pduel->new_message(MSG_ADD_COUNTER);
	message->write<uint16_t>(cttype);
	message->write<uint8_t>(current.controler);
	message->write<uint8_t>(current.location);
	message->write<uint8_t>(current.sequence);
	message->write<uint16_t>(pcount);
	pduel->game_field->raise_single_event(this, 0, EVENT_ADD_COUNTER + countertype, pduel->game_field->core.reason_effect, REASON_EFFECT, playerid, playerid, pcount);
	pduel->game_field->process_single_event();
	return TRUE;
}
int32_t card::remove_counter(uint16_t countertype, uint16_t count) {
	auto cmit = counters.find(countertype);
	if(cmit == counters.end())
		return FALSE;
	if(cmit->second[1] <= count) {
		uint16_t remains = count;
		remains -= cmit->second[1];
		cmit->second[1] = 0;
		if(cmit->second[0] <= remains)
			counters.erase(cmit);
		else cmit->second[0] -= remains;
	} else {
		cmit->second[1] -= count;
	}
	auto message = pduel->new_message(MSG_REMOVE_COUNTER);
	message->write<uint16_t>(countertype);
	message->write<uint8_t>(current.controler);
	message->write<uint8_t>(current.location);
	message->write<uint8_t>(current.sequence);
	message->write<uint16_t>(count);
	return TRUE;
}
int32_t card::is_can_add_counter(uint8_t playerid, uint16_t countertype, uint16_t count, uint8_t singly, uint32_t loc) {
	effect_set eset;
	if(count > 0) {
		if(!pduel->game_field->is_player_can_place_counter(playerid, this, countertype, count))
			return FALSE;
		if(!loc && (!(current.location & LOCATION_ONFIELD) || !is_position(POS_FACEUP)))
			return FALSE;
		if((countertype & COUNTER_NEED_ENABLE) && is_status(STATUS_DISABLED))
			return FALSE;
	}
	uint32_t check = countertype & COUNTER_WITHOUT_PERMIT;
	if(!check) {
		filter_effect(EFFECT_COUNTER_PERMIT + (countertype & 0xffff), &eset);
		for(const auto& peffect : eset) {
			uint32_t prange = peffect->get_value();
			if(loc)
				check = loc & prange;
			else if(current.location & LOCATION_ONFIELD) {
				uint32_t filter = TRUE;
				if(peffect->target) {
					pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
					pduel->lua->add_param(this, PARAM_TYPE_CARD);
					filter = pduel->lua->check_condition(peffect->target, 2);
				}
				check = current.is_location(prange) && is_position(POS_FACEUP) && filter;
			} else
				check = TRUE;
			if(check)
				break;
		}
		eset.clear();
	}
	if(!check)
		return FALSE;
	uint16_t cttype = countertype & ~COUNTER_NEED_ENABLE;
	int32_t limit = -1;
	int32_t cur = 0;
	auto cmit = counters.find(cttype);
	if(cmit != counters.end())
		cur = cmit->second[0] + cmit->second[1];
	filter_effect(EFFECT_COUNTER_LIMIT + cttype, &eset);
	for(const auto& peffect : eset)
		limit = peffect->get_value();
	if(limit > 0 && (cur + (singly ? 1 : count) > limit))
		return FALSE;
	return TRUE;
}
int32_t card::get_counter(uint16_t countertype) {
	auto cmit = counters.find(countertype);
	if(cmit == counters.end())
		return 0;
	return cmit->second[0] + cmit->second[1];
}
void card::set_material(card_set* materials) {
	if(!materials) {
		material_cards.clear();
	} else
		material_cards = *materials;
	for(auto& pcard : material_cards)
		pcard->current.reason_card = this;
	effect_set eset;
	filter_effect(EFFECT_MATERIAL_CHECK, &eset);
	for(const auto& peffect : eset) {
		peffect->get_value(this);
	}
}
void card::add_card_target(card* pcard) {
	effect_target_cards.insert(pcard);
	pcard->effect_target_owner.insert(this);
	for(auto& it : target_effect) {
		if(it.second->is_disable_related())
			pduel->game_field->add_to_disable_check_list(pcard);
	}
	auto message = pduel->new_message(MSG_CARD_TARGET);
	message->write(get_info_location());
	message->write(pcard->get_info_location());
}
void card::cancel_card_target(card* pcard) {
	auto cit = effect_target_cards.find(pcard);
	if(cit != effect_target_cards.end()) {
		effect_target_cards.erase(cit);
		pcard->effect_target_owner.erase(this);
		for(auto& it : target_effect) {
			if(it.second->is_disable_related())
				pduel->game_field->add_to_disable_check_list(pcard);
		}
		auto message = pduel->new_message(MSG_CANCEL_TARGET);
		message->write(get_info_location());
		message->write(pcard->get_info_location());
	}
}
void card::clear_card_target() {
	for(auto& pcard : effect_target_owner) {
		pcard->effect_target_cards.erase(this);
		for(auto& it : pcard->target_effect) {
			if(it.second->is_disable_related())
				pduel->game_field->add_to_disable_check_list(this);
		}
	}
	for(auto& pcard : effect_target_cards) {
		pcard->effect_target_owner.erase(this);
		for(auto& it : target_effect) {
			if(it.second->is_disable_related())
				pduel->game_field->add_to_disable_check_list(pcard);
		}
		for(auto it = pcard->single_effect.begin(); it != pcard->single_effect.end();) {
			auto rm = it++;
			effect* peffect = rm->second;
			if((peffect->owner == this) && peffect->is_flag(EFFECT_FLAG_OWNER_RELATE))
				pcard->remove_effect(peffect, rm);
		}
	}
	effect_target_owner.clear();
	effect_target_cards.clear();
}
void card::filter_effect(int32_t code, effect_set* eset, uint8_t sort) {
	effect* peffect;
	auto rg = single_effect.equal_range(code);
	for(auto eit = rg.first; eit != rg.second;) {
		peffect = eit->second;
		++eit;
		if(peffect->is_available() && (!peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE) || is_affect_by_effect(peffect)))
			eset->push_back(peffect);
	}
	for (auto& pcard : equiping_cards) {
		rg = pcard->equip_effect.equal_range(code);
		for(auto eit = rg.first; eit != rg.second;) {
			peffect = eit->second;
			++eit;
			if (peffect->is_available() && is_affect_by_effect(peffect))
				eset->push_back(peffect);
		}
	}
	for(auto& pcard : effect_target_owner) {
		rg = pcard->target_effect.equal_range(code);
		for(auto eit = rg.first; eit != rg.second;) {
			peffect = eit->second;
			++eit;
			if(peffect->is_available() && peffect->is_target(this) && is_affect_by_effect(peffect))
				eset->push_back(peffect);
		}
	}
	for (auto& pcard : xyz_materials) {
		rg = pcard->xmaterial_effect.equal_range(code);
		for(auto eit = rg.first; eit != rg.second;) {
			peffect = eit->second;
			++eit;
			if (peffect->type & EFFECT_TYPE_FIELD)
				continue;
			if (peffect->is_available() && is_affect_by_effect(peffect))
				eset->push_back(peffect);
		}
	}
	rg = pduel->game_field->effects.aura_effect.equal_range(code);
	for(auto eit = rg.first; eit != rg.second;) {
		peffect = eit->second;
		++eit;
		if (!peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET) && peffect->is_available()
						&& peffect->is_target(this) && is_affect_by_effect(peffect))
			eset->push_back(peffect);
	}
	if(sort)
		std::sort(eset->begin(), eset->end(), effect_sort_id);
}
void card::filter_single_effect(int32_t code, effect_set* eset, uint8_t sort) {
	effect* peffect;
	auto rg = single_effect.equal_range(code);
	for (; rg.first != rg.second; ++rg.first) {
		peffect = rg.first->second;
		if (peffect->is_available() && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE))
			eset->push_back(peffect);
	}
	if(sort)
		std::sort(eset->begin(), eset->end(), effect_sort_id);
}
void card::filter_single_continuous_effect(int32_t code, effect_set* eset, uint8_t sort) {
	auto rg = single_effect.equal_range(code);
	for (; rg.first != rg.second; ++rg.first)
		eset->push_back(rg.first->second);
	for (auto& pcard : equiping_cards) {
		rg = pcard->equip_effect.equal_range(code);
		for (; rg.first != rg.second; ++rg.first)
			eset->push_back(rg.first->second);
	}
	for(auto& pcard : effect_target_owner) {
		rg = pcard->target_effect.equal_range(code);
		for(; rg.first != rg.second; ++rg.first) {
			effect* peffect = rg.first->second;
			if(peffect->is_target(pcard))
				eset->push_back(peffect);
		}
	}
	for (auto& pcard : xyz_materials) {
		rg = pcard->xmaterial_effect.equal_range(code);
		for (; rg.first != rg.second; ++rg.first) {
			effect* peffect = rg.first->second;
			if (peffect->type & EFFECT_TYPE_FIELD)
				continue;
			eset->push_back(peffect);
		}
	}
	if(sort)
		std::sort(eset->begin(), eset->end(), effect_sort_id);
}
// refresh this->immune_effect
void card::filter_immune_effect() {
	effect* peffect;
	immune_effect.clear();
	auto rg = single_effect.equal_range(EFFECT_IMMUNE_EFFECT);
	for (; rg.first != rg.second; ++rg.first) {
		immune_effect.push_back(rg.first->second);
	}
	for (auto& pcard : equiping_cards) {
		rg = pcard->equip_effect.equal_range(EFFECT_IMMUNE_EFFECT);
		for (; rg.first != rg.second; ++rg.first) {
			immune_effect.push_back(rg.first->second);
		}
	}
	for (auto& pcard : effect_target_owner) {
		rg = pcard->target_effect.equal_range(EFFECT_IMMUNE_EFFECT);
		for (; rg.first != rg.second; ++rg.first) {
			peffect = rg.first->second;
			if(peffect->is_target(this))
				immune_effect.push_back(peffect);
		}
	}
	for (auto& pcard : xyz_materials) {
		rg = pcard->xmaterial_effect.equal_range(EFFECT_IMMUNE_EFFECT);
		for (; rg.first != rg.second; ++rg.first) {
			peffect = rg.first->second;
			if (peffect->type & EFFECT_TYPE_FIELD)
				continue;
			immune_effect.push_back(peffect);
		}
	}
	rg = pduel->game_field->effects.aura_effect.equal_range(EFFECT_IMMUNE_EFFECT);
	for (; rg.first != rg.second; ++rg.first) {
		peffect = rg.first->second;
		if (peffect->is_target(this))
			immune_effect.push_back(peffect);
	}
	std::sort(immune_effect.begin(), immune_effect.end(), effect_sort_id);
}
// for all disable-related peffect of this,
// 1. put all cards in the target of peffect into effects.disable_check_set, effects.disable_check_list
// 2. add equiping_target of peffect into effects.disable_check_set, effects.disable_check_list
void card::filter_disable_related_cards() {
	for (auto& it : indexer) {
		effect* peffect = it.first;
		if (peffect->is_disable_related()) {
			if (peffect->type & EFFECT_TYPE_FIELD)
				pduel->game_field->update_disable_check_list(peffect);
			else if ((peffect->type & EFFECT_TYPE_EQUIP) && equiping_target)
				pduel->game_field->add_to_disable_check_list(equiping_target);
			else if((peffect->type & EFFECT_TYPE_TARGET) && !effect_target_cards.empty()) {
				for(auto& target : effect_target_cards)
					pduel->game_field->add_to_disable_check_list(target);
			} else if((peffect->type & EFFECT_TYPE_XMATERIAL) && overlay_target)
				pduel->game_field->add_to_disable_check_list(overlay_target);
		}
	}
}
// put all summon procedures except ordinay summon in peset (see is_can_be_summoned())
// return value:
// -2 = this has a EFFECT_LIMIT_SUMMON_PROC, 0 available
// -1 = this has a EFFECT_LIMIT_SUMMON_PROC, at least 1 available
// 0 = no EFFECT_LIMIT_SUMMON_PROC, and ordinary summon ia not available
// 1 = no EFFECT_LIMIT_SUMMON_PROC, and ordinary summon ia available
int32_t card::filter_summon_procedure(uint8_t playerid, effect_set* peset, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone) {
	effect_set eset;
	filter_effect(EFFECT_LIMIT_SUMMON_PROC, &eset);
	if(eset.size()) {
		for(const auto& peffect : eset) {
			if(check_summon_procedure(peffect, playerid, ignore_count, min_tribute, zone))
				peset->push_back(peffect);
		}
		if(peset->size())
			return -1;
		return -2;
	}
	eset.clear();
	filter_effect(EFFECT_SUMMON_PROC, &eset);
	for(const auto& peffect : eset) {
		if(check_summon_procedure(peffect, playerid, ignore_count, min_tribute, zone))
			peset->push_back(peffect);
	}
	// ordinary summon
	if(!pduel->game_field->is_player_can_summon(SUMMON_TYPE_NORMAL, playerid, this, playerid))
		return FALSE;
	if(pduel->game_field->check_unique_onfield(this, playerid, LOCATION_MZONE))
		return FALSE;
	int32_t rcount = get_summon_tribute_count();
	int32_t min = rcount & 0xffff;
	int32_t max = (rcount >> 16) & 0xffff;
	if(!pduel->game_field->is_player_can_summon(SUMMON_TYPE_ADVANCE, playerid, this, playerid))
		max = 0;
	if(min < min_tribute)
		min = min_tribute;
	if(max < min)
		return FALSE;
	if(!ignore_count && !pduel->game_field->core.extra_summon[playerid]
			&& pduel->game_field->core.summon_count[playerid] >= pduel->game_field->get_summon_count_limit(playerid)) {
		eset.clear();
		filter_effect(EFFECT_EXTRA_SUMMON_COUNT, &eset);
		for(const auto& peffect : eset) {
			std::vector<int32_t> retval;
			peffect->get_value(this, 0, &retval);
			int32_t new_min = retval.size() > 0 ? retval[0] : 0;
			int32_t new_zone = retval.size() > 1 ? retval[1] : 0x1f;
			int32_t releasable = retval.size() > 2 ? (retval[2] < 0 ? 0xff00ff + retval[2] : retval[2]) : 0xff00ff;
			if(new_min < min)
				new_min = min;
			new_zone &= zone;
			if(pduel->game_field->check_tribute(this, new_min, max, 0, current.controler, new_zone, releasable))
				return TRUE;
		}
	} else
		return pduel->game_field->check_tribute(this, min, max, 0, current.controler, zone);
	return FALSE;
}
int32_t card::check_summon_procedure(effect* peffect, uint8_t playerid, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone) {
	if(!peffect->check_count_limit(playerid))
		return FALSE;
	uint8_t toplayer = playerid;
	if(peffect->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
		if(peffect->o_range)
			toplayer = 1 - playerid;
	}
	if(!pduel->game_field->is_player_can_summon(peffect->get_value(this), playerid, this, toplayer))
		return FALSE;
	if(pduel->game_field->check_unique_onfield(this, toplayer, LOCATION_MZONE))
		return FALSE;
	// the script will check min_tribute, Duel.CheckTribute()
	if(!ignore_count && !pduel->game_field->core.extra_summon[playerid]
			&& pduel->game_field->core.summon_count[playerid] >= pduel->game_field->get_summon_count_limit(playerid)) {
		effect_set eset;
		filter_effect(EFFECT_EXTRA_SUMMON_COUNT, &eset);
		for(const auto& peff : eset) {
			std::vector<int32_t> retval;
			peff->get_value(this, 0, &retval);
			int32_t new_min_tribute = retval.size() > 0 ? retval[0] : 0;
			int32_t new_zone = retval.size() > 1 ? retval[1] : 0x1f001f;
			int32_t releasable = retval.size() > 2 ? (retval[2] < 0 ? 0xff00ff + retval[2] : retval[2]) : 0xff00ff;
			if(new_min_tribute < (int32_t)min_tribute)
				new_min_tribute = min_tribute;
			if (peffect->is_flag(EFFECT_FLAG_SPSUM_PARAM) && peffect->o_range)
				new_zone = (new_zone >> 16) | (new_zone & 0xffff << 16);
			new_zone &= zone;
			if(is_summonable(peffect, new_min_tribute, new_zone, releasable, peffect))
				return TRUE;
		}
	} else
		return is_summonable(peffect, min_tribute, zone);
	return FALSE;
}
// put all set procedures except ordinay set in peset (see is_can_be_summoned())
int32_t card::filter_set_procedure(uint8_t playerid, effect_set* peset, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone) {
	effect_set eset;
	filter_effect(EFFECT_LIMIT_SET_PROC, &eset);
	if(eset.size()) {
		for(const auto& peffect : eset) {
			if(check_set_procedure(peffect, playerid, ignore_count, min_tribute, zone))
				peset->push_back(peffect);
		}
		if(peset->size())
			return -1;
		return -2;
	}
	eset.clear();
	filter_effect(EFFECT_SET_PROC, &eset);
	for(const auto& peffect : eset) {
		if(check_set_procedure(peffect, playerid, ignore_count, min_tribute, zone))
			peset->push_back(peffect);
	}
	if(!pduel->game_field->is_player_can_mset(SUMMON_TYPE_NORMAL, playerid, this, playerid))
		return FALSE;
	int32_t rcount = get_set_tribute_count();
	int32_t min = rcount & 0xffff;
	int32_t max = (rcount >> 16) & 0xffff;
	if(!pduel->game_field->is_player_can_mset(SUMMON_TYPE_ADVANCE, playerid, this, playerid))
		max = 0;
	if(min < min_tribute)
		min = min_tribute;
	if(max < min)
		return FALSE;
	if(!ignore_count && !pduel->game_field->core.extra_summon[playerid]
			&& pduel->game_field->core.summon_count[playerid] >= pduel->game_field->get_summon_count_limit(playerid)) {
		eset.clear();
		filter_effect(EFFECT_EXTRA_SET_COUNT, &eset);
		for(const auto& peff : eset) {
			std::vector<int32_t> retval;
			peff->get_value(this, 0, &retval);
			int32_t new_min = retval.size() > 0 ? retval[0] : 0;
			int32_t new_zone = retval.size() > 1 ? retval[1] : 0x1f;
			int32_t releasable = retval.size() > 2 ? (retval[2] < 0 ? 0xff00ff + retval[2] : retval[2]) : 0xff00ff;
			if(new_min < min)
				new_min = min;
			new_zone &= zone;
			if(pduel->game_field->check_tribute(this, new_min, max, 0, current.controler, new_zone, releasable, POS_FACEDOWN_DEFENSE))
				return TRUE;
		}
	} else
		return pduel->game_field->check_tribute(this, min, max, 0, current.controler, zone, 0xff00ff, POS_FACEDOWN_DEFENSE);
	return FALSE;
}
int32_t card::check_set_procedure(effect* peffect, uint8_t playerid, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone) {
	if(!peffect->check_count_limit(playerid))
		return FALSE;
	uint8_t toplayer = playerid;
	if(peffect->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
		if(peffect->o_range)
			toplayer = 1 - playerid;
	}
	if(!pduel->game_field->is_player_can_mset(peffect->get_value(this), playerid, this, toplayer))
		return FALSE;
	if(!ignore_count && !pduel->game_field->core.extra_summon[playerid]
			&& pduel->game_field->core.summon_count[playerid] >= pduel->game_field->get_summon_count_limit(playerid)) {
		effect_set eset;
		filter_effect(EFFECT_EXTRA_SET_COUNT, &eset);
		for(const auto& peff : eset) {
			std::vector<int32_t> retval;
			peff->get_value(this, 0, &retval);
			int32_t new_min_tribute = retval.size() > 0 ? retval[0] : 0;
			int32_t new_zone = retval.size() > 1 ? retval[1] : 0x1f001f;
			int32_t releasable = retval.size() > 2 ? (retval[2] < 0 ? 0xff00ff + retval[2] : retval[2]) : 0xff00ff;
			if (peffect->is_flag(EFFECT_FLAG_SPSUM_PARAM) && peffect->o_range)
				new_zone = (new_zone >> 16) | (new_zone & 0xffff << 16);
			if(new_min_tribute < (int32_t)min_tribute)
				new_min_tribute = min_tribute;
			new_zone &= zone;
			if(is_summonable(peffect, new_min_tribute, new_zone, releasable, peff))
				return TRUE;
		}
	} else
		return is_summonable(peffect, min_tribute, zone);
	return FALSE;
}
void card::filter_spsummon_procedure(uint8_t playerid, effect_set* peset, uint32_t summon_type) {
	auto pr = field_effect.equal_range(EFFECT_SPSUMMON_PROC);
	uint8_t toplayer;
	uint8_t topos;
	for(auto eit = pr.first; eit != pr.second;) {
		effect* peffect = eit->second;
		++eit;
		if(peffect->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
			topos = (uint8_t)peffect->s_range;
			if(peffect->o_range == 0)
				toplayer = playerid;
			else
				toplayer = 1 - playerid;
		} else {
			topos = POS_FACEUP;
			toplayer = playerid;
		}
		if(!peffect->is_available() || !peffect->check_count_limit(playerid))
			continue;
		if(is_spsummonable(peffect)
				&& ((topos & POS_FACEDOWN) || !pduel->game_field->check_unique_onfield(this, toplayer, LOCATION_MZONE))) {
			effect* sumeffect = pduel->game_field->core.reason_effect;
			if(!sumeffect)
				sumeffect = peffect;
			std::vector<int32_t> retval;
			peffect->get_value(this, 0, &retval);
			uint32_t sumtype = retval.size() > 0 ? retval[0] : 0;
			uint32_t zone = retval.size() > 1 ? retval[1] : 0xff;
			if(zone != 0xff && pduel->game_field->get_useable_count(this, toplayer, LOCATION_MZONE, playerid, LOCATION_REASON_TOFIELD, zone, nullptr) <= 0)
				continue;
			if(summon_type != 0 && summon_type != sumtype)
				continue;
			if(!pduel->game_field->is_player_can_spsummon(sumeffect, sumtype, topos, playerid, toplayer, this))
				continue;
			peset->push_back(peffect);
		}
	}
}
void card::filter_spsummon_procedure_g(uint8_t playerid, effect_set* peset) {
	auto pr = field_effect.equal_range(EFFECT_SPSUMMON_PROC_G);
	for(auto eit = pr.first; eit != pr.second;) {
		effect* peffect = eit->second;
		++eit;
		if(!peffect->is_available() || !peffect->check_count_limit(playerid))
			continue;
		if(current.controler != playerid && !peffect->is_flag(EFFECT_FLAG_BOTH_SIDE))
			continue;
		effect* oreason = pduel->game_field->core.reason_effect;
		uint8_t op = pduel->game_field->core.reason_player;
		pduel->game_field->core.reason_effect = peffect;
		pduel->game_field->core.reason_player = this->current.controler;
		pduel->game_field->save_lp_cost();
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		if(pduel->lua->check_condition(peffect->condition, 2))
			peset->push_back(peffect);
		pduel->game_field->restore_lp_cost();
		pduel->game_field->core.reason_effect = oreason;
		pduel->game_field->core.reason_player = op;
	}
}
// return: an effect with code which affects this or 0
effect* card::is_affected_by_effect(int32_t code) {
	effect* peffect;
	auto rg = single_effect.equal_range(code);
	for(auto eit = rg.first; eit != rg.second;) {
		peffect = eit->second;
		++eit;
		if (peffect->is_available() && (!peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE) || is_affect_by_effect(peffect)))
			return peffect;
	}
	for (auto& pcard : equiping_cards) {
		rg = pcard->equip_effect.equal_range(code);
		for(auto eit = rg.first; eit != rg.second;) {
			peffect = eit->second;
			++eit;
			if (peffect->is_available() && is_affect_by_effect(peffect))
				return peffect;
		}
	}
	for (auto& pcard : effect_target_owner) {
		rg = pcard->target_effect.equal_range(code);
		for(auto eit = rg.first; eit != rg.second;) {
			peffect = eit->second;
			++eit;
			if (peffect->is_available() && peffect->is_target(this) && is_affect_by_effect(peffect))
				return peffect;
		}
	}
	for (auto& pcard : xyz_materials) {
		rg = pcard->xmaterial_effect.equal_range(code);
		for(auto eit = rg.first; eit != rg.second;) {
			peffect = eit->second;
			++eit;
			if (peffect->type & EFFECT_TYPE_FIELD)
				continue;
			if (peffect->is_available() && is_affect_by_effect(peffect))
				return peffect;
		}
	}
	rg = pduel->game_field->effects.aura_effect.equal_range(code);
	for(auto eit = rg.first; eit != rg.second;) {
		peffect = eit->second;
		++eit;
		if (!peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET) && peffect->is_target(this)
			&& peffect->is_available() && is_affect_by_effect(peffect))
			return peffect;
	}
	return 0;
}
effect* card::is_affected_by_effect(int32_t code, card* target) {
	effect* peffect;
	auto rg = single_effect.equal_range(code);
	for(auto eit = rg.first; eit != rg.second;) {
		peffect = eit->second;
		++eit;
		if (peffect->is_available() && (!peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE) || is_affect_by_effect(peffect))
						&& peffect->get_value(target))
			return peffect;
	}
	for (auto& pcard : equiping_cards) {
		rg = pcard->equip_effect.equal_range(code);
		for(auto eit = rg.first; eit != rg.second;) {
			peffect = eit->second;
			++eit;
			if (peffect->is_available() && is_affect_by_effect(peffect) && peffect->get_value(target))
				return peffect;
		}
	}
	for (auto& pcard : equiping_cards) {
		rg = pcard->equip_effect.equal_range(code);
		for(auto eit = rg.first; eit != rg.second;) {
			peffect = eit->second;
			++eit;
			if (peffect->type & EFFECT_TYPE_FIELD)
				continue;
			if (peffect->is_available() && is_affect_by_effect(peffect) && peffect->get_value(target))
				return peffect;
		}
	}
	for (auto& pcard : effect_target_owner) {
		rg = pcard->target_effect.equal_range(code);
		for(auto eit = rg.first; eit != rg.second;) {
			peffect = eit->second;
			++eit;
			if (peffect->is_available() && peffect->is_target(this) && is_affect_by_effect(peffect) && peffect->get_value(target))
				return peffect;
		}
	}
	for (auto& pcard : xyz_materials) {
		rg = pcard->xmaterial_effect.equal_range(code);
		for(auto eit = rg.first; eit != rg.second;) {
			peffect = eit->second;
			++eit;
			if (peffect->type & EFFECT_TYPE_FIELD)
				continue;
			if (peffect->is_available() && is_affect_by_effect(peffect) && peffect->get_value(target))
				return peffect;
		}
	}
	rg = pduel->game_field->effects.aura_effect.equal_range(code);
	for(auto eit = rg.first; eit != rg.second;) {
		peffect = eit->second;
		++eit;
		if (!peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET) && peffect->is_available()
						&& peffect->is_target(this) && is_affect_by_effect(peffect) && peffect->get_value(target))
			return peffect;
	}
	return 0;
}
int32_t card::get_card_effect(uint32_t code) {
	effect* peffect;
	int32_t i = 0;
	for (auto rg = single_effect.begin(); rg != single_effect.end();) {
		peffect = rg->second;
		rg++;
		if ((code == 0 || peffect->code == code) && peffect->is_available() && (!peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE) || is_affect_by_effect(peffect))) {
			interpreter::pushobject(pduel->lua->current_state, peffect);
			i++;
		}
	}
	for (auto rg = field_effect.begin(); rg != field_effect.end(); ++rg) {
		peffect = rg->second;
		if ((code == 0 || peffect->code == code) && is_affect_by_effect(peffect)) {
			interpreter::pushobject(pduel->lua->current_state, peffect);
			i++;
		}
	}
	for (auto cit = equiping_cards.begin(); cit != equiping_cards.end(); ++cit) {
		for (auto rg = (*cit)->equip_effect.begin(); rg != (*cit)->equip_effect.end();) {
			peffect = rg->second;
			rg++;
			if ((code == 0 || peffect->code == code) && peffect->is_available() && is_affect_by_effect(peffect)) {
				interpreter::pushobject(pduel->lua->current_state, peffect);
				i++;
			}
		}
	}
	for(auto& pcard : effect_target_owner) {
		auto rg = pcard->target_effect.equal_range(code);
		for(auto eit = rg.first; eit != rg.second;) {
			peffect = eit->second;
			++eit;
			if((code == 0 || peffect->code == code) && peffect->is_available() && peffect->is_target(this) && is_affect_by_effect(peffect)) {
				interpreter::pushobject(pduel->lua->current_state, peffect);
				i++;
			}
		}
	}
	for (auto cit = xyz_materials.begin(); cit != xyz_materials.end(); ++cit) {
		for (auto rg = (*cit)->xmaterial_effect.begin(); rg != (*cit)->xmaterial_effect.end();) {
			peffect = rg->second;
			rg++;
			if (peffect->type & EFFECT_TYPE_FIELD)
				continue;
			if ((code == 0 || peffect->code == code) && peffect->is_available() && is_affect_by_effect(peffect)) {
				interpreter::pushobject(pduel->lua->current_state, peffect);
				i++;
			}
		}
	}
	for (auto rg = pduel->game_field->effects.aura_effect.begin(); rg != pduel->game_field->effects.aura_effect.end();) {
		peffect = rg->second;
		rg++;
		if ((code == 0 || peffect->code == code) && !peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET) && peffect->is_target(this)
			&& peffect->is_available() && is_affect_by_effect(peffect)) {
			interpreter::pushobject(pduel->lua->current_state, peffect);
			i++;
		}
	}
	return i;
}
int32_t card::fusion_check(group* fusion_m, group* cg, uint32_t chkf) {
	effect* peffect = 0;
	auto ecit = single_effect.find(EFFECT_FUSION_MATERIAL);
	for (; ecit != single_effect.end(); ++ecit) {
		peffect = ecit->second;
		if (!peffect->condition || peffect->code != EFFECT_FUSION_MATERIAL)
			continue;
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(fusion_m, PARAM_TYPE_GROUP);
		pduel->lua->add_param(cg, PARAM_TYPE_GROUP);
		pduel->lua->add_param(chkf, PARAM_TYPE_INT);
		effect* oreason = pduel->game_field->core.reason_effect;
		uint8_t op = pduel->game_field->core.reason_player;
		pduel->game_field->core.reason_effect = peffect;
		pduel->game_field->core.reason_player = peffect->get_handler_player();
		int32_t res = pduel->lua->check_condition(peffect->condition, 4);
		pduel->game_field->core.reason_effect = oreason;
		pduel->game_field->core.reason_player = op;
		if(res)
			return TRUE;
	}
	return FALSE;
}
void card::fusion_filter_valid(group* fusion_m, group* cg, uint32_t chkf, effect_set* eset) {
	effect* peffect = 0;
	auto ecit = single_effect.find(EFFECT_FUSION_MATERIAL);
	for (; ecit != single_effect.end(); ++ecit) {
		peffect = ecit->second;
		if (!peffect->condition || peffect->code != EFFECT_FUSION_MATERIAL)
			continue;
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(fusion_m, PARAM_TYPE_GROUP);
		pduel->lua->add_param(cg, PARAM_TYPE_GROUP);
		pduel->lua->add_param(chkf, PARAM_TYPE_INT);
		effect* oreason = pduel->game_field->core.reason_effect;
		uint8_t op = pduel->game_field->core.reason_player;
		pduel->game_field->core.reason_effect = peffect;
		pduel->game_field->core.reason_player = peffect->get_handler_player();
		int32_t res = pduel->lua->check_condition(peffect->condition, 4);
		pduel->game_field->core.reason_effect = oreason;
		pduel->game_field->core.reason_player = op;
		if (res)
			eset->push_back(peffect);
	}
}
int32_t card::check_fusion_substitute(card* fcard) {
	effect_set eset;
	filter_effect(EFFECT_FUSION_SUBSTITUTE, &eset);
	if(eset.size() == 0)
		return FALSE;
	for(const auto& peffect : eset)
		if(!peffect->value || peffect->get_value(fcard))
			return TRUE;
	return FALSE;
}
int32_t card::is_not_tuner(card* scard, uint8_t playerid) {
	if(!(get_type(scard, SUMMON_TYPE_SYNCHRO, playerid) & TYPE_TUNER))
		return TRUE;
	effect_set eset;
	filter_effect(EFFECT_NONTUNER, &eset);
	for(const auto& peffect : eset)
		if(!peffect->value || peffect->get_value(scard))
			return TRUE;
	return FALSE;
}
int32_t card::check_unique_code(card* pcard) {
	if(!unique_code)
		return FALSE;
	if(unique_code == 1) {
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		return pduel->lua->get_function_value(unique_function, 1);
	}
	uint32_t code1 = pcard->get_code();
	uint32_t code2 = pcard->get_another_code();
	if(code1 == unique_code || (code2 && code2 == unique_code))
		return TRUE;
	return FALSE;
}
void card::get_unique_target(card_set* cset, int32_t controler, card* icard) {
	cset->clear();
	for(int32_t p = 0; p < 2; ++p) {
		if(!unique_pos[p])
			continue;
		const auto& player = pduel->game_field->player[controler ^ p];
		if(unique_location & LOCATION_MZONE) {
			for(auto& pcard : player.list_mzone) {
				if(pcard && (pcard != icard) && pcard->is_position(POS_FACEUP) && !pcard->get_status(STATUS_SPSUMMON_STEP)
					&& check_unique_code(pcard))
					cset->insert(pcard);
			}
		}
		if(unique_location & LOCATION_SZONE) {
			for(auto& pcard : player.list_szone) {
				if(pcard && (pcard != icard) && pcard->is_position(POS_FACEUP) && check_unique_code(pcard))
					cset->insert(pcard);
			}
		}
	}
}
int32_t card::check_cost_condition(int32_t ecode, int32_t playerid) {
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, ecode, &eset, FALSE);
	filter_effect(ecode, &eset);
	for(const auto& peffect : eset) {
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(!pduel->lua->check_condition(peffect->cost, 3))
			return FALSE;
	}
	return TRUE;
}
int32_t card::check_cost_condition(int32_t ecode, int32_t playerid, int32_t sumtype) {
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, ecode, &eset, FALSE);
	filter_effect(ecode, &eset);
	for(const auto& peffect : eset) {
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
		if(!pduel->lua->check_condition(peffect->cost, 4))
			return FALSE;
	}
	return TRUE;
}
// check if this is a normal summonable card
int32_t card::is_summonable_card() {
	if(!(data.type & TYPE_MONSTER) || (data.type & (TYPE_RITUAL | TYPE_SPSUMMON
					  | TYPE_FUSION | TYPE_SYNCHRO | TYPE_XYZ | TYPE_LINK
					  | TYPE_TOKEN | TYPE_TRAPMONSTER)))
		return FALSE;
	return !is_affected_by_effect(EFFECT_UNSUMMONABLE_CARD);
}
int32_t card::is_fusion_summonable_card(uint32_t summon_type) {
	if(!(data.type & TYPE_FUSION))
		return FALSE;
	summon_type |= SUMMON_TYPE_FUSION;
	effect_set eset;
	filter_effect(EFFECT_SPSUMMON_CONDITION, &eset);
	for(const auto& peffect : eset) {
		pduel->lua->add_param(pduel->game_field->core.reason_effect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pduel->game_field->core.reason_player, PARAM_TYPE_INT);
		pduel->lua->add_param(summon_type, PARAM_TYPE_INT);
		pduel->lua->add_param((void*)0, PARAM_TYPE_INT);
		pduel->lua->add_param((void*)0, PARAM_TYPE_INT);
		if(!peffect->check_value_condition(5))
			return FALSE;
	}
	return TRUE;
}
// check the condition of sp_summon procedure peffect
int32_t card::is_spsummonable(effect* peffect) {
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_effect = peffect;
	pduel->game_field->core.reason_player = this->current.controler;
	pduel->game_field->save_lp_cost();
	pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
	pduel->lua->add_param(this, PARAM_TYPE_CARD);
	pduel->lua->add_param(pduel->game_field->core.must_use_mats, PARAM_TYPE_GROUP);
	pduel->lua->add_param(pduel->game_field->core.only_use_mats, PARAM_TYPE_GROUP);
	uint32_t result = FALSE;
	uint32_t param_count = 4;
	if(pduel->game_field->core.forced_summon_minc) {
		pduel->lua->add_param(pduel->game_field->core.forced_summon_minc, PARAM_TYPE_INT);
		pduel->lua->add_param(pduel->game_field->core.forced_summon_maxc, PARAM_TYPE_INT);
		param_count += 2;
	}
	if (pduel->lua->check_condition(peffect->condition, param_count))
		result = TRUE;
	pduel->game_field->restore_lp_cost();
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	return result;
}
// check the condition of summon/set procedure peffect
int32_t card::is_summonable(effect* peffect, uint8_t min_tribute, uint32_t zone, uint32_t releasable, effect* exeffect) {
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_effect = peffect;
	pduel->game_field->core.reason_player = this->current.controler;
	pduel->game_field->save_lp_cost();
	uint32_t result = FALSE;
	pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
	pduel->lua->add_param(this, PARAM_TYPE_CARD);
	pduel->lua->add_param(min_tribute, PARAM_TYPE_INT);
	pduel->lua->add_param(zone, PARAM_TYPE_INT);
	pduel->lua->add_param(releasable, PARAM_TYPE_INT);
	pduel->lua->add_param(exeffect, PARAM_TYPE_EFFECT);
	if(pduel->lua->check_condition(peffect->condition, 6))
		result = TRUE;
	pduel->game_field->restore_lp_cost();
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	return result;
}
// if this does not have a summon procedure, it will check ordinary summon
// ignore_count: ignore the summon count in this turn or not
// peffect: effects that change the ordinary summon procedure (c80921533)
// min_tribute: the limit of min tribute number by EFFECT_EXTRA_SUMMON_COUNT
// return: whether playerid can summon this or not
int32_t card::is_can_be_summoned(uint8_t playerid, uint8_t ignore_count, effect* peffect, uint8_t min_tribute, uint32_t zone) {
	if(!is_summonable_card())
		return FALSE;
	{
		auto reason_effect = pduel->game_field->core.reason_effect;
		if(reason_effect && reason_effect->get_handler() == this)
			reason_effect->status |= EFFECT_STATUS_SUMMON_SELF;
	}
	if(!ignore_count && (pduel->game_field->core.extra_summon[playerid] || !is_affected_by_effect(EFFECT_EXTRA_SUMMON_COUNT))
					&& pduel->game_field->core.summon_count[playerid] >= pduel->game_field->get_summon_count_limit(playerid))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_SUMMON_COST, playerid)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	if(current.location == LOCATION_MZONE) {
		if(is_position(POS_FACEDOWN)
						|| !is_affected_by_effect(EFFECT_GEMINI_SUMMONABLE)
						|| is_affected_by_effect(EFFECT_GEMINI_STATUS)
						|| !pduel->game_field->is_player_can_summon(SUMMON_TYPE_GEMINI, playerid, this, playerid)
						|| is_affected_by_effect(EFFECT_CANNOT_SUMMON)) {
			pduel->game_field->restore_lp_cost();
			return FALSE;
		}
	} else if(current.location == LOCATION_HAND) {
		if(is_affected_by_effect(EFFECT_CANNOT_SUMMON)) {
			pduel->game_field->restore_lp_cost();
			return FALSE;
		}
		effect_set proc;
		int32_t res = filter_summon_procedure(playerid, &proc, ignore_count, min_tribute, zone);
		if(peffect) {
			if(res < 0 || !check_summon_procedure(peffect, playerid, ignore_count, min_tribute, zone)) {
				pduel->game_field->restore_lp_cost();
				return FALSE;
			}
		} else {
			if(!proc.size() && (!res || res == -2)) {
				pduel->game_field->restore_lp_cost();
				return FALSE;
			}
		}
	}
	pduel->game_field->restore_lp_cost();
	return TRUE;
}
// return: the min/max number of tribute for an ordinary advance summon of this
int32_t card::get_summon_tribute_count() {
	int32_t min = 0, max = 0;
	int32_t minul = 0, maxul = 0;
	int32_t level = get_level();
	if(level < 5)
		return 0;
	else if(level < 7)
		min = max = 1;
	else
		min = max = 2;
	effect_set eset;
	filter_effect(EFFECT_DECREASE_TRIBUTE, &eset);
	for(const auto& peffect : eset) {
		int32_t dec = peffect->get_value(this);
		if(!peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT)) {
			if(minul < (dec & 0xffff))
				minul = dec & 0xffff;
			if(maxul < (dec >> 16))
				maxul = dec >> 16;
		} else if(peffect->count_limit > 0) {
			min -= dec & 0xffff;
			max -= dec >> 16;
		}
	}
	min -= minul;
	max -= maxul;
	if(min < 0) min = 0;
	if(max < min) max = min;
	return min + (max << 16);
}
int32_t card::get_set_tribute_count() {
	int32_t min = 0, max = 0;
	int32_t level = get_level();
	if(level < 5)
		return 0;
	else if(level < 7)
		min = max = 1;
	else
		min = max = 2;
	effect_set eset;
	filter_effect(EFFECT_DECREASE_TRIBUTE_SET, &eset);
	if(eset.size()) {
		int32_t dec = eset.back()->get_value(this);
		min -= dec & 0xffff;
		max -= dec >> 16;
	}
	if(min < 0) min = 0;
	if(max < min) max = min;
	return min + (max << 16);
}
int32_t card::is_can_be_flip_summoned(uint8_t playerid) {
	if(is_status(STATUS_FORM_CHANGED))
		return FALSE;
	if((is_status(STATUS_SUMMON_TURN) || is_status(STATUS_FLIP_SUMMON_TURN) || is_status(STATUS_SPSUMMON_TURN)) &&
		(summon_player == current.controler || !pduel->game_field->is_flag(DUEL_CAN_REPOS_IF_NON_SUMPLAYER)))
		return FALSE;
	if(announce_count > 0)
		return FALSE;
	if(current.location != LOCATION_MZONE)
		return FALSE;
	if(!(current.position & POS_FACEDOWN))
		return FALSE;
	if(pduel->game_field->check_unique_onfield(this, playerid, LOCATION_MZONE))
		return FALSE;
	if(!pduel->game_field->is_player_can_flipsummon(playerid, this))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_FLIP_SUMMON))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_CHANGE_POSITION))
		return FALSE;
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_FLIPSUMMON_COST, playerid)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	pduel->game_field->restore_lp_cost();
	return TRUE;
}
// check if this can be sp_summoned by EFFECT_SPSUMMON_PROC
// call filter_spsummon_procedure()
int32_t card::is_special_summonable(uint8_t playerid, uint32_t summon_type) {
	if(!(data.type & TYPE_MONSTER))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_SPECIAL_SUMMON))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_SPSUMMON_COST, playerid, summon_type)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	effect_set eset;
	filter_spsummon_procedure(playerid, &eset, summon_type);
	pduel->game_field->restore_lp_cost();
	return eset.size();
}
int32_t card::is_can_be_special_summoned(effect* reason_effect, uint32_t sumtype, uint8_t sumpos, uint8_t sumplayer, uint8_t toplayer, uint8_t nocheck, uint8_t nolimit, uint32_t zone) {
	if(reason_effect->get_handler() == this)
		reason_effect->status |= EFFECT_STATUS_SUMMON_SELF;
	if(current.location == LOCATION_MZONE)
		return FALSE;
	if(current.location == LOCATION_REMOVED && (current.position & POS_FACEDOWN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_REVIVE_LIMIT) && !is_status(STATUS_PROC_COMPLETE)) {
		if((!nolimit && (current.location & (LOCATION_GRAVE | LOCATION_REMOVED | LOCATION_SZONE)))
			|| (!nocheck && !nolimit && (current.location & (LOCATION_DECK | LOCATION_HAND))))
			return FALSE;
		if(!nolimit && (data.type & TYPE_PENDULUM) && current.location == LOCATION_EXTRA && (current.position & POS_FACEUP))
			return FALSE;
	}
	if((data.type & TYPE_PENDULUM) && current.location == LOCATION_EXTRA && (current.position & POS_FACEUP)
		&& (sumtype == SUMMON_TYPE_FUSION || sumtype == SUMMON_TYPE_SYNCHRO || sumtype == SUMMON_TYPE_XYZ))
		return FALSE;
	if((sumpos & POS_FACEDOWN) && pduel->game_field->is_player_affected_by_effect(sumplayer, EFFECT_DEVINE_LIGHT))
		sumpos = (sumpos & POS_FACEUP) | ((sumpos & POS_FACEDOWN) >> 1);
	if(!(sumpos & POS_FACEDOWN) && pduel->game_field->check_unique_onfield(this, toplayer, LOCATION_MZONE))
		return FALSE;
	sumtype |= SUMMON_TYPE_SPECIAL;
	if((sumplayer == 0 || sumplayer == 1) && !pduel->game_field->is_player_can_spsummon(reason_effect, sumtype, sumpos, sumplayer, toplayer, this))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_SPECIAL_SUMMON))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(zone != 0xff) {
		if(pduel->game_field->get_useable_count(this, toplayer, LOCATION_MZONE, sumplayer, LOCATION_REASON_TOFIELD, zone, 0) <= 0)
			return FALSE;
	}
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_SPSUMMON_COST, sumplayer, sumtype)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	if(!nocheck) {
		effect_set eset;
		if(!(data.type & TYPE_MONSTER)) {
			pduel->game_field->restore_lp_cost();
			return FALSE;
		}
		filter_effect(EFFECT_SPSUMMON_CONDITION, &eset);
		for(const auto& peffect : eset) {
			pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(sumplayer, PARAM_TYPE_INT);
			pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
			pduel->lua->add_param(sumpos, PARAM_TYPE_INT);
			pduel->lua->add_param(toplayer, PARAM_TYPE_INT);
			if(!peffect->check_value_condition(5)) {
				pduel->game_field->restore_lp_cost();
				return FALSE;
			}
		}
	}
	pduel->game_field->restore_lp_cost();
	return TRUE;
}
// if this does not have a set set procedure, it will check ordinary set (see is_can_be_summoned())
int32_t card::is_setable_mzone(uint8_t playerid, uint8_t ignore_count, effect* peffect, uint8_t min_tribute, uint32_t zone) {
	if(!is_summonable_card())
		return FALSE;
	{
		auto reason_effect = pduel->game_field->core.reason_effect;
		if(reason_effect && reason_effect->get_handler() == this)
			reason_effect->status |= EFFECT_STATUS_SUMMON_SELF;
	}
	if(current.location != LOCATION_HAND)
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_MSET))
		return FALSE;
	if(!ignore_count && (pduel->game_field->core.extra_summon[playerid] || !is_affected_by_effect(EFFECT_EXTRA_SET_COUNT))
					&& pduel->game_field->core.summon_count[playerid] >= pduel->game_field->get_summon_count_limit(playerid))
		return FALSE;
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_MSET_COST, playerid)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	effect_set eset;
	int32_t res = filter_set_procedure(playerid, &eset, ignore_count, min_tribute, zone);
	if(peffect) {
		if(res < 0 || !check_set_procedure(peffect, playerid, ignore_count, min_tribute, zone)) {
			pduel->game_field->restore_lp_cost();
			return FALSE;
		}
	} else {
		if(!eset.size() && (!res || res == -2)) {
			pduel->game_field->restore_lp_cost();
			return FALSE;
		}
	}
	pduel->game_field->restore_lp_cost();
	return TRUE;
}
int32_t card::is_setable_szone(uint8_t playerid, uint8_t ignore_fd) {
	if(!(data.type & TYPE_FIELD) && !ignore_fd && pduel->game_field->get_useable_count(this, current.controler, LOCATION_SZONE, current.controler, LOCATION_REASON_TOFIELD) <= 0)
		return FALSE;
	if(data.type & TYPE_MONSTER && !is_affected_by_effect(EFFECT_MONSTER_SSET))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_SSET))
		return FALSE;
	if(!pduel->game_field->is_player_can_sset(playerid, this))
		return FALSE;
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_SSET_COST, playerid)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	pduel->game_field->restore_lp_cost();
	return TRUE;
}
// return: this is affected by peffect or not
int32_t card::is_affect_by_effect(effect* peffect) {
	if(is_status(STATUS_SUMMONING) && (peffect && peffect->code != EFFECT_CANNOT_DISABLE_SUMMON && peffect->code != EFFECT_CANNOT_DISABLE_SPSUMMON))
		return FALSE;
	if(!peffect || peffect->is_flag(EFFECT_FLAG_IGNORE_IMMUNE))
		return TRUE;
	if(peffect->is_immuned(this))
		return FALSE;
	return TRUE;
}
int32_t card::is_destructable() {
	if(overlay_target)
		return FALSE;
	if(current.location & (LOCATION_GRAVE | LOCATION_REMOVED))
		return FALSE;
	return TRUE;
}
int32_t card::is_destructable_by_battle(card* pcard) {
	if(is_affected_by_effect(EFFECT_INDESTRUCTABLE_BATTLE, pcard))
		return FALSE;
	return TRUE;
}
effect* card::check_indestructable_by_effect(effect* peffect, uint8_t playerid) {
	if(!peffect)
		return 0;
	effect_set eset;
	filter_effect(EFFECT_INDESTRUCTABLE_EFFECT, &eset);
	for(const auto& peff : eset) {
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		if(peff->check_value_condition(3))
			return peff;
	}
	return 0;
}
int32_t card::is_destructable_by_effect(effect* peffect, uint8_t playerid) {
	if(!is_affect_by_effect(peffect))
		return FALSE;
	if(check_indestructable_by_effect(peffect, playerid))
		return FALSE;
	effect_set eset;
	eset.clear();
	filter_effect(EFFECT_INDESTRUCTABLE, &eset);
	for(const auto& peff : eset) {
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(REASON_EFFECT, PARAM_TYPE_INT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(peff->check_value_condition(3)) {
			return FALSE;
			break;
		}
	}
	eset.clear();
	filter_effect(EFFECT_INDESTRUCTABLE_COUNT, &eset);
	for(const auto& peff : eset) {
		if(peff->is_flag(EFFECT_FLAG_COUNT_LIMIT)) {
			if(peff->count_limit == 0)
				continue;
			pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(REASON_EFFECT, PARAM_TYPE_INT);
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			if(peff->check_value_condition(3)) {
				return FALSE;
				break;
			}
		} else {
			pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(REASON_EFFECT, PARAM_TYPE_INT);
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			int32_t ct = peff->get_value(3);
			if(ct) {
				auto it = indestructable_effects.emplace(peff->id, 0);
				if(it.first->second + 1 <= ct) {
					return FALSE;
					break;
				}
			}
		}
	}
	return TRUE;
}
int32_t card::is_removeable(uint8_t playerid, int32_t pos, uint32_t reason) {
	if(current.location == LOCATION_REMOVED)
		return FALSE;
	if(!pduel->game_field->is_player_can_remove(playerid, this, reason))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_REMOVE))
		return FALSE;
	if((data.type & TYPE_TOKEN) && (pos & POS_FACEDOWN))
		return FALSE;
	return TRUE;
}
int32_t card::is_removeable_as_cost(uint8_t playerid, int32_t pos) {
	uint32_t redirect = 0;
	uint32_t dest = LOCATION_REMOVED;
	if(current.location == LOCATION_REMOVED)
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_USE_AS_COST))
		return FALSE;
	if(!is_removeable(playerid, pos, REASON_COST))
		return FALSE;
	int32_t redirchk = FALSE;
	auto op_param = sendto_param;
	sendto_param.location = dest;
	if (current.location & LOCATION_ONFIELD)
		redirect = leave_field_redirect(REASON_COST) & 0xffff;
	if(redirect) {
		redirchk = TRUE;
		dest = redirect;
	}
	redirect = destination_redirect(dest, REASON_COST) & 0xffff;
	if(redirect) {
		redirchk = TRUE;
		dest = redirect;
	}
	sendto_param = op_param;
	if(dest != LOCATION_REMOVED || (redirchk && (pos & POS_FACEDOWN)))
		return FALSE;
	return TRUE;
}
int32_t card::is_releasable_by_summon(uint8_t playerid, card* pcard) {
	if(is_status(STATUS_SUMMONING))
		return FALSE;
	if(overlay_target)
		return FALSE;
	if(current.location & (LOCATION_GRAVE | LOCATION_REMOVED))
		return FALSE;
	if(!pduel->game_field->is_player_can_release(playerid, this))
		return FALSE;
	if(is_affected_by_effect(EFFECT_UNRELEASABLE_SUM, pcard))
		return FALSE;
	if(pcard->is_affected_by_effect(EFFECT_TRIBUTE_LIMIT, this))
		return FALSE;
	return TRUE;
}
int32_t card::is_releasable_by_nonsummon(uint8_t playerid) {
	if(is_status(STATUS_SUMMONING))
		return FALSE;
	if(overlay_target)
		return FALSE;
	if(current.location & (LOCATION_GRAVE | LOCATION_REMOVED))
		return FALSE;
	if((current.location == LOCATION_HAND) && (data.type & (TYPE_SPELL | TYPE_TRAP)))
		return FALSE;
	if(!pduel->game_field->is_player_can_release(playerid, this))
		return FALSE;
	if(is_affected_by_effect(EFFECT_UNRELEASABLE_NONSUM))
		return FALSE;
	return TRUE;
}
int32_t card::is_releasable_by_effect(uint8_t playerid, effect* peffect) {
	if(!peffect)
		return TRUE;
	effect_set eset;
	filter_effect(EFFECT_UNRELEASABLE_EFFECT, &eset);
	for(const auto& peff : eset) {
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		if(peff->check_value_condition(3))
			return FALSE;
	}
	return TRUE;
}
int32_t card::is_capable_send_to_grave(uint8_t playerid) {
	if(is_affected_by_effect(EFFECT_CANNOT_TO_GRAVE))
		return FALSE;
	if(!pduel->game_field->is_player_can_send_to_grave(playerid, this))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_send_to_hand(uint8_t playerid) {
	if(is_status(STATUS_LEAVE_CONFIRMED))
		return FALSE;
	if((current.location == LOCATION_EXTRA) && is_extra_deck_monster())
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_TO_HAND))
		return FALSE;
	if(is_extra_deck_monster() && !is_capable_send_to_deck(playerid))
		return FALSE;
	if(!pduel->game_field->is_player_can_send_to_hand(playerid, this))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_send_to_deck(uint8_t playerid) {
	if(is_status(STATUS_LEAVE_CONFIRMED))
		return FALSE;
	if((current.location == LOCATION_EXTRA) && is_extra_deck_monster())
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_TO_DECK))
		return FALSE;
	if(!pduel->game_field->is_player_can_send_to_deck(playerid, this))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_send_to_extra(uint8_t playerid) {
	if(!is_extra_deck_monster() && !(data.type & TYPE_PENDULUM))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_TO_DECK))
		return FALSE;
	if(!pduel->game_field->is_player_can_send_to_deck(playerid, this))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_cost_to_grave(uint8_t playerid) {
	uint32_t redirect = 0;
	uint32_t dest = LOCATION_GRAVE;
	if(data.type & TYPE_TOKEN)
		return FALSE;
	if((data.type & TYPE_PENDULUM) && (current.location & LOCATION_ONFIELD) && !is_affected_by_effect(EFFECT_CANNOT_TO_DECK))
		return FALSE;
	if(current.location == LOCATION_GRAVE)
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_USE_AS_COST))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_TO_GRAVE_AS_COST))
		return FALSE;
	if(!is_capable_send_to_grave(playerid))
		return FALSE;
	auto op_param = sendto_param;
	sendto_param.location = dest;
	if(current.location & LOCATION_ONFIELD)
		redirect = leave_field_redirect(REASON_COST) & 0xffff;
	if(redirect) dest = redirect;
	redirect = destination_redirect(dest, REASON_COST) & 0xffff;
	if(redirect) dest = redirect;
	sendto_param = op_param;
	if(dest != LOCATION_GRAVE)
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_cost_to_hand(uint8_t playerid) {
	uint32_t redirect = 0;
	uint32_t dest = LOCATION_HAND;
	if(data.type & (TYPE_TOKEN) || is_extra_deck_monster())
		return FALSE;
	if(current.location == LOCATION_HAND)
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_USE_AS_COST))
		return FALSE;
	if(!is_capable_send_to_hand(playerid))
		return FALSE;
	auto op_param = sendto_param;
	sendto_param.location = dest;
	if(current.location & LOCATION_ONFIELD)
		redirect = leave_field_redirect(REASON_COST) & 0xffff;
	if(redirect) dest = redirect;
	redirect = destination_redirect(dest, REASON_COST) & 0xffff;
	if(redirect) dest = redirect;
	sendto_param = op_param;
	if(dest != LOCATION_HAND)
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_cost_to_deck(uint8_t playerid) {
	uint32_t redirect = 0;
	uint32_t dest = LOCATION_DECK;
	if(data.type & (TYPE_TOKEN) || is_extra_deck_monster())
		return FALSE;
	if(current.location == LOCATION_DECK)
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_USE_AS_COST))
		return FALSE;
	if(!is_capable_send_to_deck(playerid))
		return FALSE;
	auto op_param = sendto_param;
	sendto_param.location = dest;
	if(current.location & LOCATION_ONFIELD)
		redirect = leave_field_redirect(REASON_COST) & 0xffff;
	if(redirect) dest = redirect;
	redirect = destination_redirect(dest, REASON_COST) & 0xffff;
	if(redirect) dest = redirect;
	sendto_param = op_param;
	if(dest != LOCATION_DECK)
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_cost_to_extra(uint8_t playerid) {
	uint32_t redirect = 0;
	uint32_t dest = LOCATION_EXTRA;
	if(!is_extra_deck_monster())
		return FALSE;
	if(current.location == LOCATION_EXTRA)
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_USE_AS_COST))
		return FALSE;
	if(!is_capable_send_to_deck(playerid))
		return FALSE;
	auto op_param = sendto_param;
	sendto_param.location = dest;
	if(current.location & LOCATION_ONFIELD)
		redirect = leave_field_redirect(REASON_COST) & 0xffff;
	if(redirect) dest = redirect;
	redirect = destination_redirect(dest, REASON_COST) & 0xffff;
	if(redirect) dest = redirect;
	sendto_param = op_param;
	if(dest != LOCATION_EXTRA)
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_attack() {
	if(!is_position(POS_FACEUP_ATTACK) && !(is_position(POS_FACEUP_DEFENSE) && is_affected_by_effect(EFFECT_DEFENSE_ATTACK)))
		return FALSE;
	if(is_affected_by_effect(EFFECT_FORBIDDEN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_ATTACK) && !is_affected_by_effect(EFFECT_UNSTOPPABLE_ATTACK))
		return FALSE;
	if(is_affected_by_effect(EFFECT_ATTACK_DISABLED) && !is_affected_by_effect(EFFECT_UNSTOPPABLE_ATTACK))
		return FALSE;
	if(pduel->game_field->is_player_affected_by_effect(pduel->game_field->infos.turn_player, EFFECT_SKIP_BP))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_attack_announce(uint8_t playerid) {
	if(!is_capable_attack())
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_ATTACK_ANNOUNCE) && !is_affected_by_effect(EFFECT_UNSTOPPABLE_ATTACK))
		return FALSE;
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_ATTACK_COST, playerid)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	pduel->game_field->restore_lp_cost();
	return TRUE;
}
int32_t card::is_capable_change_position(uint8_t playerid) {
	if(is_status(STATUS_FORM_CHANGED))
		return FALSE;
	if((is_status(STATUS_SUMMON_TURN) || is_status(STATUS_FLIP_SUMMON_TURN) || is_status(STATUS_SPSUMMON_TURN)) &&
		(summon_player == current.controler || !pduel->game_field->is_flag(DUEL_CAN_REPOS_IF_NON_SUMPLAYER)))
		return FALSE;
	if((data.type & TYPE_LINK) && (data.type & TYPE_MONSTER))
		return FALSE;
	if(announce_count > 0)
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_CHANGE_POSITION))
		return FALSE;
	if(pduel->game_field->is_player_affected_by_effect(playerid, EFFECT_CANNOT_CHANGE_POSITION))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_change_position_by_effect(uint8_t /*playerid*/) {
	if((data.type & TYPE_LINK) && (data.type & TYPE_MONSTER))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_turn_set(uint8_t playerid) {
	if((data.type & (TYPE_TOKEN)) || ((data.type & TYPE_LINK) && (data.type & TYPE_MONSTER)))
		return FALSE;
	if(is_position(POS_FACEDOWN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_TURN_SET))
		return FALSE;
	if(pduel->game_field->is_player_affected_by_effect(playerid, EFFECT_CANNOT_TURN_SET))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_change_control() {
	if(is_affected_by_effect(EFFECT_CANNOT_CHANGE_CONTROL))
		return FALSE;
	return TRUE;
}
int32_t card::is_control_can_be_changed(int32_t ignore_mzone, uint32_t zone) {
	if(current.controler == PLAYER_NONE)
		return FALSE;
	if(current.location != LOCATION_MZONE)
		return FALSE;
	if(!ignore_mzone && pduel->game_field->get_useable_count(this, 1 - current.controler, LOCATION_MZONE, current.controler, LOCATION_REASON_CONTROL, zone) <= 0)
		return FALSE;
	if(!pduel->game_field->is_flag(DUEL_TRAP_MONSTERS_NOT_USE_ZONE) && ((get_type() & TYPE_TRAPMONSTER)
											 && pduel->game_field->get_useable_count(this, 1 - current.controler, LOCATION_SZONE, current.controler, LOCATION_REASON_CONTROL) <= 0))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_CHANGE_CONTROL))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_be_battle_target(card* pcard) {
	if(is_affected_by_effect(EFFECT_CANNOT_BE_BATTLE_TARGET, pcard))
		return FALSE;
	if(pcard->is_affected_by_effect(EFFECT_CANNOT_SELECT_BATTLE_TARGET, this))
		return FALSE;
	if(is_affected_by_effect(EFFECT_IGNORE_BATTLE_TARGET, pcard))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_be_effect_target(effect* peffect, uint8_t playerid) {
	if(is_status(STATUS_SUMMONING) || is_status(STATUS_BATTLE_DESTROYED))
		return FALSE;
	if(current.location & (LOCATION_DECK | LOCATION_EXTRA | LOCATION_HAND))
		return FALSE;
	effect_set eset;
	filter_effect(EFFECT_CANNOT_BE_EFFECT_TARGET, &eset);
	for(const auto& peff : eset) {
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(peff->get_value(peffect, 1))
			return FALSE;
	}
	eset.clear();
	peffect->get_handler()->filter_effect(EFFECT_CANNOT_SELECT_EFFECT_TARGET, &eset);
	for(const auto& peff : eset) {
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		if(peff->get_value(peffect, 1))
			return FALSE;
	}
	return TRUE;
}
int32_t card::is_capable_overlay(uint8_t playerid) {
	if(data.type & TYPE_TOKEN)
		return FALSE;
	if(!(current.location & LOCATION_ONFIELD) && is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(current.controler != playerid && !is_capable_change_control())
		return FALSE;
	return TRUE;
}
int32_t card::is_can_be_fusion_material(card* fcard, uint64_t summon_type, uint8_t playerid) {
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	effect_set eset;
	filter_effect(EFFECT_CANNOT_BE_FUSION_MATERIAL, &eset);
	for(const auto& peffect : eset) {
		pduel->lua->add_param(summon_type, PARAM_TYPE_INT);
		if(peffect->get_value(fcard, 1))
			return FALSE;
	}
	eset.clear();
	filter_effect(EFFECT_CANNOT_BE_MATERIAL, &eset);
	for(const auto& peffect : eset) {
		pduel->lua->add_param(summon_type, PARAM_TYPE_INT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(peffect->get_value(fcard, 2))
			return FALSE;
	}
	eset.clear();
	if (fcard) {
		filter_effect(EFFECT_EXTRA_FUSION_MATERIAL, &eset);
		if(eset.size()) {
			for(const auto& peffect : eset)
				if(peffect->get_value(fcard))
					return TRUE;
			return FALSE;
		}
	} else {
		if (!(current.location & LOCATION_ONFIELD) && !(data.type & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_EXTRA_FUSION_MATERIAL))
			return FALSE;
	}
	return TRUE;
}
int32_t card::is_can_be_synchro_material(card* scard, uint8_t playerid, card* /*tuner*/) {
	if(data.type & (TYPE_XYZ | TYPE_LINK) && !(is_affected_by_effect(EFFECT_RANK_LEVEL) || is_affected_by_effect(EFFECT_RANK_LEVEL_S)))
		return FALSE;
	if(!(get_type(scard, SUMMON_TYPE_SYNCHRO, playerid) & TYPE_MONSTER))
		return FALSE;
	if(scard && current.location == LOCATION_MZONE && current.controler != scard->current.controler && !is_affected_by_effect(EFFECT_SYNCHRO_MATERIAL))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	effect_set eset;
	filter_effect(EFFECT_CANNOT_BE_SYNCHRO_MATERIAL, &eset);
	for(const auto& peffect : eset)
		if(peffect->get_value(scard))
			return FALSE;
	eset.clear();
	filter_effect(EFFECT_CANNOT_BE_MATERIAL, &eset);
	for(const auto& peffect : eset) {
		pduel->lua->add_param(SUMMON_TYPE_SYNCHRO, PARAM_TYPE_INT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(peffect->get_value(scard, 2))
			return FALSE;
	}
	return TRUE;
}
int32_t card::is_can_be_ritual_material(card* scard, uint8_t playerid) {
	if(!(get_type() & TYPE_MONSTER))
		return FALSE;
	effect_set eset;
	filter_effect(EFFECT_CANNOT_BE_MATERIAL, &eset);
	for(const auto& peffect : eset) {
		pduel->lua->add_param(SUMMON_TYPE_RITUAL, PARAM_TYPE_INT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(peffect->get_value(scard, 2))
			return FALSE;
	}
	if(current.location == LOCATION_GRAVE) {
		eset.clear();
		filter_effect(EFFECT_EXTRA_RITUAL_MATERIAL, &eset);
		for(const auto& peffect : eset)
			if(peffect->get_value(scard))
				return TRUE;
		return FALSE;
	}
	return TRUE;
}
int32_t card::is_can_be_xyz_material(card* scard, uint8_t playerid) {
	if(data.type & TYPE_TOKEN)
		return FALSE;
	if(!(get_type(scard, SUMMON_TYPE_XYZ, playerid) & TYPE_MONSTER))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	effect_set eset;
	filter_effect(EFFECT_CANNOT_BE_XYZ_MATERIAL, &eset);
	for(const auto& peffect : eset)
		if(peffect->get_value(scard))
			return FALSE;
	eset.clear();
	filter_effect(EFFECT_CANNOT_BE_MATERIAL, &eset);
	for(const auto& peffect : eset) {
		pduel->lua->add_param(SUMMON_TYPE_XYZ, PARAM_TYPE_INT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(peffect->get_value(scard, 2))
			return FALSE;
	}
	return TRUE;
}
int32_t card::is_can_be_link_material(card* scard, uint8_t playerid) {
	if(!(get_type(scard, SUMMON_TYPE_LINK, playerid) & TYPE_MONSTER))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	effect_set eset;
	filter_effect(EFFECT_CANNOT_BE_LINK_MATERIAL, &eset);
	for(const auto& peffect : eset)
		if(peffect->get_value(scard))
			return FALSE;
	eset.clear();
	filter_effect(EFFECT_CANNOT_BE_MATERIAL, &eset);
	for(const auto& peffect : eset) {
		pduel->lua->add_param(SUMMON_TYPE_LINK, PARAM_TYPE_INT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(peffect->get_value(scard, 2))
			return FALSE;
	}
	return TRUE;
}
int32_t card::is_can_be_material(card* scard, uint64_t sumtype, uint8_t playerid) {
	if(sumtype & SUMMON_TYPE_FUSION)
		return is_can_be_fusion_material(scard, sumtype, playerid);
	if(sumtype & SUMMON_TYPE_SYNCHRO)
		return is_can_be_synchro_material(scard, playerid);
	if(sumtype & SUMMON_TYPE_RITUAL)
		return is_can_be_ritual_material(scard, playerid);
	if(sumtype & SUMMON_TYPE_XYZ)
		return is_can_be_xyz_material(scard, playerid);
	if(sumtype & SUMMON_TYPE_LINK)
		return is_can_be_link_material(scard, playerid);
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	effect_set eset;
	filter_effect(EFFECT_CANNOT_BE_MATERIAL, &eset);
	for(const auto& peffect : eset) {
		pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(peffect->get_value(scard, 2))
			return FALSE;
	}
	return TRUE;
}
bool card::recreate(uint32_t code) {
	if(!code)
		return false;
	data = pduel->read_card(code);
	return true;
}
