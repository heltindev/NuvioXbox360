@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
if errorlevel 1 exit /b 1
pushd "%~dp0"
if not exist "obj\VmxTests" mkdir "obj\VmxTests"
cl /nologo /EHsc /W3 tests\NuvioVmxWeightTests.cpp /Fo"obj\VmxTests\\" /Fe"obj\VmxTests\NuvioVmxWeightTests.exe"
if errorlevel 1 exit /b 1
obj\VmxTests\NuvioVmxWeightTests.exe
if errorlevel 1 exit /b 1
obj\VmxTests\NuvioVmxWeightTests.exe --mismatch
if errorlevel 1 exit /b 1
cl /nologo /c tests\NuvioVmxForbiddenCompile.c /Fo"obj\VmxTests\forbidden.obj" >"obj\VmxTests\forbidden.log" 2>&1
if not errorlevel 1 exit /b 1
findstr /C:"NUVIO_FORBIDDEN_XENON_vmladduhm" "obj\VmxTests\forbidden.log" >nul
if errorlevel 1 exit /b 1
echo PASS: forbidden Xenon intrinsic refused at compile time
set RESULT=0
popd
exit /b %RESULT%
