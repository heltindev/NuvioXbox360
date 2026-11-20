@echo off
powershell.exe -NoProfile -File "%~dp0start_transcoder.ps1" %*
if errorlevel 1 pause
