/*
 * duel.h
 *
 *  Created on: 2010-4-8
 *      Author: Argon
 */

#ifndef DUEL_H_
#define DUEL_H_

#include "common.h"
#include <random>
#include <set>
#include <unordered_set>
#include <vector>
#include <deque>

class card;
class group;
class effect;
class field;
class interpreter;
struct loc_info;

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
	std::vector<uint8_t> cached_query;
	interpreter* lua;
	field* game_field;
	std::mt19937 random;
	std::unordered_set<card*> cards;
	std::unordered_set<card*> assumes;
	std::unordered_set<group*> groups;
	std::unordered_set<group*> sgroups;
	std::unordered_set<effect*> effects;
	std::unordered_set<effect*> uncopy;
	
	duel();
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
	void set_responsei(uint32 resp);
	void set_responseb(byte* resp, size_t len);
	int32 get_next_integer(int32 l, int32 h);
	duel_message* new_message(uint32_t message);
	void* payload1;
	void* payload2;
	void* payload3;
private:
	std::deque<duel_message> messages;
	group* register_group(group* pgroup);
};

#endif /* DUEL_H_ */
