@echo off
call "D:\VSBuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
set OUT=%~dp0out
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /std:c++17 /EHsc /W4 /LD directshow-camera\source.cpp /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" /link /DEF:directshow-camera\\source.def /OUT:"%OUT%\BandHeartBeatDirectShow.dll" ole32.lib strmiids.lib

