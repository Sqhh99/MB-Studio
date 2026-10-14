# MB-Studio changes

Changes of MB-Studio relative to upstream MB-System. The MB-System change log
is in `ChangeLog.md`.

## 0.3.0

MB-Studio 0.3 follows the review in
[project-review-and-workbench-roadmap.md](project-review-and-workbench-roadmap.md):
results are correct and traceable first, then the workbench, 3D terrain
viewing, sounding editing and GMT cartography.

- **Jobs that cannot collide.** Every job declares the survey lines and files
  it reads and writes. The job service never runs a job that writes what
  another job reads or writes, and conflicting jobs keep the order in which
  they were submitted; independent jobs still run in parallel. The Jobs panel
  says which job a queued job waits for. Product names are reserved while a
  job is queued or running, so two jobs never write the same grid.
- **Retries are new attempts.** Running a project job again (Jobs panel,
  history, `project.retry`) creates a new history entry (attempt 2, 3, ...)
  with its own job folder and outputs and a link to the attempt it repeats.
  Every job records its request, so it can also be repeated in a later
  session, or loaded into its processing panel to change the settings.
- **Data versions.** A survey line has an original, a preprocessed and a
  processed file; the line inspector shows the version you choose and says
  which one it shows.
- **What needs updating.** Survey lines count their edit, correction and
  source revisions and remember which ones their processed file contains;
  products remember what they were made from. The project panel shows
  *edits pending*, *processing required*, *outdated* and *pending*; the
  properties panel and the tool bar show the recommended next step; an
  outdated product is made again from its recorded recipe with one click.
- **Projected grids on the map.** Grids in UTM or local transverse Mercator
  coordinates (as written by `mbgrid -J`) are placed on the map; the job
  message says whether the result is on the map or why not.
- **3D terrain view.** A grid as a lit surface: orbit, pan, zoom, fit, top,
  front and side views, perspective or orthographic projection, vertical
  exaggeration, colour tables and legend, the value under the cursor (from
  the grid, not from the picture), distance and slope between two points and
  a profile. Cells without data are holes, interpolated cells are stippled.
- **Sounding editor.** The soundings of selected survey lines in 3D with the
  saved edits applied; rectangle, lasso, brush and point selection (replace,
  add with Shift, remove with Ctrl), flag and unflag, undo and redo, colours
  by depth, line, flag state or beam, and a linked ping view across the
  track. Every sounding keeps its line, ping time, multiplicity and beam;
  saving appends edit events to the line's edit save file (`.esf`) and sets
  `EDITSAVEMODE`/`EDITSAVEFILE` for `mbprocess`, as MB-System's editors do.
- **GMT figures.** A figure panel makes bathymetric maps with contours,
  survey overviews, grid quality maps and perspective views with GMT (colour
  scale, scale bar, title; PNG, PDF or PostScript). Each figure job runs in
  its own GMT session and temporary folder, so figure jobs can run at the
  same time. Missing Ghostscript (needed for PNG and PDF) is reported before
  the job starts. Figures are products with their recipe: they open in a
  figure tab, can be made again, and their settings can be changed.
- **Workbench.**
  - An MB-Studio title bar with the menus, the project name, a mark for
    unsaved edits and the window buttons. On Windows the window keeps the
    native frame behaviour (Snap, double click, the system menu, resizing at
    the edges); `MBSTUDIO_NATIVE_TITLEBAR=1` brings back the system title
    bar. The window size and position are restored onto a screen that still
    exists.
  - Every grid, 3D view, survey line, editor and figure tab has its own
    document, so several can be open at once; the document area splits into
    two groups side by side to compare them. Open tabs, the split and each
    document's view (colours, zoom, camera, version) are saved with the
    project.
  - The properties panel shows the settings of the active document, so the
    grid view no longer has a sidebar of its own; a tool options bar shows the
    tools of the active document; the status bar shows the active tool, the
    selection, the cursor position and the data version.
  - Menus File, Edit, View, Data, Process, Analysis, Window and Help;
    workspace presets (Overview, Bathymetry editing, Modelling, Cartography,
    Delivery) with a saved custom layout.
  - Neutral grey themes with compact controls.
- **Command line.** `project.retry`, `project.figure`, `soundings.info` and
  `figure.check`; `project.info` lists the line and product states and the
  next step.
- Project files are version 2; projects of 0.2 are read and updated.
- Plugin ABI version 3: plugins built for 0.2 must be rebuilt.

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
