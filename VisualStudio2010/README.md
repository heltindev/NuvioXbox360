# NUVIO — Xbox 360 project

Run `BUILD-RELEASE.cmd` with Visual Studio 2010 and the Xbox 360 XDK installed.
Alternatively, open `NUVIO.sln` and build **Release | Xbox 360** after building
the dependencies. Output: `Release/NUVIO.xex`, mirrored from `bin/Release`.

See the [main README](../README.md), [development guide](../docs/DEVELOPMENT.md)
and [cloud add-on notes](../docs/CLOUD-ADDONS.md) for controls, limits and protocols.

Test helpers: run_core_tests.bat, run_auth_tests.bat, run_cloud_tests.bat,
run_http_tests.bat, run_torrent_tests.bat and run_vmx_tests.bat.
They do not prove successful hardware playback.

Preserve ThirdParty sources and license notices. Missing third-party PDB warnings
are nonfatal only when the overall build succeeds.

**The maintainer still reports a video-loading error. This release is experimental.**
Track [issue #4](https://github.com/heltindev/NuvioXbox360/issues/4).
