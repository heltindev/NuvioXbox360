@echo off
setlocal EnableExtensions
rem Use the script's folder only when it is the actual VS2010 project.
set "PROJECT=%~dp0"
if not exist "%PROJECT%NUVIO.sln" set "PROJECT=%USERPROFILE%\Documents\NuvioXbox360-main\VisualStudio2010\"
if not exist "%PROJECT%NUVIO.sln" goto missing_project
cd /d "%PROJECT%"
if errorlevel 1 goto missing_project
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
goto finished
:missing_project
echo.
echo ERRO: NUVIO.sln nao encontrado na pasta do CMD ou em:
echo %USERPROFILE%\Documents\NuvioXbox360-main\VisualStudio2010
echo Mova o CMD para a pasta VisualStudio2010 do projeto.
:finished
echo.
pause
endlocal
