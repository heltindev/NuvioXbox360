@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
if errorlevel 1 exit /b 1
pushd "%~dp0"
if not exist "obj\NativeTests" mkdir "obj\NativeTests"
cl /nologo /EHsc /W3 /I"src" "tests\NuvioCoreTests.cpp" "src\NuvioApp.cpp" "src\NuvioCatalog.cpp" /Fo"obj\NativeTests\\" /Fe"obj\NativeTests\NuvioCoreTests.exe"
if errorlevel 1 exit /b 1
"obj\NativeTests\NuvioCoreTests.exe"
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
