# 🎮 NUVIO.XEX — Streaming UI Preview

NUVIO.XEX is an independent, unofficial Nuvio-inspired homebrew project for Xbox 360 RGH/JTAG.

## ⚠️ Early prototype

This preview is intended **only for testing on Xenia or an Xbox 360 RGH/JTAG**. It has an Admin profile selector, a TV-style left sidebar, browseable demo catalog rows and optional IMDb metadata imported locally. It does **not** download poster artwork or play audio/video.

## Included in the source

- Visual Studio 2010 `.sln` and `.vcxproj` for an Xbox 360 XDK toolset.
- Portable C++98 navigation model including Admin profile selection, home, details, library and settings.
- Xbox controller input and a 720p Nuvio-inspired streaming-style interface.
- Small demo catalog plus local IMDb TSV importer (up to 240 titles; no posters or streams).
- Runnable navigation/importer tests and installation instructions.

## Test instructions

After a successful build **with a licensed Xbox 360 XDK**, place `NUVIO.XEX.xex` on a USB drive, open it in Aurora and test the screen and buttons. If the executable fails to start, keep the build logs and record the XDK version and console model.

**Do not publish an `.xex` release until it actually compiles and hardware tests have been performed.**

Read [ROADMAP.md](https://github.com/heltindev/NuvioXbox360/blob/main/ROADMAP.md) for future features.
