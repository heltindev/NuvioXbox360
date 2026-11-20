@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
if errorlevel 1 exit /b 1
pushd "%~dp0"
if not exist "obj\CloudTests" mkdir "obj\CloudTests"
cl /nologo /EHsc /W3 /D_CRT_SECURE_NO_WARNINGS /I"tests\stubs" /I"src" /I"ThirdParty\Headers" /I"ThirdParty\x360-homebrew-libs\https" "tests\NuvioCloudTests.cpp" /TP "ThirdParty\Headers\qrcodegen.c" /Fo"obj\CloudTests\\" /Fe"obj\CloudTests\NuvioCloudTests.exe"
if errorlevel 1 exit /b 1
"obj\CloudTests\NuvioCloudTests.exe"
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
