@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
if errorlevel 1 exit /b 1
pushd "%~dp0"
if not exist "obj\NativeTests" mkdir "obj\NativeTests"
cl /nologo /EHsc /W3 /I"ThirdParty\x360-homebrew-libs\https" tests\NuvioTorrentTests.cpp /Fo"obj\NativeTests\\" /Fe"obj\NativeTests\NuvioTorrentTests.exe" /link ws2_32.lib
if errorlevel 1 exit /b 1
obj\NativeTests\NuvioTorrentTests.exe %*
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
