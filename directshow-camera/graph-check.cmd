@echo off
call "D:\VSBuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
cl /nologo /std:c++17 /O2 /EHsc /MT /W4 /DBH_ISOLATED_TEST /Fodirectshow-camera\out\source_test.obj /LD directshow-camera\source.cpp /link /DEF:directshow-camera\source.def /IMPLIB:directshow-camera\out\source_test.lib /OUT:directshow-camera\out\BandHeartBeatDirectShow-test.dll ole32.lib strmiids.lib advapi32.lib gdi32.lib user32.lib
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /O2 /EHsc /MT /W4 /DBH_ISOLATED_TEST /Fodirectshow-camera\out\graph_probe.obj directshow-camera\graph_probe.cpp /link /OUT:directshow-camera\out\graph-probe.exe ole32.lib strmiids.lib
if errorlevel 1 exit /b %errorlevel%
directshow-camera\out\graph-probe.exe

if errorlevel 1 exit /b %errorlevel%
directshow-camera\out\graph-probe.exe --rgb24

for %%F in (15 20 24 25 30) do (
 directshow-camera\out\graph-probe.exe --rgb24 %%F --consumer
 if errorlevel 1 exit /b 1
)

cl /nologo /std:c++17 /EHsc /MT /W4 /Fodirectshow-camera\out\consumer_format_probe.obj directshow-camera\consumer_format_probe.cpp /link /OUT:directshow-camera\out\consumer-format-probe.exe ole32.lib strmiids.lib
if errorlevel 1 exit /b %errorlevel%
directshow-camera\out\consumer-format-probe.exe
if errorlevel 1 exit /b %errorlevel%
if exist dist\desktop-v5\BandHeartBeatDirectShow.dll directshow-camera\out\consumer-format-probe.exe dist\desktop-v5\BandHeartBeatDirectShow.dll --old

if errorlevel 1 exit /b %errorlevel%
directshow-camera\out\graph-probe.exe --rgb24 30 --consumer --fail-commit

if errorlevel 1 exit /b %errorlevel%
directshow-camera\out\graph-probe.exe --rgb32 30 --consumer --contention
if errorlevel 1 exit /b %errorlevel%
directshow-camera\out\graph-probe.exe --rgb24 30 --consumer --contention
