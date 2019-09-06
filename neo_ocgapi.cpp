#include "neo_ocgapi.h"
#include "interpreter.h"
#include "duel.h"

#define DUEL static_cast<class duel*>(duel) 

OCGAPI void OCG_GetVersion(int* major, int* minor) {
	if(major)
		*major = OCG_VERSION_MAJOR;
	if(minor)
		*minor = OCG_VERSION_MINOR;
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
