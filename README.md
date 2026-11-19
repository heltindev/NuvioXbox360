<div align="center">
  <img src="assets/banner.svg" alt="NUVIO.XEX — native Xbox 360 project" width="100%" />

  # NUVIO.XEX 🎮

  **An unofficial, open-source Nuvio-inspired application for Xbox 360 RGH/JTAG.**

  ![Platform](https://img.shields.io/badge/Platform-Xbox%20360%20RGH%20%2F%20JTAG-21c9ad)
  ![Editor](https://img.shields.io/badge/Project-Visual%20Studio%202010-5865f2)
  ![Stage](https://img.shields.io/badge/Status-Streaming%20UI%20Preview-eab04f)

  **[🧰 OPEN THE VISUAL STUDIO PROJECT](VisualStudio2010/)** · **[📋 ROADMAP](ROADMAP.md)** · **[📥 RELEASES](https://github.com/heltindev/NuvioXbox360/releases)**
</div>

---

> [!IMPORTANT]
> **This is a streaming-style interface preview, not a video player.** The UI includes an **Admin** profile, poster-shaped placeholders, browse rows and an optional offline IMDb metadata import. It does **not** play videos or download posters. The Xbox 360 XDK build has compiled on the maintainer's PC, but actual Xbox 360 hardware has not been verified.

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
| Admin-only profile chooser | ✅ Implemented; profile creation planned |
| 720p TV-style sidebar and navigable catalog rows | ✅ Compiled with XDK 21256; visual test pending |
| Fictional demo catalog and bounded offline IMDb metadata importer | ✅ Implemented; import your own datasets |
| Real film posters, network-based discovery and rich search | 🚧 Not implemented |
| Audio/video playback and network features | 🚧 Not implemented |

The app opens with **Who's watching? → Admin**. Press **A** to enter the Nuvio-inspired catalog home, navigate the sidebar and rows with the **D-pad**, open metadata details with **A**, switch back to the profile with **B**, and exit with **Back**. All poster artwork currently consists of **procedural placeholders**, not actual movie imagery.

### 🎬 Want to try real IMDb title names?

The Xbox cannot download the entire IMDb database or its posters automatically. You can import a **small local subset of IMDb title metadata (up to 240 titles in this preview)** for your own noncommercial testing:

1. Download the official `title.basics.tsv.gz` and `title.ratings.tsv.gz` files from [IMDb Non-Commercial Datasets](https://developer.imdb.com/non-commercial-datasets/) on your PC; review IMDb's usage terms.
2. Run `python tools/import_imdb.py --basics title.basics.tsv.gz --ratings title.ratings.tsv.gz --output catalog.tsv` in this repository.
3. Copy the generated `catalog.tsv` beside `NUVIO.XEX.xex` on the Xbox or Xenia test folder.

The importer is optional: without the file, the app shows a visibly labeled **Demo Catalog** of fictional titles. See the [simple IMDb import guide](docs/IMDB_CATALOG.md).

## 📁 Repository structure

| Path | Purpose |
|---|---|
| `VisualStudio2010/NUVIO.XEX.sln` | Open this solution in Visual Studio 2010 |
| `VisualStudio2010/NUVIO.XEX.vcxproj` | Xbox 360 project configuration |
| `VisualStudio2010/src/XboxMain.cpp` | Profile UI, artwork placeholders, XDK graphics and controller |
| `VisualStudio2010/src/NuvioApp.cpp` | Admin profile and TV-style navigation logic |
| `VisualStudio2010/src/NuvioCatalog.cpp` | Local offline catalog parser and demo data |
| `tools/import_imdb.py` | Optional PC importer for IMDb TSV metadata |
| `VisualStudio2010/tests/` | PC-runnable navigation and catalog tests |
| `docs/DEVELOPMENT.md` | Local build instructions |
| `docs/PORTING.md` | Long-term Nuvio feature plan |

## 🤝 Credits, license, and distribution

- Unofficial community project. Not affiliated with NuvioMedia or Microsoft.
- [Nuvio Mobile](https://github.com/NuvioMedia/NuvioMobile) and [Nuvio TV](https://github.com/NuvioMedia/NuvioTV) are upstream design/architecture references. **Their copyrighted implementation is not copied into this independent C++ codebase.**
- This repository retains its existing [Apache-2.0 license](LICENSE). If upstream GPL-covered implementation is copied later, a licensing review and compliance changes will be necessary.
- Microsoft XDK tools and files are proprietary and are **not bundled** or hosted here. Obtain and use them only under appropriate rights.
- Download future tested builds from [Releases](https://github.com/heltindev/NuvioXbox360/releases); maintainers publish them manually after hardware testing.
