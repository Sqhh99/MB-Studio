# MB-Studio changes

Changes of MB-Studio relative to upstream MB-System. The MB-System change log
is in `ChangeLog.md`.

## 0.2.0

MB-Studio becomes a project-centred workbench that integrates the MB-System
processing chain instead of a collection of separate pages.

- **Projects.** A project (`<name>.mbsproj`, JSON) keeps the survey lines,
  products, map layers, the open views and the complete processing history.
  Survey data stay where they are. MB-Studio identifies the formats, writes
  the MB-System datalists itself (`datalists/`), creates the `.inf`, `.fbt`
  and `.fnv` files and reads the metadata of every line in the background.
  Files and folders can be added from the menu, the project panel or by drag
  and drop.
- **Processing chain.** Panels for preprocessing (`mbpreprocess`), cleaning
  (`mbclean`, `mbareaclean`), corrections (`mbset`, `mblevitus`, `mbotps`),
  processing (`mbprocess`), gridding (`mbgrid`), mosaicing (`mbbackangle`,
  `mbmosaic`) and export (GeoTIFF, ESRI ASCII, XYZ, glTF, LAS) work on the
  selected survey lines. Parameters are typed (units, ranges, choices that
  depend on each other), mapped explicitly to the program options, and the
  panel shows the exact commands, the products, an estimate of the grid size
  and warnings (for example that interpolated cells are not measured data)
  before anything runs. Gridding uses processed files automatically
  (`$PROCESSED` datalists).
- **Background jobs.** A job service runs the programs with structured
  argument lists on worker threads, with a queue, cancel, retry, a log file
  per job and progress messages; the Jobs panel shows them. The history keeps
  every job's commands, parameters, inputs, outputs, exit code, times and log.
- **Map.** The project map draws the navigation tracks of all survey lines
  (click to select, Ctrl and Shift to extend) and the grids as layers with
  opacity, colour table and shading, with a graticule, a scale bar and a
  cursor readout of position and depth.
- **Grid quality.** The grid view shows the share of empty cells, the data
  count and standard deviation grids written by `mbgrid -M`, and hatches
  cells that were filled by interpolation, so they are not mistaken for
  measured data. The colour range can be set by hand.
- **Workbench.** Menus, toolbar, a project panel, document tabs, a
  properties panel, the jobs and log panel and a command palette
  (Ctrl+Shift+P). Views and menu actions are contributions that plugins
  register in the kernel, so a plugin adds a view without changing the main
  window. The panel layout is remembered.
- **Command line.** `mbstudio --command project.create|project.add|project.grid|project.info`,
  `grid.stats`, `processing.grid`, `jobs.list` and `jobs.cancel`.
- **Fixes.** MB-System programs started by MB-Studio find the programs they
  start themselves (`mbdatalist -O` runs `mbinfo`, `mbcopy` and `mblist`)
  because the program folder is put first on their `PATH`.
- Plugin ABI version 2: plugins built for 0.1 must be rebuilt.

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
