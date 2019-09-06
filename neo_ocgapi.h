#ifndef OCGAPI_H
#define OCGAPI_H
#include <stdint.h>

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#if defined(WIN32) && defined(OCGCORE_EXPORT_FUNCTIONS)
#define OCGAPI EXTERN_C __declspec(dllexport)
#else
#define OCGAPI EXTERN_C
#endif

#define OCG_VERSION_MAJOR 4
#define OCG_VERSION_MINOR 0

/*** CORE INFORMATION ***/
OCGAPI void OCG_GetVersion(int* major, int* minor);
/* OCGAPI void OCG_GetName(const char** name); Maybe created by git hash? */

typedef void* OCG_Duel; /* Opaque pointer to a Duel class */

typedef struct CardData {
	uint32_t code;
	uint32_t alias;
	uint64_t setcode;
	uint32_t type;
	uint32_t level;
	uint32_t attribute;
	uint32_t race;
	int32_t attack;
	int32_t defense;
	uint32_t lscale;
	uint32_t rscale;
	uint32_t link_marker;
}CardData;

typedef int (*OCG_DataReader)(void* payload, int code, CardData* data);
typedef int (*OCG_ScriptReader)(void* payload, OCG_Duel duel, const char* scriptName);
typedef enum OCG_LogTypes {
	OCG_LOG_TYPE_ERROR,
	OCG_LOG_TYPE_FROM_SCRIPT,
	OCG_LOG_TYPE_UNDEFINED
}OCG_LogTypes;
typedef int (*OCG_LogHandler)(void* payload, char* string, int type);

/*** DUEL CREATION AND DESTRUCTION ***/
typedef struct OCG_DuelOptions {
	int seed;
	int flags;
	OCG_DataReader cardReader;
	void* payload1; // relayed to cardReader
	OCG_ScriptReader scriptReader;
	void* payload2; // relayed to scriptReader
	OCG_LogHandler logHandler;
	void* payload3; // relayed to errorHandler
}OCG_DuelOptions;
typedef enum OCG_DuelCreationStatus {
	OCG_DUEL_CREATION_SUCCESS,
	OCG_DUEL_CREATION_NO_OUTPUT,
	OCG_DUEL_CREATION_NOT_CREATED,
	OCG_DUEL_CREATION_NULL_DATA_READER,
	OCG_DUEL_CREATION_NULL_SCRIPT_READER
}OCG_DuelCreationStatus;
OCGAPI int OCG_CreateDuel(OCG_Duel* duel, OCG_DuelOptions options);
OCGAPI void OCG_DestroyDuel(OCG_Duel duel);

/*** DUEL SET UP ***/
typedef struct OCG_Player {
	int startingLP;
	int startingDrawCount;
	int drawCountPerTurn;
}OCG_Player;
OCGAPI void OCG_DuelPlayer(OCG_Duel duel, int pos, OCG_Player options);

typedef struct OCG_NewCardInfo {
	uint8_t team; /* either 0 or 1 */
	uint8_t duelist; /* index of original owner */
	uint32_t code;
	uint8_t controller;
	uint32_t location;
	uint32_t sequence;
	uint32_t position;
}OCG_NewCardInfo;
OCGAPI void OCG_DuelNewCard(OCG_Duel duel, OCG_NewCardInfo info);

OCGAPI int OCG_StartDuel(OCG_Duel duel);

/*** DUEL PROCESSING AND QUERYING ***/
typedef enum OCG_DuelStatus {
	OCG_DUEL_STATUS_END, /* Duel ended */
	OCG_DUEL_STATUS_AWAITING, /* Duel needs a response */
	OCG_DUEL_STATUS_CONTINUE /* Duel can continue execution */
}OCG_DuelStatus;
OCGAPI int OCG_DuelProcess(OCG_Duel duel);
OCGAPI const void* OCG_DuelGetMessage(OCG_Duel duel, int* retlen);

OCGAPI void OCG_DuelSetResponse(OCG_Duel duel, void* buffer, int length);

OCGAPI int OCG_LoadScript(OCG_Duel duel, char* buffer, int length, char* scriptName); //direct call to load_script


/* TODO queries */

#endif // OCGAPI_H
