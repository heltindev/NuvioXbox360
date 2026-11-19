# From Nuvio to NUVIO.XEX — Porting plan

NUVIO.XEX is an **unofficial native rewrite for Xbox 360**, inspired by the [open-source Nuvio Mobile project](https://github.com/NuvioMedia/NuvioMobile). The original application uses Kotlin Multiplatform/Compose and supports modern mobile systems. That framework cannot simply be built for the Xbox 360's PowerPC processor and legacy operating system.

Rather than copying its platform-specific code, this project reimplements relevant behavior using native C and the open-source Xbox 360 `xsdk`.

| Nuvio capability / user experience | Xbox 360 implementation approach | Status |
|---|---|---|
| Browsing catalogs | Stremio catalog endpoints + controller UI | Early prototype |
| Discovering stream sources | Stremio stream endpoints | Early prototype |
| Movie artwork/metadata | Metadata responses + texture atlas renderer | Planned |
| Series and episodes | Metadata video IDs, season/episode selection | Planned |
| Video playback | PowerPC-compatible decoder, audio mixer, buffering | Not started |
| Subtitles | Subtitle endpoint, parser, on-screen renderer | Not started |
| Addon management | Saved manifest list and controller settings | Manual text config only |
| Watchlist/favorites | Local storage on HDD/USB | Not started |
| Continue watching | Persist playback position and history | Not started |
| Settings/profiles | Xbox-friendly local configuration | Not started |

## Engineering constraints

- Xbox 360 has substantially less memory and weaker modern codec support than current phones or TVs.
- Direct BitTorrent streaming needs a **real torrent engine**; an `infoHash` or magnet alone is not a playable video.
- Many external services require HTTPS and may change their APIs.
- DRM-protected platforms cannot be assumed to work.
- Console hardware testing is mandatory before calling any release stable.

## Sources and licensing

- [NuvioMobile upstream repository](https://github.com/NuvioMedia/NuvioMobile) (GPL-3.0): feature and architectural reference, not vendored source.
- [Stremio addon protocol](https://github.com/Stremio/stremio-addon-sdk/blob/master/docs/protocol.md): endpoint format.
- [xsdk](https://github.com/xenosworks/xsdk): open-source homebrew SDK.
- This repository retains its existing Apache-2.0 license for its independent implementation. Any future copying of upstream GPL-covered code would require separate license review and compliance.
