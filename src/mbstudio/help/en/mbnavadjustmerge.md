# mbnavadjustmerge — Create, edit and merge MBnavadjust navigation projects

`mbnavadjustmerge` is the command line interface to MBnavadjust projects, which adjust swath navigation by matching bathymetric features where survey lines overlap or cross. It can create a project, import swath data, find crossings, automatically pick navigation ties, invert for corrected navigation, apply that navigation to the swath files, and edit or merge projects. It makes the navigation adjustment workflow scriptable, and on Windows it is the way to work with MBnavadjust projects, because the interactive `mbnavadjust` program is not available.

## Synopsis

    mbnavadjustmerge --input=project_path [--input=project_path] [--output=project_path]
    mbnavadjustmerge --create-project=project_path [settings] [commands]
    mbnavadjustmerge --input=project_path [settings] [commands] [--verbose] [--help]

    settings:  --section-length=km --section-soundings=count --contour-interval=meters
               --color-interval=meters --tick-interval=meters --label-interval=meters
               --decimation=value --smoothing=value --zoffsetwidth=meters
    commands:  --import=path[:format] --import-as-survey=path[:format] --find-crossings
               --autopick --autopick-horizontal
               --autopick-crossing-type=all|mediocre|good|better|true
               --autopick-scope=all|survey|withsurvey|block|file|withfile|withsection
               --autopick-survey=survey --autopick-survey2=survey --autopick-file=file
               --autopick-section=section --autopick-overlap-threshold=percent
               --invert-navigation --update-grids --apply-navigation
               --set-global-tie=file:section[:snav]/xoffset/yoffset/zoffset[/xsigma/ysigma/zsigma]
               --set-tie=file1:section1/file2:section2/xoffset/yoffset/zoffset[/xsigma/ysigma/zsigma]
               --set-ties-xyz-all --set-ties-xyonly-all --set-ties-zonly-all
               --unset-tie=file1:section1/file2:section2 --skip-unset-crossings
               --update-file=file --update-all-files
               --import-tie-list=file --export-tie-list=file
               --triangulate --triangulate-all ...

## Description

A project is given by the path of its `.nvh` file or its `.dir` directory. `--create-project` makes a new, empty project (it cannot be combined with `--input`); `--input` opens an existing one. The settings options (section length and soundings, contour display, smoothing, z-offset search width) can be given when creating a project or later, and are always applied before any other command.

`--import` adds swath data: either a datalist (no format given) or a single file with its format id (`file:format`). Each file is split into sections according to the current section settings, and crossings with previously imported data are found automatically. Files that continue in time belong to the same survey; a time gap starts a new survey unless `--import-as-survey` is used.

`--autopick` computes a 3D (east, north, vertical) bathymetric misfit grid at each unanalyzed crossing and sets a tie at the minimum misfit when it is well defined; `--autopick-horizontal` uses only the horizontal offset. The autopick filters restrict which crossings are considered (default: all crossings with at least 10% overlap). `--invert-navigation` solves the network of ties and global ties for a corrected navigation model inside the project; `--apply-navigation` then writes the solution into the swath files' processing parameters, so that the next `mbprocess` run applies it. `--update-grids` regenerates the project's reference bathymetry grids from the current navigation.

With two `--input` projects, the second project is added to the first; with `--output`, both are copied into a new combined project and the ties already made are kept. When several commands are given, they are executed in the order they appear on the command line. Files, sections and surveys are identified by their numeric ids in the project (for example `3:0` is file 3, section 0).

## Common options

