# 🎮 NUVIO.XEX — Visual Studio 2010 starter

This folder is the starting point for our **Xbox 360 RGH/JTAG C++ application**.

## Quick start

1. Install Visual Studio 2010 with **Visual C++** selected.
2. Use an appropriately licensed Xbox 360 XDK installation that supports Visual Studio 2010.
3. Double-click **`NUVIO.XEX.sln`**.
4. In the Visual Studio toolbar, choose **Debug** and **Xbox 360**.
5. Choose **Build → Build Solution** (or press `F7`).
6. On a successful build, look under **`Debug/`** for `NUVIO.XEX.xex`, or **`Release/`** for a Release build. The `bin/` folder contains symbol files, not the actual `.xex` on the tested XDK.

If the **Xbox 360** build platform is not offered, stop and check the XDK integration. The normal PC `Win32` or `x64` platform does not produce Xbox 360 code.

> [!WARNING]
> The dashboard now **compiles on the maintainer's XDK 21256**, but its updated visuals and operation on real Xbox 360 hardware still need testing. Other legacy XDK releases may require compiler or linker adjustments.

## What the buttons do

| Button | Behavior |
| --- | --- |
| D-pad | Change selected section on the home screen |
| A | Open Movies, Series, My Library, or Settings |
| B | Return to the home screen |
| Back | Exit the app |

The dashboard now displays a **NUVIO header, visible bitmap text, four colored cards, and a highlighted selection**. A section screen provides a clear return instruction. There are **no posters, film lists, sound or video playback** yet. We will add them incrementally.

## Source files

- `src/XboxMain.cpp`: console-specific entry point, rectangle-based dashboard, bitmap text and controller.
- `src/NuvioApp.h` and `src/NuvioApp.cpp`: small navigation engine that also runs on a PC.
- `tests/NuvioCoreTests.cpp`: standalone test for the navigation engine.

If you have a PC C++ compiler such as MinGW or G++, you can run the core test without the Xbox 360 SDK:

```bash
g++ -std=c++98 -Wall -Wextra -Werror \
  VisualStudio2010/tests/NuvioCoreTests.cpp \
  VisualStudio2010/src/NuvioApp.cpp -o nuvio_core_tests
./nuvio_core_tests
```

Run those commands from the **repository root**. They do not create an `.xex`.

## Where we go from here

1. Confirm the Xbox 360 project platform is detected by Visual Studio.
2. Test the freshly compiled dashboard in Xenia Canary.
3. Check video output and controller input on an RGH Xbox 360.
4. Build a real Nuvio-style 720p dashboard on top of the working console app.
5. Add metadata, artwork and a media player in later milestones.

See the [full roadmap](../ROADMAP.md) and [development guide](../docs/DEVELOPMENT.md).
