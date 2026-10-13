# mbfnv2navlab — Convert an fnv navigation file to Kongsberg Navlab binary

`mbfnv2navlab` converts an MB-System fast navigation (fnv) text file into a Kongsberg Navlab smoothed navigation binary file (the `navlab_smooth.bin` format). Use it when navigation that has been corrected or adjusted in MB-System must be supplied back to processing as a Navlab-format navigation source, for example to `mbpreprocess` with navigation format 12. It is the reverse of `mbnavlab2fnv`.

## Synopsis

    mbfnv2navlab [-I input.fnv] [-O navlab_smooth.bin] [-V] [-H]
    mbfnv2navlab [--input=file] [--output=file] [--verbose] [--help]

## Description

The input is a tab-separated fnv text file, as written by MB-System programs such as `mbpreprocess` or `mbnavlab2fnv`. Lines beginning with `#` and blank lines are skipped. Each data line must contain at least ten tab-separated fields: date and time (`yyyy mm dd hh mm ss.ssssss`, space-delimited inside the field), epoch seconds, longitude, latitude, heading, speed (km/hr), draft (m), roll, pitch and heave. Four optional swath-edge fields may follow; they are accepted but ignored.

The output contains one record per input line, each made of 21 little-endian IEEE 754 double-precision values (168 bytes) with no file header. Time is taken from the epoch-seconds field. Latitude, longitude, draft (written as depth), heading, roll and pitch are copied directly, and speed is converted from km/hr to m/s and written to both speed fields (10 and 11). Heave is read but discarded, because the Navlab format has no heave field.

All other fields of the Navlab record (4, 5, 9 and 12-20: position offsets, vertical velocity, uncertainties and covariance terms) are set to 0.0, so the output cannot reproduce an original Navlab file exactly. When the file is used as a navigation source in `mbpreprocess` or `mbprocess`, only the populated fields are used and the zeroed fields have no effect. The sample rate of the output equals that of the input fnv file, usually much lower than the 100 Hz of original Navlab output.

Lines with fewer than ten tab fields, and records whose timestamp is zero, negative or not later than the previous record, are skipped. With `-V` each skipped line is reported with its line number and reason, followed by a summary of lines read, records written and lines skipped.

## Common options

- `-I input.fnv` — Input fnv text file (long form `--input=file`). If omitted, the program reads from standard input.
- `-O navlab_smooth.bin` — Output Navlab binary file (long form `--output=file`). If omitted, the program writes to standard output.
- `-V` — Verbose mode (long form `--verbose`): prints the file names, reports skipped lines and prints a final summary to standard error.
- `-H` — Prints the program name, usage and the fnv-to-Navlab field mapping, then exits (long form `--help`).

## Examples

Convert an fnv file to a Navlab binary file:

    mbfnv2navlab -I mission.fnv -O adjusted_nav.bin

Convert with progress reporting, using the long option forms:

    mbfnv2navlab --verbose --input=mission.fnv --output=adjusted_nav.bin

Use the resulting file as the navigation, sensor depth, attitude and heading source when preprocessing Kongsberg KMALL data (format 261):

    mbpreprocess --format=261 --input=datalist.mb-1 --nav-file=adjusted_nav.bin --nav-file-format=12 --sensordepth-file=adjusted_nav.bin --sensordepth-file-format=12 --attitude-file=adjusted_nav.bin --attitude-file-format=12 --heading-file=adjusted_nav.bin --heading-file-format=12

## Notes

- On Windows always write the binary output with `-O`. Redirecting standard output into a file (`> file.bin`) can corrupt binary data in the command prompt.
- The input must be tab-separated. Files whose columns are separated only by spaces are skipped line by line (run with `-V` to see why).
- Input and output paths containing spaces must be quoted, e.g. `-I "D:\Survey 1\mission.fnv"`.

## See also

`mbnavlab2fnv`, `mbpreprocess`, `mbprocess`, `mblist`, `mbnavlist`. The complete reference is the MB-System manual page mbfnv2navlab(1) (English).
