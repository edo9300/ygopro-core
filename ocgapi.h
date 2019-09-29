/*
 * Copyright (C) 2019  Edoardo Lolletti (edo9300) and Dylam De La Torre (DyXel)
 * See version history for a full list of authors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
/**
 * @file
 * @brief The main C-level interface to ocgcore, exported to DLLs on Windows
 */
#ifndef OCGAPI_H
#define OCGAPI_H
#include "ocgapi_types.h"

/** @cond FALSE */
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
/** @endcond */

/**
 * Writes the ocgcore API version numbers at the provided addresses if they're not NULL.
 */
OCGAPI void OCG_GetVersion(int* major, int* minor);
/* OCGAPI void OCG_GetName(const char** name); Maybe created by git hash? */

/** 
 * \defgroup OCGAPI_DuelCreateDestroy Duel creation and destruction
 * @{
 */
/**
 * Creates a new duel simulation with the specified \p options and saves the pointer in \p duel.
 * @return OCG_DuelCreationStatus enum values
 */
OCGAPI int OCG_CreateDuel(OCG_Duel* duel, OCG_DuelOptions options);
/**
 * Deallocates the \p duel instance created by OCG_CreateDuel.
 */
OCGAPI void OCG_DestroyDuel(OCG_Duel duel);
/**
 * Add the card specified by \p info to the \p duel.
 */
OCGAPI void OCG_DuelNewCard(OCG_Duel duel, OCG_NewCardInfo info);
/**
 * Start the \p duel simulation and state machine.
 * Call this after all options and cards for the duel have been loaded.
 */
OCGAPI void OCG_StartDuel(OCG_Duel duel);
/** @} */

/**
 * \defgroup OCGAPI_DuelProcess Duel processing
 * @{
 */
/**
 * Runs the state machine to start the \p duel or after a waiting state requiring a player response.
 * Returns are OCG_DuelStatus enum values.
 * @return OCG_DUEL_STATUS_END Duel ended
 * @return OCG_DUEL_STATUS_AWAITING Player response required
 * @return OCG_DUEL_STATUS_CONTINUE
 */
OCGAPI int OCG_DuelProcess(OCG_Duel duel);
/**
 * The main interface to the simulation.
 * Returns a pointer to the internal buffer containing all messages from the \p duel simulation.
 * Subsequent calls invalidate previous buffers, so make a copy!
 * The size of the buffer is written to \p length if it's not NULL.
 *
 * MESSAGE STRUCTURE TODO
 */
OCGAPI void* OCG_DuelGetMessage(OCG_Duel duel, uint32_t* length);
/**
 * Sets the next player response for the \p duel simulation.
 * Subsequent calls overwrite previous responses if not processed.
 * @param buffer Pointer to player response message
 * @param length Size of buffer
 *
 * RESPONSE STRUCTURE TODO
 */ 
OCGAPI void OCG_DuelSetResponse(OCG_Duel duel, void* buffer, uint32_t length);
/**
 * Load a Lua card script or supporting script for the specified \p duel.
 * @param buffer Lua script code
 * @param length Size of buffer
 * @param name Unique identifier of the script
 * @return Positive on success, zero on failure
 */
OCGAPI int OCG_LoadScript(OCG_Duel duel, const char* buffer, uint32_t length, const char* name);
/** @} */

/** 
 * \defgroup OCGAPI_DuelQuery Querying active duel states
 * @{
 */
/**
 * Returns the number of cards in the specified zone.
 */
OCGAPI uint32_t OCG_DuelQueryCount(OCG_Duel duel, uint8_t team, uint32_t loc);
/**
 * Returns a pointer to an internal buffer for the FIRST card matching the query.
 * The size of the buffer is written to \p length if it's not NULL.
 * Subsequent calls invalidate previous queries.
 */
OCGAPI void* OCG_DuelQuery(OCG_Duel duel, uint32_t* length, OCG_QueryInfo info);
/**
 * Returns a pointer to an internal buffer for the ALL cards matching the query.
 * The size of the buffer is written to \p length if it's not NULL.
 * Subsequent calls invalidate previous queries.
 */
OCGAPI void* OCG_DuelQueryLocation(OCG_Duel duel, uint32_t* length, OCG_QueryInfo info);
/**
 * Returns a pointer to an internal buffer containing card counts for every zone in the game.
 * The size of the buffer is written to \p length if it's not NULL.
 * Subsequent calls invalidate previous queries.
 */
OCGAPI void* OCG_DuelQueryField(OCG_Duel duel, uint32_t* length);
/** @} */

#endif /* OCGAPI_H */
