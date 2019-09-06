#include "neo_ocgapi.h"
#include "interpreter.h"
#include "duel.h"

using api_duel = duel;

#define DUEL static_cast<api_duel*>(duel) 

OCGAPI void OCG_GetVersion(int* major, int* minor) {
	if(major)
		*major = OCG_VERSION_MAJOR;
	if(minor)
		*minor = OCG_VERSION_MINOR;
}

OCGAPI int OCG_LoadScript(OCG_Duel duel, char* buffer, int length, char* scriptName) {
	return DUEL->lua->load_script(buffer, length, scriptName);
}
