# dump_gsf — Print the records of a GSF file as text

`dump_gsf` is a small test utility from the Generic Sensor Format (GSF) library. It reads a GSF file record by record and prints each record in text form: the beam-by-beam contents of every swath bathymetry ping, and a one-line summary of the other record types. Use it to look inside a GSF file (MB-System format 121) when checking what a sonar or processing system has written, or to list ping times and positions quickly.

## Synopsis

    dump_gsf [-s] -f <gsf filename> [-pt] [-t mm/dd/yy hh:mm:ss]

## Description

The program opens the file given with `-f` and reads every record from the beginning to the end of the file. Output goes to the screen (standard output); messages such as "Finished processing input file" go to standard error. Running `dump_gsf` without arguments prints the usage and exits.

By default every swath bathymetry ping is printed in full: the record number, the ping time (year/day-of-year hh:mm:ss.ss, UTC), latitude and longitude, heading, course and speed, the sensor id, number of beams, center beam, roll, pitch and heave, followed by a table with one line per beam. The table has a column for each beam array present in the ping, for example depth, across-track and along-track distance, travel time, beam angle, amplitudes, echo width, quality factor, receive heave, intensity time series summary (sample count, bottom detect sample, maximum intensity), quality flags and beam flags.

The other record types (GSF header, sound velocity profile, processing parameters, sensor parameters, comment, history, navigation error and swath bathymetry summary) are each reported on a single line with their record number and time.

With `-s` each ping is reduced to one line with the record number, ping time (with milliseconds), latitude and longitude. `-pt` prints the same short lines and does not pause.

The output is paged: after every 20 records (and every 22 beam lines in the full ping listing) the program prints "Press return to continue, q to quit" and waits for input. Press Enter to continue or type `q` and Enter to stop. Only `-pt` turns the pauses off.

## Common options

- `-f <gsf filename>` — The GSF file to read (required). The name is a separate argument after `-f`.
- `-s` — Short output: one line per ping (record number, time, latitude, longitude) instead of the full beam listing. The output still pauses every 20 records.
- `-pt` — Short output like `-s`, printed without pauses. Use this to list all ping times or to redirect the output to a file.
- `-t mm/dd/yy hh:mm:ss` — Start time (UTC). Records are skipped until the first ping at or after this time. Give the date and time as one quoted argument, with a two-digit year (00-68 means 2000-2068, 69-99 means 1969-1999).

## Examples

Page through the full contents of a GSF file (press Enter to continue, `q` to quit):

    dump_gsf -f 0001_20250101_000000.gsf

List the time and position of every ping in a file:

    dump_gsf -f 0001_20250101_000000.gsf -pt > pings.txt

List pings starting at 08:30 UTC on 15 June 2025:

    dump_gsf -f line12.gsf -pt -t "06/15/25 08:30:00"

Write the full beam-by-beam listing to a text file without waiting for key presses (the prompts are written into the file):

    dump_gsf -f line12.gsf < NUL > line12_dump.txt

## Notes

- The file name after `-f` cannot contain spaces and must be shorter than 128 characters. Change to the folder that holds the data (`cd /d D:\survey\gsf`) and give the plain file name.
- When the output is redirected to a file without `-pt`, the program still waits for Enter at every pause. Redirect the input from `NUL` as in the last example, or use `-pt`.
- The full listing is very long for modern multibeam data (hundreds of beams per ping); `-pt` or `mbinfo` is usually a better first look.
- Times are always shown in UTC.

## See also

`mbinfo`, `mblist`, `mbformat`, `mbcopy`. `dump_gsf` has no MB-System manual page; this page is based on the program's usage text and source code.
