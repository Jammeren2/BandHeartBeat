@echo off
rem Builds vcam\out\BandHeartBeatCamera.dll (x64, static CRT).
setlocal
set VSDEVCMD=D:\VSBuildTools\Common7\Tools\VsDevCmd.bat
if exist "%VSDEVCMD%" goto :found
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -property installationPath`) do set VSDEVCMD=%%i\Common7\Tools\VsDevCmd.bat
:found
call "%VSDEVCMD%" -arch=x64 -host_arch=x64 >nul 2>nul || exit /b 1
set SRC=%~dp0
set OUT=%SRC%out
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /std:c++17 /O2 /EHsc /MT /W4 /WX /utf-8 /DUNICODE /D_UNICODE /LD "%SRC%camera.cpp" ^
   /Fo"%OUT%\\" /Fe"%OUT%\BandHeartBeatCamera.dll" ^
   /link /DEF:"%SRC%camera.def" ole32.lib strmiids.lib gdi32.lib user32.lib
