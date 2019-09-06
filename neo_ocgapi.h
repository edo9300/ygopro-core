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

typedef enum OCG_LogTypes {
	OCG_LOG_TYPE_ERROR,
	OCG_LOG_TYPE_FROM_SCRIPT,
	OCG_LOG_TYPE_UNDEFINED
}OCG_LogTypes;

typedef enum OCG_DuelCreationStatus {
	OCG_DUEL_CREATION_SUCCESS,
	OCG_DUEL_CREATION_NO_OUTPUT,
	OCG_DUEL_CREATION_NOT_CREATED,
	OCG_DUEL_CREATION_NULL_DATA_READER,
	OCG_DUEL_CREATION_NULL_SCRIPT_READER
}OCG_DuelCreationStatus;

typedef enum OCG_DuelStatus {
	OCG_DUEL_STATUS_END,
	OCG_DUEL_STATUS_AWAITING,
	OCG_DUEL_STATUS_CONTINUE
}OCG_DuelStatus;

typedef void* OCG_Duel;

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

typedef struct OCG_Player {
	int startingLP;
	int startingDrawCount;
	int drawCountPerTurn;
}OCG_Player;

typedef int (*OCG_DataReader)(void* payload, int code, CardData* data);
typedef int (*OCG_ScriptReader)(void* payload, OCG_Duel duel, char* name);
typedef void (*OCG_LogHandler)(void* payload, char* string, int type);

typedef struct OCG_DuelOptions {
	int seed;
	int flags;
	OCG_Player team1;
	OCG_Player team2;
	OCG_DataReader cardReader;
	void* payload1; /* relayed to cardReader */
	OCG_ScriptReader scriptReader;
	void* payload2; /* relayed to scriptReader */
	OCG_LogHandler logHandler;
	void* payload3; /* relayed to errorHandler */
}OCG_DuelOptions;

typedef struct OCG_NewCardInfo {
	uint8_t team; /* either 0 or 1 */
	uint8_t duelist; /* index of original owner */
	uint32_t code;
	uint8_t con;
	uint32_t loc;
	uint32_t seq;
	uint32_t pos;
}OCG_NewCardInfo;

typedef struct OCG_QueryInfo {
	uint32_t flags;
	uint8_t con;
	uint32_t loc;
	uint32_t seq;
	uint32_t overlay_seq;
}OCG_QueryInfo;

/*** CORE INFORMATION ***/
OCGAPI void OCG_GetVersion(int* major, int* minor);
/* OCGAPI void OCG_GetName(const char** name); Maybe created by git hash? */

/*** DUEL CREATION AND DESTRUCTION ***/
OCGAPI int OCG_CreateDuel(OCG_Duel* duel, OCG_DuelOptions options);
OCGAPI void OCG_DestroyDuel(OCG_Duel duel);
OCGAPI void OCG_DuelNewCard(OCG_Duel duel, OCG_NewCardInfo info);
OCGAPI int OCG_StartDuel(OCG_Duel duel);

/*** DUEL PROCESSING AND QUERYING ***/
OCGAPI int OCG_DuelProcess(OCG_Duel duel);
OCGAPI void* OCG_DuelGetMessage(OCG_Duel duel, uint32_t* length);
OCGAPI void OCG_DuelSetResponse(OCG_Duel duel, void* buffer, uint32_t length);
OCGAPI int OCG_LoadScript(OCG_Duel duel, char* buffer, uint32_t length, char* name);

OCGAPI uint32_t OCG_DuelQueryCount(OCG_Duel duel, uint8_t team, uint8_t pos, uint32_t loc);
OCGAPI void* OCG_DuelQuery(OCG_Duel duel, int* length, OCG_QueryInfo info);
OCGAPI void* OCG_DuelQueryField(OCG_Duel duel, int* length);

#endif /* OCGAPI_H */
