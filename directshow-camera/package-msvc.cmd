@echo off
setlocal
call directshow-camera\build-msvc.cmd
if errorlevel 1 exit /b %errorlevel%
cargo build --release --offline --locked --target x86_64-pc-windows-msvc --target-dir target/standalone-build
if errorlevel 1 exit /b %errorlevel%
if not exist dist\desktop-v2 mkdir dist\desktop-v2
copy /Y target\standalone-build\x86_64-pc-windows-msvc\release\band-heart-beat.exe dist\desktop-v2\band-backend.exe
copy /Y directshow-camera\out\BandHeartBeat.exe dist\desktop-v2\
copy /Y directshow-camera\out\BandHeartBeatDirectShow.dll dist\desktop-v2\
copy /Y directshow-camera\out\camera-producer.exe dist\desktop-v2\
copy /Y directshow-camera\out\camera-admin.exe dist\desktop-v2\
copy /Y directshow-camera\STANDALONE-RU.md dist\desktop-v2\README-RU.md

copy /Y directshow-camera\install-current-user.cmd dist\desktop-v2\
copy /Y directshow-camera\rollback.cmd dist\desktop-v2\
exit /b %errorlevel%
