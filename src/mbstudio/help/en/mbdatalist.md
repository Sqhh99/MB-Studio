# mbdatalist — Parse datalists and create ancillary files

`mbdatalist` reads a datalist (a list of swath files and their format ids, possibly referencing other datalists) and prints every swath file it finds, with its format id and grid weight. It is also the usual tool for creating the ".inf", ".fbt" and ".fnv" ancillary files that speed up gridding and plotting, for checking processing status and problems, and for removing stale processing locks.

## Synopsis

    mbdatalist
        [
        --verbose {-V}
        --help {-H}
        --copy {-C}
        --report {-D}
        --format=format_id {-Fformat_id}
        --input=file {-Ifile}
        --make-ancillary {-N}
        --update-ancillary {-O}
        --processed {-P}
        --problem {-Q}
        --bounds=w/e/s/n {-Rw/e/s/n}
        --status
        --raw {-U}
        --unlock {-Y}
        --datalistp {-Z}

## Description

A datalist is a text file in which each line holds a file name, its MBIO format id and an optional grid weight, for example `0001_20250101_000000.mb58 58` or `survey2\datalist.mb-1 -1 10.0`. A datalist itself has format id -1, so datalists can be nested. `mbdatalist` descends through the whole structure and prints one line per swath file: the path, the format id and the weight (default 1.0). If a swath file instead of a datalist is given, that file alone is printed.

`mbprocess` writes "processed" files (for example `line1p.mb58` from `line1.mb58`). When the `$PROCESSED` tag appears in a datalist, or with `-P`, the processed file name is reported whenever it exists; `$RAW` or `-U` reports only the raw files. `-Z` creates the convenience datalist `datalistp.mb-1` (from `datalist.mb-1`) containing `$PROCESSED` and a reference to the input, so that gridding programs read the processed data.

`-N` creates the ancillary files for every swath file: ".inf" statistics (made by `mbinfo`), ".fbt" fast bathymetry (made by `mbcopy`, format 71) and ".fnv" fast navigation (made by `mblist`). `-O` only creates them when they are missing or older than the data file. Programs such as `mbgrid`, `mbmosaic` and `gmt mbswath` use these files to skip data outside the area of interest and to read faster.

`-R` reports only files whose ".inf" bounds overlap the given area (files without an ".inf" file are always reported), which shows which files `mbgrid` will use. `-Q` checks each file for missing files referenced in its `mbprocess` parameter file (edit save files, navigation, tide, SVP) and lists data problems recorded in the ".inf" files. `-S` reports whether each file is up to date or needs reprocessing and whether it is locked; `-Y` removes processing locks (".lck" files) left by an interrupted `mbprocess`. `-D` lists the datalist files themselves, with their recursion level.

## Common options

- `-Ifile` (`--input=file`) — Input datalist or swath file. Default: datalist.mb-1.
- `-Fformat_id` (`--format=format_id`) — Format id of the input; -1 for a datalist. Default: inferred from the file name suffix.
- `-N` (`--make-ancilliary`) — Create (overwrite) the ".inf", ".fbt" and ".fnv" files of all swath files.
- `-O` (`--update-ancilliary`) — Create the ancillary files only where they are missing or out of date.
- `-P` (`--processed`) — Report processed file names when the processed files exist.
- `-U` (`--raw`) — Report only raw (unprocessed) file names.
- `-Z` (`--datalistp`) — Write a datalist named after the input root with "p.mb-1" appended (datalistp.mb-1 by default) that sets `$PROCESSED` and references the input, then exit.
- `-Rw/e/s/n` (`--bounds=w/e/s/n`) — Report only files with data inside these bounds, judged from their ".inf" files.
- `-Q` (`--problem`) — Report missing processing files and data problems.
- `-S` (`--status`) — Report processing status and locks.
- `-Y` (`--unlock`) — Remove processing locks from the listed files.
- `-D` (`--report`) — List the datalist files of the recursive structure instead of the swath files.
- `-C` (`--copy`) — Copy the listed swath files and their ancillary files to the current directory and write a datalist.mb-1 there.
- `-V` (`--verbose`), `-H` (`--help`) — Verbose messages; print usage and exit.

## Examples

List all swath files referenced by the datalist in the current directory:

    mbdatalist

Create missing or out-of-date ".inf", ".fbt" and ".fnv" files for a whole survey:

    mbdatalist -I datalist.mb-1 -O

Create datalistp.mb-1 so that later programs use the processed files:

    mbdatalist -I datalist.mb-1 -Z

Show which files contain data inside an area, using processed files where they exist:

    mbdatalist -I datalist.mb-1 -P -R140.5/141.0/35.0/35.4

## Notes

- The long forms of `-N` and `-O` are spelled `--make-ancilliary` and `--update-ancilliary` in this version; the spellings `--make-ancillary` and `--update-ancillary` shown in the program's usage text are not accepted. The short forms `-N` and `-O` always work.
- Paths in a datalist are relative to the folder of the datalist. Quote paths with spaces on the command line.
- `-N` and `-O` run `mbinfo`, `mbcopy` and `mblist` for each file, which can take a long time for large surveys.
- The `-C` copy function uses the Unix `cp` command internally and may not work in a standard Windows command prompt; copy the files with Explorer or `copy` instead.

## See also

`mbmakedatalist`, `mbinfo`, `mbprocess`, `mbgrid`, `mbmosaic`, `mbset`, `mbformat`. The complete reference is the MB-System manual page mbdatalist(1) (English).
