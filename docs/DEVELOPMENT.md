# 🛠️ Building NUVIO on Windows

This repository now uses a **Visual Studio 2010 Xbox 360 project**. It no longer requires a separate Linux cross-compiler or a cloud build service.

## 1. Install Visual Studio 2010

When selecting installation features, keep **Visual C++** enabled. The x64 compiler tools are optional for this project; Itanium, Visual Basic, C#, F#, web, Office, database and SharePoint components are not required.

## 2. Install a compatible Xbox 360 XDK

The Xbox 360 build platform is supplied by a **separately installed and appropriately licensed** Xbox 360 XDK, not by ordinary Visual Studio 2010. Do not put Microsoft SDK headers, libraries, executables, or installers in this repository.

With the XDK integration installed, Visual Studio should offer an **Xbox 360** platform for C++ projects. Depending on the XDK version and Windows version, legacy setup may require repair or compatibility work. We have **not** verified the particular XDK version shown in the maintainer's screenshot.

## 3. Open the project

1. Download the repository ZIP from GitHub and extract it.
2. Open `VisualStudio2010/NUVIO.sln` in Visual Studio 2010.
3. Select **Debug | Xbox 360**.
4. Select **Build → Build Solution**.
5. Check the build output. If successful, find `VisualStudio2010/bin/Debug/NUVIO.xex`.

If the project reports that the **Xbox 360 platform is missing**, stop and troubleshoot the XDK installation. Selecting Win32/x64 instead does not fix it.

If the Xbox build reports compiler, API, or linker errors, save the **exact first error message** and the XDK version. The starter intentionally does not claim support for all XDK releases. The real `.XEX` has not yet been built or console-tested here.

## 4. Local test without the XDK

Only the NUVIO menu-state engine is platform-independent. With G++ or Clang installed, run from the repository root:

```bash
g++ -std=c++98 -Wall -Wextra -Werror \
  VisualStudio2010/tests/NuvioCoreTests.cpp \
  VisualStudio2010/src/NuvioApp.cpp -o nuvio_core_tests
./nuvio_core_tests
```

This test confirms screen-state transitions. It does **not** compile the Xbox executable or test graphics.

## 5. Test with Aurora and publish manually

After a successful `.XEX` build, copy it to a FAT32 USB drive and follow [INSTALL-ON-XBOX.txt](../INSTALL-ON-XBOX.txt). Once hardware testing is successful, a maintainer can manually create a [GitHub Release](https://github.com/heltindev/NuvioXbox360/releases) and attach a ZIP with the `.xex` and installation guide.

See [RELEASE_NOTES.md](RELEASE_NOTES.md) for an honest preview warning. No downloadable release should be presented as a working media player at this stage.

## Architecture and licensing

- `VisualStudio2010/src/NuvioApp.*`: SDK-independent C++98 application state.
- `VisualStudio2010/src/XboxMain.cpp`: Xbox 360-specific controller and graphics smoke test using XDK headers.
- `VisualStudio2010/NUVIO.vcxproj`: legacy `Xbox 360` build platform, Debug and Release.
- The original [Nuvio Mobile](https://github.com/NuvioMedia/NuvioMobile) is GPL-3.0 software and a feature reference. No upstream implementation has been copied into this independent starter.
- Microsoft XDK is proprietary. Use it only under applicable licenses and do not redistribute its files.
