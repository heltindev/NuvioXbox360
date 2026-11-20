# NUVIO alpha 2 checklist

| Area | Implemented and checked | Remaining validation |
| --- | --- | --- |
| Startup/UI | Native XEX builds; larger sidebar; input and navigation tests | Physical console launch |
| HTTP player | Native decoder integration; HTTP byte/range/redirect tests; earlier local-file Xenia smoke | Latest HTTP-to-decoder end-to-end test; pause/seek/A-V sync on hardware |
| Torrent | Native v1 client; metadata/piece integrity, byte seek, corrupt-data and multi-file tests | Public swarms/DHT and full torrent-to-decoder hardware playback |
| Conversion | Authenticated FFmpeg service; HEVC and HDR HTTP integration tests; H.264/SDR/AAC output | Real AV1 sample; incompatible magnet through an actual TorrServer; long movies |
| Performance | Bounded artwork, packet and torrent caches; no animation | Peak total memory and sustained frame rate on hardware |
| Delivery | New XEX and corresponding source/package | Physical console results; published release URL after GitHub sign-in |

Unchecked hardware items are not claimed as completed. See VALIDATION.md and TRANSCODING.md for methods and limits.
