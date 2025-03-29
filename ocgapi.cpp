/*
 * Copyright (c) 2019, Dylam De La Torre, (DyXel)
 * Copyright (c) 2019-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <cstring> //std::memcpy
#include <new> //std::nothrow
#include <vector>
#include "ocgapi.h"
#include "interpreter.h"
#include "duel.h"
#include "field.h"
#include "effect.h"

OCGAPI void OCG_GetVersion(int* major, int* minor) {
	if(major)
		*major = OCG_VERSION_MAJOR;
	if(minor)
		*minor = OCG_VERSION_MINOR;
}

OCGAPI int OCG_CreateDuel(OCG_Duel* out_ocg_duel, OCG_DuelOptions options) {
	if(out_ocg_duel == nullptr)
		return OCG_DUEL_CREATION_NO_OUTPUT;
	if(options.cardReader == nullptr) {
		*out_ocg_duel = nullptr;
		return OCG_DUEL_CREATION_NULL_DATA_READER;
	}
	if(options.scriptReader == nullptr) {
		*out_ocg_duel = nullptr;
		return OCG_DUEL_CREATION_NULL_SCRIPT_READER;
	}
	if(options.logHandler == nullptr) {
		options.logHandler = [](void* /*payload*/, const char* /*string*/, int /*type*/) {};
		options.payload3 = nullptr;
	}
	if(options.cardReaderDone == nullptr) {
		options.cardReaderDone = [](void* /*payload*/, OCG_CardData* /*data*/) {};
		options.payload4 = nullptr;
	}
	auto* duelPtr = new (std::nothrow) duel(options);
	if(duelPtr == nullptr)
		return OCG_DUEL_CREATION_NOT_CREATED;
	*out_ocg_duel = static_cast<OCG_Duel>(duelPtr);
	return OCG_DUEL_CREATION_SUCCESS;
}

OCGAPI void OCG_DestroyDuel(OCG_Duel ocg_duel) {
	if(ocg_duel)
		delete static_cast<duel*>(ocg_duel);
}

OCGAPI void OCG_DuelNewCard(OCG_Duel ocg_duel, OCG_NewCardInfo info) {
	auto* pduel = static_cast<duel*>(ocg_duel);
	if(bit::popcnt(info.loc) != 1)
		return;
	auto& game_field = *(pduel->game_field);
	if(info.duelist == 0) {
		if(game_field.is_location_useable(info.con, info.loc, info.seq)) {
			card* pcard = pduel->new_card(info.code);
			pcard->owner = info.team;
			game_field.add_card(info.con, pcard, (uint8_t)info.loc, (uint8_t)info.seq);
			pcard->current.position = info.pos;
			if(!(info.loc & LOCATION_ONFIELD) || (info.pos & POS_FACEUP)) {
				pcard->enable_field_effect(true);
				game_field.adjust_instant();
			}
			if(info.loc & LOCATION_ONFIELD) {
				if(info.loc == LOCATION_MZONE)
					pcard->set_status(STATUS_PROC_COMPLETE, TRUE);
			}
		}
	} else {
		if(info.team > 1 || !(info.loc & (LOCATION_DECK | LOCATION_EXTRA)))
			return;
		card* pcard = pduel->new_card(info.code);
		auto& player = game_field.player[info.team];
		if(info.duelist > player.extra_lists_main.size()) {
			player.extra_lists_main.resize(info.duelist);
			player.extra_lists_extra.resize(info.duelist);
			player.extra_lists_hand.resize(info.duelist);
			player.extra_extra_p_count.resize(info.duelist);
		}
		--info.duelist;
		pcard->current.location = static_cast<uint8_t>(info.loc);
		pcard->owner = info.team;
		pcard->current.controler = info.team;
		pcard->current.position = POS_FACEDOWN_DEFENSE;
		auto& list = (info.loc == LOCATION_DECK) ? player.extra_lists_main[info.duelist] : player.extra_lists_extra[info.duelist];
		list.push_back(pcard);
		pcard->current.sequence = static_cast<uint32_t>(list.size() - 1);
	}
}

OCGAPI void OCG_StartDuel(OCG_Duel ocg_duel) {
	auto* pduel = static_cast<duel*>(ocg_duel);
	pduel->game_field->emplace_process<Processors::Startup>();
}

