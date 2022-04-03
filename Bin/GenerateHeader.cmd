@echo off

rem Generate unified header file from all library source
"../Build/Release/Heady.exe" --define HEADY_HEADER_ONLY --source "../Source" --output "../Include/Heady.hpp" --excluded "clara.hpp Main.cpp"


