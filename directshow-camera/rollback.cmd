@echo off
cd /d "%~dp0"
camera-producer.exe --stop
camera-admin.exe --unregister
pause
