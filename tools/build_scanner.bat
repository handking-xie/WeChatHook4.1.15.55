@echo off
setlocal
call "D:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
cl /EHsc /utf-8 /O2 /std:c++20 "d:\WeChat-Hook\tools\offset_scanner.cpp" /Fe:"d:\WeChat-Hook\tools\offset_scanner.exe" /Fo:"d:\WeChat-Hook\tools\offset_scanner.obj"
endlocal
