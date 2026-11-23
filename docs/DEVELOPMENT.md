# Building NUVIO on Windows

## Requirements

Visual Studio 2010 with Visual C++ and a separately installed, appropriately
licensed Xbox 360 XDK. Ordinary Win32/x64 compilers do not replace the Xbox build
platform. Do not redistribute Microsoft headers, libraries, tools or installers.

## Release build

Extract the repository preserving its directory layout, then run
`VisualStudio2010/BUILD-RELEASE.cmd`. It calls the dependency bootstrap, native
TLS build, decoder build and Release Xbox 360 application rebuild. Network
access is needed only when pinned dependencies are missing.

Alternatively, from VisualStudio2010:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\prepare_release.ps1
```

Wait for READY. The main output is Release/NUVIO.xex, mirrored from bin/Release.
Open NUVIO.sln in VS2010 and select Release | Xbox 360 for manual builds after
building dependencies. If the platform is missing, repair the XDK integration;
selecting Win32/x64 will not produce an Xbox executable.

C1083 mbedtls/ssl.h and LNK1181 video.lib indicate missing dependencies or library
builds. C4244 numeric-conversion and missing third-party PDB warnings are nonfatal
only if the overall build succeeds. Save the first fatal error when reporting.

## Tests

Windows batch helpers in VisualStudio2010 include run_core_tests.bat,
run_auth_tests.bat, run_cloud_tests.bat, run_torrent_tests.bat and run_vmx_tests.bat.
They exercise isolated code/fixtures and do not validate real media playback.

Portable core example:

```sh
g++ -std=c++98 -Wall -Wextra VisualStudio2010/tests/NuvioCoreTests.cpp VisualStudio2010/src/NuvioApp.cpp VisualStudio2010/src/NuvioCatalog.cpp -o nuvio_core_tests
./nuvio_core_tests
```

The decoder build and application post-build step run tools/audit_xenon_vmx.py.
See DECODER-XENON.md for host tests and the separate native pixel verifier.

## Architecture

- NuvioApp: portable C++98 state and navigation.
- XboxMain/NuvioGraphics: Xbox input, UI, rendering and serialized request jobs.
- NuvioNativeBackend: device authentication, profiles, add-ons, catalogs and sources.
- NuvioTorrent: bounded native v1 TCP byte source with SHA-1 verification.
- FFPlayer/ffglue: FFmpeg, GPU YUV output, XAudio2 and source read/seek.
- tools/transcode_server.py: optional external authenticated conversion worker.

## Publishing and status

Package only the XEX, documentation, corresponding source and license notices.
Exclude account sessions, private configuration, personal caches and XDK files.
Verify artifact hashes and identify the exact source commit in release metadata.

Keep this release experimental: the maintainer still reports video-loading
failure. Current hardware playback, performance, subtitles and provider behavior
remain pending. See KNOWN_ISSUES.md, VALIDATION.md and RELEASE_NOTES.md.
