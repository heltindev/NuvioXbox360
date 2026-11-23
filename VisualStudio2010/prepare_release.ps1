# One-command Xbox 360 Release preparation: restore pinned third-party sources,
# build their static libraries, then rebuild NUVIO.xex with the installed XDK.
# Run from a PowerShell window on the Windows machine with VS2010 + Xbox 360 XDK.
$ErrorActionPreference='Stop'
$msbuild=Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\MSBuild.exe'
if(-not (Test-Path -LiteralPath $msbuild)) {
 throw ('MSBuild for Visual Studio 2010 was not found: '+$msbuild)
}
foreach($script in @('bootstrap_dependencies.ps1','build_native_tls.ps1','build_decoder.ps1')) {
 $file=Join-Path $PSScriptRoot $script
 if(-not (Test-Path -LiteralPath $file)){throw ('Missing build helper: '+$file)}
 Write-Output ('== RUNNING '+$script+' ==')
 & $file
 if(-not $?){throw ('Build helper failed: '+$script)}
}
$ssl=Join-Path $PSScriptRoot 'ThirdParty\x360-homebrew-libs\external\mbedtls\include\mbedtls\ssl.h'
$video=Join-Path $PSScriptRoot 'ThirdParty\x360-homebrew-libs\build\Release\lib\video.lib'
if(-not (Test-Path -LiteralPath $ssl)){throw ('Missing header: '+$ssl)}
if(-not (Test-Path -LiteralPath $video)){throw ('Missing decoder library: '+$video)}
Write-Output '== REBUILD NUVIO RELEASE XBOX 360 =='
& $msbuild (Join-Path $PSScriptRoot 'NUVIO.sln') '/t:Rebuild' '/p:Configuration=Release' '/p:Platform=Xbox 360' '/v:minimal' '/nologo'
if($LASTEXITCODE -ne 0){throw ('NUVIO XDK build failed with exit code '+$LASTEXITCODE)}
$generated=Join-Path $PSScriptRoot 'bin\Release\NUVIO.xex'
if(-not (Test-Path -LiteralPath $generated)){throw ('Build reported success but XEX missing at '+$generated)}
$built=Get-Item -LiteralPath $generated
Write-Output ('NUVIO_XEX_READY: '+$built.FullName)
Write-Output ('Built: '+$built.LastWriteTime.ToString('yyyy-MM-dd HH:mm:ss')+' / bytes: '+$built.Length)
