<div align="center">
  <img src="assets/banner.svg" alt="NUVIO.XEX — a native Xbox 360 media app" width="100%" />

  # NUVIO.XEX 🎮

  **An unofficial, open-source Nuvio reimplementation for modded Xbox 360 consoles.**

  ![Platform](https://img.shields.io/badge/Platform-Xbox%20360%20RGH%20%2F%20JTAG-21c9ad)
  ![Stage](https://img.shields.io/badge/Stage-Early%20preview-8973ea)
  ![Language](https://img.shields.io/badge/Language-English-4b9bff)

  **[📥 DOWNLOAD FROM RELEASES](https://github.com/heltindev/NuvioXbox360/releases)** · **[📋 ROADMAP](ROADMAP.md)** · **[❓ INSTALL HELP](INSTALL-ON-XBOX.txt)**
</div>

---

> [!WARNING]
> **This is not a working video player yet.** The current code is an early technical preview: it can display an interface and read catalogs/addon sources, but it **cannot play videos or download torrents**. If the Releases page does not contain a downloadable build, no Xbox-tested version is available yet.

## ✨ What is NUVIO.XEX?

NUVIO.XEX is a native Xbox 360 project inspired by the open-source [Nuvio](https://github.com/NuvioMedia/NuvioMobile) experience: discover movies and series, choose sources, and eventually watch media using an Xbox 360 controller.

The original Nuvio is built with Kotlin Multiplatform for modern devices. Xbox 360 uses a different processor and system, so this project is a **new C implementation using [xsdk](https://github.com/xenosworks/xsdk)**, not a copy of the Android app. We are porting its features in stages and tracking each one honestly.

<table>
  <tr><th>Feature</th><th>Current status</th></tr>
  <tr><td>Dark, controller-friendly 720p interface</td><td>✅ Code ready; console test pending</td></tr>
  <tr><td>Cinemeta catalog</td><td>✅ Implemented; console test pending</td></tr>
  <tr><td>Torrentio and Brazuca source discovery</td><td>✅ Addon manifest integration</td></tr>
  <tr><td>HTTP and torrent metadata recognition</td><td>✅ Parser tests passing</td></tr>
  <tr><td>Video/audio player</td><td>🚧 Not implemented</td></tr>
  <tr><td>Native BitTorrent download</td><td>🚧 Not implemented</td></tr>
  <tr><td>Posters, subtitles, profiles, favorites</td><td>🗓️ Planned</td></tr>
</table>

## 📥 Download and install (no programming required)

<details open>
<summary><strong>Step 1 — Download the app</strong></summary>

1. Open the **[Releases page](https://github.com/heltindev/NuvioXbox360/releases)**.
2. Choose the latest available release. Read the warning and feature list.
3. Under **Assets**, download the `NUVIO-XEX-Preview.zip` file, if one is available.

**No ZIP means no release has passed the build checks yet.** Please do not download random `.xex` files from third-party sites.
</details>

<details>
<summary><strong>Step 2 — Put it on your Xbox 360</strong></summary>

1. Extract the ZIP to a folder on a **FAT32 USB drive**.
2. Keep `NUVIO.XEX`, `cacert.pem`, `catalog_manifest.txt`, and `stream_addons.txt` in the **same folder**.
3. On your Xbox 360 with **RGH/JTAG**, open **Aurora → File Manager**.
4. Browse to the USB folder and launch `NUVIO.XEX`.

You will need an internet connection and the correct console date/time for HTTPS.
</details>

<details>
<summary><strong>Step 3 — Use the controller</strong></summary>

| Button | Action |
|---|---|
| D-pad ↑ / ↓ | Move through items |
| A | Select |
| B | Go back |
| Y | Refresh catalog |
| Back button | Exit |

The current preview is intended to test **catalog → addon → sources**. It does not play the selected video.
</details>

## 🧭 What comes next?

The development plan is in **[ROADMAP.md](ROADMAP.md)**. Our biggest milestones are:

1. Produce and test a native `.XEX` on real Xbox 360 RGH hardware.
2. Add a real video and audio player with buffering.
3. Investigate direct torrent playback or authorized HTTP streaming without requiring a PC.
4. Add the core Nuvio experience: posters, series episodes, search, subtitles, favorites, watch progress, and settings.

## 📁 Project files

| Path | What it does |
|---|---|
| `src/main.c` | Native UI and controller input |
| `src/stremio.c` | Stremio addon protocol parsing |
| `src/net.c` | HTTP/HTTPS client |
| `catalog_manifest.txt` | Default catalog manifest (Cinemeta) |
| `stream_addons.txt` | Configurable stream addon manifests |
| `tests/` | Local parser tests |
| `docs/DEVELOPMENT.md` | Developer/build instructions |
| `docs/PORTING.md` | Nuvio feature-by-feature porting plan |

## 🤝 Credits, licenses, and safety

- Independent community project — **not affiliated with NuvioMedia, Microsoft, Xbox, or Stremio**.
- The original [Nuvio Mobile](https://github.com/NuvioMedia/NuvioMobile) is GPL-3.0; its source is a reference, not included here.
- Our code follows this repository's existing [Apache-2.0 license](LICENSE). Bundled cJSON retains its MIT notices.
- External build dependencies keep their own licenses.
- Use only media sources you are authorized to access. Streaming addons do not grant distribution rights.

> **Are you a maintainer?** See [Developer documentation](docs/DEVELOPMENT.md) for build automation and manual release instructions. Visitors should use **Releases** for downloads.
