@ECHO OFF
SETLOCAL

SET LUA_VERSION=5.3.6
SET LUA_ZIP="tmp\lua-%LUA_VERSION%.zip"

REM ensure the script is always running inside the core's root directory
CD %~dp0..

MKDIR tmp

IF EXIST "lua\src" RMDIR /s /q "lua\src"
IF EXIST "lua\src" EXIT /B 1
IF EXIST %LUA_ZIP% GOTO EXTRACT

CALL "%~DP0cmd\download.bat" ^
	https://github.com/lua/lua/archive/refs/tags/v%LUA_VERSION%.zip %LUA_ZIP%
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

:EXTRACT
CALL "%~DP0cmd/unzip.bat" %LUA_ZIP% lua
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
REN "lua/lua-%LUA_VERSION%" src
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

SET LUACONF="lua\src\luaconf.h"
>>%LUACONF%  ECHO.
>>%LUACONF%  ECHO #ifdef LUA_USE_WINDOWS
>>%LUACONF%  ECHO #undef LUA_USE_WINDOWS
>>%LUACONF%  ECHO #undef LUA_DL_DLL
>>%LUACONF%  ECHO #undef LUA_USE_C89
>>%LUACONF%  ECHO #endif
>>%LUACONF%  ECHO.

EXIT /B
