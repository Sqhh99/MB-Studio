# mbtime — Convert between calendar time and Unix epoch time

`mbtime` converts a time value between Unix epoch time (seconds since 1970/01/01 00:00:00) and calendar time. MB-System documentation, data listings and many auxiliary files express time as epoch seconds ("time_d"), so this program is handy for checking time stamps, preparing time windows for other programs, or reading `mblist` and navigation output.

## Synopsis

    mbtime [-Mtime_d -Tyear/month/day/hour/minute/second -V -H]
        --calendar-time=year/month/day/hour/minute/second {-Tyear/month/day/hour/minute/second}
        --epoch-time=time_d {-Mtime_d}
        --help {-H}
        --verbose {-V}

## Description

The input time is given either as epoch seconds with `-M` or as a calendar time with `-T`. The program writes the same instant in the other form to standard output: `-M` produces a calendar time of the form `yyyy/mm/dd/hh/mm/ss.ssssss`, and `-T` produces epoch seconds with six decimal places.

Calendar times are year, month, day, hour, minute and decimal seconds separated by slashes; the seconds may include a fractional part down to microseconds. All times are treated as UTC, consistent with the rest of MB-System.

Only one conversion is performed per run. If both `-M` and `-T` are given, the one that appears last on the command line is used. Without either option the program converts epoch time 0 and prints `1970/01/01/00/00/00.000000`.

## Common options

- `-Mtime_d` — Input time in Unix epoch seconds (long form `--epoch-time=time_d`); the calendar time is printed.
- `-Tyear/month/day/hour/minute/second` — Input calendar time (long form `--calendar-time=...`); the epoch seconds are printed.
- `-V` — Verbose mode (long form `--verbose`): also prints the program name and MB-System version.
- `-H` — Prints a short description and the usage, then exits (long form `--help`).

## Examples

Convert 6 June 2008 18:37:14 to epoch seconds (prints `1212777434.000000`):

    mbtime -T2008/06/06/18/37/14.0

Convert the epoch time 1212777434.0 to calendar time (prints `2008/06/06/18/37/14.000000`):

    mbtime -M1212777434.0

Convert a calendar time with fractional seconds, using the long option form:

    mbtime --calendar-time=2025/01/01/12/30/05.250

## Notes

- The month and day are a calendar month and day of month, not a day of year.
- Options may also be given in lower case (`-m`, `-t`, `-v`, `-h`).

## See also

`mbinfo`, `mblist`, `mbnavlist`, `mbmapscale`. The complete reference is the MB-System manual page mbtime(1) (English).
