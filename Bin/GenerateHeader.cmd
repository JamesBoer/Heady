@echo off
setlocal

set HEADY_EXE=../Build/Release/Heady.exe
IF NOT EXIST "%HEADY_EXE%" (
  set HEADY_EXE=../Build/Debug/Heady.exe
)

IF NOT EXIST "%HEADY_EXE%" (
  echo Error: Heady.exe not found in Build/Release or Build/Debug.
  echo Please build the Heady project first ^(see BuildMSVC.cmd^).
  exit /b 1
)

rem Generate unified header file from all library source
"%HEADY_EXE%" --define HEADY_HEADER_ONLY --source "../Source" --output "../Include/Heady.hpp" --excluded "clara.hpp Main.cpp"
set HEADY_RESULT=%ERRORLEVEL%

IF NOT %HEADY_RESULT% EQU 0 (
  echo Heady failed with error code %HEADY_RESULT%
  exit /b %HEADY_RESULT%
)

echo Heady.hpp generated successfully.
exit /b 0