- `--input=project_path` — Existing project (`.nvh` file or `.dir` directory). Give it twice to merge two projects.
- `--output=project_path` — New project receiving the merged input projects. Without it, the second input project is added to the first.
- `--create-project=project_path` — Create a new, empty project; other commands on the same line are applied to it.
- `--section-length=km` — Maximum section length in kilometers for files imported afterwards.
- `--section-soundings=count` — Maximum number of soundings in a section for files imported afterwards.
- `--smoothing=value` — Smoothing weight of the navigation inversion (default 3.0; useful range about 0.1 to 10).
- `--zoffsetwidth=meters` — Width of the vertical offset search range used for misfit grids and autopicking.
- `--import=path[:format]` — Import a datalist, or a single swath file when its format id is appended. May be repeated.
- `--import-as-survey=path[:format]` — As `--import`, but all imported files form one new survey regardless of time gaps.
- `--find-crossings` — Search for new crossings and recompute overlaps (useful after merging projects or surveys, or after changing discontinuities).
- `--autopick` — Automatically pick ties (3D offsets) at unanalyzed crossings; `--autopick-horizontal` picks horizontal offsets only.
- `--autopick-crossing-type=all|mediocre|good|better|true` — Only consider crossings with at least 10%, 25% or 50% overlap, or only true crossings of the tracks.
- `--autopick-scope=all|survey|withsurvey|block|file|withfile|withsection` — Restrict autopicking to a survey, a pair of surveys (`block`), a file or a section, selected with `--autopick-survey`, `--autopick-survey2`, `--autopick-file` and `--autopick-section`.
- `--autopick-overlap-threshold=percent` — Minimum overlap of a crossing to be autopicked (default 10).
- `--invert-navigation` — Invert ties and global ties for a corrected navigation model.
- `--apply-navigation` — Write the navigation solution to the swath files' processing parameters (run after `--invert-navigation`, then run `mbprocess`).
- `--update-grids` — Regenerate the project's reference bathymetry grids from the current navigation.
- `--set-global-tie=file:section[:snav]/xoffset/yoffset/zoffset[/xsigma/ysigma/zsigma]` — Tie a navigation point to the absolute (e.g. GPS) reference frame; offsets in meters.
- `--set-tie=file1:section1/file2:section2/xoffset/yoffset/zoffset[/xsigma/ysigma/zsigma]` — Add or replace a tie between two sections (the crossing is created if needed).
- `--set-ties-xyz-all`, `--set-ties-xyonly-all`, `--set-ties-zonly-all` — Set the mode of all ties (variants exist per file, survey or block, e.g. `--set-ties-zonly-by-block=survey1/survey2`).
- `--unset-tie=file1:section1/file2:section2` — Delete one tie (also `--unset-ties-with-file=file`, `--unset-ties-by-block=survey1/survey2`).
- `--skip-unset-crossings` — Mark all unanalyzed crossings as skipped.
- `--update-file=file`, `--update-all-files` — Refresh the beam flags stored in the project from the current processed swath files (after re-editing).
- `--export-tie-list=file`, `--import-tie-list=file` — Write the ties to a text file, or import ties exported from another project.
- `--triangulate` — Pre-generate the section triangle meshes used for contouring (`--triangulate-all` recreates all of them).
- `--verbose` — Print more information; `--help` prints the full option list.

## Examples

Create a project, set 0.1 km sections and import the survey datalist in one step:

    mbnavadjustmerge --create-project=Survey2025.nvh --section-length=0.1 --import=datalist.mb-1

Autopick ties at crossings with at least 25% overlap and export the ties for review:

    mbnavadjustmerge --input=Survey2025.nvh --autopick --autopick-crossing-type=good --export-tie-list=ties_autopick.txt

Invert for the corrected navigation, apply it to the swath files, then reprocess:

    mbnavadjustmerge --input=Survey2025.nvh --invert-navigation --apply-navigation
    mbprocess -I datalist.mb-1

Merge two existing projects into a new one:

    mbnavadjustmerge --input=Navadjust20140515m1.nvh --input=Navadjust20140515m2.nvh --output=Navadjust20140515All.nvh

## Notes

- Unsupervised autopicks are frequently wrong. Review them (for example with `--export-tie-list`) and delete or correct bad ties before relying on the inversion.
- For a single swath file, `--import` needs the format id (e.g. `--import=0001_20250101_000000.all:58`); the file name suffix is not used to guess it.
- `--update-grids` and project merging run helper commands (`mbdatalist`, `mbprocess`, `mbgrid` and file utilities) through the system shell; on Windows these steps may not complete. If the reference grids are not updated, make them with `mbgrid`.
- In this build `--unset-ties-all` and its alias `--unset-all-ties` expect a value; write `--unset-ties-all=1` (the value is ignored).
- Quote project and data paths that contain spaces.

## See also

`mbprocess`, `mbset`, `mbdatalist`, `mbgrid`, `mbnavlist`, `mbnavadjust` (interactive, not available on Windows). The complete reference is the MB-System manual page mbnavadjustmerge(1) (English).
