# mblist — List selected values from swath data as text or binary tables

`mblist` reads swath data files and writes a table of selected values (time, position, depth, attitude, amplitude, sidescan and many more) to the screen or to a file. Use it to extract centre-beam profiles, navigation, or "longitude latitude depth" soundings for other software, and to inspect individual beams and pixels during processing.

## Synopsis

    mblist
        --beam-range=beam_start/beam_end|A|Xexcludepercent {-Mbeam_start/beam_end}
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --binary {-A}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --check-mode=check {-Ucheck}
        --decimate=decimate {-Kdecimate}
        --delimiter=delimiter {-Gdelimiter}
        --dump-mode=dumpmode {-Ddumpmode}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --netcdf {-C}
        --no-value-check {-Q}
        --output-file=outfile {-Xoutfile}
        --output-format=output_format {-Ooutput_format}
        --ping-average=pings {-Ppings}
        --pixel-range=pixel_start/pixel_end|A {-Npixel_start/pixel_end}
        --projection=projection {-Jprojection}
        --secondary-file=secondaryfile {-Ysecondaryfile}
        --segment=segment {-Zsegment}
        --speed-minimum=speed {-Sspeed}
        --time-gap=timegap {-Ttimegap}
        --use-feet {-W}
        --verbose {-V}

## Description

By default `mblist` writes one line per ping with tab-separated columns. The columns are chosen with `-O`, a string in which each character selects one value; the default is `TXYHSLZ` (time string, longitude, latitude, heading, speed, cumulative along-track distance, topography). Other delimiters can be set with `-G`, binary output (double precision) with `-A`, and netCDF CDL output with `-C`.

In the default mode the depth, amplitude and sidescan values come from the beam and pixel closest to vertical below the sonar, and positions are those of the sonar. When `-M` or `-N` selects a range of beams or pixels, one line is written for each selected beam or pixel and positions are those of the beams or pixels on the seafloor. `-D` is a shortcut for complete "longitude latitude value" dumps of all beams or pixels, which are convenient input for gridding programs.

Flagged and null values are left out by default. `-Q` or `-U` changes this so that flagged values, and optionally null values, are also written.

Frequently used `-O` characters: `T` time yyyy/mm/dd/hh/mm/ss, `t` time yyyy mm dd hh mm ss, `M` Unix time in decimal seconds, `X` longitude, `Y` latitude, `Z` topography (positive up), `z` depth (positive down), `H` heading, `S` speed (km/hr), `L` along-track distance (km), `l` along-track distance (m), `D` across-track distance (m), `E` along-track distance of the beam (m), `B` amplitude, `b` sidescan, `C` sonar altitude, `c` sonar depth, `R` roll, `P` pitch, `r` heave, `F` numeric beam flag, `f` beam flag character, `#` beam or pixel number, `N` ping count. Prefix characters modify the next value: `-` changes the sign, `/` inverts it, `^` (before `X` or `Y`) gives easting or northing in the projection set with `-J` (UTM by default), `=` and `+` take the value from the port-most or starboard-most valid beam. Further codes starting with `,` (travel times, angles, beam range) and `.` (format-specific raw values, file name, secondary file columns) are described in the manual page. The special string `%fnv` selects the columns used for ".fnv" fast navigation files (`tMXYHScRPr=X=Y+X+Y`).

## Common options

