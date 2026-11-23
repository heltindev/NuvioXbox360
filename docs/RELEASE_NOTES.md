# NUVIO v0.1.0-alpha.2 — Xbox UI8 experimental pre-release

**Known failure: the maintainer still gets an error and the video does not load.**
This version is for development testing. It is not a stable or completed media
player. End-to-end playback remains open in [issue #4](https://github.com/heltindev/NuvioXbox360/issues/4).

## Changes

- Search keyboard accepts input during artwork/catalog loading and displays live
  edited text. Old deferred input is discarded on screen changes.
- Account status/email page and explicit Sign out button, followed by a fresh QR.
- Split purple login layout, scannable white QR background and corrected Settings icons.
- Four sidebar items, rounded cards/focus and visible details actions.
- Session catalog/image caches, add-on search handling and serialized backend requests.
- Experimental native torrent metadata retries and bounded SHA-1-verified pieces.
- H.264 Xenon weighted-prediction adaptation with C fallback, intrinsic restrictions,
  opcode auditing and improved decoder failure reporting.
- English repository documentation, explicit known-issue list and source/dependency notices.

## What is still missing

- Fix the reported video-loading failure and isolate its root cause.
- Confirm current local-file, HTTP and torrent playback in the complete app on Xbox.
- Validate public peers/DHT, metadata, buffering, pause/seek, audio sync and long playback.
- Check UI8 search, QR scanning and account switching on physical hardware.
- PIN-protected profiles, subtitle support and currently disabled settings remain incomplete.
- Catalog/image cache persistence, peak RAM/performance measurements and remaining
  synchronous network operations need further work.

Native HEVC/AV1/DRM and video above 720p are unsupported. uTP, BitTorrent v2 and
IPv6 are not implemented. The optional PC converter prepares an entire file;
it is not an on-console real-time solution. Real AV1/TorrServer paths are unverified.

## Validation boundaries

Windows navigation/keyboard, isolated authentication, controlled torrent tests
and XDK syntax checks passed. A newer local XEX is available and the linked image
contains UI8. An older standalone Xenia local-file smoke test decoded a short
H.264/AAC sample. Xenon host comparisons and opcode audits passed.

**These checks do not demonstrate successful playback of the user's sources.**
Current hardware pixel equality, long playback, A/V sync and provider compatibility
are pending. See [validation](VALIDATION.md) and [known issues](KNOWN_ISSUES.md).

## Installation

Download `NUVIO.xex` or the Xbox360 ZIP, copy it to a writable Aurora/XeXMenu
folder and launch that file. Check `Xbox UI8 - 23.11.2026` in Settings. The source
ZIP requires VS2010/XDK to build. Do not distribute private session/config files.

The release manifest and `SHA256SUMS.txt` identify the attached build. Corresponding
source and license notices accompany this independent, unofficial GPL-containing
player distribution. Proprietary XDK components are not included.
