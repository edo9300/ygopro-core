/*
 * interface.cpp
 *
 *  Created on: 2010-5-2
 *      Author: Argon
 */
#include <stdio.h>
#include <string.h>
#include "ocgapi.h"
#include "duel.h"
#include "card.h"
#include "group.h"
#include "effect.h"
#include "field.h"
#include "interpreter.h"
#include <set>
#include <mutex>

static script_reader sreader = default_script_reader;
static card_reader creader = default_card_reader;
static message_handler mhandler = default_message_handler;

static std::set<duel*> duel_set;
static std::mutex duel_set_mutex;

extern "C" DECL_DLLEXPORT int get_api_version(int* min) {
	if(min)
		*min = DUEL_API_VERSION_MINOR;
	return DUEL_API_VERSION_MAJOR;
}
extern "C" DECL_DLLEXPORT void set_script_reader(script_reader f) {
	sreader = f;
}
extern "C" DECL_DLLEXPORT void set_card_reader(card_reader f) {
	creader = f;
}
extern "C" DECL_DLLEXPORT void set_message_handler(message_handler f) {
	mhandler = f;
}
byte* read_script(const char* script_name, int* len) {
	return sreader(script_name, len);
}
uint32 read_card(uint32 code, card_data* data) {
	return creader(code, data);
}
uint32 handle_message(void* pduel, uint32 msg_type) {
	return mhandler(pduel, msg_type);
}
byte* default_script_reader(const char* script_name, int* slen) {
	static byte buffer[0x20000];
	FILE *fp;
	char sname[256] = "./script/";
	strcat(sname, script_name);//default script name: c%d.lua
	fp = fopen(sname, "rb");
	if (!fp)
		return 0;
	size_t len = fread(buffer, 1, sizeof(buffer), fp);
	fclose(fp);
	if(len >= sizeof(buffer))
		return 0;
	*slen = len;
	return buffer;
}
uint32 default_card_reader(uint32 code, card_data* data) {
	return 0;
}
uint32 default_message_handler(void* pduel, uint32 message_type) {
	return 0;
}
extern "C" DECL_DLLEXPORT ptr create_duel(uint32 seed) {
	duel* pduel = new duel();
	pduel->random.seed(seed);
	std::lock_guard<std::mutex> guard(duel_set_mutex);
	duel_set.insert(pduel);
	return (ptr)pduel;
}
extern "C" DECL_DLLEXPORT void start_duel(ptr pduel, int32 options) {
	duel* pd = (duel*)pduel;
	pd->game_field->core.duel_options |= options;
	int32 duel_rule = 5;
	switch(options) {
	case MASTER_RULE_1: {
		duel_rule = 1;
	break;
	}
	case MASTER_RULE_2: {
		duel_rule = 2;
	break;
	}
	case MASTER_RULE_3: {
		duel_rule = 3;
	break;
	}
	case MASTER_RULE_4: {
		duel_rule = 4;
	break;
	}
	}
	pd->game_field->core.duel_rule = duel_rule;
	pd->game_field->core.shuffle_hand_check[0] = FALSE;
	pd->game_field->core.shuffle_hand_check[1] = FALSE;
	pd->game_field->core.shuffle_deck_check[0] = FALSE;
	pd->game_field->core.shuffle_deck_check[1] = FALSE;
	if(pd->game_field->player[0].start_count > 0)
		pd->game_field->draw(0, REASON_RULE, PLAYER_NONE, 0, pd->game_field->player[0].start_count);
	if(pd->game_field->player[1].start_count > 0)
		pd->game_field->draw(0, REASON_RULE, PLAYER_NONE, 1, pd->game_field->player[1].start_count);
	for(int p = 0; p < 2; p++) {
		for(std::vector<card*>::size_type l = 0; l < pd->game_field->player[p].extra_lists_main.size(); l++) {
			auto& main = pd->game_field->player[p].extra_lists_main[l];
			auto& hand = pd->game_field->player[p].extra_lists_hand[l];
			for(int i = 0; i < pd->game_field->player[p].start_count && main.size(); ++i) {
				card* pcard = main.back();
				main.pop_back();
				hand.push_back(pcard);
				pcard->current.controler = p;
				pcard->current.location = LOCATION_HAND;
				pcard->current.sequence = hand.size() - 1;
				pcard->current.position = POS_FACEDOWN;
			}

		}
	}
	pd->game_field->add_process(PROCESSOR_TURN, 0, 0, 0, 0, 0);
}
extern "C" DECL_DLLEXPORT void end_duel(ptr pduel) {
	duel* pd = (duel*)pduel;
	std::lock_guard<std::mutex> guard(duel_set_mutex);
	if(duel_set.count(pd)) {
		duel_set.erase(pd);
		delete pd;
	}
}
extern "C" DECL_DLLEXPORT void set_player_info(ptr pduel, int32 playerid, int32 lp, int32 startcount, int32 drawcount) {
	duel* pd = (duel*)pduel;
	if (lp > 0) {
		pd->game_field->player[playerid].lp = lp;
		pd->game_field->player[playerid].start_lp = lp;
	}
	if(startcount >= 0)
		pd->game_field->player[playerid].start_count = startcount;
	if(drawcount >= 0)
		pd->game_field->player[playerid].draw_count = drawcount;
}
extern "C" DECL_DLLEXPORT void get_log_message(ptr pduel, byte* buf) {
	strcpy((char*)buf, ((duel*)pduel)->strbuffer);
}
extern "C" DECL_DLLEXPORT int32 get_message(ptr pduel, byte* buf) {
	int32 len = ((duel*)pduel)->read_buffer(buf);
	if(buf != nullptr)
		((duel*)pduel)->clear_buffer();
	return len;
}
extern "C" DECL_DLLEXPORT int32 process(ptr pduel) {
	duel* pd = (duel*)pduel;
	int flag = 0;
	do {
		int flag = pd->game_field->process();
		pd->generate_buffer();
	} while(pd->buff.size() == 0 && flag == 0);
	return flag;
}
extern "C" DECL_DLLEXPORT void new_card(ptr pduel, uint32 code, uint8 owner, uint8 playerid, uint8 location, uint8 sequence, uint8 position, int32 duelist) {
	duel* ptduel = (duel*)pduel;
	if(duelist == 0) {
		if(ptduel->game_field->is_location_useable(playerid, location, sequence)) {
			card* pcard = ptduel->new_card(code);
			pcard->owner = owner;
			ptduel->game_field->add_card(playerid, pcard, location, sequence);
			pcard->current.position = position;
			if(!(location & LOCATION_ONFIELD) || (position & POS_FACEUP)) {
				pcard->enable_field_effect(true);
				ptduel->game_field->adjust_instant();
			}
			if(location & LOCATION_ONFIELD) {
				if(location == LOCATION_MZONE)
					pcard->set_status(STATUS_PROC_COMPLETE, TRUE);
			}
		}
	} else {
		if(owner > 1 || !(location & 0x41))
			return;
		duelist--;
		card* pcard = ptduel->new_card(code);
		auto& player = ptduel->game_field->player[owner];
		if(duelist >= player.extra_lists_main.size()) {
			player.extra_lists_main.resize(duelist + 1);
			player.extra_lists_extra.resize(duelist + 1);
			player.extra_lists_hand.resize(duelist + 1);
			player.extra_extra_p_count.resize(duelist + 1);
		}
		switch(location) {
			case LOCATION_DECK:
				player.extra_lists_main[duelist].push_back(pcard);
				pcard->owner = owner;
				pcard->current.controler = owner;
				pcard->current.location = LOCATION_DECK;
				pcard->current.sequence = player.extra_lists_main[duelist].size() - 1;
				pcard->current.position = POS_FACEDOWN_DEFENSE;
				break;
			case LOCATION_EXTRA:
				player.extra_lists_extra[duelist].push_back(pcard);
				pcard->owner = owner;
				pcard->current.controler = owner;
				pcard->current.location = LOCATION_EXTRA;
				pcard->current.sequence = player.extra_lists_extra[duelist].size() - 1;
				pcard->current.position = POS_FACEDOWN_DEFENSE;
				break;
		}


	}
}
template<typename T>
void insert_value(std::vector<uint8_t>& vec, T val) {
	const auto vec_size = vec.size();
	const auto val_size = sizeof(T);
	vec.resize(vec_size + val_size);
	std::memcpy(&vec[vec_size], &val, val_size);
}
extern "C" DECL_DLLEXPORT int32 get_cached_query(ptr pduel, byte * buf) {
	duel* ptduel = (duel*)pduel;
	int len = ptduel->cached_query.size();
	if(len)
		memcpy(buf, ptduel->cached_query.data(), len);
	ptduel->cached_query.clear();
	return len;
}
extern "C" DECL_DLLEXPORT int32 query_card(ptr pduel, uint8 playerid, uint8 location, uint8 sequence, int32 query_flag, byte* buf, int32 use_cache, int32 ignore_cache) {
	if(playerid != 0 && playerid != 1)
		return 0;
	duel* ptduel = (duel*)pduel;
	card* pcard = 0;
	ptduel->cached_query.clear();
	location &= 0x7f;
	if(location & LOCATION_ONFIELD)
		pcard = ptduel->game_field->get_field_card(playerid, location, sequence);
	else {
		field::card_vector* lst = 0;
		if(location == LOCATION_HAND)
			lst = &ptduel->game_field->player[playerid].list_hand;
		else if(location == LOCATION_GRAVE)
			lst = &ptduel->game_field->player[playerid].list_grave;
		else if(location == LOCATION_REMOVED)
			lst = &ptduel->game_field->player[playerid].list_remove;
		else if(location == LOCATION_EXTRA)
			lst = &ptduel->game_field->player[playerid].list_extra;
		else if(location == LOCATION_DECK)
			lst = &ptduel->game_field->player[playerid].list_main;
		if(!lst || sequence >= lst->size())
			pcard = 0;
		else {
			pcard = (*lst)[sequence];
		}
	}
	if(pcard) {
		auto len = pcard->get_infos(query_flag, use_cache, ignore_cache);
		if(buf != nullptr) {
			if(len)
				memcpy(buf, ptduel->cached_query.data(), ptduel->cached_query.size());
			ptduel->cached_query.clear();
		}
		return len;
	}
	else {
		if(buf != nullptr) {
			*((int32*)buf) = 4;
		} else {
			insert_value<int32>(ptduel->cached_query, 4);
		}
		return 4;
	}
}
extern "C" DECL_DLLEXPORT int32 query_field_count(ptr pduel, uint8 playerid, uint8 location) {
	duel* ptduel = (duel*)pduel;
	if(playerid != 0 && playerid != 1)
		return 0;
	auto& player = ptduel->game_field->player[playerid];
	if(location == LOCATION_HAND)
		return player.list_hand.size();
	if(location == LOCATION_GRAVE)
		return player.list_grave.size();
	if(location == LOCATION_REMOVED)
		return player.list_remove.size();
	if(location == LOCATION_EXTRA)
		return player.list_extra.size();
	if(location == LOCATION_DECK)
		return player.list_main.size();
	if(location == LOCATION_MZONE) {
		uint32 count = 0;
		for(auto cit = player.list_mzone.begin(); cit != player.list_mzone.end(); ++cit)
			if(*cit) count++;
		return count;
	}
	if(location == LOCATION_SZONE) {
		uint32 count = 0;
		for(auto cit = player.list_szone.begin(); cit != player.list_szone.end(); ++cit)
			if(*cit) count++;
		return count;
	}
	return 0;
}
extern "C" DECL_DLLEXPORT int32 query_field_card(ptr pduel, uint8 playerid, uint8 location, int32 query_flag, byte* buf, int32 use_cache, int32 ignore_cache) {
	if(playerid != 0 && playerid != 1)
		return 0;
	duel* ptduel = (duel*)pduel;
	ptduel->cached_query.clear();
	auto& player = ptduel->game_field->player[playerid];
	if(location == LOCATION_MZONE) {
		for(auto cit = player.list_mzone.begin(); cit != player.list_mzone.end(); ++cit) {
			card* pcard = *cit;
			if(pcard) {
				pcard->get_infos(query_flag, use_cache, ignore_cache);
			} else {
				insert_value<int32>(ptduel->cached_query, 4);
			}
		}
	} else if(location == LOCATION_SZONE) {
		for(auto cit = player.list_szone.begin(); cit != player.list_szone.end(); ++cit) {
			card* pcard = *cit;
			if(pcard) {
				pcard->get_infos(query_flag, use_cache, ignore_cache);
			} else {
				insert_value<int32>(ptduel->cached_query, 4);
			}
		}
	} else {
		field::card_vector* lst;
		if(location == LOCATION_HAND)
			lst = &player.list_hand;
		else if(location == LOCATION_GRAVE)
			lst = &player.list_grave;
		else if(location == LOCATION_REMOVED)
			lst = &player.list_remove;
		else if(location == LOCATION_EXTRA)
			lst = &player.list_extra;
		else if(location == LOCATION_DECK)
			lst = &player.list_main;
		for(auto& pcard : *lst) {
			pcard->get_infos(query_flag, use_cache, ignore_cache);
		}
	}
	int len = ptduel->cached_query.size();
	if(buf != nullptr) {
		if(len)
			memcpy(buf, ptduel->cached_query.data(), ptduel->cached_query.size());
		ptduel->cached_query.clear();
	}
	return len;
}
extern "C" DECL_DLLEXPORT int32 query_field_info(ptr pduel, byte* buf) {
	duel* ptduel = (duel*)pduel;
	ptduel->cached_query.clear();
	//byte* p = buf;
	insert_value<int8_t>(ptduel->cached_query, MSG_RELOAD_FIELD);
	insert_value<int8_t>(ptduel->cached_query, ptduel->game_field->core.duel_rule + ((ptduel->game_field->is_flag(SPEED_DUEL) ? 1 : 0) << 4));
	for(int playerid = 0; playerid < 2; ++playerid) {
		auto& player = ptduel->game_field->player[playerid];
		insert_value<int32_t>(ptduel->cached_query, player.lp);
		for(auto cit = player.list_mzone.begin(); cit != player.list_mzone.end(); ++cit) {
			card* pcard = *cit;
			if(pcard) {
				insert_value<int8_t>(ptduel->cached_query, 1);
				insert_value<int8_t>(ptduel->cached_query, pcard->current.position);
				insert_value<int32_t>(ptduel->cached_query, pcard->xyz_materials.size());
			} else {
				insert_value<int8_t>(ptduel->cached_query, 0);
			}
		}
		for(auto cit = player.list_szone.begin(); cit != player.list_szone.end(); ++cit) {
			card* pcard = *cit;
			if(pcard) {
				insert_value<int8_t>(ptduel->cached_query, 1);
				insert_value<int8_t>(ptduel->cached_query, pcard->current.position);
			} else {
				insert_value<int8_t>(ptduel->cached_query, 0);
			}
		}
		insert_value<uint32_t>(ptduel->cached_query, player.list_main.size());
		insert_value<uint32_t>(ptduel->cached_query, player.list_hand.size());
		insert_value<uint32_t>(ptduel->cached_query, player.list_grave.size());
		insert_value<uint32_t>(ptduel->cached_query, player.list_remove.size());
		insert_value<uint32_t>(ptduel->cached_query, player.list_extra.size());
		insert_value<uint32_t>(ptduel->cached_query, player.extra_p_count);
	}
	insert_value<int32_t>(ptduel->cached_query, ptduel->game_field->core.current_chain.size());
	for(const auto& ch : ptduel->game_field->core.current_chain) {
		effect* peffect = ch.triggering_effect;
		insert_value<int32_t>(ptduel->cached_query, peffect->get_handler()->data.code);
		loc_info info = peffect->get_handler()->get_info_location();
		insert_value<uint8>(ptduel->cached_query, info.controler);
		insert_value<uint8>(ptduel->cached_query, info.location);
		insert_value<uint32>(ptduel->cached_query, info.sequence);
		insert_value<uint32>(ptduel->cached_query, info.position);
		insert_value<uint8>(ptduel->cached_query, ch.triggering_controler);
		insert_value<uint8>(ptduel->cached_query, (uint8)ch.triggering_location);
		insert_value<uint32>(ptduel->cached_query, ch.triggering_sequence);
		insert_value<uint64>(ptduel->cached_query, peffect->description);
	}
	int len = ptduel->cached_query.size();
	if(buf != nullptr) {
		if(len)
			memcpy(buf, ptduel->cached_query.data(), ptduel->cached_query.size());
		ptduel->cached_query.clear();
	}
	return len;
}
extern "C" DECL_DLLEXPORT void set_responsei(ptr pduel, int32 value) {
	((duel*)pduel)->set_responsei(value);
}
extern "C" DECL_DLLEXPORT void set_responseb(ptr pduel, byte* buf, size_t len) {
	((duel*)pduel)->set_responseb(buf, len);
}
extern "C" DECL_DLLEXPORT int32 preload_script(ptr pduel, char* script, int32 len, int32 scriptlen, char* scriptbuff) {
	if(scriptlen)
		return ((duel*)pduel)->lua->load_script(scriptbuff, scriptlen, script);
	return ((duel*)pduel)->lua->load_script(script);
}
