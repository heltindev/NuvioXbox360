@echo off
setlocal
cd /d "%~dp0"
echo ====================================================
echo NUVIO Xbox 360 - Restaurar bibliotecas e compilar XEX
echo Pasta: %CD%
echo ====================================================
echo.
echo Este processo guarda backup do bootstrap anterior e
echo baixa os scripts de compilacao do repositorio NuvioXbox360.
echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; $r=(Get-Location).Path; $u='https://raw.githubusercontent.com/heltindev/NuvioXbox360/fix/native-torrent-peer-discovery/VisualStudio2010'; $b=Join-Path $r 'bootstrap_dependencies.ps1'; if(Test-Path -LiteralPath $b){Copy-Item -LiteralPath $b -Destination ($b+'.backup-'+(Get-Date -Format 'yyyyMMddHHmmss'))}; Invoke-WebRequest -UseBasicParsing -Uri ($u+'/bootstrap_dependencies.ps1') -OutFile $b; Invoke-WebRequest -UseBasicParsing -Uri ($u+'/prepare_release.ps1') -OutFile (Join-Path $r 'prepare_release.ps1'); & (Join-Path $r 'prepare_release.ps1')"
if errorlevel 1 goto failed
echo.
echo Compilacao concluida. Confira bin\Release\NUVIO.xex
goto finished
:failed
echo.
echo ERRO: Preparacao ou compilacao falhou.
echo Copie a primeira mensagem de erro do PowerShell e envie no chat.
:finished
echo.
pause
endlocal
