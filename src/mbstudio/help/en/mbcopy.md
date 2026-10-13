# mbcopy — copy swath files with format conversion, windowing and averaging

`mbcopy` copies a swath sonar data file to a new file, optionally converting it to another MBIO format, cutting it to a time or geographic window, averaging pings, or merging bathymetry from a second file. It is the general-purpose tool for format translation and for extracting subsets of a survey line.

## Synopsis

    mbcopy
        --bathymetry-only {-D}
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --comment-file=commentfile {-Ccommentfile}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=iformat/oformat/mformat {-Fiformat/oformat/mformat}
        --help {-H}
        --input=infile {-Iinfile}
        --lonflip=lonflip {-Llonflip}
        --merge-file=mergefile {-Mmergefile}
        --output=outfile {-Ooutfile}
        --ping-average=pings {-Ppings}
        --sleep-factor=sleep_factor {-Qsleep_factor}
        --speed-minimum=speed {-Sspeed}
        --strip-comments {-N}
        --time-gap=timegap {-Ttimegap}
        --verbose {-V}

## Description

`mbcopy` reads one swath file (not a datalist) and writes one output file. When the input and output formats belong to the same sonar system, the complete data stream is copied. When they belong to different systems, only the time stamp, navigation and beam values (bathymetry, amplitude, sidescan) are carried over; the same partial copy happens whenever ping averaging is used.

Data can be restricted to a time window (`-B`, `-E`), a geographic window (`-R`) and a minimum ship speed (`-S`). If the begin time is after the end time, the data between the two times are excluded instead.

With `-M` the bathymetry and beam flags are taken from a merge file and written together with all other data from the input file. This combines two processing streams of the same raw data, for example corrected bathymetry exported by other software with the original attitude and backscatter records.

By default `mbcopy` adds comment records describing how it was run, and passes on the comments of the input. Without `-I` it reads standard input, and without `-O` it writes to standard output.

## Common options

- `-I<file>` / `--input=<file>` — input swath file. Default: standard input.
- `-O<file>` / `--output=<file>` — output swath file. Default: standard output.
- `-F<iformat>[/<oformat>[/<mformat>]]` / `--format=...` — input, output and merge file format ids. If only the input format is given, the output format is the same. If omitted, the input format is inferred from the input file name (for example `.mb58`), and the merge format from the merge file name.
- `-B<yr/mo/da/hr/mn/sc>` / `--begin-time=...` — start of the time window. Default: 1962/2/21/10/30/0.
- `-E<yr/mo/da/hr/mn/sc>` / `--end-time=...` — end of the time window. Default: 2062/2/21/10/30/0.
- `-R<west/east/south/north>` / `--bounds=...` — copy only data inside these longitude/latitude bounds.
- `-P<pings>` / `--ping-average=<pings>` — average this many pings into one output ping; `0` chooses the number automatically so that alongtrack spacing matches acrosstrack beam spacing. Default: 1 (no averaging).
- `-S<speed>` / `--speed-minimum=<speed>` — skip pings with ship speed below this value in km/h. Default: 0.
- `-T<timegap>` / `--time-gap=<timegap>` — time gap between pings, in minutes, regarded as a data gap. Default: 1.
- `-M<mergefile>` / `--merge-file=<mergefile>` — take bathymetry and beam flags from this file.
- `-D` / `--bathymetry-only` — write only bathymetry (no amplitude or sidescan); works only with output format 71. This is how fast bathymetry (`.fbt`) files are made.
- `-N` / `--strip-comments` — do not write `mbcopy` comments and drop the input comments. Given twice (`-N -N`), additionally pass only bathymetry, navigation, heading and attitude records (for formats that distinguish them).
- `-C<commentfile>` / `--comment-file=<commentfile>` — insert the lines of a text file as comments at the start of the output.
- `-L<lonflip>` / `--lonflip=<lonflip>` — longitude range: `-1` for -360 to 0, `0` for -180 to 180, `1` for 0 to 360.
- `-Q<sleep_factor>` / `--sleep-factor=<sleep_factor>` — pause before each record for the ping interval times this factor, to simulate a logging system.
- `-V` / `--verbose` — print record counts and messages.

## Examples

Convert a raw Hydrosweep file (format 21) to format 24, keeping only data between 145W and 140W and 10S and 5S:

    mbcopy -F21/24 -R-145/-140/-10/-5 -Ihs_raw -Ohs_binary

Extract one hour of a Kongsberg file into a new file of the same format:

    mbcopy -I0001_20250101_000000.mb58 -B2025/01/01/10/00/00 -E2025/01/01/11/00/00 -Oline1_part.mb58

Combine the raw Simrad data (format 56, written as format 57) with corrected bathymetry from a GSF file (format 121):

    mbcopy -F56/57/121 -I0015_20040212_052714_raw.all -M20040212_052714.gsf -O20040212_052714.mb57

Write a compact bathymetry-only copy in format 71:

    mbcopy -F58/71 -D -I0001_20250101_000000.mb58 -O0001_20250101_000000.mb71

## Notes

- Always give `-O`: swath data are binary and must not be written to the console.
- Not every combination of input and output formats makes sense; converting between different sonar systems loses the system-specific records.
- Quote file names that contain spaces, for example `-I"D:\Survey 2025\line1.mb58"`.

## See also

`mbformat`, `mbinfo`, `mbdatalist`, `mbpreprocess`, `mbprocess`, `mblist`. The complete reference is the MB-System manual page mbcopy(1) (English).
