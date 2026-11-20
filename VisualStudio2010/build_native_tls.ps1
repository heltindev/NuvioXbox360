$ErrorActionPreference = 'Stop'
$root = (Join-Path $PSScriptRoot 'ThirdParty\x360-homebrew-libs')
$msbuild = 'C:\Windows\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe'
$common = @('/t:Build','/p:Configuration=Release','/p:Platform=Xbox 360','/v:minimal','/nologo')
foreach ($relative in @('external\mbedtls\mbedtls.vcxproj','log\log.vcxproj','https\https.vcxproj')) {
  $project = Join-Path $root $relative
  if (-not (Test-Path -LiteralPath $project)) { throw ('Missing dependency project ' + $project) }
  Write-Output ('BUILDING ' + $relative)
  & $msbuild $project @common
  Write-Output ('EXIT ' + $LASTEXITCODE)
  if ($LASTEXITCODE -ne 0) { throw ('Failed building ' + $relative) }
}
Write-Output 'NATIVE_TLS_LIBRARIES_READY'
