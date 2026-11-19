# 🎮 NUVIO.XEX — Visual Studio 2010 starter

This folder is the ready-to-build streaming-style UI preview for our **Xbox 360 RGH/JTAG C++ application**.

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
| D-pad | Move around catalogs and the left sidebar |
| A | Pick Admin, open a sidebar section or view catalog details |
| B | Back to the catalog, or return to the Admin profile selector |
| Back | Exit the app |

The UI now starts at **Who's watching?**, with a single **Admin** profile. A opens a dark, TV-style catalog with a thin sidebar and highlighted poster-shaped cards. The default catalog contains fictional demo titles. You can import an offline **IMDb metadata** selection using [the importer](../docs/IMDB_CATALOG.md). The card artwork is abstract placeholder art; there is still **no streaming, video or audio playback**.

## Source files

- `src/XboxMain.cpp`: console-specific entry point, rectangle-based dashboard, bitmap text and controller.
- `src/NuvioApp.h` and `src/NuvioApp.cpp`: small navigation engine that also runs on a PC.
- `src/NuvioCatalog.h` and `src/NuvioCatalog.cpp`: safe, bounded catalog parser and demo titles.
- `tests/NuvioCoreTests.cpp`: standalone tests for navigation and offline metadata.

If you have a PC C++ compiler such as MinGW or G++, you can run the core test without the Xbox 360 SDK:

```bash
g++ -std=c++98 -Wall -Wextra -Werror \
  VisualStudio2010/tests/NuvioCoreTests.cpp \
  VisualStudio2010/src/NuvioApp.cpp \
  VisualStudio2010/src/NuvioCatalog.cpp -o nuvio_core_tests
./nuvio_core_tests VisualStudio2010/tests/catalog_fixture.tsv
```

Run those commands from the **repository root**. They do not create an `.xex`.

## Where we go from here

1. Confirm the Xbox 360 project platform is detected by Visual Studio.
2. Test the freshly compiled dashboard in Xenia Canary.
3. Check video output and controller input on an RGH Xbox 360.
4. Confirm the new Admin chooser and TV-style rows render correctly.
5. Add licensed real poster images, richer catalogs and a media player later.

See the [full roadmap](../ROADMAP.md) and [development guide](../docs/DEVELOPMENT.md).
