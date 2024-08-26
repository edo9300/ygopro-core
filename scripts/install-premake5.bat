@ECHO OFF
SETLOCAL

SET PREMAKE_VERSION=5.0.0-beta2

CALL "%~DP0cmd/download.bat" ^
	https://github.com/premake/premake-core/releases/download/v%PREMAKE_VERSION%/premake-%PREMAKE_VERSION%-windows.zip premake.zip
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

CALL "%~DP0cmd/unzip.bat" premake.zip .
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

EXIT /B