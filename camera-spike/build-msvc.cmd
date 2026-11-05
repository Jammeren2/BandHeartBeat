@echo off
setlocal
call "D:\VSBuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%
set OUT=%~dp0out
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /std:c++17 /EHsc /W4 /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" /c "%~dp0main.cpp" /Fo"%OUT%\main.obj"
if errorlevel 1 exit /b %errorlevel%
link /nologo "%OUT%\main.obj" /OUT:"%OUT%\camera-api-boundary.exe" /SUBSYSTEM:CONSOLE mfsensorgroup.lib mfplat.lib mfreadwrite.lib
exit /b %errorlevel%
