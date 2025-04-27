# YGOPro script engine.

A bleeding-edge fork of [YGOPro core](https://github.com/Fluorohydride/ygopro) with updates to accommodate for new cards and features. It is incompatible with forks not derived from this one.

This is the core logic and lua script processor of YGOPro. It maintains a state engine with an exposed API for Lua scripts. This library can be built independently of YGOPro clients and power server technologies.

## Building
A C++17 compliant compiler is needed (Visual Studio 2017, GCC 7, Clang 5.0.0 or any newer version)
The project uses the lua library, compiled from source alongside the main project
and the premake5 build system.

Clone this repo recursively
```
git clone --recurse-submodules  https://github.com/edo9300/ygopro-core
```

### Visual Studio
Double click the file `generate.bat` in the scripts folder,
select your installed Visual Studio version and it will download the needed resources
and generate the Visual Studio solution, which can then be found in `build/ocgcore.sln`.

Pick the appropriate configuration and architecture and build the solution.
The `ocgcore` solution to build a static library, the `ocgcoreshared` solution to build a dynamic library.

### POSIX (including mingw)
You'll need to have premake5 installed.
To manually download premake on your system, run
```
./scripts/install-premake5.sh linux|windows|macosx #pass the right value according to your os
```
To configure the project run
```
./premake5 gmake2
```
To build run
```
make -Cbuild TARGET config=CONFIG
```
`TARGET` can either be `ocgcore` to build a static library or `ocgcoreshared` to build a dynamic library.

`CONFIG` can either be `debug` or `release`, on mingw the values can be instead `debug_win32`, `debug_x64`, `release_win32`, `release_x64`

### Android
You'll need to have the Android NDK installed (r16b or newer) and `ndk-build` available in your path.

The android project needs no configuration, just run the command
```
ndk-build
```

## License

EDOPro's core is free/libre and open source software licensed under the GNU Affero General Public License, version 3 or later. Please see [LICENSE](https://github.com/edo9300/ygopro-core/blob/master/LICENSE) and [COPYING](https://github.com/edo9300/ygopro-core/blob/master/COPYING) for more details.

Yu-Gi-Oh! is a trademark of Shueisha and Konami. This is not affiliated with or endorsed by Shueisha or Konami.

## C API for clients

### Informative

#### `void OCG_GetVersion(int* major, int* minor)`

Writes the ocgcore API version numbers at the provided addresses if they're not NULL.

### Lifecycle

#### `int OCG_CreateDuel(OCG_Duel* duel, OCG_DuelOptions options)`

Creates a new duel simulation with the specified `options` and saves the pointer in `duel`. No members of `options` may be NULL pointers or uninitialized. Returns a status code of type `OCG_DuelCreationStatus`.

#### `void OCG_DestroyDuel(OCG_Duel duel)`

Deallocates the `duel` instance created by `OCG_CreateDuel`.

#### `void OCG_DuelNewCard(OCG_Duel duel, OCG_NewCardInfo info)`

Add the card specified by `info` to the `duel`. This calls the provided `OCG_DataReader` handler with `info.code` and `OCG_ScriptReader` if the card script has not been loaded yet.

#### `void OCG_StartDuel(OCG_Duel duel)`

Start the `duel` simulation and state machine. Call this after all options and cards for the duel have been loaded.

### Processing a duel

#### `int OCG_DuelProcess(OCG_Duel duel)`

Runs the state machine to start the `duel` or after a waiting state requiring a player response. Returns are `OCG_DuelStatus` enum values.
- `OCG_DUEL_STATUS_END` Duel ended
- `OCG_DUEL_STATUS_AWAITING` Player response required
- `OCG_DUEL_STATUS_CONTINUE`

#### `void* OCG_DuelGetMessage(OCG_Duel duel, uint32_t* length)`

The main interface to the simulation. Returns a pointer to the internal buffer containing all binary messages from the `duel` simulation. Subsequent calls invalidate previous buffers, so make a copy! The size of the buffer is written to `length` if it's not NULL.

See `common.h` for a list of all messages. The best protocol definitions for the message structure may be found at [YGOpen](https://github.com/DyXel/ygopen).

#### `void OCG_DuelSetResponse(OCG_Duel duel, const void* buffer, uint32_t length)`

Sets the next player response for the `duel` simulation. Subsequent calls overwrite previous responses if not processed. The contents of the provided `buffer` are copied internally, assuming it contains `length` bytes.

#### `int OCG_LoadScript(OCG_Duel duel, const char* buffer, uint32_t length, const char* name)`

Load a Lua card script or supporting script for the specified `duel.` Generally you do not call this directly except to load global scripts; instead you want to call this from your `OCG_ScriptReader` handler provided to `OCG_CreateDuel`. Returns positive on success and zero on failure.
- `buffer` Lua script code
- `length` Size of `buffer`
- `name` Unique identifier of the script

### Querying active duel states

#### `uint32_t OCG_DuelQueryCount(OCG_Duel duel, uint8_t team, uint32_t loc)`

Returns the number of cards in the specified zone.

#### `void* OCG_DuelQuery(OCG_Duel duel, uint32_t* length, OCG_QueryInfo info)`

Returns a pointer to an internal buffer for the FIRST card matching the query. The size of the buffer is written to `length` if it's not NULL. Subsequent calls invalidate previous queries.

#### `void* OCG_DuelQueryLocation(OCG_Duel duel, uint32_t* length, OCG_QueryInfo info)`

Returns a pointer to an internal buffer for the ALL cards matching the query. The size of the buffer is written to `length` if it's not NULL. Subsequent calls invalidate previous queries.

#### `void* OCG_DuelQueryField(OCG_Duel duel, uint32_t* length)`

Returns a pointer to an internal buffer containing card counts for every zone in the game. The size of the buffer is written to `length` if it's not NULL. Subsequent calls invalidate previous queries.

## Lua API for card scripts

See `interpreter.cpp`.
