@echo off
call "D:\VSBuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
set OUT=%~dp0out
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /std:c++17 /O2 /EHsc /MT /W4 /Fo"%OUT%\source.obj" /LD directshow-camera\source.cpp /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" /link /DEF:directshow-camera\\source.def /IMPLIB:"%OUT%\source.lib" /OUT:"%OUT%\BandHeartBeatDirectShow.dll" ole32.lib strmiids.lib advapi32.lib gdi32.lib user32.lib
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /O2 /EHsc /MT /W4 /Fo"%OUT%\probe.obj" directshow-camera\probe.cpp /link /OUT:"%OUT%\filter-probe.exe" ole32.lib strmiids.lib advapi32.lib gdi32.lib user32.lib

if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /O2 /EHsc /MT /utf-8 /W4 /Fo"%OUT%\producer.obj" directshow-camera\producer.cpp /link /OUT:"%OUT%\camera-producer.exe" ole32.lib mfplat.lib mfreadwrite.lib mfuuid.lib mf.lib windowsapp.lib winhttp.lib gdi32.lib user32.lib
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /O2 /EHsc /MT /W4 /Fo"%OUT%\camera_admin.obj" directshow-camera\camera_admin.cpp /link /OUT:"%OUT%\camera-admin.exe" ole32.lib oleaut32.lib strmiids.lib
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /O2 /EHsc /MT /utf-8 /W4 /Fo"%OUT%\desktop.obj" directshow-camera\desktop.cpp /link /SUBSYSTEM:WINDOWS /OUT:"%OUT%\BandHeartBeat.exe" user32.lib gdi32.lib shell32.lib advapi32.lib uxtheme.lib dwmapi.lib
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /O2 /EHsc /MT /utf-8 /W4 /DBH_ISOLATED_TEST /Fo"%OUT%\producer_test.obj" directshow-camera\producer.cpp /link /OUT:"%OUT%\camera-producer-test.exe" ole32.lib mfplat.lib mfreadwrite.lib mfuuid.lib mf.lib windowsapp.lib winhttp.lib gdi32.lib user32.lib
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /O2 /EHsc /MT /utf-8 /W4 /DBH_ISOLATED_TEST /Fo"%OUT%\controls_probe.obj" directshow-camera\controls_probe.cpp /link /OUT:"%OUT%\controls-probe.exe" user32.lib gdi32.lib
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /O2 /EHsc /MT /utf-8 /W4 /DBH_ISOLATED_TEST /Fo"%OUT%\desktop_test.obj" directshow-camera\desktop.cpp /link /SUBSYSTEM:WINDOWS /OUT:"%OUT%\desktop-test.exe" user32.lib gdi32.lib shell32.lib advapi32.lib uxtheme.lib dwmapi.lib
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /O2 /EHsc /MT /utf-8 /W4 /Fo"%OUT%\tracking_probe.obj" directshow-camera\tracking_probe.cpp /link /OUT:"%OUT%\tracking-probe.exe"
