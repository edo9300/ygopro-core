#include "neo_ocgapi.h"
#include "duel.h"

struct OCG_Duel {
	duel duel;
}

OCGAPI int OCG_LoadScript(OCG_Duel* duel, char* buffer, int length, char* scriptName) {
	return duel->duel.lua->load_script(buffer, length, scriptName)
}
