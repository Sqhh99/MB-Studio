# mbsegyinfo — Summarize the contents of a SEGY file

`mbsegyinfo` reads a SEGY seismic or subbottom profiler file and reports basic
statistics about it. Use it to check the SEGY files written by `mbextractsegy`
(or any other SEGY source) and to create the `.sinf` information files that
`mbsegygrid` and `mbsegypsd` use to size their grids.

## Synopsis

    mbsegyinfo -Ifile [-Llonflip -O -H -V]
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --output-file {-O}
        --verbose {-V}

## Description

The report shows the file header values (number of channels, sample interval,
number of samples per trace, trace length, sample data format, CDP fold), the
number of traces, the minimum, maximum and range of shot, trace, RP (CMP)
numbers, delays, ranges, source and receiver elevations and water depths, the
time and position of the first and last traces, and the geographic bounding box
of the data.

By default the report is printed to the screen. With `-O` it is written instead
to a file named by adding `.sinf` to the input file name, e.g.
`line_0001_env.segy.sinf`. Other MB-System programs read these `.sinf` files;
`mbextractsegy` creates them automatically for every SEGY file it writes, and
`mbsegygrid` and `mbsegypsd` run `mbsegyinfo -O` themselves when the `.sinf`
file is missing or older than the SEGY file.

`mbsegyinfo` reads one SEGY file per run; it does not accept datalists.

## Common options

- `-I<file>` — Input SEGY file. Required.
- `-O` — Write the report to `<file>.sinf` instead of the screen.
- `-L<lonflip>` — Longitude range of the reported values: `-1` = -360 to 0,
  `0` = -180 to 180, `1` = 0 to 360. Default: as set by `mbdefaults`.
- `-V` — Verbose mode; also prints the program version and read errors.
- `-H` — Print a short help text and exit.

## Examples

Show the statistics of a SEGY file:

    mbsegyinfo -I sbp_0001_env.segy

Create (or refresh) the `.sinf` file `sbp_0001_env.segy.sinf`:

    mbsegyinfo -I sbp_0001_env.segy -O

Save the report under a name of your choice by redirecting the screen output:

    mbsegyinfo -I sbp_0001_env.segy > sbp_0001_report.txt

## Notes

- Quote file names that contain spaces, e.g. `-I "Line 12.segy"`.
- The `.sinf` file is written next to the input file.

## See also

`mbextractsegy`, `mbsegylist`, `mbsegygrid`, `mbsegypsd`, `mbinfo`. The complete
reference is the MB-System manual page mbsegyinfo(1) (English).