OCGAPI int OCG_DuelProcess(OCG_Duel ocg_duel) {
	auto* pduel = static_cast<duel*>(ocg_duel);
	pduel->buff.clear();
	auto flag = OCG_DUEL_STATUS_END;
	do {
		flag = pduel->game_field->process();
		pduel->generate_buffer();
	} while(pduel->buff.size() == 0 && flag == OCG_DUEL_STATUS_CONTINUE);
	return flag;
}

OCGAPI void* OCG_DuelGetMessage(OCG_Duel ocg_duel, uint32_t* length) {
	auto* pduel = static_cast<duel*>(ocg_duel);
	pduel->generate_buffer();
	if(length)
		*length = static_cast<uint32_t>(pduel->buff.size());
	return pduel->buff.data();
}

OCGAPI void OCG_DuelSetResponse(OCG_Duel ocg_duel, const void* buffer, uint32_t length) {
	auto* pduel = static_cast<duel*>(ocg_duel);
	pduel->set_response(buffer, length);
}

OCGAPI int OCG_LoadScript(OCG_Duel ocg_duel, const char* buffer, uint32_t length, const char* name) {
	auto* pduel = static_cast<duel*>(ocg_duel);
	return pduel->lua->load_script(buffer, length, name);
}

OCGAPI uint32_t OCG_DuelQueryCount(OCG_Duel ocg_duel, uint8_t team, uint32_t loc) {
	auto* pduel = static_cast<duel*>(ocg_duel);
	if(team > 1)
		return 0;
	if(bit::popcnt(loc) != 1)
		return 0;
	auto& player = pduel->game_field->player[team];
	if(loc == LOCATION_HAND)
		return static_cast<uint32_t>(player.list_hand.size());
	if(loc == LOCATION_GRAVE)
		return static_cast<uint32_t>(player.list_grave.size());
	if (loc == LOCATION_REMOVED)
		return static_cast<uint32_t>(player.list_remove.size());
	if (loc == LOCATION_EXTRA)
		return static_cast<uint32_t>(player.list_extra.size());
	if (loc == LOCATION_DECK)
		return static_cast<uint32_t>(player.list_main.size());
	uint32_t count = 0;
	if(loc == LOCATION_MZONE) {
		for(auto& pcard : player.list_mzone)
			if(pcard) ++count;
	}
	if(loc == LOCATION_SZONE) {
		for(auto& pcard : player.list_szone)
			if(pcard) ++count;
	}
	return count;
}
template<typename T>
void insert_value_int(std::vector<uint8_t>& vec, T val) {
	const auto vec_size = vec.size();
	const auto val_size = sizeof(T);
	vec.resize(vec_size + val_size);
	std::memcpy(&vec[vec_size], &val, val_size);
}
template<typename T, typename T2>
ForceInline void insert_value(std::vector<uint8_t>& vec, T2 val) {
	insert_value_int<T>(vec, static_cast<T>(val));
}

OCGAPI void* OCG_DuelQuery(OCG_Duel ocg_duel, uint32_t* length, OCG_QueryInfo info) {
	auto* pduel = static_cast<duel*>(ocg_duel);
	if(bit::popcnt(info.loc & ~LOCATION_OVERLAY) != 1)
		return 0;
	pduel->query_buffer.clear();
	card* pcard = nullptr;
	if(info.loc & LOCATION_OVERLAY) {
		auto olcard = pduel->game_field->get_field_card(info.con, (info.loc & LOCATION_OVERLAY), info.seq);
		if(olcard && olcard->xyz_materials.size() > info.overlay_seq) {
			pcard = olcard->xyz_materials[info.overlay_seq];
		}
	} else {
		pcard = pduel->game_field->get_field_card(info.con, info.loc, info.seq);
	}
	if(pcard == nullptr) {
		if(length)
			*length = 0;
		return nullptr;
	} else {
		pcard->get_infos(info.flags);
	}
	if(length)
		*length = static_cast<uint32_t>(pduel->query_buffer.size());
	return pduel->query_buffer.data();
}

