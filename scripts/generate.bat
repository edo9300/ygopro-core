@ECHO OFF
SETLOCAL

REM ensure the script is always running inside the core's root directory
CD %~DP0..

CALL "%~DP0install-premake5.bat"

:SELECT
CLS
ECHO Select installed visual studio version
ECHO 1.vs2017
ECHO 2.vs2017 (Windows XP, requires toolset v141_xp to be installed)
ECHO 3.vs2019
ECHO 4.vs2022

ECHO.

SET /P OP="Enter your choice: "

IF %OP% == 4 GOTO VS2022
IF %OP% == 3 GOTO VS2019
IF %OP% == 2 GOTO VS2017_XP
IF %OP% == 1 GOTO VS2017

ECHO Invalid Option
GOTO SELECT

:CONFIGURE
premake5 %VISUAL_STUDIO_VERSION% %PREMAKE_EXTRA_FLAGS%
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
START build/ocgcore.sln

EXIT /B


:VS2017_XP
  SET PREMAKE_EXTRA_FLAGS="--oldwindows=true"
:VS2017
  SET VISUAL_STUDIO_VERSION=vs2017
  GOTO CONFIGURE

:VS2019
  SET VISUAL_STUDIO_VERSION=vs2019
  GOTO CONFIGURE

:VS2022
  SET VISUAL_STUDIO_VERSION=vs2022
  GOTO CONFIGURE
