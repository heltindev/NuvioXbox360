@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0prepare_release.ps1"
if errorlevel 1 (
    echo BUILD FAILED. Do not copy an older executable.
    pause
    exit /b 1
)
echo Copy Release\NUVIO.xex to your Xbox and check Xbox UI8 in Settings.
pause
