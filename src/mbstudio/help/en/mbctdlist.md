# mbctdlist — List CTD records stored in swath data files

`mbctdlist` lists the CTD (conductivity, temperature, depth) records contained in swath data files, together with time and navigation values. Some ROV and AUV mapping platforms log CTD data in the sonar data stream; use this program to extract it, for example to check water properties or to derive sound speed information.

## Synopsis

    mbctdlist
        --binary {-A}
        --decimate=decimate {-Ddecimate}
        --delimiter=delimiter {-Gdelimeter}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --output-format=output_format {-Ooutput_format}
        --segment=segment {-Zsegment}
        --verbose {-V}

## Description

For every CTD record found, `mbctdlist` writes one line containing the values selected with `-O`. Conductivity (S/m), temperature (degrees C), salinity and water sound speed are available, as well as the raw voltages of up to eight auxiliary sensors. Time, position, heading, speed, sonar depth and altitude come from the survey records and are interpolated to the time of each CTD record. If a file contains no survey records, no CTD records are listed.

Output is tab-separated text by default; `-G` sets another delimiter, `-A` writes binary double precision values and `-D` decimates the output. Only data formats that store CTD records produce output (originally only format 88, Reson 7k).

Characters available in `-O`: `T` time yyyy/mm/dd/hh/mm/ss, `t` time yyyy mm dd hh mm ss, `J` and `j` year and day-of-year time strings, `M` Unix time in decimal seconds, `m` seconds since the first record, `U` Unix time in integer seconds, `u` integer seconds since the first record, `V` interval between records (s), `X` longitude, `x` longitude in degrees and decimal minutes, `Y` latitude, `y` latitude in degrees and decimal minutes, `H` heading, `h` course made good, `L` along-track distance (km), `l` along-track distance (m), `C` conductivity, `c` temperature, `S` salinity, `s` sound speed (m/s), `P` potential temperature, `1` to `8` sensor voltages. With the prefix `^`: `^C` sonar altitude (m), `^c` sonar depth (m), `^S` speed (km/hr), `^s` speed made good (km/hr). The prefixes `-` (change sign) and `/` (invert) modify the next value. The default is `TXYHCc^cSs` (time, longitude, latitude, heading, conductivity, temperature, sonar depth, salinity, sound speed).

## Common options

- `-Ifile` (`--input=file`) — Input swath file or datalist. Default: datalist.mb-1.
- `-Fformat` (`--format=format`) — MBIO format id; -1 for a datalist. Default: inferred from the file name suffix.
- `-Ooutput_format` (`--output-format=output_format`) — Characters selecting the output columns (see above and the Notes). Default: `TXYHCc^cSs`.
- `--decimate=decimate` — Output only every decimate-th record. Default: 1.
- `-Gdelimiter` (`--delimiter=delimiter`) — Column separator for text output. Default: tab.
- `-A` (`--binary`) — Write binary double precision values instead of text.
- `-Zsegment` (`--segment=segment`) — Write a line containing segment before the output of each input file (`>` for GMT multisegment files). Text output only.
- `-Llonflip` (`--longitude-domain=lonflip`) — Longitude range: -1 = -360 to 0, 0 = -180 to 180, 1 = 0 to 360. Default: 0.
- `-V` (`--verbose`), `-H` (`--help`) — Verbose messages; print usage and exit.

## Examples

List the CTD records of a Reson 7k file with the default columns:

    mbctdlist -I 20080904_231809p.mb88 > ctd.txt

List the CTD records of all files in a datalist, with every 10th record only and comma separators:

    mbctdlist -I datalist.mb-1 --decimate=10 -G, > ctd.csv

Replace the default columns with ten of your own (Unix time, longitude, latitude, sonar depth, temperature, salinity, sound speed, conductivity, heading; the argument is quoted because `^` is special in cmd.exe):

    mbctdlist -I 20080904_231809p.mb88 "-OMXY^ccSsCH" > ctd_custom.txt

## Notes

- In this version a custom `-O` string always produces ten field codes: the characters you give replace the start of the default `TXYHCc^cSs` (where `^c` counts as two characters), and characters beyond the tenth are ignored. To control all columns, give exactly ten characters.
- Use the long form `--decimate=N` for decimation; the short form `-D` does not take a value correctly in this version.
- Output goes to the screen; redirect it with `> file.txt` to save it. In cmd.exe put arguments containing `^` or `>` in double quotes.
- Files without CTD records produce no output.

## See also

`mblist`, `mbnavlist`, `mbsvplist`, `mbinfo`, `mbdatalist`, `mblevitus`. The complete reference is the MB-System manual page mbctdlist(1) (English).
