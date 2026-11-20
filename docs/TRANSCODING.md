# Automatic conversion service

The Xbox client supports an optional authenticated Python/FFmpeg conversion worker.
Conversion runs on a PC/server, outside Xbox memory. Output is MP4, H.264 Main 3.1,
up to 1280x720/24 fps, 8-bit SDR, with AAC stereo 48 kHz. HDR PQ/HLG input uses
linear-light tone mapping through zscale/tonemap and is tagged BT.709.

## Start on the Windows PC

Run `tools/start_transcoder.cmd` under your normal Windows account. It uses FFmpeg
and FFprobe from PATH, or the existing Stremio installation. Python 3.10+ is required.
The script generates a private token and `nuvio.cfg` with your LAN address, and copies
the configuration to `VisualStudio2010/Release`. Copy both `NUVIO.xex` and that
`nuvio.cfg` to the same writable folder on Xbox. Keep the PC and server process running.
The Xbox must be able to reach TCP 8787 on that PC; the script does not change firewall rules.

For sources without reliable codec labels, use:

```powershell
tools/start_transcoder.ps1 -Mode always
```

`auto` preserves direct playback of sources not labelled incompatible. `always`
prepares every source through the conversion worker, including unknown codecs.
Provider HTTP headers are passed only to the conversion worker/source as required.
Do not publish `nuvio.cfg` or `transcode-token.local`.

For incompatible magnet sources, the conversion worker needs an existing TorrServer:

```powershell
tools/start_transcoder.ps1 -TorrentServer http://YOUR-TORRSERVER:8090
```

Native 720p-compatible magnet playback remains a separate experimental path inside
the XEX and does not require the conversion service. Conversion does not implement
a second torrent client in Python. A TorrServer URL must be real and reachable.

## Preparation and limits

The service converts the complete file before playback. During preparation the
Xbox shows a conversion message and remains on the player screen; B exits. The
client retries pending jobs for up to 20 minutes. Exiting does not cancel a job
already queued on the server, so selecting it later can reuse the prepared file.
This is not real-time transcoding. Long/large sources can take substantial time.

Only one FFmpeg process converts at once; at most four jobs can be pending. The
cache target is 6 GiB, individual output is capped below 4 GiB, and stale files
are evicted. At least 4 GiB free disk is required. Inputs must have a finite known
duration of at most three hours. Live streams and DRM are unsupported. FFmpeg must
provide the input decoder, libx264, AAC, zscale and tonemap. AV1 capability depends
on the installed FFmpeg build; it was not exercised with a real AV1 sample here.

Prepared files support HTTP Range, so the player can pause, resume and seek with
its normal MP4 path. Only HTTP/HTTPS media input is accepted; arbitrary local paths
and shell commands are rejected. The service is intended for a trusted LAN or an
HTTPS reverse proxy. Authentication is required for every endpoint.

## Validation

`tools/test_transcode_server.py --ffmpeg PATH --ffprobe PATH` creates synthetic
HEVC and 10-bit HDR samples, serves them over loopback HTTP, submits conversion
jobs, downloads the output, checks byte ranges and rejects unauthenticated access.
FFprobe verifies H.264 <=720p, yuv420p/BT.709, AAC stereo, and matching stream durations.
This validates PC-side conversion, not video playback on physical Xbox hardware.

Reference: https://ffmpeg.org/ffmpeg-filters.html#tonemap
