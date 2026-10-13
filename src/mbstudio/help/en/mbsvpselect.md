# mbsvpselect — Choose the best sound velocity profile for each swath file

`mbsvpselect` chooses, for every swath file in a survey, the most appropriate of a set of available sound velocity profiles (SVPs) and turns on bathymetry recalculation by raytracing through that profile. The user supplies a datalist of swath files, a list of SVP files and a selection method (nearest in position, nearest in time, or combinations). Use it when a survey contains many casts and assigning profiles to files by hand with `mbset` would be tedious.

## Synopsis

    mbsvpselect
        --check-zero-position {-N}
        --help {-H}
        --input=datalist {-Idatalist}
        --method=mode[/period_or_range[/seasonal]] {-Pmode[/period_or_range[/seasonal]]}
        --svplist=svplist {-Ssvplist}
        --verbose {-V}

## Description

The position and time of each swath file are read from its `.inf` file, so the ancillary `.inf`, `.fbt` and `.fnv` files must be created first (for example with `mbdatalist -N`). The mid-point position of each file is used for distance calculations (geodesics on the WGS84 ellipsoid) and its start time for time comparisons.

The SVP files are listed in an svplist, analogous to a datalist: one SVP file per line, and an svplist may reference other svplists. Each SVP file is a text file of `depth velocity` pairs (meters, meters/second) with a header line giving the time and location of the cast. Supported headers are the one written by `mbsvplist` (`## MB-SVP yyyy/mm/dd hh:mm:ss longitude latitude`) and the CARIS header (`Section yyyy-ddd hh:mm:ss latitude longitude`, positions as degree:minute:second).

Five selection methods are available through `-P`:
- `-P0` — nearest in position (default).
- `-P1` — nearest in time.
- `-P2` or `-P2/period` — nearest in position among the SVPs within the time period (hours, default 10) of the file; if none falls inside the period, the nearest in position is used.
- `-P3`, `-P3/range` or `-P3/range/0` — nearest in time among the SVPs within the range (meters, default 10000) of the file.
- `-P3/range/1` — nearest in month (seasonal, regardless of year) among the SVPs within the range.

For each swath file the program runs `mbset -I file -PSVPFILE:svpfile`, which sets the SVP file in the file's parameter (`.par`) file and enables raytracing. The pairs of swath file and selected SVP are also written to `result.txt` in the current directory. The bathymetry is only recalculated when `mbprocess` is run afterwards.

## Common options

- `-Idatalist` — Datalist of the swath files to process (long form `--input=datalist`). Default: `datalist.mb-1`.
- `-Ssvplist` — List of SVP files (long form `--svplist=svplist`). Default: `svplist.mb-1`.
- `-Pmode[/period_or_range[/seasonal]]` — SVP selection method as described above (long form `--method=...`). Times are in hours, ranges in meters. Default: `-P0`.
- `-N` — Check for swath files whose `.inf` positions are erroneously 0 longitude, 0 latitude and avoid using them for the distance calculation (long form `--check-zero-position`). SVP positions are not affected.
- `-V` — Verbose mode (long form `--verbose`): prints the file and SVP information and the computed distances or time differences.
- `-H` — Prints a short description and the usage, then exits (long form `--help`).

## Examples

A datalist `datalist.mb-1` lists the swath files, and `svplist.mb-1` lists the SVP files, one per line:

    svp_001.svp
    svp_002.svp
    svp_003.svp

Create the ancillary files first, then assign to each swath file the closest SVP collected within 50 hours, checking for zero positions:

    mbdatalist -I datalist.mb-1 -N
    mbsvpselect -N -V -I datalist.mb-1 -S svplist.mb-1 -P2/50

Assign the SVP nearest in time among those within 5 km of each file, then reprocess:

    mbsvpselect -I datalist.mb-1 -S svplist.mb-1 -P3/5000
    mbprocess -I datalist.mb-1

## Notes

- When the program finishes it prints "End the program press ENTER" and waits; press Enter to return to the prompt.
- Run the program in the directory that contains the datalist and the swath files; `result.txt` is written there. SVP file names are passed to `mbset` as written in the svplist, so relative names are resolved from the current directory.
- Avoid spaces in swath and SVP file names and paths: the `mbset` command line is built without quotes.
- The `.inf` files must be up to date; files without an `.inf` file are not found.

## See also

`mbset`, `mbprocess`, `mbsvplist`, `mbdatalist`, `mblevitus`. The complete reference is the MB-System manual page mbsvpselect(1) (English).
