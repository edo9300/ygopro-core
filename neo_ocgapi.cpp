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
		return 0;
	auto duelPtr = new class duel();
	if(duelPtr == nullptr)
		return 0;
	duelPtr->random.seed(options.seed);
	duelPtr->game_field->core.duel_options = options.flags;
	if(options.cardReader) {
		duelPtr->read_card = options.cardReader;
		duelPtr->payload1 = options.payload1;
	}
	if(options.scriptReader) {
		duelPtr->read_script = options.scriptReader;
		duelPtr->payload2 = options.payload2;
	}
	if(options.errorHandler) {
		duelPtr->handle_message = options.errorHandler;
		duelPtr->payload3 = options.payload3;
	}
	*duel = static_cast<OCG_Duel>(duelPtr);
	return 1;
}

OCGAPI void OCG_DestroyDuel(OCG_Duel duel) {
	delete DUEL;
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
