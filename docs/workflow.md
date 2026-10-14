# Processing a survey with MB-Studio

This guide takes a multibeam survey (for example the Kongsberg EM122 lines of
*PatchSurveyEM122*) from the raw files to a terrain model, an edited and
processed data set and printable maps in MB-Studio 0.3. Every step runs the
MB-System programs (and GMT for maps); MB-Studio writes the datalists,
chooses the file names, runs the programs in the background and records what
it did.

## 1. Create a project

**File → New project…** (Ctrl+N). Choose a name and the folder the project is
created in, and optionally a folder with survey data. The project folder
contains:

| Path | Contents |
|------|----------|
| `<name>.mbsproj` | the project: lines and their revisions, products and what they were made from, layers, open documents, history |
| `datalists/` | `raw.mb-1`, `work.mb-1` and `workp.mb-1` (`$PROCESSED`) for use with the command line |
| `processed/` | files written by preprocessing |
| `products/grids`, `products/mosaics`, `products/figures` | grids and mosaics with their quality grids, GMT figures |
| `exports/` | exported files |
| `jobs/<n>/` | the datalists, files and `job.log` of each job |

The survey files stay where they are. MB-System datalists cannot hold paths
with spaces, so the path from the project to the data must not contain spaces
(the project folder itself may).

## 2. Add the survey lines

**Data → Add survey data…** (Ctrl+I), the **+** buttons of the project panel,
or drag files or folders onto the window. Folders are searched recursively;
MB-Studio identifies the MB-System format of each file and skips cache and
product files. A background job then creates the `.inf`, `.fbt` and `.fnv`
files (`mbdatalist -O`) and reads the statistics of every line. The lines
appear on the **Map** as soon as their navigation is known.

Select lines in the project panel or on the map (click; Ctrl+click adds,
Shift+click extends). Processing steps work on the selected lines, or on all
lines when nothing is selected.

## 3. Process

The tool bar and the **Process** menu open the processing panels on the right.
Each panel shows the parameters, the exact commands it will run, the products
it will write and warnings, and **Run** starts a background job (see the
**Jobs** tab at the bottom: progress, cancel, run again, log).

1. **Preprocess** — raw vendor formats (Kongsberg `.all` / `.kmall`, Reson
   `.s7k`, ...) are converted to the MB-System processing format
   (`mbpreprocess`); for EM122 `.all` (format 58) this writes format 59
   files into `processed/`. Later steps use these files.
2. **Clean** — automatic flagging of outliers by slope, depth range, distance
   from the median, outer beams, spikes and noisy pings (`mbclean`), and by
   area statistics (`mbareaclean`). Flags go to edit files; the data are not
   changed.
3. **Edit soundings** — see section 4.
4. **Corrections** — sound velocity (a profile file or the Levitus
   climatology), roll and pitch bias, draft and tide (`mbset`, `mblevitus`,
   `mbotps`).
5. **Process** — applies edits and corrections (`mbprocess`) and writes the
   processed files (`...p.mb59`).
6. **Grid** — `mbgrid` with cell size, algorithm (weighted mean, median,
   minimum/maximum, beam footprint), gap interpolation and coordinates
   (geographic or UTM). The panel estimates the grid size and memory. The
   quality grids (data count and standard deviation) are written by default.
   Gridding always uses the processed files where they exist.
7. **Mosaic** — amplitude or sidescan mosaics (`mbmosaic`), optionally after
   a grazing angle correction (`mbbackangle`, `mbprocess`).
8. **Figure** — maps made with GMT, see section 7.
9. **Export** — a grid as GeoTIFF, ESRI ASCII grid, XYZ or glTF 3D model, or
   the soundings as a LAS point cloud or XYZ text.

Jobs that work on the same survey lines or files never run at the same time:
a grid waits for the cleaning of its lines, and the Jobs panel says which job
it waits for. Independent jobs run side by side.

## 4. Edit soundings

Select survey lines and choose **Edit → Sounding editor** (Ctrl+Shift+E), or
*Edit soundings* in the menu of a line. The editor shows the soundings of the
lines in 3D with the flags they have now (the automatic cleaning and earlier
edits included) and, below, the beams of the current ping across the track.

- The **tool options bar** has the tools: navigate (orbit with the left
  button, pan with the right one, zoom with the wheel; a click shows the ping
  of a sounding), rectangle, lasso, brush and point selection. Shift adds to
  the selection, Ctrl removes from it.
