@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
if errorlevel 1 exit /b 1
pushd "%~dp0"
if not exist "obj\AuthTests" mkdir "obj\AuthTests"
cl /nologo /EHsc /W3 /I"tests\stubs" /I"src" /I"ThirdParty\Headers" /I"ThirdParty\x360-homebrew-libs\https" "tests\NuvioAuthTests.cpp" /TP "ThirdParty\Headers\qrcodegen.c" /Fo"obj\AuthTests\\" /Fe"obj\AuthTests\NuvioAuthTests.exe"
if errorlevel 1 exit /b 1
"obj\AuthTests\NuvioAuthTests.exe"
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
