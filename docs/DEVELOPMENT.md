# 🛠️ Build NUVIO.XEX on your own PC

NUVIO.XEX is built **locally**. The project maintainer compiles and tests the `.XEX` on their own computer, then uploads a ZIP manually under [GitHub Releases](https://github.com/heltindev/NuvioXbox360/releases). No cloud build or publishing service is required.

> [!IMPORTANT]
> **Visual Studio 2010 and the Microsoft Xbox 360 SDK do not directly compile this project.** The current code uses the separate, open-source **[xsdk](https://github.com/xenosworks/xsdk)** C APIs and a PowerPC toolchain. Building it with the Microsoft XDK would first require porting the rendering, controller, networking, and application startup code to Microsoft XDK APIs. Do not mix these two SDKs or assume an `.exe` installer will create `NUVIO.XEX`.

## What you need

| Item | Why |
| --- | --- |
| Windows 10/11 with WSL 2 (Debian), or native Linux | Host environment for the supported xsdk build |
| About 10 GB of free space | Initial compiler build and temporary files |
| CMake 3.25+, Ninja, GCC, Git, Python | Build tools |
| [xsdk](https://github.com/xenosworks/xsdk) and its PowerPC compiler | Builds an Xbox 360 executable |
| [glslang-xenon](https://github.com/xenosworks/glslang-xenon) | Graphics tool needed to build xsdk |
| [curl-xenon](https://github.com/xenosworks/curl-xenon) and [mbedtls-xenon](https://github.com/xenosworks/mbedtls-xenon) | HTTP/HTTPS libraries |
| Xbox 360 RGH/JTAG running Aurora | Hardware test |

**Recommended for Windows users:** Follow the official [xsdk getting started guide](https://github.com/xenosworks/xsdk/blob/main/docs/GETTING_STARTED.md) for setup details and compiler troubleshooting. Commands below are run **inside Debian on WSL**, not in Windows Command Prompt or the Visual Studio IDE.

## 1. Prepare Debian / WSL

On Windows, open PowerShell and run:

```powershell
wsl --install -d Debian
```

After installation, open Debian and run:

```bash
sudo apt update
sudo apt install -y build-essential bison flex texinfo libgmp-dev libmpfr-dev \
  libmpc-dev curl xz-utils patch python3 cmake ninja-build git zip
```

## 2. Install xsdk and its PowerPC compiler

```bash
git clone https://github.com/xenosworks/xsdk.git "$HOME/xsdk"
PREFIX="$HOME/xsdk-gcc" WORK="$HOME/xsdk-gcc-work" JOBS="$(nproc)" \
  bash "$HOME/xsdk/tools/toolchain/build.sh"
"$HOME/xsdk-gcc/bin/powerpc-xsdk-elf-gcc" --version
```

**The first compiler build takes a while.** If it fails, inspect the logs in `~/xsdk-gcc-work/logs/`. Running the script again can reuse completed stages.

## 3. Prepare the graphics compiler and xsdk

```bash
git clone --depth 1 --branch xenon https://github.com/xenosworks/glslang-xenon.git "$HOME/glslang-xenon"
(cd "$HOME/glslang-xenon" && python3 update_glslang_sources.py)
cmake -S "$HOME/glslang-xenon" -B "$HOME/glslang-xenon/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DENABLE_OPT=ON \
  -DSPIRV_SKIP_EXECUTABLES=ON -DGLSLANG_TESTS=OFF
cmake --build "$HOME/glslang-xenon/build" --parallel "$(nproc)"
cp "$HOME/glslang-xenon/build/StandAlone/glslang" "$HOME/xsdk-gcc/bin/glslang"
make -C "$HOME/xsdk" -j "$(nproc)" \
  TOOLCHAIN="$HOME/xsdk-gcc/bin/powerpc-xsdk-elf-"
```

## 4. Get networking libraries and build NUVIO.XEX

```bash
git clone --depth 1 --branch xenon https://github.com/xenosworks/mbedtls-xenon.git "$HOME/mbedtls-xenon"
git clone --depth 1 --branch xenon https://github.com/xenosworks/curl-xenon.git "$HOME/curl-xenon"
```

Open a Debian terminal in **the folder containing this repository's `CMakeLists.txt`**, then run:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$HOME/xsdk/cmake/xsdk.cmake" \
  -DXSDK_TOOLCHAIN="$HOME/xsdk-gcc" \
  -DMBEDTLS_XENON_DIR="$HOME/mbedtls-xenon" \
  -DCURL_XENON_DIR="$HOME/curl-xenon" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel "$(nproc)"
```

If the build succeeds, look for `build/NUVIO.xex` and `build/cacert.pem`. **This is not proof that the application works on the console:** test it on actual Xbox hardware before publishing.

## 5. Make the ZIP on your PC

From the repository root, after a successful build:

```bash
mkdir -p dist
cp build/NUVIO.xex dist/NUVIO.XEX
cp build/cacert.pem catalog_manifest.txt stream_addons.txt INSTALL-ON-XBOX.txt dist/
(
  cd dist
  zip -9 NUVIO-XEX-Preview.zip NUVIO.XEX cacert.pem \
    catalog_manifest.txt stream_addons.txt INSTALL-ON-XBOX.txt
  sha256sum NUVIO-XEX-Preview.zip > SHA256SUMS.txt
)
```

The download file is `dist/NUVIO-XEX-Preview.zip`. See [INSTALL-ON-XBOX.txt](../INSTALL-ON-XBOX.txt) for copying it to an RGH/JTAG console running Aurora.

## 6. Publish a Release manually

1. Open [Releases](https://github.com/heltindev/NuvioXbox360/releases) while signed in as a repository maintainer.
2. Select **Draft a new release** and create a new version tag (for example, `v0.3.0-alpha.1`).
3. Give it a clear title and paste the notes from [RELEASE_NOTES.md](RELEASE_NOTES.md).
4. Upload `dist/NUVIO-XEX-Preview.zip` and `dist/SHA256SUMS.txt`.
5. Select **Set as a pre-release**, because playback and hardware compatibility are not ready yet.
6. Publish **only after** the build succeeds and you have assessed its hardware behavior.

Visitors download releases; they do **not** need a compiler, SDK, or Visual Studio installed.

## Local parser tests (without Xbox hardware)

```bash
gcc -std=c17 -Wall -Wextra -Werror -Isrc -Ithird_party/cjson \
  tests/test_stremio.c src/stremio.c third_party/cjson/cJSON.c \
  -lm -o test_stremio
./test_stremio
```

## Architecture and safety

The app is an independent C rewrite using xsdk, the [Stremio addon protocol](https://github.com/Stremio/stremio-addon-sdk/blob/master/docs/protocol.md), and bundled cJSON (MIT notice retained).

- Do not install unverified SDK packages from unknown sources or upload licensed Microsoft SDK files to this public repository.
- Never commit credentials, personal API tokens, or private account data.
- Keep HTTPS certificate verification enabled.
- Only access media sources you are authorized to use.
- Video/audio playback and BitTorrent downloading are **not implemented yet**.
