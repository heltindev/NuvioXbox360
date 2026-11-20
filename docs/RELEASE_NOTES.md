# NUVIO v0.1.0-alpha.2

Experimental pre-release for Xbox 360 RGH/JTAG. Physical console startup and end-to-end playback have not been verified. This build is intended for testing.

## Changes

- Larger sidebar buttons and labels when focused; left-stick and D-pad navigation.
- Small artwork cache and background network jobs to reduce Xbox RAM use.
- Native FFmpeg video, YUV rendering and XAudio2 stereo audio integration, limited to 720p.
- Experimental native BitTorrent v1 metadata retrieval, tracker/DHT discovery, SHA-1 piece verification, small cache and multi-file selection.
- More reliable HTTP playback: redirects, validated byte ranges, and detection of incomplete or malformed responses.
- Optional authenticated Python/FFmpeg conversion service. HTTP sources can be prepared as H.264 Main 3.1, up to 720p/24 fps, SDR BT.709, with AAC stereo 48 kHz. PQ/HLG HDR uses tone mapping.
- Conversion job preparation runs on the player worker. The UI shows conversion status and allows returning with B.
- Source code synchronized with the compiled application, with pinned dependency restoration and license notices.

## Development validation

- Navigation, analog input, authentication and cloud add-on tests passed.
- Native torrent client passed controlled Windows tests: a complete 4.37 MB sample, metadata and piece verification, seeks, multi-file selection and rejection of corrupted data.
- Production HTTP transport passed Windows socket tests for MP4 bytes, redirects, chunks, ranges and truncated downloads. HTTPS is not covered by that host test.
- The conversion service passed real HTTP tests using synthetic 1080p HEVC and 10-bit HDR sources. FFprobe verified H.264 <=720p, SDR, AAC stereo and matching audio/video durations.
- An earlier native decoder smoke test played an open H.264/AAC local file in Xenia. The latest end-to-end network/player build has not completed an emulator or physical-console test.

## Conversion setup

Run tools/start_transcoder.cmd on a Windows PC with Python 3.10+, FFmpeg and FFprobe. The script also supports an existing Stremio FFmpeg installation. Copy the generated private nuvio.cfg alongside NUVIO.xex. Keep that PC and the service running and reachable from Xbox. Do not upload your configuration/token.

The service converts the complete video before playback; it is not real-time streaming conversion. Long videos may take several minutes or exceed the 20-minute client preparation window. One conversion runs at a time, with a bounded disk cache. Only finite videos up to three hours are supported.

Incompatible magnet sources require a real TorrServer for the external conversion service. Compatible native magnet playback remains a separate experimental path. No public torrent/conversion server is included. AV1 input requires a suitable installed FFmpeg decoder and has not been tested with a real sample.

## Installation

Copy NUVIO.xex to a writable folder accessible through Aurora or XeXMenu. For optional conversion, also copy the generated nuvio.cfg to that folder. Keep your existing private session file on the console. A/Start pauses or resumes, Left/Right seeks ten seconds, and B exits playback.

## Known limitations

Physical Xbox launch, sustained playback, public swarm discovery, exact A/V sync on hardware, subtitles, provider compatibility and peak total RAM still need hardware validation. 4K/HEVC/AV1/HDR are not decoded natively by this Xbox build; use the optional conversion worker. DRM is unsupported. Conversion can fail if the provider, codec filters, disk capacity or torrent resolver is unavailable.

The project is unofficial and is not affiliated with Microsoft or NuvioMedia. The combined player executable includes GPL components; corresponding sources, pinned upstream references and license notices are provided.
