# Restore pinned third-party sources without replacing local project customizations.
# In particular, verify BOTH FFmpeg and mbedTLS: a partial ZIP can contain
# libavcodec/utils.c but still be missing include/mbedtls/ssl.h.
$ErrorActionPreference='Stop'
$revision='43a3969ec2a5103218a4d7120cceac7ea494e846'
$destination=Join-Path $PSScriptRoot 'ThirdParty\x360-homebrew-libs'
$required=@(
 'external\ffmpeg360\libavcodec\utils.c',
 'external\ffmpeg360\vcproj\external\zlib\zlib.vcxproj',
 'external\mbedtls\include\mbedtls\ssl.h',
 'external\mbedtls\mbedtls.vcxproj',
 'https\https.vcxproj',
 'video\video.vcxproj'
)
$missing=@($required | Where-Object { -not (Test-Path -LiteralPath (Join-Path $destination $_)) })
if($missing.Count -eq 0) {
 Write-Output 'Pinned FFmpeg and mbedTLS sources already present.'
 return
}
Write-Output ('Restoring missing third-party sources: '+($missing -join ', '))
$downloadRoot=Join-Path ([IO.Path]::GetTempPath()) ('nuvio-dependencies-'+$revision)
New-Item -ItemType Directory -Path $downloadRoot -Force | Out-Null
$archive=Join-Path $downloadRoot 'upstream.zip'
if(-not (Test-Path -LiteralPath $archive)) {
 Invoke-WebRequest -UseBasicParsing -Uri "https://github.com/HG-ERIK/x360-homebrew-libs/archive/$revision.zip" -OutFile $archive
}
$source=Join-Path $downloadRoot ('x360-homebrew-libs-'+$revision)
if(-not (Test-Path -LiteralPath $source)) {
 Expand-Archive -LiteralPath $archive -DestinationPath $downloadRoot
}
if(-not (Test-Path -LiteralPath $source)){throw 'Dependency archive did not contain the expected source directory.'}
foreach($file in Get-ChildItem -LiteralPath $source -Recurse -File) {
 $relative=$file.FullName.Substring($source.Length+1)
 $target=Join-Path $destination $relative
 if(-not (Test-Path -LiteralPath $target)) {
  New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
  Copy-Item -LiteralPath $file.FullName -Destination $target
 }
}
$missing=@($required | Where-Object { -not (Test-Path -LiteralPath (Join-Path $destination $_)) })
if($missing.Count -gt 0){throw ('Third-party restore incomplete: '+($missing -join ', '))}
Write-Output 'Pinned mbedTLS and decoder sources restored. Build libraries before NUVIO.vcxproj.'
