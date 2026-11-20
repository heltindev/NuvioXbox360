@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
if errorlevel 1 exit /b 1
pushd "%~dp0"
if not exist "obj\NativeTests" mkdir "obj\NativeTests"
cl /nologo /EHsc /W3 /I"tests\http_stubs" /I"ThirdParty\x360-homebrew-libs\log" tests\NuvioHttpTests.cpp /Fo"obj\NativeTests\\" /Fe"obj\NativeTests\NuvioHttpTests.exe" /link ws2_32.lib
if errorlevel 1 exit /b 1
obj\NativeTests\NuvioHttpTests.exe %*
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
