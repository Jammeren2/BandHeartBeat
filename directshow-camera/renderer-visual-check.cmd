@echo off
call "D:\VSBuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
cl /nologo /std:c++17 /O2 /EHsc /MT /utf-8 /W4 /Fo"directshow-camera\out\renderer_visual_probe.obj" directshow-camera\renderer_visual_probe.cpp /link /OUT:"directshow-camera\out\renderer-visual-probe.exe" gdiplus.lib user32.lib gdi32.lib
if errorlevel 1 exit /b %errorlevel%
directshow-camera\out\renderer-visual-probe.exe
