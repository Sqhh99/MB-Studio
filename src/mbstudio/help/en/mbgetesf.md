# mbgetesf — extract beam flags from a swath file into an edit save file

`mbgetesf` reads the beam flags stored in a swath data file and writes them as an edit save file (`.esf`), the same kind of file produced by `mbclean` and the interactive editors. Use it to transfer editing done on one version of a dataset (for example an already edited or converted file) to another version of the same data, without editing again.

## Synopsis

    mbgetesf
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format_id {-Fformat_id}
        --help {-H}
        --input=infile {-Iinfile}
        --kluge=value {-Kvalue}
        --mode=value {-Mvalue}
        --output=esffile {-Oesffile}
        --verbose {-V}

## Description

Bathymetry editing marks bad beams with flags. Normally those flags are kept in an edit save file and applied to the raw data by `mbprocess`. Sometimes, however, the flags exist only inside a data file, for example in a file that was edited in a format that keeps only part of the original data stream, or in data processed elsewhere. `mbgetesf` turns the flags of such a file into edit events (time stamp, beam number and action) and writes them to an edit save file.

If the output is named after the target swath file with `.esf` appended, `mbclean` picks it up automatically as a starting point. To apply the edits directly, enable them in the target file's parameter file with `mbset` (`EDITSAVEMODE`, `EDITSAVEFILE`) and run `mbprocess`.

By default only flagged beams are written (`-M1`). Modes 2 and 3 also write null beams, or every beam including good ones, which is useful when earlier editing nulled originally good beams or unflagged originally flagged beams. Modes 4 to 6 write a file with an implicit default state for all beams not listed (good, null, or chosen by format); this is the form used for "global edit files" (`.gef`) by `mbpreprocess`.

The program reads a single swath file (not a datalist). If no input file is given it reads standard input, and if no output file is given the edit events are written to standard output.

## Common options

- `-I<file>` / `--input=<file>` — swath data file to read. Default: standard input.
- `-O<esffile>` / `--output=<esffile>` — edit save file to write. Default: standard output.
- `-F<format_id>` / `--format=<format_id>` — MBIO format id of the input; normally inferred from the file name.
- `-M<mode>` / `--mode=<mode>` — which beams are written: `1` flagged beams only (default), `2` flagged and null beams, `3` all beams; `4`, `5`, `6` write an implicit-default edit file whose unlisted beams are good or null depending on the format (`4`), null (`5`) or good (`6`).
- `-B<yr/mo/da/hr/mn/sc>` / `--begin-time=...` — start of the time window. Default: 1962/2/21/10/30/0.
- `-E<yr/mo/da/hr/mn/sc>` / `--end-time=...` — end of the time window. Default: 2062/2/21/10/30/0.
- `-K<value>` / `--kluge=<value>` — `1` corrects a one-beam index offset found in some EM300/EM3000 data in HDCS format (format 151); no effect for other formats. Default: 0.
- `-V` / `--verbose` — print the mode and counts of beams written.
- `-H` / `--help` — print a short help message and exit.

## Examples

Extract the flags from an edited format 22 file and save them under the name of the corresponding format 24 file:

    mbgetesf -F22 -Ihs_ew9103_143.mb22 -Ohs_ew9103_143.mb24.esf -V

Enable the extracted edits for the format 24 file and process it:

    mbset -Ihs_ew9103_143.mb24 -PEDITSAVEMODE:1 -PEDITSAVEFILE:hs_ew9103_143.mb24.esf
    mbprocess -Ihs_ew9103_143.mb24

Write edit events for all beams (good, flagged and null) of one hour of data:

    mbgetesf -Iedited_line.mb57 -M3 -B2025/01/01/10/00/00 -E2025/01/01/11/00/00 -Oraw_line.mb57.esf

## Notes

- Both files must contain the same pings: edit events are matched by time stamp and beam number.
- An existing edit save file with the output name is overwritten, so do not write over the `.esf` file of a file whose earlier edits you want to keep.
- The output is a binary file; always give `-O` rather than displaying it in the console.

## See also

`mbclean`, `mbset`, `mbprocess`, `mbvoxelclean`, `mbareaclean`, `mbdumpesf`. The complete reference is the MB-System manual page mbgetesf(1) (English).
