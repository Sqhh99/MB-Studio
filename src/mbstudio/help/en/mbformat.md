# mbformat — Identify and describe MB-System swath data formats

`mbformat` describes the swath data formats supported by the MBIO library. It can list all supported formats, describe one format id, or work out which format id MB-System will assume for a given file name. Use it when you need the format id for `-F` options or for the entries of a datalist.

## Synopsis

    mbformat
        --format=format_id {-Fformat_id}
        --help {-H}
        --html {-W}
        --input=file {-Ifile}
        --list-ids {-L}
        --list-root {-K}
        --verbose {-V}

## Description

Without options, `mbformat` lists every supported format with its id, name, informal description and attributes (sonar, number of beams, data types, file type). With `-F` it prints the description of one format id.

With `-I` it tests a file name against the prefix and suffix conventions built into MB-System (for example ".mb58", ".all", ".s7k", ".xtf") and reports the inferred format id. It also determines the file "root", which is the file name without the recognised suffix or prefix. The decision is based mainly on the file name; for some suffixes (for example ".all") the first bytes of the file are also read to tell format variants apart, so the file should exist.

`-L` reduces the output to the format id number alone (0 if no valid id was found), and `-K` prints the file root followed by the format id. These short forms are handy in scripts. `-W` writes the list of all formats as an HTML page.

## Common options

- `-Fformat_id` (`--format=format_id`) — Describe this format id. If neither `-F` nor `-I` is given, all formats are listed.
- `-Ifile` (`--input=file`) — Infer the format id from this file name.
- `-L` (`--list-ids`) — Print only the format id number (0 if none could be identified).
- `-K` (`--list-root`) — Print the file root and the format id.
- `-W` (`--html`) — Output the list of all formats as HTML.
- `-V` (`--verbose`) — Also print the program version.
- `-H` (`--help`) — Print a short description and the usage, then exit.

## Examples

List all supported formats:

    mbformat

Describe format 58 (Kongsberg 3rd generation multibeam vendor format, EM122/EM302/EM710 and similar):

    mbformat -F58

Find the format id that MB-System assumes for a Kongsberg ".all" file:

    mbformat -I 0001_20250101_000000.all

Print only the root of a file name and its format id:

    mbformat -K -I 0001_20250101_000000.mb58

Save the list of formats as an HTML page:

    mbformat -W > formats.html

## Notes

- If the file name has no recognised suffix, `mbformat` reports that it is unable to infer the format; specify the format explicitly with `-F` in other programs.
- The format id is also used in datalists: each line holds a file name followed by its format id.

## See also

`mbinfo`, `mbdatalist`, `mbmakedatalist`, `mbcopy`, `mbpreprocess`, `mbdefaults`. The complete reference is the MB-System manual page mbformat(1) (English).
