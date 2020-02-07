/*
 * duel.cpp
 *
 *  Created on: 2010-5-2
 *      Author: Argon
 */

#include "duel.h"
#include "interpreter.h"
#include "field.h"
#include "card.h"
#include "effect.h"
#include "group.h"

duel::duel(OCG_DuelOptions options) {
	read_card_api = options.cardReader;
	read_card_payload = options.payload1;
	read_script = options.scriptReader;
	read_script_payload = options.payload2;
	handle_message = options.logHandler;
	handle_message_payload = options.payload3;
	read_card_done = options.cardReaderDone;
	read_card_done_payload = options.payload4;
	lua = new interpreter(this);
	game_field = new field(this);
	game_field->temp_card = new_card(0);
	clear_buffer();
}
duel::~duel() {
	for(auto& pcard : cards)
		delete pcard;
	for(auto& pgroup : groups)
		delete pgroup;
	for(auto& peffect : effects)
		delete peffect;
	delete lua;
	delete game_field;
}
void duel::clear() {
	for(auto& pcard : cards)
		delete pcard;
	for(auto& pgroup : groups)
		delete pgroup;
	for(auto& peffect : effects)
		delete peffect;
	delete game_field;
	cards.clear();
	groups.clear();
	effects.clear();
	game_field = new field(this);
	game_field->temp_card = new_card(0);
}
card* duel::new_card(uint32 code) {
	card* pcard = new card(this);
	cards.insert(pcard);
	if(code)
		read_card(code, &(pcard->data));
	pcard->data.code = code;
	lua->register_card(pcard);
	return pcard;
}
group* duel::register_group(group* pgroup) {
	groups.insert(pgroup);
	if(lua->call_depth)
		sgroups.insert(pgroup);
	lua->register_group(pgroup);
	return pgroup;
}
group* duel::new_group() {
	group* pgroup = new group(this);
	return register_group(pgroup);
}
group* duel::new_group(card* pcard) {
	group* pgroup = new group(this, pcard);
	return register_group(pgroup);
}
group* duel::new_group(const card_set& cset) {
	group* pgroup = new group(this, cset);
	return register_group(pgroup);
}
effect* duel::new_effect() {
	effect* peffect = new effect(this);
	effects.insert(peffect);
	lua->register_effect(peffect);
	return peffect;
}
void duel::delete_card(card* pcard) {
	cards.erase(pcard);
	delete pcard;
}
void duel::delete_group(group* pgroup) {
	lua->unregister_group(pgroup);
	groups.erase(pgroup);
	sgroups.erase(pgroup);
	delete pgroup;
}
void duel::delete_effect(effect* peffect) {
	lua->unregister_effect(peffect);
	effects.erase(peffect);
	delete peffect;
}
int32 duel::read_buffer(byte* buf) {
	generate_buffer();
	if(buf != nullptr)
		if(buff.size())
			std::memcpy(buf, buff.data(), buff.size());
	return buff.size();
}
void duel::generate_buffer() {
	for(auto& message : messages) {
		uint32_t size = message.data.size();
		if(size) {
			write_buffer(&size, sizeof(size));
			write_buffer(message.data.data(), size);
		}
	}
	messages.clear();
}
void duel::release_script_group() {
	for(auto& pgroup : sgroups) {
		if(pgroup->is_readonly == 0) {
			lua->unregister_group(pgroup);
			groups.erase(pgroup);
			delete pgroup;
		}
	}
	sgroups.clear();
}
void duel::restore_assumes() {
	for(auto& pcard : assumes)
		pcard->assume.clear();
	assumes.clear();
}
void duel::write_buffer(void* data, size_t size) {
	const auto vec_size = buff.size();
	buff.resize(vec_size + size);
	if(size)
		std::memcpy(&buff[vec_size], data, size);
}
void duel::clear_buffer() {
	buff.clear();
}
void duel::set_response(const void* resp, size_t len) {
	game_field->returns.clear();
	game_field->returns.data.resize(len);
	if(len)
		std::memcpy(game_field->returns.data.data(), resp, len);
}
int32 duel::get_next_integer(int32 l, int32 h) {
	return (std::uniform_int_distribution<>(l, h))(random);
}
duel::duel_message* duel::new_message(uint32_t message) {
	messages.emplace_back(message);
	return &(*messages.rbegin());
}
card_data const* duel::read_card(uint32_t code, card_data* copyable) {
	card_data* ret;
	auto search = data_cache.find(code);
	if(search != data_cache.end()) {
		ret = &(search->second);
	} else {
		OCG_CardData data;
		read_card_api(read_card_payload, code, &data);
		ret = &(data_cache.emplace(code, &data).first->second);
		read_card_done(read_card_done_payload, &data);
	}
	if(copyable)
		*copyable = *ret;
	return ret;
}
duel::duel_message::duel_message(uint8_t _message) :message(_message) {
	write(message);
}
void duel::duel_message::write(void* buff, size_t size) {
	const auto vec_size = data.size();
	data.resize(vec_size + size);
	if(size)
		std::memcpy(&data[vec_size], buff, size);
}
void duel::duel_message::write(loc_info loc) {
	write<uint8_t>(loc.controler);
	write<uint8_t>(loc.location);
	write<uint32_t>(loc.sequence);
	write<uint32_t>(loc.position);
}

card_data::card_data(OCG_CardData* data) {
#define COPY(val) this->val = data->val;
	COPY(code);
	COPY(alias);
	COPY(type);
	COPY(level);
	COPY(attribute);
	COPY(race);
	COPY(attack);
	COPY(defense);
	COPY(lscale);
	COPY(rscale);
	COPY(link_marker);
#undef COPY
	if(data->setcodes) {
		uint16_t* setptr = data->setcodes;
		while(*setptr != 0) {
			this->setcodes.insert(*setptr);
			setptr++;
		}
	}
}
