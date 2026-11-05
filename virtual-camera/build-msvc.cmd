@echo off
setlocal
call "D:\VSBuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%
set OUT=%~dp0out
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /std:c++17 /EHsc /W4 /LD virtual-camera\source_dll.cpp /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" /link /OUT:"%OUT%\BandHeartBeatSource.dll" ole32.lib mfplat.lib mfsensorgroup.lib mfuuid.lib
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /EHsc /W4 virtual-camera\source_probe.cpp /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" /link /OUT:"%OUT%\source-probe.exe" ole32.lib mfplat.lib mfsensorgroup.lib
cl /nologo /std:c++17 /EHsc /W4 virtual-camera\activation_probe.cpp /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I"C:\Program Files (x86)\Windows Kits\10.0.26100.0\shared" /link /OUT:"%OUT%\activation-probe.exe" ole32.lib mfplat.lib mfsensorgroup.lib
exit /b %errorlevel%
