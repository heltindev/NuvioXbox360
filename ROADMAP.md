# ✅ NUVIO.XEX — Development roadmap

This checklist separates **source code written** from **hardware-tested features**. Everything is an incremental reimplementation for the Xbox 360.

## Phase 1 — Clean project foundation

- [x] Remove the previous media-source integrations and their dependencies
- [x] Add a Visual Studio 2010 Xbox 360 `.sln` and `.vcxproj`
- [x] Add an independent C++98 NUVIO navigation engine
- [x] Add host-runnable navigation tests
- [x] Add a console-side Direct3D and controller smoke test (source only)
- [x] Explain the legacy XDK requirements and local build process

## Phase 2 — First real Xbox 360 build

- [x] Verify Visual C++ and Xbox 360 XDK 21256 build tools on the maintainer's PC
- [x] Build the project with the `Xbox 360` platform available
- [x] Compile the initial `NUVIO.XEX.xex` successfully in Visual Studio 2010 (XDK 21256)
- [x] Compile the new profile and streaming-style UI with XDK 21256 (PC build only)
- [ ] Confirm the updated streaming-style screen in Xenia
- [ ] Run the executable from Aurora on a real RGH/JTAG Xbox 360
- [ ] Confirm the 720p screen displays correctly
- [ ] Confirm controller navigation and clean exit
- [ ] Publish the first clearly labeled manual preview Release

## Phase 3 — Nuvio interface

- [x] Add an Admin-only profile selector and Nuvio-inspired dark gradient
- [x] Add a vertical sidebar, selectable catalog rows, and details screen
- [x] Add fictional procedural poster placeholders (not genuine posters)
- [x] Add empty library and basic settings screens
- [x] Add catalog browsing by controller (text search is still planned)
- [ ] Add actual licensed poster artwork and better text rendering
- [ ] Add create/edit/delete profile actions and local profile persistence
- [ ] Implement proper keyboard-driven search and rich title details
- [ ] Cache textures safely within console memory limits

## Phase 4 — Nuvio features

- [x] Design a bounded local C++98 catalog with a TSV parser
- [x] Add a PC-only importer for official IMDb noncommercial metadata datasets
- [ ] Add pagination for the full IMDb-scale dataset (current preview: 240 entries)
- [ ] Add metadata, descriptions, genres and seasons/episodes
- [ ] Implement a native Xbox 360 video/audio playback pipeline
- [ ] Add buffering, pause/resume, seeking and subtitles
- [ ] Save favorites, watch history and playback progress locally
- [ ] Expose a simple controller-based settings screen

## Phase 5 — Compatibility and releases

- [ ] Test multiple Xbox 360 motherboard revisions and Aurora versions
- [ ] Investigate game crashes, memory pressure and codec limits
- [ ] Add tests for local storage, navigation and UI behavior
- [ ] Include a beginner-friendly installation guide with each manual Release
- [ ] Review licensing if any original Nuvio GPL-covered code is ever ported

**Current stage:** Admin chooser, streaming-style UI, navigation and offline IMDb metadata import. The `.xex` compiled with the maintainer's Xbox 360 XDK 21256 on Windows; Xenia visual verification and real hardware tests remain pending. **No streaming playback yet.**