- `-Ifile` (`--input=file`) — Input swath file or datalist. Default: datalist.mb-1.
- `-Fformat` (`--format=format`) — MBIO format id; -1 for a datalist. Default: inferred from the file name suffix.
- `-Ooutput_format` (`--output-format=output_format`) — Characters selecting the output columns (see above). Default: `TXYHSLZ`.
- `-Mbeam_start/beam_end` (`--beam-range=...`) — Output records for this range of beams (numbered from 0 on the port side). `-MA` outputs all beams; `-MXpercent` outputs all beams except the outer percent.
- `-Npixel_start/pixel_end` (`--pixel-range=...`) — Output records for this range of sidescan pixels; `-NA` outputs all pixels.
- `-Ddumpmode` (`--dump-mode=dumpmode`) — Shortcut dumps: 1 = format from `-O` (default), 2 = lon lat depth, 3 = lon lat topography, 4 = lon lat amplitude, 5 = lon lat sidescan. Overrides `-O`, `-M` and `-N`.
- `-Ucheck` (`--check-mode=check`) — Handling of flagged and null values: 0 = omit them (default), 1 = output flagged values, skip nulls, 2 = output flagged values and nulls as zero, 3 = output flagged values and nulls as NaN, 4 = output flagged and null values as NaN.
- `-Q` (`--no-value-check`) — Output all values, flagged values unchanged and nulls as zero (same as `-U2`).
- `-Xoutfile` (`--output-file=outfile`) — Write to this file instead of the screen.
- `-Gdelimiter` (`--delimiter=delimiter`) — Column separator for text output. Default: tab.
- `-A` (`--binary`) — Write binary double precision values instead of text.
- `-Jprojection` (`--projection=projection`) — Projected coordinate system for `^X` / `^Y`, for example `UTM12N`, `epsg32612`, or `U` for UTM with the zone taken from the first data.
- `-Kdecimate` (`--decimate=decimate`) — Output only every decimate-th record. Default: 1.
- `-Ppings` (`--ping-average=pings`) — Average this many pings into one. Default: 1.
- `-Rwest/east/south/north` (`--bounds=...`) — Use only data within these bounds.
- `-Byr/mo/da/hr/mn/sc`, `-Eyr/mo/da/hr/mn/sc` (`--begin-time=...`, `--end-time=...`) — Use only data between these times.
- `-Zsegment` (`--segment=segment`) — Separate the text output of each input file with a line containing segment (`>` for GMT multisegment files). `swathfile` or `datalist` writes "#" followed by the source file or datalist path.
- `-Ysecondaryfile` (`--secondary-file=secondaryfile`) — Text file whose first column is Unix time and up to 19 further columns are data; values are interpolated to the ping times and printed with `.1C` ... `.19C` codes in `-O`.
- `-W` (`--use-feet`) — Output depths and distances in feet.
- `-C` (`--netcdf`) — Output netCDF CDL; with `-A`, convert to a binary netCDF file using `ncgen`.
- `-V` (`--verbose`), `-H` (`--help`) — Verbose messages; print usage and exit.

## Examples

Write time, position and depth of the centre beam of every ping to a text file:

    mblist -I 0001_20250101_000000.mb58 -OtXYz -X centre.txt

Dump longitude, latitude and depth of all valid soundings in a datalist:

    mblist -I datalist.mb-1 -D2 -X soundings.xyz

List beam number, beam flag character and depth for every beam of one file, including flagged beams:

    mblist -I line1.mb88 -MA -O#fz -U1 -X beams.txt

Output UTM easting, northing and depth with commas as separators (the format string is quoted because `^` is special in cmd.exe):

    mblist -I datalist.mb-1 -MA "-O^X^Yz" -JU -G, -X soundings_utm.csv

## Notes

- In cmd.exe the characters `^`, `>`, `|`, `&` and `%` have special meanings. Put arguments containing them in double quotes, for example `"-O^X^Yz"` or `"-Z>"`. In a batch file write `%%fnv` instead of `%fnv`.
- Without `-X` the output goes to the screen; use `-X` or redirect with `> file.txt` to save it.
- Binary netCDF output (`-C -A`) needs the netCDF `ncgen` program on the PATH; the result is written to the file given with `-X` (mblist.nc if none is given).

## See also

`mbinfo`, `mbnavlist`, `mbdatalist`, `mbformat`, `mbprocess`, `mbgrid`. The complete reference is the MB-System manual page mblist(1) (English).
