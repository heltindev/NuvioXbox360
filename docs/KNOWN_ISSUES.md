# Known issues and limitations

Updated November 23, 2026. Release candidate: v0.1.0-alpha.2 / Xbox UI8.

## Confirmed report: video does not load

The maintainer reports that opening a video still produces an error and does
not load the video. **This remains unresolved.** Track it in
[issue #4](https://github.com/heltindev/NuvioXbox360/issues/4).
The latest exact message, source and platform have not been supplied, so the
root cause has not been isolated.

Earlier reports included TCP peer `ut_metadata` failures, stalled preparation
and black screens. These symptoms are historical context, not proof that the
latest failure has the same cause. No live-source fix is claimed by this release.

## UI8 fixes requiring confirmation

Search keyboard input was blocked during artwork/catalog loading and the input
box displayed the last submitted query instead of the edited text. UI8 changes
both behaviors and clears deferred input on screen transitions. Local regression
tests passed; physical-device input and live add-on search still need confirmation.

The account page and sign-out-to-QR flow are implemented. QR rendering now uses
a white quiet zone and whole-pixel modules. Camera scanning, visual layout and
real account switching still need device testing.

## Unimplemented or restricted

| Area | Limitation |
| --- | --- |
| Native video | Up to 1280×720; no native HEVC/H.265, AV1, DRM, 1080p or 4K |
| Torrent | v1/SHA-1 TCP without protocol encryption; no uTP, v2-only, IPv6 or full swarm support |
| Metadata | Requires reachable compatible peers; verified metadata does not guarantee video playback |
| Profiles | PIN-protected profiles cannot be unlocked |
| Cache | Catalog/images are session caches; persistence across app restart is not implemented |
| Network responsiveness | Some account/profile HTTPS operations remain synchronous |
| Settings | Dimmed appearance/layout/integration/group-session/system actions are not implemented |
| Subtitles | Not validated as a complete supported playback feature |
| Conversion | External PC required; prepares full file; real AV1/TorrServer/long-video paths pending |
| Hardware | Current end-to-end playback, long-run stability, exact A/V sync, peak RAM and frame rate unverified |

## Reporting a failure

Include the UI version, Xbox model or Xenia version, steps, complete visible error,
and codec/resolution when known. An authorized public test source or synthetic
sample is preferable. State whether the failure occurs before metadata, during
buffering, while opening the container or after playback starts.

Do not attach session files, refresh tokens, passwords, private configuration or
signed/private stream URLs. Generic test results do not replace a reproduction
of the reported problem.
