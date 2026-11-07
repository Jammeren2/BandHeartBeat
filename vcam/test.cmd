@echo off
rem Builds and runs graph_test.exe against out\BandHeartBeatCamera.dll.
setlocal
call "%~dp0build.cmd" || exit /b 1
call "D:\VSBuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul 2>nul
set OUT=%~dp0out
cl /nologo /std:c++17 /O2 /EHsc /W3 /utf-8 "%~dp0graph_test.cpp" /Fo"%OUT%\\" /Fe"%OUT%\graph_test.exe" ^
   /link ole32.lib strmiids.lib || exit /b 1
"%OUT%\graph_test.exe" 32 30 || exit /b 1
"%OUT%\graph_test.exe" 24 15 || exit /b 1
