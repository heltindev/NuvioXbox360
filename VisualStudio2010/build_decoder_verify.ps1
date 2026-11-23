$ErrorActionPreference='Stop'
$workspaceRoot=Split-Path $PSScriptRoot -Parent
& python (Join-Path $workspaceRoot 'tools\prepare_xenon_verify.py')
if($LASTEXITCODE -ne 0){throw 'Could not prepare native H.264 verification fixtures.'}
$msbuild=Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\MSBuild.exe'
& $msbuild (Join-Path $PSScriptRoot 'NUVIO-DECODER-VERIFY.vcxproj') /t:Rebuild '/p:Configuration=Release' '/p:Platform=Xbox 360' /v:minimal /nologo
if($LASTEXITCODE -ne 0){throw 'Native H.264 verification build failed.'}
& python (Join-Path $workspaceRoot 'tools\audit_xenon_vmx.py') (Join-Path $PSScriptRoot 'bin\XenonVerify\NUVIO-DECODER-VERIFY.exe') --report (Join-Path $PSScriptRoot 'obj\xenon-verify-opcodes.json')
if($LASTEXITCODE -ne 0){throw 'Verification executable has unsupported Xenon opcodes.'}
$target=Join-Path $PSScriptRoot 'Release\DecoderVerify\NUVIO-DECODER-VERIFY.xex'
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'bin\XenonVerify\NUVIO-DECODER-VERIFY.xex') -Destination $target
Write-Output ('VERIFICATION_XEX_READY: '+$target)
