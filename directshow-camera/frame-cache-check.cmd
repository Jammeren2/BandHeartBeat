@echo off
call "D:\VSBuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
cl /nologo /std:c++17 /O2 /EHsc /MT /W4 /DBH_ISOLATED_TEST /Fodirectshow-camera\out\source_test.obj /LD directshow-camera\source.cpp /link /DEF:directshow-camera\source.def /IMPLIB:directshow-camera\out\source_test.lib /OUT:directshow-camera\out\BandHeartBeatDirectShow-test.dll ole32.lib strmiids.lib advapi32.lib gdi32.lib user32.lib
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /O2 /EHsc /MT /W4 /DBH_ISOLATED_TEST /Fodirectshow-camera\out\frame_cache_probe.obj directshow-camera\frame_cache_probe.cpp /link /OUT:directshow-camera\out\frame-cache-probe.exe ole32.lib strmiids.lib
if errorlevel 1 exit /b %errorlevel%
directshow-camera\out\frame-cache-probe.exe
