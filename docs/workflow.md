# Processing a survey with MB-Studio

This guide takes a multibeam survey (for example the Kongsberg EM122 lines of
*PatchSurveyEM122*) from the raw files to a terrain model in MB-Studio 0.2.
Every step runs the MB-System programs; MB-Studio writes the datalists,
chooses the file names, runs the programs in the background and records what
it did.

## 1. Create a project

**File → New project…** (Ctrl+N). Choose a name and the folder the project is
created in, and optionally a folder with survey data. The project folder
contains:

| Path | Contents |
|------|----------|
| `<name>.mbsproj` | the project: lines, products, layers, open views, history |
| `datalists/` | `raw.mb-1`, `work.mb-1` and `workp.mb-1` (`$PROCESSED`) for use with the command line |
| `processed/` | files written by preprocessing |
| `products/grids`, `products/mosaics` | grids and mosaics with their quality grids |
| `exports/` | exported files |
| `jobs/<n>/` | the datalists, files and `job.log` of each job |

The survey files stay where they are. MB-System datalists cannot hold paths
with spaces, so the path from the project to the data must not contain spaces
(the project folder itself may).

## 2. Add the survey lines

**File → Add survey data…** (Ctrl+I), the **+** buttons of the project panel,
or drag files or folders onto the window. Folders are searched recursively;
MB-Studio identifies the MB-System format of each file and skips cache and
product files. A background job then creates the `.inf`, `.fbt` and `.fnv`
files (`mbdatalist -O`) and reads the statistics of every line. The lines
appear on the **Map** as soon as their navigation is known.

Select lines in the project panel or on the map (click; Ctrl+click adds,
Shift+click extends). Processing steps work on the selected lines, or on all
lines when nothing is selected.

## 3. Process

The toolbar and the **Process** menu open the processing panels on the right.
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
3. **Corrections** — sound velocity (a profile file or the Levitus
   climatology), roll and pitch bias, draft and tide (`mbset`, `mblevitus`,
   `mbotps`).
4. **Process** — applies edits and corrections (`mbprocess`) and writes the
   processed files (`...p.mb59`).
5. **Grid** — `mbgrid` with cell size, algorithm (weighted mean, median,
   minimum/maximum, beam footprint), gap interpolation and coordinates
   (geographic or UTM). The panel estimates the grid size and memory. The
   quality grids (data count and standard deviation) are written by default.
   Gridding always uses the processed files where they exist.
6. **Mosaic** — amplitude or sidescan mosaics (`mbmosaic`), optionally after
   a grazing angle correction (`mbbackangle`, `mbprocess`).
7. **Export** — a grid as GeoTIFF, ESRI ASCII grid, XYZ or glTF 3D model, or
   the soundings as a LAS point cloud or XYZ text.

## 4. Check the result

Grids are added to the project and shown on the map (eye button in the
product list; opacity, colour table and shading in the properties panel).
Double-click a grid to open it in the **Grid** view, which shows:

- the share of **empty cells**;
- with the quality grids, the **data count** and **standard deviation** per
  cell (the *Show* selector), and the share of cells that were filled by
  **interpolation**, which are hatched on the surface — interpolation fills
  gaps, it does not measure the seafloor;
- the value, count, standard deviation and interpolation state under the
  cursor.

**View → Processing history** lists every job with its commands, products and
log, so each result can be reproduced from the command line.

## Without the user interface

The same project functions are commands of the `mbstudio` executable:

```
mbstudio --command project.create dir=D:\Surveys\EM122 name=EM122
mbstudio --command project.add project=D:\Surveys\EM122 path=D:\Data\PatchSurveyEM122
mbstudio --command project.grid project=D:\Surveys\EM122 spacing=25 name=bathymetry
mbstudio --command project.info project=D:\Surveys\EM122
```
