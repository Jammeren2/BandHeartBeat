@echo off
setlocal
call directshow-camera\build-msvc.cmd
if errorlevel 1 exit /b %errorlevel%
cargo build --release --offline --locked --target x86_64-pc-windows-msvc --target-dir target/standalone-build
if errorlevel 1 exit /b %errorlevel%
if not exist dist\standalone mkdir dist\standalone
copy /Y target\standalone-build\x86_64-pc-windows-msvc\release\band-heart-beat.exe dist\standalone\BandHeartBeat.exe
copy /Y directshow-camera\out\BandHeartBeatDirectShow.dll dist\standalone\
copy /Y directshow-camera\out\camera-producer.exe dist\standalone\
copy /Y directshow-camera\out\camera-admin.exe dist\standalone\
copy /Y directshow-camera\STANDALONE-RU.md dist\standalone\README-RU.md

copy /Y directshow-camera\install-current-user.cmd dist\standalone\
copy /Y directshow-camera\rollback.cmd dist\standalone\
exit /b %errorlevel%
