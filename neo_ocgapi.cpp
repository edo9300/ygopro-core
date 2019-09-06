#include "neo_ocgapi.h"
#include "interpreter.h"
#include "duel.h"
#include "field.h"

#define DUEL (static_cast<class duel*>(duel))

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
		duelPtr->handle_message = (OCG_LogHandler)&[](void*, char*, int) { return 0; };
		duelPtr->payload3 = nullptr;
	} else {
		duelPtr->handle_message = options.logHandler;
		duelPtr->payload3 = options.payload3;
	}
	*duel = static_cast<OCG_Duel>(duelPtr);
	return OCG_DUEL_CREATION_SUCCESS;
}

OCGAPI void OCG_DestroyDuel(OCG_Duel duel) {
	delete DUEL;
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

OCGAPI const void* OCG_DuelGetMessage(OCG_Duel duel, int* retlen) {
	DUEL->generate_buffer();
	if(retlen)
		*retlen = DUEL->buff.size();
	return DUEL->buff.data();
}

OCGAPI int OCG_LoadScript(OCG_Duel duel, char* buffer, int length, char* scriptName) {
	return DUEL->lua->load_script(buffer, length, scriptName);
}
