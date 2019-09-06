#include "neo_ocgapi.h"
#include "interpreter.h"
#include "duel.h"
#include "field.h"

#define DUEL (static_cast<class duel*>(duel))

int DefaultLogHandler(void* payload, char* string, int type);

OCGAPI void OCG_GetVersion(int* major, int* minor) {
	if(major)
		*major = OCG_VERSION_MAJOR;
	if(minor)
		*minor = OCG_VERSION_MINOR;
}

OCGAPI int OCG_CreateDuel(OCG_Duel* duel, OCG_DuelOptions options) {
	if(duel == nullptr)
		return OCG_DUEL_CREATION_NO_OUTPUT;
	auto duelPtr = new class duel();
	if(duelPtr == nullptr)
		return OCG_DUEL_CREATION_NOT_CREATED;
	duelPtr->random.seed(options.seed);
	duelPtr->game_field->core.duel_options = options.flags;
	if(options.cardReader == nullptr) {
		delete duelPtr;
		return OCG_DUEL_CREATION_NULL_DATA_READER;
	}
	if(options.scriptReader == nullptr) {
		delete duelPtr;
		return OCG_DUEL_CREATION_NULL_SCRIPT_READER;
	}
	duelPtr->read_card = options.cardReader;
	duelPtr->payload1 = options.payload1;
	duelPtr->read_script = options.scriptReader;
	duelPtr->payload2 = options.payload2;
	if(options.logHandler == nullptr) {
		duelPtr->handle_message = &DefaultLogHandler;
		duelPtr->payload3 = nullptr;
	} else {
		duelPtr->handle_message = options.logHandler;
		duelPtr->payload3 = options.payload3;
	}
	auto& team = options.team1;
	for(int i = 0; i < 2; i++, team = options.team2) {
		duelPtr->game_field->player[i].lp = team.startingLP;
		duelPtr->game_field->player[i].start_lp = team.startingLP;
		duelPtr->game_field->player[i].start_count = team.startingDrawCount;
		duelPtr->game_field->player[i].draw_count = team.drawCountPerTurn;
	}
	*duel = static_cast<OCG_Duel>(duelPtr);
	return OCG_DUEL_CREATION_SUCCESS;
}

OCGAPI void OCG_DestroyDuel(OCG_Duel duel) {
	delete DUEL;
}

OCGAPI void OCG_DuelNewCard(OCG_Duel duel, OCG_NewCardInfo info) {
	auto game_field = DUEL->game_field;
	if(info.duelist == 0) {
		if(game_field->is_location_useable(info.con, info.loc, info.seq)) {
			card* pcard = DUEL->new_card(info.code);
			pcard->owner = info.team;
			game_field->add_card(info.con, pcard, info.loc, info.seq);
			pcard->current.position = info.pos;
			if(!(info.loc & LOCATION_ONFIELD) || (info.pos & POS_FACEUP)) {
				pcard->enable_field_effect(true);
				game_field->adjust_instant();
			}
			if(info.loc & LOCATION_ONFIELD) {
				if(info.loc == LOCATION_MZONE)
					pcard->set_status(STATUS_PROC_COMPLETE, TRUE);
			}
		}
	} else {
		if(info.team > 1 || !(info.loc & (LOCATION_DECK | LOCATION_EXTRA)))
			return;
		info.duelist--;
		card* pcard = DUEL->new_card(info.code);
		auto& player = game_field->player[info.team];
		if(info.duelist >= player.extra_lists_main.size()) {
			player.extra_lists_main.resize(info.duelist + 1);
			player.extra_lists_extra.resize(info.duelist + 1);
			player.extra_lists_hand.resize(info.duelist + 1);
			player.extra_extra_p_count.resize(info.duelist + 1);
		}
		pcard->current.location = info.loc;
		pcard->owner = info.team;
		pcard->current.controler = info.team;
		pcard->current.position = POS_FACEDOWN_DEFENSE;
		if(info.loc == LOCATION_DECK){
			auto& list = player.extra_lists_main[info.duelist];
			list.push_back(pcard);
			pcard->current.sequence = list.size() - 1;
		} else {
			auto& list = player.extra_lists_extra[info.duelist];
			list.push_back(pcard);
			pcard->current.sequence = list.size() - 1;
		}
	}
}

OCGAPI int OCG_StartDuel(OCG_Duel duel) {
	auto game_field = DUEL->game_field;
	game_field->core.shuffle_hand_check[0] = FALSE;
	game_field->core.shuffle_hand_check[1] = FALSE;
	game_field->core.shuffle_deck_check[0] = FALSE;
	game_field->core.shuffle_deck_check[1] = FALSE;
	game_field->add_process(PROCESSOR_STARTUP, 0, 0, 0, 0, 0);
	const p0start_count = game_field->player[0].start_count;
	if(p0start_count > 0)
		game_field->draw(0, REASON_RULE, PLAYER_NONE, 0, p0start_count);
	const p1start_count = game_field->player[1].start_count;
	if(p1start_count > 0)
		game_field->draw(0, REASON_RULE, PLAYER_NONE, 1, p1start_count);
	for(int p = 0; p < 2; p++) {
		const auto list_size = game_field->player[p].extra_lists_main.size();
		for(decltype(list_size) l = 0; l < list_size; l++) {
			auto& main = game_field->player[p].extra_lists_main[l];
			auto& hand = game_field->player[p].extra_lists_hand[l];
			const int start_count = game_field->player[p].start_count;
			for(int i = 0; i < start_count && main.size(); ++i) {
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
	game_field->add_process(PROCESSOR_TURN, 0, 0, 0, 0, 0);
	return 1;
}

OCGAPI int OCG_DuelProcess(OCG_Duel duel) {
	DUEL->buff.clear();
	int flag = 0;
	do {
		flag = DUEL->game_field->process();
		DUEL->generate_buffer();
	} while(DUEL->buff.size() == 0 && flag == PROCESSOR_FLAG_CONTINUE);
	return flag;
}

OCGAPI const void* OCG_DuelGetMessage(OCG_Duel duel, int* length) {
	DUEL->generate_buffer();
	if(length)
		*length = DUEL->buff.size();
	return DUEL->buff.data();
}

OCGAPI void OCG_DuelSetResponse(OCG_Duel duel, void* buffer, int length) {
	DUEL->set_response(static_cast<uint8_t*>(buffer), length);
}

OCGAPI int OCG_LoadScript(OCG_Duel duel, char* buffer, int length, char* name) {
	return DUEL->lua->load_script(buffer, length, name);
}

int DefaultLogHandler(void* payload, char* string, int type) {
	return 0;
}
