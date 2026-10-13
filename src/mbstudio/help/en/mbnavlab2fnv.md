# mbnavlab2fnv — Convert a Kongsberg Navlab binary file to fnv navigation

`mbnavlab2fnv` converts a Kongsberg Navlab smoothed navigation binary file (typically `navlab_smooth.bin`) into an MB-System fast navigation (fnv) text file. Navlab is the post-processing package that computes smoothed navigation and attitude for Kongsberg Hugin AUVs. Use this program to bring that solution into MB-System, for example as the navigation, sensor depth, attitude and heading source for `mbpreprocess`.

## Synopsis

    mbnavlab2fnv [-I navlab_smooth.bin] [-O output.fnv] [-V] [-H]
    mbnavlab2fnv [--input=file] [--output=file] [--verbose] [--help]

## Description

The input file has no header and consists of 168-byte records, each holding 21 little-endian IEEE 754 double-precision values, nominally sampled at 100 Hz. The program uses time (Unix epoch seconds), latitude, longitude, depth, heading, roll, pitch and speed through water from each record; the remaining fields (uncertainties, covariance terms, vertical velocity) are ignored.

The output is a tab-separated fnv text file. The first line is a `##` comment naming the columns. Every following line contains date and time (`yyyy mm dd hh mm ss.ssssss`, space-delimited inside the first field), epoch seconds, longitude, latitude, heading, speed (km/hr), draft (m), roll, pitch and heave. The Navlab depth is written as draft, speed is converted from m/s to km/hr, and the calendar time is computed in UTC.

Heave is not stored in the Navlab format and is always written as 0.0; obtain heave from the sonar data stream if it is required. The optional port and starboard swath-edge columns of some fnv files are not written, because no swath geometry is available.

Records whose timestamp is not later than the previous record are skipped. With `-V` each skipped record is reported, and the numbers of records read and written are printed at the end.

## Common options

- `-I navlab_smooth.bin` — Input Navlab binary file (long form `--input=file`). If omitted, the program reads from standard input.
- `-O output.fnv` — Output fnv text file (long form `--output=file`). If omitted, the program writes to standard output.
- `-V` — Verbose mode (long form `--verbose`): prints the file names, reports skipped records and prints a final summary to standard error.
- `-H` — Prints the program name, usage and the Navlab field layout, then exits (long form `--help`).

## Examples

Convert a Navlab binary file to an fnv file:

    mbnavlab2fnv -I navlab_smooth.bin -O mission.fnv

Convert with progress reporting, using the long option forms:

    mbnavlab2fnv --verbose --input=navlab_smooth.bin --output=mission.fnv

Use the fnv file (navigation format 9) as the navigation, sensor depth, attitude and heading source when preprocessing Kongsberg KMALL data (format 261):

    mbpreprocess --format=261 --input=datalist.mb-1 --nav-file=mission.fnv --nav-file-format=9 --sensordepth-file=mission.fnv --sensordepth-file-format=9 --attitude-file=mission.fnv --attitude-file-format=9 --heading-file=mission.fnv --heading-file-format=9

## Notes

- On Windows always give the binary input with `-I`. Feeding the binary file through standard input (`< navlab_smooth.bin`) is not reliable in the command prompt.
- `mbpreprocess` can also read the Navlab binary file directly with format 12; converting to fnv is useful when you want to inspect, edit or plot the navigation as text.
- Quote paths containing spaces, e.g. `-I "D:\AUV dive 12\navlab_smooth.bin"`.

## See also

`mbfnv2navlab`, `mbpreprocess`, `mbprocess`, `mbnavlist`. The complete reference is the MB-System manual page mbnavlab2fnv(1) (English).
