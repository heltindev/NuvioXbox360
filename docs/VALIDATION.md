# Validation and its limits

## Current status — November 23, 2026

**The maintainer reports an error and a video that does not load. End-to-end
playback is unresolved.** [Issue #4](https://github.com/heltindev/NuvioXbox360/issues/4)
tracks the report. The exact latest error/platform/source is not available.

## UI8 checks

- Navigation/controller-keyboard tests passed, including editing during catalog
  and artwork loading, query submission, back navigation and Account access.
- Authentication tests use the actual backend with an isolated mock transport:
  logout clears credentials/profile state, a fresh device session can start
  without reboot, and the generated QR retains a four-module quiet zone.
- XboxMain.cpp and NuvioGraphics.cpp passed XDK syntax checking.
- The newer local XEX is newer than the UI8 source changes, and its linked
  executable contains the UI8 marker. Integrity is recorded in release-ui8.json.
- Physical-device visuals, QR camera scanning and real account switching remain pending.

## Earlier isolated tests

- FFmpeg/video/guard and the native application built with VS2010/XDK.
- Analog input: deadzone, dominant axis, 330 ms initial repeat and 110 ms repeats.
- Add-on authentication/profile ownership, source headers and library routes.
- HTTP transport byte comparison, redirects, chunks, ranges and malformed/truncated
  responses on a Windows socket/XNet shim. TLS is stubbed in that host test.
- An earlier separate native FFPlayer/FFmpeg executable ran in Xenia Canary 54386cb
  with the public Sintel trailer (https://media.w3.org/2010/05/sintel/trailer.mp4):
  H.264 854x480/24 fps and AAC stereo/48 kHz; 52 frames decoded, 48 displayed,
  zero dropped in the short test, with an active XAudio2 voice/buffer.

The Xenia local-file test is not the complete current launcher/network/player
path. A later torrent test XEX did not finish emulator startup. Neither result
establishes successful current HTTP/torrent playback on physical Xbox.

## Controlled torrent tests

NuvioTorrent.cpp independently implements BEP 3/5/9/10/15 in C++98.
Windows checks covered SHA-1 vectors, bounded bencode, hex/base32 magnet hashes,
loopback UDP tracker/TCP peer, extension negotiation, metadata/piece integrity,
4,372,373-byte sample comparison, seek, cancellation, multi-file selection and
corrupted-data rejection. Later cases exercised larger/out-of-order metadata,
peer refusal and retry. None uses the user's films or external peers.

Run `VisualStudio2010/run_torrent_tests.bat`, then
`python tools/test_torrent_integration.py` for controlled fixture cases.
The fixtures are test tools, not a required PC server for the native P2P path.

Metadata is limited to 1 MiB, peers to 64 and peer messages to 128 KiB. Two
verified pieces of up to 4 MiB each are cached. No full movie is kept in RAM.

## Memory and decoder work

The UI uses at most twelve poster textures, one backdrop and bounded metadata.
Current requested posters are 192x288 with bilinear scaling; driver alignment
and additional allocations mean texture bounds are not a total RAM measurement.
Oversized images are rejected before decompression, and new image downloads
stop when available physical memory falls below 32 MiB.

The player uses three YUV frames, byte/count-bounded packet queues, packets up
to 2 MiB, stereo audio and a maximum 1280x720 video size. UI artwork is released
on player entry; leaving destroys the player and its buffers.

Xenon weight-kernel host tests and opcode controls passed. Historical metrics
remain in validation-xenon.json. Current pixel equality, native VMX execution,
memory peak, sustained frame rate and long-run stability need Xbox measurements.
See [decoder validation](DECODER-XENON.md).

## External conversion tests

Synthetic 1080p HEVC and 10-bit PQ/BT.2020 clips converted over loopback HTTP on
the PC. Output checks found H.264 <=720p/yuv420p/BT.709 SDR and AAC stereo, with
stream durations within 120 ms. Authentication and ranges passed. The Xbox
demux worker polls complete-file preparation jobs; no converter runs on Xbox.

Real AV1 input, actual TorrServer resolution and long movies remain unverified.
PC conversion does not demonstrate console playback or real-time transcoding.

## Still required

Reproduce issue #4; test the current local-file, HTTP and torrent paths through
the full app; measure real peer behavior, cancellation, pause/seek, exact A/V
sync, total memory and long playback on physical Xbox 360. Do not interpret
isolated host results as proof that all codecs, providers or streams work.

Upstream bases: HG-ERIK/x360-homebrew-libs 43a3969ec2a5103218a4d7120cceac7ea494e846
and HG-ERIK/simple360player 5d235c1fd24c098ccc1b2463c1b4265bf2e7844c.
