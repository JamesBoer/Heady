@echo off

rem Generate unified header file from all library source
"../Build/Release/Heady.exe" --source "../Source" --output "../Include/Heady.hpp" --excluded "clara.hpp Main.cpp"


