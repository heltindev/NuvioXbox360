# NUVIO for Xbox 360 — v0.1.0-alpha.2 / UI8

An experimental native TV client for Xbox 360 RGH/JTAG, inspired by NuvioMedia.

> **VIDEO PLAYBACK IS STILL FAILING. THIS IS NOT A STABLE STREAMING RELEASE.**
> On November 23, 2026, the maintainer reported that selecting a video still
> produces an error and the video does not load. End-to-end playback remains
> unresolved. UI, torrent and decoder changes do not establish working playback.

[Releases](https://github.com/heltindev/NuvioXbox360/releases) ·
[Playback issue #4](https://github.com/heltindev/NuvioXbox360/issues/4) ·
[Known issues](docs/KNOWN_ISSUES.md) · [Roadmap](ROADMAP.md)

## Current status

| Area | Implemented | Still missing or failing |
| --- | --- | --- |
| Interface | Four sidebar items, rounded cards and focus, details actions, controller input | Final visual/controller checks on physical hardware |
| Search | Live keyboard text, input while artwork loads, queued searches and result navigation | Confirmation with real add-ons on Xbox after the UI8 fix |
| Account | Official QR pairing, saved session, profiles, Account page and Sign out | Real-device QR scanning and account-switch regression checks |
| Catalog/library | Add-on discovery, cloud library and bounded session caches | Catalog/image cache does not survive app exit |
| Playback | FFmpeg, GPU YUV rendering and XAudio2 integration | **Video still fails to load in the maintainer's test** |
| Torrent | Experimental v1 TCP client, trackers/DHT, metadata and piece verification | Real-peer metadata reliability and full torrent-to-decoder playback |
| Formats | Native limit of 1280×720; incompatible-source warnings | No native HEVC, AV1, DRM or higher-resolution decoding |
| Conversion | Optional PC-side authenticated FFmpeg service | Complete-file preparation, external dependencies and hardware validation |

See [the issue tracker](https://github.com/heltindev/NuvioXbox360/issues) for
reported failures. A passing build or isolated test is not a playback guarantee.

## What's new in UI8

- Search accepts controller keyboard input during catalog/artwork downloads and
  displays the text being edited. Long text follows the cursor.
- Settings → You (`Voce`) → Account (`Conta`) shows account status and email.
  Sign out waits for the active request, clears local account state and opens a
  fresh QR login. It does not delete the cloud account or cloud library.
- Split purple login layout, a white QR background and whole-pixel QR modules.
- Correct category icons: person, palette, play, chain links and gear.
- Deferred input is discarded when changing screens.

The GitHub documentation is in English. Existing in-app Portuguese labels are
referenced above so users can find the same controls in this build.

Earlier UI7 changes added rounded selection outlines, visible details buttons,
cache/search handling, torrent metadata retries and H.264 Xenon safety work.
These are implementation changes; playback issue #4 remains open.

## Download and install

Use the files attached to the [latest experimental release](https://github.com/heltindev/NuvioXbox360/releases).
The source archives alone do not contain a ready-to-run XEX.

1. Copy `NUVIO.xex` to a writable USB/HDD folder accessible from Aurora or XeXMenu.
2. Launch that exact executable. If your installation uses `default.xex`, replace
   the file your dashboard actually launches with the new build under that name.
3. Use the official QR flow at [nuvio.tv/link](https://nuvio.tv/link), then select a profile.
4. Check **Xbox UI8 - 23.11.2026** in Settings to distinguish it from an old build.
5. Treat media selection as an experimental test; video playback is unresolved.

Never include `nuvio_session.dat`, private `nuvio.cfg`, tokens or personal caches
in bug reports. The release packages exclude local account/configuration data.
See [the installation guide](INSTALL-ON-XBOX.txt).

## Build from source

Requires Visual Studio 2010 with Visual C++ and a separately installed Xbox 360 XDK.
Run `VisualStudio2010/BUILD-RELEASE.cmd` from the extracted repository. It restores
pinned missing dependencies and rebuilds the required libraries and application.

Alternatively, from the `VisualStudio2010` directory:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\prepare_release.ps1
```

Wait for `READY`. The output is `VisualStudio2010/Release/NUVIO.xex`, also copied
from `VisualStudio2010/bin/Release/NUVIO.xex`. If a build fails, do not redistribute
an older executable left in either directory. See [development](docs/DEVELOPMENT.md).

Missing `mbedtls/ssl.h` (C1083) or `video.lib` (LNK1181) means dependencies have
not been restored/built. C4244 numeric-conversion warnings are not fatal errors.
Do not redistribute proprietary XDK headers, tools, libraries or installers.

## Controls

| Screen | Controls |
| --- | --- |
| Home | D-pad/sticks: navigate; A: details; X: refresh; Y: search; LB/RB: catalog groups |
| Search keyboard | A: key/action; X: delete; Y: space; LB/RB: shift; Start: search; B: Home |
| Search results | A: details; B: edit the query; X: refresh |
| Details | A: episodes/sources; X: save to library |
| Sources | A: try playback; X: refresh; B: back |
| Player | A/Start: pause; Left/Right: seek 10 seconds; B: leave; A/B: dismiss playback error |
| Settings | A: open implemented options; B: Home; Y: Account |
| Account | A on Sign out: clear session and reopen QR; B: Settings |
| Media → Streaming P2P | A: toggle P2P, hide statistics or clear torrent metadata cache; B: Settings |
| Add-ons | A: toggle/install; X: remove; Y: synchronize; B: back |

Unimplemented settings are dimmed and disabled. PIN-protected profiles cannot be
unlocked in this client yet.

## Playback, formats and torrents

For controlled tests, prefer authorized **H.264/AVC up to 720p with AAC stereo**.
Even these sources are not guaranteed to work through the full application yet.
The player also includes legacy codecs, subject to the bundled FFmpeg demuxers.
Native HEVC/H.265, AV1, DRM, 1080p and 4K are unsupported by this build's limits.
Known incompatible links are dimmed and display a clear error; unknown formats
are inspected by the decoder when opened.

HTTP/HTTPS uses provider headers, bounded redirects and HTTP Range. Magnet,
infoHash and `torrent://hash/fileIdx` sources use an experimental in-XEX torrent
client, with trackers/DHT IPv4, `ut_metadata`, SHA-1 checks and multi-file selection.
It supports v1 TCP peers without protocol encryption, metadata up to 1 MiB and
two cached pieces of up to 4 MiB each. It does not implement uTP, BitTorrent v2,
IPv6 or native codec conversion. Solving metadata retrieval does not add codecs.

P2P settings are saved in `nuvio_p2p.cfg`. The last verified metadata entry may be
saved in `nuvio_last_torrent.meta`; movie data remains in bounded temporary memory.
The loading percentage represents preparation phases and initial buffering, not
the percentage of the entire movie downloaded. Peer availability affects results.

## Optional conversion service

`tools/start_transcoder.cmd` starts a PC-side Python/FFmpeg worker. It prepares a
complete H.264 Main 3.1 MP4, up to 720p/24 fps, SDR BT.709 and AAC stereo. The PC
must remain running and reachable; this is **not real-time conversion on Xbox**.
Incompatible magnets require an actual TorrServer for that external path. No
public server is supplied. AV1 input depends on the PC's FFmpeg and remains untested.
Read [conversion setup and limits](docs/TRANSCODING.md).

## Validation and remaining work

- UI8 navigation/keyboard and isolated authentication tests passed on Windows;
  Xbox UI/graphics sources passed the XDK syntax check.
- A newer local XEX and linked executable are available; the linked image
  contains the UI8 marker. File hashes are recorded in the release manifest.
- Controlled Windows tracker/peer tests covered metadata, piece checks, seek,
  multi-file torrents and corrupted-data rejection.
- An **earlier**, separate local-file Xenia smoke test decoded H.264 854×480/AAC:
  52 frames decoded, 48 displayed and zero dropped in that short test.
- H.264 Xenon host checks and opcode audits passed. Native pixel equality and
  performance of that adaptation still require the Xbox verifier.

**None of those checks resolves the maintainer's current video-loading failure.**
Current public-swarm playback, sustained A/V synchronization, provider behavior,
peak total RAM, subtitles and physical Xbox reliability remain pending.

[Validation details](docs/VALIDATION.md) · [Decoder work](docs/DECODER-XENON.md) ·
[Release notes](docs/RELEASE_NOTES.md) · [Checklist](docs/CHECKLIST.md)

## License and project status

This is an independent, unofficial project, unaffiliated with Microsoft or
NuvioMedia. Original NUVIO source retains its Apache-2.0 notice. The combined
player includes GPL components and is distributed with corresponding source,
license texts and pinned upstream references. Read [NOTICE.md](NOTICE.md).
