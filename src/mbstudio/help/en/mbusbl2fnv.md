# mbusbl2fnv — Convert a USBL tracking CSV file to fnv navigation

`mbusbl2fnv` converts a USBL (ultra-short baseline) ROV tracking file in CSV form into an MB-System fast navigation (fnv) text file. Use it to bring USBL positions and ROV depth into MB-System, for example as the navigation and sensor depth source for `mbpreprocess` when processing ROV-mounted sonar data.

## Synopsis

    mbusbl2fnv [-I usbl.csv] [-O output.fnv] [-V] [-H]
    mbusbl2fnv [--input=usbl.csv] [--output=output.fnv] [--verbose] [--help]

## Description

The input CSV file has a header line followed by data lines with four comma-separated fields: time in Unix epoch seconds (UTC), latitude in decimal degrees (positive north), longitude in decimal degrees (positive east) and ROV depth in meters (positive down). Only lines whose first non-blank character can start a number are read, so the header and comment lines beginning with `#` are skipped automatically. Lines that do not contain exactly four numbers, and records whose time is not later than the previous record, are skipped.

The output is a tab-separated fnv text file. The first line is a `##` comment naming the columns; each following line contains date and time (`yyyy mm dd hh mm ss.ssssss`), epoch seconds, longitude, latitude, heading, speed (km/hr), draft (m), roll, pitch and heave. The ROV depth is written as the draft. Heading, speed, roll, pitch and heave are not part of USBL tracking data and are written as 0.0; the optional swath-edge columns of some fnv files are not written.

Without `-I` the program reads standard input, and without `-O` it writes to standard output.

## Common options

- `-I usbl.csv` — Input USBL tracking CSV file (long form `--input=usbl.csv`). Default: standard input.
- `-O output.fnv` — Output fnv file (long form `--output=output.fnv`). Default: standard output.
- `-V` — Verbose mode (long form `--verbose`): prints the file names, reports skipped lines and prints the numbers of records read, written and skipped.
- `-H` — Prints the usage and the expected CSV layout, then exits (long form `--help`).

## Examples

Convert a USBL tracking file to an fnv file:

    mbusbl2fnv -I usbl_track.csv -O rov.fnv

Convert with progress reporting, using the long option forms:

    mbusbl2fnv --verbose --input=usbl_track.csv --output=rov.fnv

Use the fnv file (navigation format 9) as the navigation and sensor depth source in `mbpreprocess`:

    mbpreprocess --input=datalist.mb-1 --nav-file=rov.fnv --nav-file-format=9 --sensordepth-file=rov.fnv --sensordepth-file-format=9

## Notes

- Because heading, roll, pitch and heave are written as 0.0, do not use the fnv file as heading or attitude source; take these from the vehicle's own sensors.
- USBL fixes are often noisy; check and, if necessary, clean the positions before using them in processing.
- Redirection works in the command prompt as well, e.g. `mbusbl2fnv < usbl_track.csv > rov.fnv`. Quote paths containing spaces.

## See also

`mbpreprocess`, `mbprocess`, `mbnavlab2fnv`, `mbfnv2navlab`, `mbminirovnav`, `mbnavlist`. The complete reference is the MB-System manual page mbusbl2fnv(1) (English).
