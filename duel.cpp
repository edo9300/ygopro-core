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
#include "ocgapi.h"

duel::duel() {
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
		::read_card(code, &(pcard->data));
	else
		pcard->data.clear();
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
	if(buf != nullptr)
		std::memcpy(buf, buff.data(), buff.size());
	return buff.size();
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
	std::memcpy(&buff[vec_size], data, size);
}
template<typename T>
void duel::write_buffer(T data) {
	write_buffer(&data, sizeof(T));
}
void duel::write_info_location(loc_info* loc) {
	if(loc) {
		write_buffer8(loc->controler);
		write_buffer8(loc->location);
		write_buffer32(loc->sequence);
		write_buffer32(loc->position);
	} else {
		write_buffer16(0);
		write_buffer64(0);
	}
}
void duel::write_buffer64(uint64 value) {
	write_buffer<uint64>(value);
}
void duel::write_buffer32(uint32 value) {
	write_buffer<uint32>(value);
}
void duel::write_buffer16(uint16 value) {
	write_buffer<uint16>(value);
}
void duel::write_buffer8(uint8 value) {
	write_buffer<uint8>(value);
}
void duel::clear_buffer() {
	buff.clear();
}
void duel::set_responsei(uint32 resp) {
	game_field->returns.at<int32>(0) = resp;
}
void duel::set_responseb(byte* resp, size_t len) {
	game_field->returns.clear();
	game_field->returns.data.resize(len);
	std::memcpy(game_field->returns.data.data(), resp, len);
}
int32 duel::get_next_integer(int32 l, int32 h) {
	return (std::uniform_int_distribution<>(l, h))(random);
}
