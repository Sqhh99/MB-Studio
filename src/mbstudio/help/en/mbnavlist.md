# mbnavlist — List navigation and attitude from swath data files

`mbnavlist` reads swath data files and writes a table of navigation values (time, position, heading, speed, sonar depth, roll, pitch, heave and so on) to the screen. Use it to extract ship or vehicle tracks for plotting, to check navigation quality, or to list the asynchronous navigation or attitude records stored in a data file.

## Synopsis

    mbnavlist
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --binary {-A}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --data-kind=kind {-Kkind}
        --decimate=decimate {-Ddecimate}
        --delimiter=delimiter {-Gdelimiter}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --nav-channel=navchannel {-Nnavchannel}
        --output-format=output_format {-Ooutput_format}
        --projection=projection {-Jprojection}
        --segment=segment {-Zsegment}
        --speed-minimum=speed {-Sspeed}
        --time-gap=timegap {-Ttimegap}
        --verbose {-V}

## Description

Output is tab-separated text by default; `-G` sets another delimiter, `-A` writes binary double precision values, and `-D` decimates the output. The columns are selected with `-O`; the default is `tMXYHs` (time string, Unix time, longitude, latitude, heading, speed made good).

By default navigation is taken from the record type that is the "navigation source" of the data format. Many formats also store asynchronous navigation in separate records, sometimes from several navigation systems. `-N0` lists the primary asynchronous navigation and `-N1`, `-N2` or `-N3` list auxiliary navigation channels. If the requested records are not in the file, nothing is written; run `mbinfo -N` to see which record types a file contains.

`-K` makes output occur at the time of another record type, for example `-K18` for attitude records, so that roll, pitch and heave are listed at their full sampling rate while position and other values are interpolated onto those times. `-K` overrides `-N`.

Characters available in `-O`: `T` time yyyy/mm/dd/hh/mm/ss, `t` time yyyy mm dd hh mm ss, `J` and `j` year and day-of-year time strings, `M` Unix time in decimal seconds, `m` seconds since the first record, `U` Unix time in integer seconds, `u` integer seconds since the first record, `V` interval between records (s), `X` longitude, `x` longitude in degrees and decimal minutes, `Y` latitude, `y` latitude in degrees and decimal minutes, `H` heading, `h` course made good, `S` speed (km/hr), `s` speed made good (km/hr), `L` along-track distance (km), `l` along-track distance (m), `c` sonar depth (m), `p` draft (m), `R` roll, `P` pitch, `r` heave. The prefixes `-` (change sign), `/` (invert) and `^` (easting or northing for the following `X` or `Y`, in the projection set with `-J`) modify the next value.

## Common options

- `-Ifile` (`--input=file`) — Input swath file or datalist. Default: datalist.mb-1.
- `-Fformat` (`--format=format`) — MBIO format id; -1 for a datalist. Default: inferred from the file name suffix.
- `-Ooutput_format` (`--output-format=output_format`) — Characters selecting the output columns. Default: `tMXYHs`.
- `-Nnavchannel` (`--nav-channel=navchannel`) — List asynchronous navigation: 0 = primary channel, 1 to 3 = auxiliary channels.
- `-Kkind` (`--data-kind=kind`) — Record type that triggers output, for example 12 = navigation, 17 = heading, 18 = attitude, 28 to 30 = auxiliary navigation 1 to 3, 53 = NMEA GGA. See the manual page for the full list.
- `-Ddecimate` (`--decimate=decimate`) — Output only every decimate-th record. Default: 1.
- `-Gdelimiter` (`--delimiter=delimiter`) — Column separator for text output. Default: tab.
- `-A` (`--binary`) — Write binary double precision values instead of text.
- `-Jprojection` (`--projection=projection`) — Projected coordinate system for `^X` / `^Y`, for example `UTM32N`, `epsg32632`, or `U` for UTM with the zone taken from the first data.
- `-Zsegment` (`--segment=segment`) — Separate the output of each input file with a line containing segment (`>` for GMT multisegment files). `swathfile` or `datalist` writes "#" followed by the source file or datalist path.
- `-Rwest/east/south/north` (`--bounds=...`) — Use only data within these bounds.
- `-Byr/mo/da/hr/mn/sc`, `-Eyr/mo/da/hr/mn/sc` (`--begin-time=...`, `--end-time=...`) — Use only data between these times.
- `-Llonflip` (`--longitude-domain=lonflip`) — Longitude range: -1 = -360 to 0, 0 = -180 to 180, 1 = 0 to 360. Default: 0.
- `-Sspeed` (`--speed-minimum=speed`), `-Ttimegap` (`--time-gap=timegap`) — Minimum speed (km/hr) and maximum time gap (minutes) used when reading.
- `-V` (`--verbose`), `-H` (`--help`) — Verbose messages; print usage and exit.

## Examples

List Unix time, longitude and latitude of one file and save them to a text file:

    mbnavlist -I 0001_20250101_000000.mb58 -OMXY > nav.txt

Write the track of every file in a datalist as a GMT multisegment file (the `>` is quoted so that cmd.exe does not treat it as a redirection):

    mbnavlist -I datalist.mb-1 -OXY "-Z>" > tracks.txt

List roll, pitch and heave at the full rate of the attitude records:

    mbnavlist -I line1.mb88 -K18 -OMRPr > attitude.txt

List the primary asynchronous navigation as UTM easting and northing:

    mbnavlist -I line1.mb88 -N0 "-OtM^X^Y" -JU > nav_utm.txt

## Notes

- The output is written to the screen; redirect it with `> file.txt` to save it. There is no output file option.
- In cmd.exe put arguments containing `^`, `>`, `|`, `&` or `%` in double quotes.

## See also

`mblist`, `mbinfo`, `mbdatalist`, `mbctdlist`, `mbauvloglist`, `mbformat`. The complete reference is the MB-System manual page mbnavlist(1) (English).
