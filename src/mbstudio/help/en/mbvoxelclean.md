# mbvoxelclean — 3D voxel density filter to flag and unflag soundings

`mbvoxelclean` cleans swath bathymetry with a three-dimensional density filter: soundings that fall in sparsely populated voxels are treated as noise and flagged, while dense voxels are treated as real targets such as the seafloor. It is intended mainly for very dense point clouds such as subsea lidar, and is used in the editing stage before `mbprocess`.

## Synopsis

    mbvoxelclean [--verbose --help --input=datalist --format=value
        --voxel-size=xysize[/zsize] --occupy-threshold=value
        --count-flagged --flag-empty --ignore-empty
        --unflag-occupied --ignore-occupied --neighborhood=value
        --range-minimum=value --range-maximum=value
        --acrosstrack-minimum=value --acrosstrack-maximum=value
        --amplitude-minimum=value --amplitude-maximum=value]

## Description

The input is one swath file or a datalist referencing many swath files; each file is read and filtered separately. All soundings of a file are loaded into memory and assigned to cubic (or rectangular) voxels of the requested size. A voxel holding at least the threshold number of soundings is "occupied" (a valid target); a voxel with fewer soundings is "empty".

Two actions can then be applied: previously unflagged soundings in empty voxels are flagged as bad (the default, `--flag-empty`), and previously flagged soundings in occupied voxels can be unflagged (`--unflag-occupied`, off by default). The `--neighborhood` option grows the occupied region by a number of voxels, which relaxes the flagging close to dense regions.

While the data are read, optional simple filters on range from the sensor, acrosstrack distance and beam amplitude are also applied. These filters act before the density filter, so `--unflag-occupied` can undo flags they set.

The program does not modify the swath files. Every change of a beam flag is written to the edit save file of the swath file (`<file>.esf`), which is created or appended to, and the `mbprocess` parameter file (`<file>.par`) is updated so that `mbprocess` applies the edits. These are the same edit save files used by `mbclean` and the interactive editors. Where an `.fbt` (fast bathymetry) file exists it is read instead of the full file. When lock files are in use, files locked by another program are skipped.

## Common options

- `--input=datalist` — swath file or datalist to process. Default: `datalist.mb-1`.
- `--format=value` — MBIO format id of the input; a negative value means the input is a datalist. Normally inferred from the file name.
- `--voxel-size=xysize[/zsize]` — horizontal voxel size and optional vertical size in metres; if `zsize` is omitted it equals `xysize`. Default: 0.05 m.
- `--occupy-threshold=value` — minimum number of soundings for a voxel to be occupied. Default: 5.
- `--count-flagged` — also count previously flagged soundings when deciding whether a voxel is occupied (normally only unflagged soundings are counted).
- `--flag-empty` — flag unflagged soundings in empty voxels (default).
- `--ignore-empty` — leave soundings in empty voxels unchanged.
- `--unflag-occupied` — unflag flagged soundings in occupied voxels.
- `--ignore-occupied` — leave flagged soundings in occupied voxels flagged (default).
- `--neighborhood=value` — treat voxels within `value` voxels (in x, y and z) of an occupied voxel as occupied too. Default: 0.
- `--range-minimum=value`, `--range-maximum=value` — flag unflagged soundings closer to or farther from the sensor than this range (m).
- `--acrosstrack-minimum=value`, `--acrosstrack-maximum=value` — flag unflagged soundings farther to port than the minimum or farther to starboard than the maximum acrosstrack distance (m).
- `--amplitude-minimum=value`, `--amplitude-maximum=value` — flag unflagged soundings whose beam amplitude is below the minimum or above the maximum.
- `--verbose` — report per-file and total counts of flagged and unflagged beams.
- `--help` — print a short help message and exit.

## Examples

Flag soundings in 4 cm voxels holding fewer than 5 soundings, for all files of a datalist, with progress output:

    mbvoxelclean --input=datalist.mb-1 --voxel-size=0.04/0.04 --occupy-threshold=5 --flag-empty --verbose

Use 10 cm voxels, also restore previously flagged soundings that lie in dense regions, and flag everything farther than 30 m from the sensor:

    mbvoxelclean --input=datalist.mb-1 --voxel-size=0.1 --occupy-threshold=8 --unflag-occupied --range-maximum=30

Apply the edits to the data afterwards:

    mbprocess -Idatalist.mb-1

## Notes

- Only long options (`--name=value`) are accepted; there are no single-letter forms.
- All soundings of a file are held in memory, so very large files with very small voxels need a lot of RAM.
- The edit save files are written next to the swath files, not to the current directory. File names inside a datalist must not contain spaces.
- For data formats that do not support beam flags, `mbprocess` nulls (zeroes) the edited soundings instead of flagging them; the program prints a warning in that case.

## See also

`mbclean`, `mbareaclean`, `mbprocess`, `mbset`, `mbinfo`, `mbdatalist`. The complete reference is the MB-System manual page mbvoxelclean(1) (English).