- **Delete** (or F) flags the selected soundings, **Ctrl+U** unflags them,
  **Ctrl+Z** and **Ctrl+Y** undo and redo. In the ping view a click on a beam
  flags or unflags it; Page Up and Page Down step through the pings.
- The properties panel shows the sounding under the cursor with its line,
  ping time, ping and beam number — exactly what an edit refers to.
- **Save edits** (Ctrl+S) appends the edits to the edit save files (`.esf`)
  of the lines and sets their `mbprocess` parameters, as mbedit and mbeditviz
  do. The data files are not changed; **Process** applies the edits.

## 5. Check the result

Grids are added to the project and shown on the map (eye button in the
product list; opacity, colour table and shading in the properties panel).
Grids in UTM coordinates are placed on the map as well. Double-click a grid
to open it in a **Grid** tab; its display settings, coverage and extent are
in the properties panel:

- the share of **empty cells**;
- with the quality grids, the **data count** and **standard deviation** per
  cell (the layer selector of the view), and the share of cells that were
  filled by **interpolation**, which are hatched — interpolation fills gaps,
  it does not measure the seafloor;
- the value, count, standard deviation and interpolation state under the
  cursor.

**Open in 3D** (Ctrl+T) shows the grid as a surface: orbit, pan and zoom with
the mouse, top, front and side views, perspective or orthographic projection
and the vertical exaggeration in the tool options bar. The measure and
profile tools take two points and show the distance, the height difference,
the slope and the profile. Cells without data are holes, cells filled by
interpolation are stippled.

Several documents can be open at once. **View → Split** (Ctrl+\\) or the split
button of the tab bar shows a document beside the others, for example a grid
next to its 3D view or two grids side by side; each document keeps its own
settings. The open documents and their views are saved with the project.

**View → Processing history** lists every job with its commands, products and
log, so each result can be reproduced from the command line. **Run again**
repeats a job as a new attempt with its own folder and products;
**Change settings…** loads its parameters into its panel.

## 6. What needs updating

The project keeps track of what each survey line's processed file contains
and what each product was made from:

| State | Meaning | Next step |
|-------|---------|-----------|
| *unprocessed* | no edits or corrections set | process when ready |
| *processing required*, *edits pending*, *corrections pending* | cleaning, sounding edits or corrections that the processed file does not contain yet | **Process** |
| *processed* | the processed file is up to date | — |
| product *pending* | made from current data, but its lines wait for processing | process, then make it again |
| product *outdated* | its lines changed after it was made | **Make again** (product menu) |

The properties panel of the project and the tool options bar show the
recommended next step.

## 7. Maps with GMT

**Analysis → Map figure (GMT)…** (Ctrl+Shift+F) opens the figure panel:
a bathymetric map with contours, a survey overview with the survey lines, a
grid quality map (data count or standard deviation) or a perspective view,
with colour table, illumination, contours, colour scale, scale bar and title,
as PNG, PDF or PostScript. The figure opens in a **Figure** tab and is a
product of the project with its recipe: it can be made again with the current
data or with changed settings.

PNG and PDF figures need Ghostscript; MB-Studio looks for it on the `PATH`
and in the default installation folders and says so before the job starts if
it is missing (install it from <https://ghostscript.com>). PostScript needs no
Ghostscript. Each figure job runs in its own GMT session, so several figures
can be made at the same time.

## 8. The workbench

The window has an MB-Studio title bar with the menus. On Windows it behaves
like a system title bar (drag, double click, Snap, Alt+Space); set
`MBSTUDIO_NATIVE_TITLEBAR=1` to use the system title bar instead.
**Window** has workspace presets for the main tasks (Overview, Bathymetry
editing, Modelling, Cartography, Delivery), and saves or resets your own
layout of the panels.

## Without the user interface

The same project functions are commands of the `mbstudio` executable:

```
mbstudio --command project.create dir=D:\Surveys\EM122 name=EM122
mbstudio --command project.add project=D:\Surveys\EM122 path=D:\Data\PatchSurveyEM122
mbstudio --command project.grid project=D:\Surveys\EM122 spacing=25 name=bathymetry
mbstudio --command project.figure project=D:\Surveys\EM122 name=map formats=png|pdf contours=50
mbstudio --command project.retry project=D:\Surveys\EM122 sequence=3
mbstudio --command project.info project=D:\Surveys\EM122
```
