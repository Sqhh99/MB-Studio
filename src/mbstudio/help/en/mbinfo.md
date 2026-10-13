# mbinfo — Report statistics and bounds of swath sonar data files

`mbinfo` reads one swath data file, or every file referenced by a datalist, and reports basic statistics: data counts, navigation totals, the first and last records, and the minimum and maximum values and geographic bounds. Run it early in a processing workflow to check what a file contains, and use it to create the ".inf" metadata files that other MB-System programs read.

## Synopsis

    mbinfo
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --comments {-C}
        --debug-record-types
        --debug-record-contents=identifier
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format_id { -Fformat_id}
        --good-nav {-G}
        --help {-H}
        --input=[file | datalist] {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --mask-dimensions=londim/latdim[/west/east/south/north] {-Mlondim/latdim[/west/east/south/north]}
        --notices {-N}
        --output-file {-O}
        --output-format=format_id {-Xformat_id}
        --ping-variances=number {-Ppings}
        --quick {-Q}
        --speed-minimum=speed {-Sspeed}
        --time-gap=timegap {-Ttimegap}
        --use-feet {-W}
        --verbose {-V}

## Description

The report lists the file name, the MBIO format id and a description of the format, any metadata inserted by `mbprocess` (sonar model, vessel name, bias parameters and so on), and counts of bathymetry, amplitude and sidescan values. For each data type it gives the number and percentage of good, zero and flagged values; "good" values are neither zero nor flagged as bad. It also reports the total time, track length and average speed, the time and position of the first and last records, and the minimum and maximum longitude, latitude, depth, altitude, amplitude and sidescan values.

An important use of `mbinfo` is to make ".inf" files. Programs such as `mbgrid` and `mbmosaic` read the ".inf" file of each swath file, when it exists, to skip files with no data in the area of interest. The `-O` option writes the report to a file named after the input file with ".inf" appended instead of to the screen. `mbdatalist -N` or `mbdatalist -O` creates ".inf" files for every file in a datalist.

The `-X` option changes the report format: `-X1` produces JSON and `-X2` produces XML. Combined with `-O`, the output files are named with an "_inf.json" or "_inf.xml" suffix.

If the input is a datalist (format -1, for example a file ending in ".mb-1"), the statistics are combined over all of the referenced files. If no format is given, `mbinfo` infers it from the file name suffix (for example ".mb58" means format 58).

`-P` turns on variance calculations: groups of pings are read and the variance of each beam (after detrending the bathymetry) and each pixel is reported, which is a crude measure of noise across the swath. `-M` adds a coarse coverage mask (a table of 0s and 1s) showing where data occur within the bounding box. Variance calculations are disabled when reading a datalist or when `-Q` is used.

## Common options

- `-Ifile` (`--input=file`) — Input swath data file or datalist. Default: stdin.
- `-Fformat_id` (`--format=format_id`) — MBIO format id of the input. A negative value (-1) means the input is a datalist. Default: inferred from the file name suffix.
- `-O` (`--output-file`) — Write the report to a file named after the input with ".inf" appended (".inf" becomes "_inf.json" or "_inf.xml" with `-X1` / `-X2`) instead of the screen.
- `-Xformat_id` (`--output-format=format_id`) — Report format: 0 = text (default), 1 = JSON, 2 = XML.
- `-N` (`--notices`) — Add notices: the data record types found, nonfatal read errors, and data problems such as zero navigation or excessive depths.
- `-G` (`--good-nav`) — Ignore zero positions and positions implying very large speeds when computing the longitude and latitude bounds. Useful for obtaining sensible plot bounds.
- `-Mlondim/latdim[/west/east/south/north]` (`--mask-dimensions=...`) — Append a coverage mask of londim x latdim bins. Without the bounds the data are read twice.
- `-Ppings` (`--ping-variances=number`) — Calculate beam and pixel variances using groups of this many pings. Default: 1 (no variances).
- `-Q` (`--quick`) — Read statistics from existing ".inf" files instead of the swath files. Variances, notices and masks are not available in this mode.
- `-C` (`--comments`) — Print the comment records found in the data.
- `-Rwest/east/south/north` (`--bounds=...`) — Use only data within these longitude and latitude bounds. Default: -360/360/-90/90.
- `-Byr/mo/da/hr/mn/sc`, `-Eyr/mo/da/hr/mn/sc` (`--begin-time=...`, `--end-time=...`) — Use only data between these times. If the begin time is after the end time, data between the two times are excluded instead.
- `-Llonflip` (`--longitude-domain=lonflip`) — Longitude range of the output: -1 = -360 to 0, 0 = -180 to 180, 1 = 0 to 360. Default: chosen from the first data.
- `-Sspeed` (`--speed-minimum=speed`) — Ignore pings with ship speed below this value (km/hr). Default: 0.
- `-Ttimegap` — Maximum time gap in minutes between pings before the data are considered to have a gap. Default: 1.
- `-W` (`--use-feet`) — Report depths in feet instead of meters.
- `-V` (`--verbose`) — Verbose output, including the program version and read error messages.
- `-H` (`--help`) — Print a short description and the usage, then exit.

## Examples

Report the statistics of a single Kongsberg file (the format is taken from the suffix):

    mbinfo -I 0001_20250101_000000.mb58

Write an ".inf" file next to the data file, including record type notices and a 10 x 10 coverage mask:

    mbinfo -I 0001_20250101_000000.mb58 -N -M10/10 -O

Report combined statistics for all files in a datalist, with sensible bounds:

    mbinfo -I datalist.mb-1 -G

Write the metadata of a file as JSON (creates "line1.mb88_inf.json"):

    mbinfo -I line1.mb88 -X1 -O

## Notes

- Quote paths that contain spaces, for example `-I "D:\Survey 2025\line1.mb58"`.
- With `-O` the ".inf" file is written next to the input file (the input path with a suffix appended), not in the current directory.
- Use the short form `-T` to set the time gap; the long form `--time-gap` is not recognised correctly by this version.
- `--debug-record-types` and `--debug-record-contents=identifier` are developer options for listing record types and dumping record contents.

## See also

`mbdatalist`, `mbformat`, `mblist`, `mbprocess`, `mbgrid`, `mbmosaic`, `mbset`. The complete reference is the MB-System manual page mbinfo(1) (English).
