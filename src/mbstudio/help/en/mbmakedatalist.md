# mbmakedatalist — Create a datalist of the swath files in a directory

`mbmakedatalist` scans a directory for files that can be recognised as swath data and writes an MB-System datalist (`datalist.mb-1` by default) listing each file with its format id. It is usually the first command when setting up the processing of a new survey: most other MB-System programs take this datalist as their input.

## Synopsis

    mbmakedatalist [-B size|--min-size=size]
                   [-F format|--format=format]
                   [-I directory|--input=directory]
                   [-L|--skip-latest]
                   [-O datalist|--output=datalist]
                   [-P|--ignore-processed]
                   [-S suffix|--suffix=suffix]
                   [-T|--no-time-sort]
                   [-H|--help] [-V|--verbose]

## Description

A datalist is a text file with one swath file and its MBIO format id per line; datalists may also reference other datalists. `mbmakedatalist` builds one automatically. Every file in the target directory (the current directory unless `-I` is given) is tested against the MB-System file naming conventions, such as `.mb88` for format 88 or the Kongsberg vendor suffixes `.all` and `.kmall`, and every recognised file is added with the format id inferred from its name. Files that are not recognised, directories, and files smaller than the `-B` threshold are skipped. The files are listed in alphabetical order.

It is usually best to give the suffix of the wanted files with `-S`, so that unrelated files that happen to match a naming convention are not included. With `-S`, processed files (the letter `p` before the suffix, e.g. `0001p.mb88`, created by `mbprocess`) can be excluded with `-P`, so a datalist of only the raw files can be made in a directory containing both. `-F` assigns one format id to all files instead of inferring it.

Kongsberg file names start with a line number that restarts at 0001 whenever logging is restarted, so an alphabetical listing is not always in time order. When the suffix given with `-S` is `.all`, `.ALL`, `.kmall`, `.KMALL`, `.mb56`, `.mb57`, `.mb58`, `.mb59` or `.mb261`, the files are sorted by the date and time embedded in their names (`NNNN_YYYYMMDD_HHMMSS...`); `-T` disables this. Kongsberg junk files named `9999.all`, `9999.ALL`, `9999.kmall` or `9999.KMALL` are ignored.

The datalist is written in the current directory (or to the path given with `-O`). If no swath file is found, no datalist is written.

## Common options

- `-S suffix` — Only consider files ending with this suffix (e.g. `.all`, `.kmall`, `.mb88`). The comparison is case sensitive.
- `-I directory` — Directory to scan. Default: the current directory.
- `-O datalist` — Name of the output datalist. Default: `datalist.mb-1`.
- `-P` — With `-S`, leave out processed files (suffix preceded by `p`).
- `-F format` — Assign this format id to all files instead of inferring it from the names.
- `-B size` — Ignore files smaller than size kilobytes.
- `-L` — Leave out the last file of the list, e.g. the file that is still being logged during acquisition.
- `-T` — Do not sort Kongsberg files into time order.
- `-V` — Print what is checked and every file added.
- `-H` — Print help and exit.

Every option also has the long form shown in the synopsis (e.g. `--suffix=.all`).

## Examples

Make `datalist.mb-1` from all recognisable swath files in the current directory and show the files added:

    mbmakedatalist -V

List only the raw Kongsberg `.all` files of the subdirectory `rawdata`, in time order, without the file still being logged, into `kongsberg.mb-1`:

    mbmakedatalist -I rawdata -S .all -P -L -O kongsberg.mb-1

The same with long options:

    mbmakedatalist --input=rawdata --suffix=.all --ignore-processed --skip-latest --output=kongsberg.mb-1

Make a datalist of `.kmall` files larger than 100 kilobytes:

    mbmakedatalist -S .kmall -B 100

## Notes

- With `-I`, the entries are written as `directory/filename` relative to the current directory, so run the following MB-System programs from the same directory, or use an absolute path with `-I`.
- The suffix match is case sensitive: `-S .all` does not match `0001_20250101_000000.ALL`.
- When Kongsberg time sorting is active, files whose names do not contain the `NNNN_YYYYMMDD_HHMMSS` pattern are dropped from the list; use `-T` if your files are named differently.
- Check the result with `mbdatalist -I datalist.mb-1` or `mbinfo -I datalist.mb-1`.

## See also

`mbdatalist`, `mbinfo`, `mbformat`, `mbpreprocess`, `mbprocess`. The complete reference is the MB-System manual page mbmakedatalist(1) (English).
