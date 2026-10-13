# Building MB-Studio on Windows

MB-Studio is built natively on Windows with the **MinGW-w64 GCC** toolchain.
All third-party libraries come from **vcpkg** (manifest mode, `vcpkg.json`).
No WSL, Cygwin or Docker is involved, and the resulting programs run on a stock
Windows 10/11 system.

The [Build workflow](../.github/workflows/build.yml) performs exactly these
steps on every pull request.

## Prerequisites

| Tool | Notes |
|---|---|
| [MSYS2](https://www.msys2.org) MinGW-w64 toolchain | `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-binutils mingw-w64-x86_64-ninja`; only the compiler is used, put `C:\msys64\mingw64\bin` on `PATH` |
| [CMake](https://cmake.org) 3.22 or newer | |
| [vcpkg](https://github.com/microsoft/vcpkg) | clone and run `bootstrap-vcpkg.bat`; the commit used by CI is the `builtin-baseline` in `vcpkg.json` |
| [Python](https://www.python.org) 3 | runs the test suite |
| [Inno Setup](https://jrsoftware.org/isinfo.php) 6.5 or newer | only to build the installer |

## Dependencies (vcpkg)

`vcpkg.json` declares NetCDF, HDF5, PROJ, GDAL, FFTW and GMT, plus Qt 6
(`qtbase`, `qtdeclarative`) for the user interface (`gui` feature, on by
default). GMT is not in the vcpkg registry; `vcpkg-overlays/ports/gmt` builds
it from the official 6.7.0 sources. The custom triplet
`vcpkg-overlays/triplets/x64-mingw-mbstudio.cmake` selects MinGW-w64, shared
libraries and release-only builds.

The first build of the dependencies takes a long time (Qt in particular).
Use a vcpkg binary cache to reuse the results:

```powershell
$env:VCPKG_BINARY_SOURCES = "clear;files,C:\vcpkg-cache,readwrite"
```

## Configure, build and test

From a PowerShell prompt in the source directory:

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-mingw-mbstudio `
  -DVCPKG_HOST_TRIPLET=x64-mingw-mbstudio `
  -DCMAKE_INSTALL_PREFIX="$PWD/stage"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Useful options:

| Option | Default on Windows | Meaning |
|---|---|---|
| `buildStudio` | `ON` | MB-Studio kernel, plugins and user interface |
| `buildTests` | `ON` | unit, functional and platform tests |
| `buildGSF` | `ON` | GSF format support |
| `buildGUIs`, `buildOpenCV`, `buildTRN` | `OFF` | X11/Motif tools, photomosaicing and TRN are not available on Windows |
| `MB_DEPLOY_DEPENDENCIES` | `ON` | copy dependency DLLs, Qt and data when installing |

To start MB-Studio from the build tree run `build\bin\mbstudio.exe`. The
plugins are found in `build\lib\mbstudio\plugins`, the MB-System programs in
`build\src\utilities`.

## Install and package

```powershell
cmake --install build          # self-contained tree in .\stage
iscc /DAppVersion=0.1.0 /DStageDir=$PWD\stage /DSourceDir=$PWD /DOutputDir=$PWD\dist packaging\windows\mbstudio.iss
```

`cmake --install` runs `cmake/MBWindowsDeploy.cmake`, which copies every DLL
the programs need (resolved with `file(GET_RUNTIME_DEPENDENCIES)`), the GMT,
PROJ and GDAL data, `gmt.exe`, and the Qt plugins and QML modules, so that the
staging directory runs without any `PATH` set-up:

```
stage\
  bin\                     MB-System programs, mbstudio.exe, gmt.exe, DLLs
  bin\platforms, bin\qml\  Qt run time
  bin\gmt_plugins\         MB-System GMT supplement (mbsystem.dll)
  lib\mbstudio\plugins\    MB-Studio plugins (mbsystem, qmlui)
  share\mbsystem, share\gmt, share\proj, share\gdal, share\doc
```

## Releases

Pushing a tag `v<version>` (matching the `VERSION` file, e.g. `v0.1.0`) runs
the [Release workflow](../.github/workflows/release.yml): it builds and tests
everything with `build.yml`, creates the installer and publishes it with its
SHA-256 checksum as a GitHub release.

## Cross-compiling from Linux (developers)

`cmake/toolchains/x86_64-w64-mingw32.cmake` cross-compiles with the
`x86_64-w64-mingw32` GCC of a Linux distribution. With Wine installed the test
suite runs under Wine (`ctest`), which is a quick way to check Windows changes
without a Windows machine:

```sh
cmake -S . -B build-win -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-w64-mingw32.cmake \
  -DCMAKE_PREFIX_PATH=/path/to/windows/dependencies \
  "-DMB_TEST_PYTHON=wine;C:/Python312/python.exe"
```
