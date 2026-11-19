# ✅ NUVIO.XEX Roadmap

This checklist tracks **implemented code**, **verified behavior**, and **missing features** separately. Nothing should be marked complete just because it looks good in a mockup.

## Phase 1 — Foundation

- [x] Native C project targeting Xbox 360 RGH/JTAG
- [x] 720p interface scaffolding with Xbox controller input
- [x] Cinemeta catalog support
- [x] Torrentio and Brazuca manifest configuration
- [x] Parse HTTP URLs, magnet links, `infoHash`, and `fileIdx`
- [x] Local tests of JSON parsing and URL construction
- [x] English-language repository, documentation, and on-screen text
- [x] Beginner-friendly installation and local build documentation

## Phase 2 — Build and hardware verification

- [ ] Set up and verify the PowerPC toolchain on the maintainer's PC
- [ ] Generate `NUVIO.XEX` and an installable ZIP
- [ ] Confirm the executable loads in **Aurora on a real Xbox 360 RGH**
- [ ] Test controller navigation and 720p display output
- [ ] Confirm certificate-verified HTTPS on real hardware
- [ ] Test Cinemeta and addon responses on real hardware
- [ ] Manually publish the first **clearly labeled preview Release** after successful local compilation
- [ ] Investigate crashes, memory consumption, and rendering performance

## Phase 3 — Actual playback

- [ ] Implement a video decoder compatible with Xbox 360 hardware
- [ ] Add audio output and video/audio synchronization
- [ ] Add buffering, pause, resume, seek, and track selection
- [ ] Benchmark H.264 playback, starting at 720p
- [ ] Choose between native BitTorrent and authorized direct-HTTP/debrid streams
- [ ] Implement the selected transport without requiring a local PC
- [ ] Handle buffering failures and unsupported codecs gracefully
- [ ] Implement subtitle rendering and language selection

## Phase 4 — Recreate the Nuvio experience

- [ ] Rich catalog views with artwork and descriptions
- [ ] Search, genre filters, sorting, and discovery
- [ ] Movie and TV metadata detail pages
- [ ] TV season/episode navigation and correct Stremio episode IDs
- [ ] Addon management from the controller
- [ ] Favorites, watchlist, and continue-watching
- [ ] Playback position/history stored locally
- [ ] Theme and accessibility settings
- [ ] Optional profile/sync support where technically feasible

## Phase 5 — Quality and distribution

- [ ] Automated device smoke tests and crash logs
- [ ] Release checksums and reproducible dependency versions
- [ ] Beginner-friendly release notes and installation guide
- [ ] Validate full build ZIP on USB FAT32 and internal HDD
- [ ] Performance tests on multiple Xbox 360 motherboard revisions
- [ ] Clearly document external dependencies and media rights

**Important:** The upstream Nuvio code is built for modern platforms, not Xbox 360. Full feature parity is a long-term goal, not a claim about the current preview. See [docs/PORTING.md](docs/PORTING.md).
