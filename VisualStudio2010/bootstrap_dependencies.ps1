# Restore the pinned upstream sources without overwriting the repository's local modifications.
$ErrorActionPreference='Stop'
$revision='43a3969ec2a5103218a4d7120cceac7ea494e846'
$destination=Join-Path $PSScriptRoot 'ThirdParty\x360-homebrew-libs'
$checkpoint=Join-Path $destination 'external\ffmpeg360\vcproj\libavcodec\libavcodec.vcxproj'
if(Test-Path -LiteralPath $checkpoint){Write-Output 'Pinned decoder sources already present.';exit 0}
$downloadRoot=Join-Path ([IO.Path]::GetTempPath()) ('nuvio-dependencies-'+$revision)
New-Item -ItemType Directory -Path $downloadRoot -Force | Out-Null
$archive=Join-Path $downloadRoot 'upstream.zip'
if(-not (Test-Path -LiteralPath $archive)){
 Invoke-WebRequest -UseBasicParsing -Uri "https://github.com/HG-ERIK/x360-homebrew-libs/archive/$revision.zip" -OutFile $archive
}
$source=Join-Path $downloadRoot ('x360-homebrew-libs-'+$revision)
if(-not (Test-Path -LiteralPath $source)){Expand-Archive -LiteralPath $archive -DestinationPath $downloadRoot}
foreach($file in Get-ChildItem -LiteralPath $source -Recurse -File){
 $relative=$file.FullName.Substring($source.Length+1)
 $target=Join-Path $destination $relative
 if(-not (Test-Path -LiteralPath $target)){
  New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
  Copy-Item -LiteralPath $file.FullName -Destination $target
 }
}
Write-Output 'Pinned sources restored. Build TLS and decoder libraries before NUVIO.vcxproj.'
