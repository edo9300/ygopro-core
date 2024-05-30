@ECHO OFF
SETLOCAL
CALL :NORMALIZEPATH %1
SET SOURCE=%RETVAL%
CALL :NORMALIZEPATH %2
SET DEST=%RETVAL%

CALL :UnZipFile "%DEST%" "%SOURCE%"
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
EXIT /B

:: ========== FUNCTIONS ==========

:NORMALIZEPATH
  SET RETVAL=%~f1
  EXIT /B

:UnZipFile <ExtractTo> <newzipfile>
SET VBS="%TEMP%\_.vbs"
IF EXIST %VBS% DEL /f /q %VBS%
>%VBS%  ECHO Set fso = CreateObject("Scripting.FileSystemObject")
>>%VBS% ECHO If NOT fso.FolderExists(%1) Then
>>%VBS% ECHO fso.CreateFolder(%1)
>>%VBS% ECHO End If
>>%VBS% ECHO set objShell = CreateObject("Shell.Application")
>>%VBS% ECHO set FilesInZip=objShell.NameSpace(%2).items
>>%VBS% ECHO Const NO_DIALOG_YES_TO_ALL = ^&H214^&
>>%VBS% ECHO objShell.NameSpace(%1).CopyHere FilesInZip, NO_DIALOG_YES_TO_ALL
>>%VBS% ECHO Set fso = Nothing
>>%VBS% ECHO Set objShell = Nothing
cscript //nologo %VBS%
SET LAST_ERROR=%ERRORLEVEL%
IF EXIST %VBS% DEL /f /q %VBS%
IF %LAST_ERROR% NEQ 0 EXIT /B %LAST_ERROR%
EXIT /B