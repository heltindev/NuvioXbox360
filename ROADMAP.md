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

- [ ] Verify Visual C++ and an appropriately licensed Xbox 360 XDK are installed
- [ ] Open the solution with the `Xbox 360` build platform available
- [x] Compile the initial `NUVIO.XEX.xex` successfully in Visual Studio 2010 (XDK 21256)
- [ ] Compile and display the updated dashboard preview in Xenia
- [ ] Run the executable from Aurora on a real RGH/JTAG Xbox 360
- [ ] Confirm the 720p screen displays correctly
- [ ] Confirm controller navigation and clean exit
- [ ] Publish the first clearly labeled manual preview Release

## Phase 3 — Nuvio interface

- [x] Add visible NUVIO bitmap text and a header bar (source implemented)
- [x] Add four controller-friendly colored menu cards (source implemented)
- [ ] Verify the on-screen dashboard and selection highlights in Xenia
- [ ] Support poster artwork and details panels
- [ ] Build Movies, Series, My Library and Settings screens
- [ ] Add search and browse navigation
- [ ] Cache textures safely within console memory limits

## Phase 4 — Nuvio features

- [ ] Design a source-independent media catalog API
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

**Current stage:** Foundation complete as source files. The Xbox 360 `.xex` build and real hardware tests remain unverified.
