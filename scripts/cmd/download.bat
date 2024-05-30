@ECHO OFF
SETLOCAL
SET URL=%1
CALL :NORMALIZEPATH %2
SET DEST=%RETVAL%

WHERE curl.exe >NUL 2>NUL
IF %ERRORLEVEL% EQU 0 GOTO DOWNLOADCURL

bitsadmin  /transfer mydownloadjob /download /priority foreground ^
	%URL% "%DEST%" > NUL

IF %ERRORLEVEL% neq 0 EXIT /B %ERRORLEVEL%

EXIT /B

:DOWNLOADCURL
  curl --retry 5 --connect-timeout 30 --location %URL% -o "%DEST%"
  EXIT /B

:NORMALIZEPATH
  SET RETVAL=%~f1
  EXIT /B
