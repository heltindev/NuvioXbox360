$ErrorActionPreference='Stop'
$msbuild=Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\MSBuild.exe'
if(-not (Test-Path -LiteralPath $msbuild)){throw 'Install Visual Studio 2010 and Xbox 360 XDK first.'}
foreach($helper in @('bootstrap_dependencies.ps1','build_native_tls.ps1','build_decoder.ps1')) {
    $file=Join-Path $PSScriptRoot $helper
    if(-not (Test-Path -LiteralPath $file)){throw ('Missing build helper: '+$file)}
    Write-Output ('RUNNING '+$helper)
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $file
    if($LASTEXITCODE -ne 0){throw ('Build failed: '+$helper)}
}
Write-Output 'REBUILDING NUVIO Xbox UI6'
$buildStarted=Get-Date
& $msbuild (Join-Path $PSScriptRoot 'NUVIO.vcxproj') /t:Rebuild '/p:Configuration=Release' '/p:Platform=Xbox 360' /v:minimal /nologo
if($LASTEXITCODE -ne 0){throw 'NUVIO build failed. Do not copy an older XEX.'}
$xex=Get-Item -LiteralPath (Join-Path $PSScriptRoot 'Release\NUVIO.xex')
if($xex.LastWriteTime -lt $buildStarted){throw 'Release XEX was not updated by this build.'}
$stream=[IO.File]::OpenRead($xex.FullName)
try {
    $magic=New-Object byte[] 4
    [void]$stream.Read($magic,0,4)
    if([Text.Encoding]::ASCII.GetString($magic) -ne 'XEX2'){throw 'Invalid XEX header.'}
} finally {$stream.Dispose()}
Write-Output ('READY: '+$xex.FullName)
Write-Output ('Build: Xbox UI6 - 23.11.2026 | '+$xex.LastWriteTime.ToString('yyyy-MM-dd HH:mm:ss'))
