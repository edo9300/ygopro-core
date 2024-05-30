@ECHO OFF
SETLOCAL

SET LUA_VERSION=5.3.6
SET LUA_ZIP=lua-%LUA_VERSION%.zip

IF EXIST %LUA_ZIP% DEL /f /q %LUA_ZIP%
IF EXIST %LUA_ZIP% EXIT /B 1
IF EXIST "lua/src" RMDIR /s /q "lua/src"
IF EXIST "lua/src" EXIT /B 1

CALL "%~DP0cmd/download.bat" ^
	https://github.com/lua/lua/archive/refs/tags/v%LUA_VERSION%.zip lua-%LUA_VERSION%.zip
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

CALL "%~DP0cmd/unzip.bat" lua-%LUA_VERSION%.zip lua
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
REN "lua/lua-%LUA_VERSION%" src
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

SET LUACONF="lua/src/luaconf.h"
>>%LUACONF%  ECHO.
>>%LUACONF%  ECHO #ifdef LUA_USE_WINDOWS
>>%LUACONF%  ECHO #undef LUA_USE_WINDOWS
>>%LUACONF%  ECHO #undef LUA_DL_DLL
>>%LUACONF%  ECHO #undef LUA_USE_C89
>>%LUACONF%  ECHO #endif
>>%LUACONF%  ECHO.

EXIT /B
