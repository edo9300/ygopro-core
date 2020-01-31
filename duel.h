/*
 * duel.h
 *
 *  Created on: 2010-4-8
 *      Author: Argon
 */

#ifndef DUEL_H_
#define DUEL_H_

#include "common.h"
#include "ocgapi.h"
#include <random>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <deque>

class card;
class group;
class effect;
class field;
class interpreter;
struct loc_info;

struct card_data {
	uint32_t code;
	uint32_t alias;
	std::set<uint16_t> setcodes;
	uint32_t type;
	uint32_t level;
	uint32_t attribute;
	uint32_t race;
	int32_t attack;
	int32_t defense;
	uint32_t lscale;
	uint32_t rscale;
	uint32_t link_marker;
	card_data(OCG_CardData* data);
	card_data() {};
};

class duel {
public:
	class duel_message {
	public:
		std::vector<uint8_t> data;
		uint8_t message;
		duel_message() :message(0) {};
		duel_message(uint8_t _message);
		void write(void* buff, size_t size);
		void write(loc_info loc);
		template<typename T>
		void write(T data) {
			write(&data, sizeof(T));
		}
	};
	typedef std::set<card*, card_sort> card_set;
	char strbuffer[256];
	std::vector<uint8_t> buff;
	std::vector<uint8_t> query_buffer;
	interpreter* lua;
	field* game_field;
	std::mt19937 random;
	std::unordered_set<card*> cards;
	std::unordered_set<card*> assumes;
	std::unordered_set<group*> groups;
	std::unordered_set<group*> sgroups;
	std::unordered_set<effect*> effects;
	std::unordered_set<effect*> uncopy;

	std::unordered_map<uint32_t, card_data> data_cache;
	
	duel() {};
	duel(OCG_DuelOptions options);
	~duel();
	void clear();
	
	card* new_card(uint32 code);
	group* new_group();
	group* new_group(card* pcard);
	group* new_group(const card_set& cset);
	effect* new_effect();
	void delete_card(card* pcard);
	void delete_group(group* pgroup);
	void delete_effect(effect* peffect);
	void release_script_group();
	void restore_assumes();
	int32 read_buffer(byte* buf);
	void generate_buffer();
	void write_buffer(void* data, size_t size);
	void clear_buffer();
	void set_response(byte* resp, size_t len);
	int32 get_next_integer(int32 l, int32 h);
	duel_message* new_message(uint32_t message);
	card_data const* read_card(uint32_t code, card_data* copyable = nullptr);
	void* read_card_payload;
	void* read_script_payload;
	void* handle_message_payload;
	void* read_card_done_payload;
	OCG_DataReader read_card_api;
	OCG_ScriptReader read_script;
	OCG_LogHandler handle_message;
	OCG_DataReaderDone read_card_done;
private:
	std::deque<duel_message> messages;
	group* register_group(group* pgroup);
};

#endif /* DUEL_H_ */
