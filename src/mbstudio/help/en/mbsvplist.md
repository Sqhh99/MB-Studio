# mbsvplist — List sound velocity profiles embedded in swath data files

`mbsvplist` extracts the water sound velocity profiles (SVPs) recorded inside swath sonar data files. Use it after preprocessing to see which SVPs a survey contains, to save them as `.svp` files, or to make the first SVP of each file the profile that `mbprocess` uses when it recalculates bathymetry. It can also list the surface sound velocity (SSV) values the sonar used for beamforming.

## Synopsis

    mbsvplist
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --counts {-C}
        --duplicates {-D}
        --format=format_id {-Fformat_id}
        --help {-H}
        --input=file {-Ifile}
        --min-num-pairs=min_num_pairs {-Nmin_num_pairs}
        --mode=mode {-Mmode}
        --output {-O}
        --process {-P}
        --source=source {-Asource}
        --ssv {-S}
        --table {-T}
        --verbose {-V}
        --zero-depth {-Z}

## Description

Swath bathymetry is calculated from raw beam angles and travel times by raytracing through a model of the speed of sound in water. Many data formats store SVPs in the data stream, often including the profiles used by the acquisition system. `mbsvplist` reads one swath file or every file referenced by a datalist (default input `datalist.mb-1`) and reports those profiles.

By default the SVPs are written as text to standard output. Only the first SVP of each file and any SVP that differs from the previous one are listed; duplicates are skipped (see `-M` and `-D`). With `-O` each SVP is instead written to its own file named `<swathfile>_NNN.svp` (NNN is the SVP number within that file, starting at `000`). These files begin with a `## MB-SVP` style header giving the time and position of the profile, which `mbsvpselect` uses to choose the best SVP for each swath file.

`-P` does the same as `-O` and also updates the `mbprocess` parameter file (`<swathfile>.par`) so that the first SVP written for each file is used when `mbprocess` recalculates the bathymetry.

`-T` prints a compact CSV table (SVP number, time, longitude, latitude, number of depth-velocity pairs) instead of full profiles, and `-C` prints only the number of unique SVPs in each file. `-S` lists the surface sound velocity used for beamforming, paired with the sonar depth, normally one value per ping; `-R` limits that SSV listing to a longitude/latitude box.

## Common options

- `-Ifile` — Input swath file or datalist. Default: `datalist.mb-1`.
- `-Fformat_id` — MBIO format id of the input. Normally inferred from the file name suffix; a negative value means the input is a datalist.
- `-O` — Write each SVP to a separate `<swathfile>_NNN.svp` file instead of standard output.
- `-P` — Like `-O`, and also set the first SVP of each file in its parameter file for use by `mbprocess`.
- `-Mmode` — SVP output mode: `0` (default) first SVP of each file plus every SVP that differs from the previous one; `1` only the first instance of each unique SVP, even across files of a datalist; `2` all SVPs including duplicates.
- `-D` — Output duplicate SVPs (same as `-M2`).
- `-Asource` — Record type used as the SVP source: `S` for sound velocity profile records (default), `C` for CTD records.
- `-Nmin_num_pairs` — Ignore SVPs with fewer than this many depth-velocity pairs (useful when a single sound speed is stored as a one-point "SVP"). Default: `0`.
- `-Z` — Replace the first depth of each SVP with zero before output (fixes profiles that do not start at the surface, which cause problems in `mbprocess`).
- `-T` — Output a CSV table summarising each SVP instead of the full profiles.
- `-C` — Output only the number of unique SVPs in each file.
- `-S` — Output surface sound velocity (SSV) and sonar depth for each ping instead of SVPs.
- `-Rwest/east/south/north` — With `-S`, output SSV values only inside this area. Default: all.
- `-V` — Verbose output (progress and record counts on the error stream).
- `-H` — Print help and exit.

## Examples

List the SVPs found in all files of the datalist on the screen:

    mbsvplist -I datalist.mb-1 -V

Write the SVPs to `.svp` files and set the first one of each file for bathymetry recalculation, then process:

    mbsvplist -I datalist.mb-1 -P -V
    mbprocess -I datalist.mb-1

Save a summary table of all SVPs (time, position, number of points) to a CSV file:

    mbsvplist -I datalist.mb-1 -T > svp_table.csv

List the surface sound velocity of one Kongsberg file inside an area:

    mbsvplist -I survey\0001_20250101_000000.all -S -R139.5/139.8/35.2/35.5 > ssv.txt

## Notes

- The `.svp` files from `-O` and `-P` are written next to the swath data files (the path is taken from the datalist), not in the current directory.
- `-P` modifies the `.par` files; run `mbprocess` afterwards to apply the new SVP.
- `-S` is an alternative output mode; do not combine it with `-O`, `-P`, `-C` or `-T`.
- Quote file names that contain spaces, e.g. `-I "my survey\datalist.mb-1"`.

## See also

`mbsvpselect`, `mbprocess`, `mbset`, `mblevitus`, `mbctdlist`, `mbinfo`. The complete reference is the MB-System manual page mbsvplist(1) (English).
