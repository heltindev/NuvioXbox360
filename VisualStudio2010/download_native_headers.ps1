$ErrorActionPreference = 'Stop'
$root = Join-Path $PSScriptRoot 'ThirdParty\Headers'
New-Item -ItemType Directory -Force -Path $root | Out-Null
$files = @(
  @('picojson.h','https://raw.githubusercontent.com/kazuho/picojson/master/picojson.h'),
  @('stb_image.h','https://raw.githubusercontent.com/nothings/stb/master/stb_image.h'),
  @('qrcodegen.h','https://raw.githubusercontent.com/nayuki/QR-Code-generator/master/c/qrcodegen.h'),
  @('qrcodegen.c','https://raw.githubusercontent.com/nayuki/QR-Code-generator/master/c/qrcodegen.c')
)
foreach ($pair in $files) {
  $destination = Join-Path $root $pair[0]
  if (-not (Test-Path $destination)) {
    Invoke-WebRequest -UseBasicParsing -Uri $pair[1] -OutFile $destination
  }
  Write-Output "$($pair[0]) $((Get-Item $destination).Length)"
}
