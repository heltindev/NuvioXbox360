# Builds everything: FFmpeg (first run only, ~10 minutes), the libraries and the example (example.xex).
#   .\build.ps1                  -> Release
#   .\build.ps1 -Configuration Debug
param([ValidateSet('Debug', 'Release')] [string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$msbuild = "$env:WINDIR\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe"
$common = @("/p:Configuration=$Configuration", '/p:Platform=Xbox 360', '/v:minimal', '/nologo')

$vcproj = Join-Path $PSScriptRoot 'external\ffmpeg360\vcproj'
foreach ($p in 'external\zlib\zlib', 'libavutil\libavutil', 'libavcodec\libavcodec', 'libavformat\libavformat', 'libswscale\libswscale') {
    $name = Split-Path $p -Leaf
    if (Test-Path "$vcproj\$(Split-Path $p)\Xbox 360\$Configuration\$name.lib") { continue }
    & $msbuild "$vcproj\$p.vcxproj" @common /p:BuildProjectReferences=false
    if ($LASTEXITCODE) { throw "$name failed" }
}

& $msbuild (Join-Path $PSScriptRoot 'example\example.vcxproj') @common
if ($LASTEXITCODE) { throw 'example failed' }
"Libraries: build\$Configuration\lib   Example: build\$Configuration\example\example.xex"
