# MB-Studio changes

Changes of MB-Studio relative to upstream MB-System. The MB-System change log
is in `ChangeLog.md`.

## 0.1.1

- **Languages.** The user interface is available in English, Japanese and
  Simplified Chinese. It follows the Windows display language; the language
  button at the bottom of the sidebar changes it at run time (saved in the
  settings; `MBSTUDIO_LANG` overrides it). Translations are JSON catalogues in
  `src/mbstudio/plugins/qmlui/i18n`, checked against the sources by
  `check_translations.py`.
- **Program help.** Every program on the Programs page has a help page in
  English, Japanese and Simplified Chinese: synopsis, description, common
  options, Windows (cmd.exe) examples and notes. The Programs page shows it in
  a Help tab (the old Help button, which runs the program with `-h`, is now
  called Usage). The pages are also installed as HTML
  (`share\doc\mbstudio\help`, Start menu entry "MB-System program help") and
  can be printed with `mbstudio --help-program NAME [--lang ja|zh_CN|en]`.
  Sources: `src/mbstudio/help/<language>/<program>.md`.
- **Fixes.**
  - The sidebar buttons no longer flicker when the pointer enters or leaves
    them (the highlight faded through a dark colour).
  - `mbgetesf -O` and `mbdumpesf` write and read edit save files in binary
    mode, so they are not corrupted on Windows.
- **Installer** in Japanese as well.

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
