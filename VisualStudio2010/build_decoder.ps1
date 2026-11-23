$ErrorActionPreference = 'Stop'
$libRoot = Join-Path $PSScriptRoot 'ThirdParty\x360-homebrew-libs'
$compiler = 'C:\Windows\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe'
$decoderRoot = Join-Path $libRoot 'external\ffmpeg360\vcproj'
$buildArgs = @('/p:Configuration=Release','/p:Platform=Xbox 360','/p:BuildProjectReferences=false','/v:minimal','/nologo')
foreach ($relative in @('external\zlib\zlib','libavutil\libavutil','libavcodec\libavcodec','libavformat\libavformat','libswscale\libswscale')) {
    Write-Output ('Building decoder dependency: ' + $relative)
    & $compiler (Join-Path $decoderRoot ($relative + '.vcxproj')) @buildArgs
    if ($LASTEXITCODE -ne 0) { throw ('Decoder build failed: ' + $relative) }
}
foreach ($relative in @('guard\guard.vcxproj','video\video.vcxproj')) {
    & $compiler (Join-Path $libRoot $relative) @buildArgs
    if ($LASTEXITCODE -ne 0) { throw ('Decoder build failed: ' + $relative) }
}
$audit=Join-Path (Split-Path $PSScriptRoot -Parent) 'tools\audit_xenon_vmx.py'
$report=Join-Path $PSScriptRoot 'obj\xenon-vmx-audit.json'
& python $audit (Join-Path $decoderRoot 'libavcodec\Xbox 360\Release\libavcodec.lib') (Join-Path $decoderRoot 'libavutil\Xbox 360\Release\libavutil.lib') --report $report
if($LASTEXITCODE -ne 0){throw 'Xenon VMX opcode audit failed; do not deploy this decoder.'}
