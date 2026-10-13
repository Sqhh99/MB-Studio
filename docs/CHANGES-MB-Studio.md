# MB-Studio changes

Changes of MB-Studio relative to upstream MB-System. The MB-System change log
is in `ChangeLog.md`.

## 0.1.0

First release: MB-System 5.8.3beta25 for native Windows with the MB-Studio
desktop application.

- **Windows port.** MB-System libraries and all command line programs build
  with MinGW-w64 and vcpkg (GMT, GDAL, PROJ, netCDF, HDF5, FFTW) and run on
  stock Windows 10/11. See [windows-port.md](windows-port.md).
- **MB-Studio.** Qt 6 / QML desktop application on a microkernel: kernel with
  plugin manager, service registry, event bus and command registry; the
  MB-System adapter plugin; the user interface plugin (Home, swath Inspector,
  Grid viewer, Programs, Formats, Plugins). The `mbstudio` executable can also
  run kernel commands without the interface.
- **Packaging.** Inno Setup installer (English and Simplified Chinese) with
  Start menu entries for MB-Studio and an MB-System command prompt, optional
  `PATH` registration, per-user or per-machine installation.
- **CI.** `build.yml` builds and tests on Windows (and Linux) for every pull
  request and smoke-tests the installed application; `release.yml` publishes
  the installer when a `v*` tag is pushed.
- **Fixes that also apply to other platforms:** option arguments containing
  spaces, `mblevitus` honours `MBSYSTEM_SHAREDIR`, googletest regular
  expressions that work on every platform.
