<div align="center">
  <img src="assets/banner.svg" alt="NUVIO.XEX — native Xbox 360 project" width="100%" />

  # NUVIO.XEX 🎮

  **An unofficial, open-source Nuvio-inspired application for Xbox 360 RGH/JTAG.**

  ![Platform](https://img.shields.io/badge/Platform-Xbox%20360%20RGH%20%2F%20JTAG-21c9ad)
  ![Editor](https://img.shields.io/badge/Project-Visual%20Studio%202010-5865f2)
  ![Stage](https://img.shields.io/badge/Status-Foundation%20only-eab04f)

  **[🧰 OPEN THE VISUAL STUDIO PROJECT](VisualStudio2010/)** · **[📋 ROADMAP](ROADMAP.md)** · **[📥 RELEASES](https://github.com/heltindev/NuvioXbox360/releases)**
</div>

---

> [!IMPORTANT]
> **This is a starter project, not a video player.** The menu state and Xbox controller prototype are implemented, but there is no catalog, video decoding, media source integration, or working streaming. The Xbox 360 XDK build has not yet been tested on actual console hardware.

## ✨ About the project

The goal is to reimagine the open-source [Nuvio Mobile](https://github.com/NuvioMedia/NuvioMobile) experience as a native Xbox 360 application. Its current codebase uses Kotlin Multiplatform; Xbox 360 requires a new PowerPC/XDK application, so this repository starts with a small, independent C++ implementation.

The project currently contains **only NUVIO-specific app scaffolding**. No third-party media integrations are included.

## 🧰 Open it in Visual Studio 2010

1. Select **Code → Download ZIP** on this repository, or clone it with Git.
2. Extract the files and open [`VisualStudio2010/NUVIO.XEX.sln`](VisualStudio2010/NUVIO.XEX.sln).
3. Make sure **Visual C++** and a separately installed, legally licensed **Xbox 360 XDK with Visual Studio integration** are available.
4. Choose **Debug | Xbox 360** in the toolbar and select **Build → Build Solution**.
5. If it compiles, look for `VisualStudio2010/Debug/NUVIO.XEX.xex` (or `Release/NUVIO.XEX.xex`) and see [INSTALL-ON-XBOX.txt](INSTALL-ON-XBOX.txt).

**If Visual Studio shows only Win32/x64 instead of Xbox 360**, the required XDK build integration is missing. Installing the standard Visual Studio C++ compiler alone will not build a console executable.

See [Visual Studio starter guide](VisualStudio2010/README.md) and [developer documentation](docs/DEVELOPMENT.md) for details.

## 🕹️ First prototype

| Feature | Status |
|---|---|
| Visual Studio 2010 Xbox 360 solution/project files | ✅ Initial version built with XDK 21256 |
| Small reusable NUVIO menu-state engine | ✅ Host tests passing |
| Xbox controller handling (D-pad, A, B, Back) | 🧪 Written; hardware test pending |
| 720p dashboard with visible text and selectable cards | ✅ Compiled with XDK 21256; visual test pending |
| Nuvio-style tile UI, posters, search, library | 🚧 Not implemented |
| Audio/video playback and network features | 🚧 Not implemented |

The Xbox dashboard preview draws a NUVIO header, four selectable cards, and controller help using built-in bitmap text. Use D-pad to change selection, **A** to open a section, **B** to return home, and **Back** to exit. **This is only an interface test**; actual content and playback are not available.

## 📁 Repository structure

| Path | Purpose |
|---|---|
| `VisualStudio2010/NUVIO.XEX.sln` | Open this solution in Visual Studio 2010 |
| `VisualStudio2010/NUVIO.XEX.vcxproj` | Xbox 360 project configuration |
| `VisualStudio2010/src/XboxMain.cpp` | XDK startup, bitmap-font UI and controller input |
| `VisualStudio2010/src/NuvioApp.cpp` | Independent navigation logic |
| `VisualStudio2010/tests/` | PC-runnable navigation tests |
| `docs/DEVELOPMENT.md` | Local build instructions |
| `docs/PORTING.md` | Long-term Nuvio feature plan |

## 🤝 Credits, license, and distribution

- Unofficial community project. Not affiliated with NuvioMedia or Microsoft.
- [Nuvio Mobile](https://github.com/NuvioMedia/NuvioMobile) is released under GPL-3.0. It is an inspiration and feature reference; **its copyrighted implementation is not copied into this independent starter**.
- This repository retains its existing [Apache-2.0 license](LICENSE). If upstream GPL-covered implementation is copied later, a licensing review and compliance changes will be necessary.
- Microsoft XDK tools and files are proprietary and are **not bundled** or hosted here. Obtain and use them only under appropriate rights.
- Download future tested builds from [Releases](https://github.com/heltindev/NuvioXbox360/releases); maintainers publish them manually after hardware testing.
