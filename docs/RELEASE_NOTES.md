# 🎮 NUVIO.XEX — Foundation Preview

NUVIO.XEX is an independent, unofficial Nuvio-inspired homebrew project for Xbox 360 RGH/JTAG.

## ⚠️ Early prototype

This release is intended **only for developers testing their own Xbox 360 hardware**. It has a small application-state engine and a controller-friendly, 720p dashboard preview with bitmap text. It does **not** browse movie catalogs or play audio/video.

## Included in the source

- Visual Studio 2010 `.sln` and `.vcxproj` for an Xbox 360 XDK toolset.
- Portable Nuvio navigation model (Home, Movies, Series, My Library and Settings).
- Xbox controller input and a colored 720p NUVIO menu with visible text.
- A locally runnable navigation test and installation instructions.

## Test instructions

After a successful build **with a licensed Xbox 360 XDK**, place `NUVIO.XEX.xex` on a USB drive, open it in Aurora and test the screen and buttons. If the executable fails to start, keep the build logs and record the XDK version and console model.

**Do not publish an `.xex` release until it actually compiles and hardware tests have been performed.**

Read [ROADMAP.md](https://github.com/heltindev/NuvioXbox360/blob/main/ROADMAP.md) for future features.
