param([int]$Port=8787,[string]$TorrentServer='',[ValidateSet('auto','always')][string]$Mode='auto')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$ffmpeg=(Get-Command ffmpeg -ErrorAction SilentlyContinue).Source
$ffprobe=(Get-Command ffprobe -ErrorAction SilentlyContinue).Source
if(-not $ffmpeg){$ffmpeg=Join-Path $env:LOCALAPPDATA 'Programs\Stremio\ffmpeg.exe'}
if(-not $ffprobe){$ffprobe=Join-Path $env:LOCALAPPDATA 'Programs\Stremio\ffprobe.exe'}
if(-not (Test-Path -LiteralPath $ffmpeg) -or -not (Test-Path -LiteralPath $ffprobe)){
 throw 'Install FFmpeg and FFprobe with libx264, AAC, zscale and tonemap, or put them on PATH.'
}
$tokenFile=Join-Path $projectRoot 'transcode-token.local'
if(-not (Test-Path -LiteralPath $tokenFile)){
 $bytes=New-Object byte[] 32
 $random=[Security.Cryptography.RandomNumberGenerator]::Create()
 $random.GetBytes($bytes);$random.Dispose()
 [IO.File]::WriteAllText($tokenFile,([BitConverter]::ToString($bytes).Replace('-','').ToLowerInvariant()))
}
$token=[IO.File]::ReadAllText($tokenFile).Trim()
$addresses=[Net.Dns]::GetHostAddresses([Net.Dns]::GetHostName()) | Where-Object {
 $_.AddressFamily -eq [Net.Sockets.AddressFamily]::InterNetwork -and -not [Net.IPAddress]::IsLoopback($_)
}
if(-not $addresses){throw 'No LAN IPv4 address found. Connect the PC and Xbox to the same network.'}
$address=$addresses[0].ToString()
$cfgFile=Join-Path $projectRoot 'nuvio.cfg'
$settings=if(Test-Path -LiteralPath $cfgFile){Get-Content -LiteralPath $cfgFile}else{@()}
$settings=@($settings | Where-Object {$_ -notmatch '^transcode_(server|token|mode)='})
$settings+=@("transcode_server=http://${address}:$Port","transcode_token=$token","transcode_mode=$Mode")
[IO.File]::WriteAllLines($cfgFile,$settings)
$release=Join-Path $projectRoot 'VisualStudio2010\Release'
if(Test-Path -LiteralPath $release){Copy-Item -LiteralPath $cfgFile -Destination (Join-Path $release 'nuvio.cfg') -Force}
Write-Output "Conversion server: http://${address}:$Port"
Write-Output 'Copy NUVIO.xex AND the generated nuvio.cfg to the same folder on Xbox.'
Write-Output 'Keep this PC and this process running. Press Ctrl+C to stop. Do not publish nuvio.cfg or the token.'
Write-Output 'Preparation converts the complete video before playback; long videos can take several minutes.'
$argsList=@((Join-Path $PSScriptRoot 'transcode_server.py'),'--host','0.0.0.0','--port',"$Port",'--ffmpeg',$ffmpeg,
 '--ffprobe',$ffprobe,'--token-file',$tokenFile,'--cache',(Join-Path $env:LOCALAPPDATA 'Nuvio\transcodes'))
if($TorrentServer){$argsList+=@('--torrent-server',$TorrentServer)}
& python @argsList
exit $LASTEXITCODE
