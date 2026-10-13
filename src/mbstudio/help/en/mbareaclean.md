# mbareaclean — Flag or unflag soundings by statistical tests in area bins

`mbareaclean` cleans swath bathymetry over an area rather than ping by ping. It divides the area into square bins, collects the soundings of all input files that fall into each bin, and applies statistical tests (median filter or standard deviation filter) to them. Soundings that fail are flagged as bad, and optionally flagged soundings that pass are restored. Use it after `mbclean`, especially where overlapping lines cover the same seafloor.

## Synopsis

    mbareaclean
        --beam-range=[-]minbeam[/maxbeam] {-N[-]minbeam[/maxbeam]}
        --bin-size=binsize {-Sbinsize}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --detection-type=type {-Ttype}
        --flag-bad {-B}
        --format=format {-Fformat}
        --help {-H}
        --input=infile {-Iinfile}
        --median-filter=threshold[/nmin[/nmax]] {-Mthreshold[/nmin[/nmax]]}
        --plane-fit=arg {-Parg}
        --std-dev-filter=threshold[/nmin] {-Dthreshold[/nmin]}
        --unflag-good {-G}
        --verbose {-V}

## Description

The input is one swath file or a datalist (default `datalist.mb-1`). The work area is set with `-R` and the bin size in meters with `-S`. When `-R` is not given, the area is taken from the `.inf` files of the input data and, unless `-S` is given, the bin size is 0.2 times the maximum sonar altitude. Existing edits are read from each file's edit save file before the tests, so previously flagged soundings are taken into account.

Two tests are available. The median filter (`-M`) marks as bad any sounding that differs from the median depth of its bin by more than threshold times the sonar altitude; with `threshold` = 0.05 a sounding more than 5 % above or below the median fails. If a bin holds more than nmax good soundings, only the nmax closest to the median remain unflagged. The standard deviation filter (`-D`) marks soundings that differ from the bin mean by more than threshold standard deviations. Each test is only applied to bins with at least nmin soundings. If neither `-M`, `-D` nor `-P` is given, the median filter is used.

With `-B` (the default) good soundings that fail are flagged; with `-G` flagged soundings that pass all tests are unflagged; both can be given together. `-N` and `-T` restrict which soundings may be changed, by beam number and by bottom detection type, while all good soundings are still used to compute the statistics. For example `-N-30 -T1 -D1` flags amplitude detections in the outer 30 beams on each side that lie more than one standard deviation from the mean.

The changes are written as edit events to the edit save file of each swath file (the swath file name with `.esf` appended), creating it if needed, and the `mbprocess` parameter files are updated. Run `mbprocess` to apply the edits.

## Common options

- `-Iinfile` — Input swath file or datalist. Default: `datalist.mb-1`.
- `-Rwest/east/south/north` — Area to bin and test; soundings outside are ignored. Default: the extent of all input data (from the `.inf` files).
- `-Sbinsize` — Bin size in meters. Default: 0.2 times the maximum sonar altitude (only when `-R` is not given).
- `-Mthreshold/nmin/nmax` — Median filter: threshold as a fraction of the sonar altitude, minimum number of soundings per bin, maximum number of good soundings per bin. Defaults: `0.25`, `10`, no maximum.
- `-Dthreshold/nmin` — Standard deviation filter: threshold in standard deviations and minimum number of soundings per bin. Defaults: `2.0`, `10`.
- `-B` — Flag good soundings that fail a test (default action).
- `-G` — Unflag flagged soundings that pass all tests.
- `-Nminbeam/maxbeam` — Only change soundings of beams minbeam to maxbeam; with a leading `-`, only beams outside that range. If maxbeam is omitted it is set symmetrically, e.g. `-N-30` affects the 29 outermost beams on each side.
- `-Ttype` — Only change soundings with this bottom detection type: `0` unknown, `1` amplitude, `2` phase, `3` lidar, `4` photogrammetry.
- `-Parg` — Plane fit test (argument required but not used); given alone, no median or standard deviation filter is applied.
- `-Fformat` — MBIO format id; a negative value means a datalist. Default: inferred from the file name suffix.
- `-V` — Verbose output, including counts of flagged and unflagged soundings per file.
- `-H` — Print help and exit.

## Examples

Median filter with 2.5 m bins and a 10 % threshold over a known area:

    mbareaclean -I datalist.mb-1 -B -M0.1/10 -S2.5 -R-122.42556/-122.41974/47.67111/47.67529 -V

Flag soundings more than two standard deviations from the mean in 5 m bins over the whole dataset:

    mbareaclean -I datalist.mb-1 -S5 -D2/10

Flag amplitude detections of the outer 30 beams that lie more than one standard deviation from the mean, then apply the edits:

    mbareaclean -I datalist.mb-1 -S5 -N-30 -T1 -D1
    mbprocess -I datalist.mb-1

## Notes

- Without `-R`, the bounds come from the `.inf` files; create them first with `mbdatalist -I datalist.mb-1 -O` if they are missing (`mbpreprocess` creates them too).
- When `-R` is given, also give `-S`; otherwise the area is simply divided into about 100 bins east-west.
- Use the upper-case `-S`; the lower-case `-s` form does not accept a value.
- The number of bins grows quickly with a large area and a small bin size; split very large surveys into several areas.
- Quote file names that contain spaces.

## See also

`mbclean`, `mbvoxelclean`, `mbprocess`, `mbdumpesf`, `mbdatalist`, `mbinfo`. The complete reference is the MB-System manual page mbareaclean(1) (English).
