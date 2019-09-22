# YGOPro script engine. [![Build Status](https://travis-ci.org/edo9300/ygopro-core.svg?branch=master)](https://travis-ci.org/edo9300/ygopro-core)

A bleeding-edge fork of [YGOPro core](https://github.com/Fluorohydride/ygopro) with updates to accommodate for new cards and features. It is incompatible with forks not derived from this one.

This is the core logic and lua script processor of YGOPro. It maintains a state engine with an exposed API for Lua scripts. This library can be built independently of YGOPro clients and power server technologies.

## Compiling
See the [main EDOPro wiki](https://github.com/edo9300/ygopro/wiki/) for details.

## Exposed Functions

- `int_32 get_api_version(int_32* min)` : This function is used to get the current version of the api.

These three function need to be provided to the core so it can get card and database information.
- `void set_script_reader(script_reader f);` : Function used by the api to load a lua script corresponding to a card, the function structure is
  ```
  unsigned char* (const char* filename, int32_t* buffer_length)
  ```
  The first parameter is the name of the script to load, it's structured in this way: `c(id of the card that has to be loaded).lua`, the buffer_length value has to be set with the total size of the buffer returned by this function, to make the api load nothing, set this value to 0. The return value is a pointer to the buffer containing the lua script loaded into memory, make sure that this buffer is declared as static or stored somewhere to avoid it being corrupted.
  The api has a built in script reader that loads the scripts from the ` script ` folder in the working directory.

- `void set_card_reader(card_reader f);` : Function used by the api to get the informations about a card, the function structure is
  ```
  uint32_t (uint32_t code, card_data* data)
  ```
  Where the first parameter is the id card to load, the ` data ` parameter is a pointer to a struct that has to be filled with the various stats of the card.
  
  ```cpp
  struct card_data {
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
  };
  ```
  The return value isn't used.

- `void set_message_handler(message_handler f);` : Function used by the api to send to the host any state message generated by the api, the function structure is
  ```
  uint32_t (void* duel, uint32_t type)
  ```
  The fist parameter is a pointer to the duel object that generated that call, that pointer has to be used then in the ` get_log_message ` function to get the actualcontent of the message. The second parameter specifies the nature of the message, 1 means that it is due an error in the lua machine, 2 means it's currently a debug message generated via a script with the proper function.
  The return value isn't used.

These functions create the game itself and then manipulate it:
- `void* create_duel(uint32_t seed);` : Create a duel instance using the provided value as seed for the internal random number generator.
  The return value is a pointer to the created duel.
- `void start_duel(void* pduel, int32_t options);` : Starts the duel corresponding to the passed object, the second parameter are the various options to use in that duel, they can be found here https://github.com/edo9300/ygopro-core/blob/master/common.h#L377.
  Call this function everything has been loaded in the current instance, so the players' decks, the various info etc. If DUEL_RELAY is passed as option, and teams are present, the duel will be a relay duel, otherwise if teams are present and DUEL_RELAY isn't passed, the duel will be a tag like duel.
- `void end_duel(void* pduel);` : Ends the duel corresponding to the passed object, after that the object is destroyed and thus the pointer is invalidated.
- `void set_player_info(void* pduel, int32_t playerid, int32_t lp, int32_t startcount, int32_t drawcount);` : Sets the various starting values for the provided playerid. This value can be either 0 or 1, with 0 being the starting player.
- `void get_log_message(void* pduel, uint8_t* buf);` : Function to be called from the message handler callback, it copies the string (with max length of 256 bytes) that generated the call.
- `int32_t get_message(void* pduel, uint8_t* buf);` : This function is actually the main interface to the api, it writes to the provided buffer the buffer internal to the api (that then gets cleared) containing all the various messages from the engine. This function's return value is the total length of the buffer. If the function is called with a null pointer (or 0) as second parameter, it won't clear the internal buffer, and it also won't be copied, but will only return the length of the message, to be used subsequently to create a buffer of the appropriate size to read the full buffer from the api.
- `int32_t process(void* pduel);` : This function makes the engine run after a stall state, it has to be called to start the duel, and after a response has been set in messages that requires it. The return value range from 0 to 2, and it represents a "flag" in the engine, with 0 there's no special flag, 1 is `PROCESSOR_FLAG_WAITING`, that means that the api actually needs a reponse to be set (which one depends on the last message in the buffer), while 2 is `PROCESSOR_FLAG_END`, when this flag is returned, that means that the duel has ended.
- `void new_card(void* pduel, uint32_t code, uint8_t owner, uint8_t controller, uint8_t location, uint8_t sequence, uint8_t position, int32_t duelist);` : Add a card in the current duel accordning to the provided parameters, the sequence parameter is only used for on field locations, and for `LOCATION_DECK`, where it assumes different meanings, with 0 meaning deck top, and 1 meaning deck bottom. The duelist parameter is used if the duel should consist of a team duel, the 1st player of a team is player 0, and going on.
- `int32_t query_field_count(void* pduel, uint8_t playerid, uint8_t location);` : Get the number of cards in the provided location.
- `int32_t get_cached_query(void* pduel, uint8_t* buf);` : All the query functions below, if called with the `buf` parameter as a null pointer, will only return the total length of the buffer, and it'll be keep cached. To get that buffer content this function has to be used, after the call, the buffer will be cleared. The return value is the total length of the buffer written.
- `int32_t query_card(void* pduel, uint8_t playerid, uint8_t location, uint8_t sequence, int32_t query_flag, uint8_t* buf, int32_t use_cache);` : This function will write to the buffer (or cache) a query containing the info of the card corresponding to the given "coordinatesd". The return value is the total length of the buffer.
- `int32_t query_field_card(void* pduel, uint8_t playerid, uint8_t location, int32_t query_flag, uint8_t* buf, int32_t use_cache);` : This function will write to the buffer (or cache) a query containing the info of all the cards in the specified location. The return value is the total length of the buffer.
- `int32_t query_field_info(void* pduel, uint8_t* buf);` : This function will write to the buffer (or cache) a `MSG_RELOAD_FIELD` packet. This packet contains all the informations about the current state of the duel. The return value is the total length of the buffer.
- `void set_responsei(void* pduel, int32_t value);` : This function is used to set an int value as response after the `process` function returned a `PROCESSOR_FLAG_WAITING`.
- `void set_responseb(void* pduel, uint8_t* buf, size_t len);` : This function is used to set a buffer as response after the `process` function returned a `PROCESSOR_FLAG_WAITING`.
- `int32_t preload_script(void* pduel, uint8_t* script, int32_t len, int32_t scriptlen, char* scriptbuff);` : This function is used to load a script before `start_duel` is called. It can be used to load scripts such as the ones used to set up puzzles. If `scriptlen` and `scriptbuff` aren't null values, the api will load the script directly from `scriptbuff` without calling the callback function set with `set_script_reader`.

# Lua functions
`interpreter.cpp`