OCGAPI void* OCG_DuelQueryLocation(OCG_Duel ocg_duel, uint32_t* length, OCG_QueryInfo info) {
	auto* pduel = static_cast<duel*>(ocg_duel);
	auto& buffer = pduel->query_buffer;
	auto populate = [&flags = info.flags, &buffer](const card_vector& list) {
		for(auto& pcard : list) {
			if(pcard == nullptr) {
				insert_value<int16_t>(buffer, 0);
			} else {
				pcard->get_infos(flags);
			}
		}
	};
	buffer.clear();
	if(info.con <= 1u && bit::popcnt(info.loc & ~LOCATION_OVERLAY) == 1) {
		if(info.loc & LOCATION_OVERLAY) {
			insert_value<int16_t>(buffer, 0);
		} else {
			auto& player = pduel->game_field->player[info.con];
			if(info.loc == LOCATION_MZONE)
				populate(player.list_mzone);
			else if(info.loc == LOCATION_SZONE)
				populate(player.list_szone);
			else if(info.loc == LOCATION_HAND)
				populate(player.list_hand);
			else if(info.loc == LOCATION_GRAVE)
				populate(player.list_grave);
			else if(info.loc == LOCATION_REMOVED)
				populate(player.list_remove);
			else if(info.loc == LOCATION_EXTRA)
				populate(player.list_extra);
			else if(info.loc == LOCATION_DECK)
				populate(player.list_main);
		}
		std::vector<uint8_t> tmp_vector;
		insert_value<uint32_t>(tmp_vector, buffer.size());
		buffer.insert(buffer.begin(), tmp_vector.begin(), tmp_vector.end());
	}
	if(length)
		*length = static_cast<uint32_t>(buffer.size());
	return buffer.data();
}

OCGAPI void* OCG_DuelQueryField(OCG_Duel ocg_duel, uint32_t* length) {
	auto* pduel = static_cast<duel*>(ocg_duel);
	auto& query = pduel->query_buffer;
	query.clear();
	//insert_value<int8_t>(query, MSG_RELOAD_FIELD);
	insert_value<uint32_t>(query, pduel->game_field->core.duel_options);
	for(int playerid = 0; playerid < 2; ++playerid) {
		auto& player = pduel->game_field->player[playerid];
		insert_value<uint32_t>(query, player.lp);
		for(auto& pcard : player.list_mzone) {
			if(pcard) {
				insert_value<uint8_t>(query, 1);
				insert_value<uint8_t>(query, pcard->current.position);
				insert_value<uint32_t>(query, pcard->xyz_materials.size());
			} else {
				insert_value<uint8_t>(query, 0);
			}
		}
		for(auto& pcard : player.list_szone) {
			if(pcard) {
				insert_value<uint8_t>(query, 1);
				insert_value<uint8_t>(query, pcard->current.position);
				insert_value<uint32_t>(query, pcard->xyz_materials.size());
			} else {
				insert_value<uint8_t>(query, 0);
			}
		}
		insert_value<uint32_t>(query, player.list_main.size());
		insert_value<uint32_t>(query, player.list_hand.size());
		insert_value<uint32_t>(query, player.list_grave.size());
		insert_value<uint32_t>(query, player.list_remove.size());
		insert_value<uint32_t>(query, player.list_extra.size());
		insert_value<uint32_t>(query, player.extra_p_count);
	}
	insert_value<uint32_t>(query, pduel->game_field->core.current_chain.size());
	for(const auto& ch : pduel->game_field->core.current_chain) {
		effect* peffect = ch.triggering_effect;
		insert_value<uint32_t>(query, peffect->get_handler()->data.code);
		loc_info info = peffect->get_handler()->get_info_location();
		insert_value<uint8_t>(query, info.controler);
		insert_value<uint8_t>(query, info.location);
		insert_value<uint32_t>(query, info.sequence);
		insert_value<uint32_t>(query, info.position);
		insert_value<uint8_t>(query, ch.triggering_controler);
		insert_value<uint8_t>(query, static_cast<uint8_t>(ch.triggering_location));
		insert_value<uint32_t>(query, ch.triggering_sequence);
		insert_value<uint64_t>(query, peffect->description);
	}
	if(length)
		*length = static_cast<uint32_t>(query.size());
	return query.data();
}
