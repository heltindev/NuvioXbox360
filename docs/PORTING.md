# Nuvio Mobile → NUVIO.XEX

The goal is an **unofficial Xbox 360 reimplementation** of the open-source [Nuvio Mobile](https://github.com/NuvioMedia/NuvioMobile) experience.

The original Nuvio uses Kotlin Multiplatform and Compose. Xbox 360 uses a different processor, graphics stack, compiler and operating system. Its modern UI code cannot be pasted into a Visual Studio 2010 Xbox 360 project.

| Feature goal | Xbox 360 approach | Status |
| --- | --- | --- |
| App navigation | C++98 state model + controller | Host-tested foundation |
| Video output | Xbox 360 Direct3D 9 screen initialization | Solid background verified in Xenia |
| Home dashboard | Bitmap text, colored cards, then textured UI | Early dashboard compiled; visual test pending |
| Movies and series | Independent content and metadata model | Planned |
| Artwork and details | Xbox-compatible textures and caching | Planned |
| Video/audio player | Hardware-constrained decoding and audio output | Not started |
| Subtitles | Native parser and on-screen renderer | Not started |
| Library and continue watching | Local save data | Not started |
| Search and settings | Controller-friendly menus | Not started |

## Constraints

- The Xbox 360 has limited memory and supports different hardware/media APIs from modern phones.
- A working media player requires separate video decoding, audio synchronization, transport and buffering work.
- A successful `.xex` link does **not** prove console compatibility.
- Licensed XDK tools may not be redistributed in this repository.

## Copyright and source policy

Nuvio Mobile is licensed under **GPL-3.0**. We use it as a product and design reference and are currently writing our own code under this repository's existing Apache-2.0 license. If we later copy original Nuvio code instead of independently reimplementing behavior, we must first review compatibility and comply with GPL requirements, including licensing and corresponding source obligations.
