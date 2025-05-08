/*
 * Copyright (c) 2010-2015, Argon Sun (Fluorohydride)
 * Copyright (c) 2017-2025, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <array>
#include <cstring> //std::memcpy
#include "card.h"
#include "duel.h"
#include "effect.h"
#include "field.h"
#include "interpreter.h"

duel::duel(const OCG_DuelOptions& options, bool& valid_lua_lib) :
	random({ options.seed[0], options.seed[1], options.seed[2], options.seed[3] }),
	read_card_callback(options.cardReader), read_script_callback(options.scriptReader),
	handle_message_callback(options.logHandler), read_card_done_callback(options.cardReaderDone),
	read_card_payload(options.payload1), read_script_payload(options.payload2),
	handle_message_payload(options.payload3), read_card_done_payload(options.payload4)
{
	lua = new interpreter(this, options, valid_lua_lib);
	if(!valid_lua_lib)
		return;
	game_field = new field(this, options);
	game_field->temp_card = new_card(0);
}
duel::~duel() {
	for(auto& pcard : cards)
		delete pcard;
	for(auto& pgroup : groups) {
		pgroup->container.clear();
		pgroup->is_iterator_dirty = true;
	}
	for(auto& peffect : effects)
		delete peffect;
	delete game_field;
	delete lua;
	// TODO: this should actually be an assertion as no group should outlive the lua state
	for(auto& pgroup : groups)
		delete pgroup;
}
#if defined(__GNUC__) || defined(__clang_analyzer__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
void duel::clear() {
	static constexpr OCG_DuelOptions default_options{ {},0,{8000,5,1},{8000,5,1} };
	for(auto& pcard : cards)
		delete pcard;
	for(auto& peffect : effects) {
		lua->unregister_effect(peffect);
		delete peffect;
	}
	delete game_field;
	//force full garbage collection to clean the groups
	lua->collect(true);
	cards.clear();
	/*
		TODO: how to properly handle groups that are still around after the field was destroyed?
		If they're still here, it means they're still living as global variables somewhere, for now we
		deal with them by clearing their containers that could have been pointing to now deleted cards
	*/
	for(auto& pgroup : groups) {
		pgroup->container.clear();
		pgroup->is_iterator_dirty = true;
	}
	effects.clear();
	game_field = new field(this, default_options);
	game_field->temp_card = new_card(0);
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
card* duel::new_card(uint32_t code) {
	card* pcard = new card(this);
	cards.insert(pcard);
	if(code)
		pcard->data = read_card(code);
	pcard->data.code = code;
	lua->register_card(pcard);
	return pcard;
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
void duel::delete_group_no_unref(group* pgroup) {
	groups.erase(pgroup);
	delete pgroup;
}
void duel::delete_effect(effect* peffect) {
	lua->unregister_effect(peffect);
	effects.erase(peffect);
	delete peffect;
}
void duel::generate_buffer() {
	for(auto& message : messages) {
		uint32_t size = static_cast<uint32_t>(message.data.size());
		if(size == 0)
			continue;
		write_buffer(&size, sizeof(size));
		write_buffer(message.data.data(), size);
	}
	messages.clear();
}
void duel::release_script_group() {
}
void duel::restore_assumes() {
	for(auto& pcard : assumes)
		pcard->assume.clear();
	assumes.clear();
}
void duel::write_buffer(const void* data, size_t size) {
	if(size == 0)
		return;
	const auto vec_size = buff.size();
	buff.resize(vec_size + size);
	std::memcpy(&buff[vec_size], data, size);
}
void duel::clear_buffer() {
	buff.clear();
}
void duel::set_response(const void* resp, size_t len) {
	game_field->returns.data.resize(len);
	if(len == 0)
		return;
	std::memcpy(game_field->returns.data.data(), resp, len);
}
// uniform integer distribution
int32_t duel::get_next_integer(int32_t l, int32_t h) {
	assert(l <= h);
	const uint64_t range = int64_t(h) - int64_t(l) + 1;
	const uint64_t lim = random.max() % range;
	uint64_t n;
	do {
		n = random();
	} while(n <= lim);
	return static_cast<int32_t>((n % range) + l);
}
duel::duel_message* duel::new_message(uint8_t message) {
	return &(messages.emplace_back(message));
}
const card_data& duel::read_card(uint32_t code) {
	if(auto search = data_cache.find(code); search != data_cache.end())
		return search->second;
	OCG_CardData data{};
	read_card_callback(read_card_payload, code, &data);
	auto ret = &(data_cache.emplace(code, data).first->second);
	read_card_done_callback(read_card_done_payload, &data);
	return *ret;
}
duel::duel_message::duel_message(uint8_t message) {
	write<uint8_t>(message);
}
void duel::duel_message::write(const void* buff, size_t size) {
	if(size == 0)
	   return;
	const auto vec_size = data.size();
	data.resize(vec_size + size);
	std::memcpy(&data[vec_size], buff, size);
}
void duel::duel_message::write(loc_info loc) {
	write<uint8_t>(loc.controler);
	write<uint8_t>(loc.location);
	write<uint32_t>(loc.sequence);
	write<uint32_t>(loc.position);
}

card_data::card_data(const OCG_CardData& data) {
#define COPY(val) val = data.val;
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
	if(data.setcodes == nullptr)
		return;
	uint16_t sc = 0;
	uint16_t* ptr = data.setcodes;
	for(;;) {
		std::memcpy(&sc, ptr++, sizeof(uint16_t));
		if(sc == 0)
			break;
		setcodes.insert(sc);
	}
}
