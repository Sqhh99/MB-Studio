# mbprocess — apply edits, navigation, SVP and corrections to swath files

`mbprocess` is the central processing program of MB-System. In one pass it reads a raw swath file and writes a processed copy with the bathymetry edits, merged navigation and attitude, recalculated bathymetry (raytracing through a sound velocity profile), bias, draft and tide corrections, and amplitude/sidescan corrections that have been configured for that file. It is normally run after editing (`mbclean`, `mbvoxelclean`, ...) and after setting parameters with `mbset`, and before gridding with `mbgrid` or `mbmosaic`.

## Synopsis

    mbprocess -Iinfile [-C -Fformat -N -Ooutfile -P -S -T -V -H]
        --format=value {-Fvalue}
        --force {-P}
        --help {-H}
        --input=file {-Ifile}
        --output=file {-Ofile}
        --print-status {-S}
        --strip-comments {-N}
        --test {-T}
        --threads=value {-Cvalue}
        --verbose {-V}

## Description

What `mbprocess` does to each file is controlled by a text parameter file named after the swath file with a `.par` suffix (for `survey.mb58` it is `survey.mb58.par`). Each line of the parameter file is a command such as `EDITSAVEFILE`, `NAVFILE`, `SVPFILE`, `ROLLBIAS` or `TIDEFILE`. Parameter files are created and modified with `mbset`, and also by programs that produce ancillary files, such as `mbclean`, `mbareaclean`, `mbvoxelclean`, `mbbackangle`, `mbsvplist` and `mbgpstide`. If no parameter file exists, `mbprocess` infers a reasonable processing path by looking for navigation and edit save files with the standard names.

The processing that can be applied includes:

- bathymetry edits (beam flags) from edit save files (`.esf`);
- merging edited navigation (`.nve`), adjusted navigation from `mbnavadjust` (`.na0`, `.na1`, ...), and separate attitude and sonar depth files;
- recalculating bathymetry by raytracing through a sound velocity profile (`.svp`), or translating between corrected and uncorrected depths;
- roll, pitch and heading bias, draft, heave and lever arm corrections, and per-beam static offsets;
- tide correction from a tide file (for example from `mbotps` or `mbgpstide`);
- data cutting by beam number, acrosstrack distance or speed;
- amplitude and sidescan corrections from the grazing angle tables made by `mbbackangle`, and sidescan recalculation for some sonars;
- inserting metadata (vessel, institution, cruise, PI, ...).

The input given with `-I` is either a single swath file or a datalist; a datalist is read recursively and every swath file in it is processed. If `-I` is omitted and a file `datalist.mb-1` exists in the current directory, it is used. The output file name follows the format id: `root.mbXX` becomes `rootp.mbXX` (for example `mydata.mb71` becomes `mydatap.mb71`); a name without the `.mbXX` suffix gets `p.mbXX` appended. Output files are written next to the input files. For each output file `mbprocess` also generates the `.inf`, `.fbt` and `.fnv` ancillary files.

By default a file is processed only if the output is missing or older than the input file, the parameter file or any ancillary file it refers to (navigation, edit save, SVP files, ...). Use `-P` to force processing of every file. With `-C`, several files of a datalist are processed in parallel threads.

For Kongsberg kmall data (format 261) bathymetry is recalculated from the travel times and angles in the #XMT datagrams written by `mbpreprocess`, so these files must be preprocessed first.

## Common options

- `-I<file>` / `--input=<file>` — swath file or datalist to process. Default: `datalist.mb-1` if it exists in the current directory.
- `-C<n>` / `--threads=<n>` — number of files processed in parallel. Default: 1; limited to the number of CPU cores.
- `-F<format>` / `--format=<format>` — MBIO format id of the input. Normally taken from the parameter file or inferred from the `.mbXX` suffix. Only used when the input is a single file.
- `-O<file>` / `--output=<file>` — output file name for a single input file, overriding the default `rootp.mbXX` name. Ignored when the input is a datalist.
- `-P` / `--force` — process every file, even if its output is up to date.
- `-T` / `--test` — test mode: report which files would be processed, without processing anything.
- `-S` / `--print-status` — print the status of each file (up to date, out of date, locked) with the modification times used for the decision.
- `-N` / `--strip-comments` — do not copy comment records from the input, and do not embed the comments describing the processing parameters.
- `-V` / `--verbose` — print the processing parameters and record counts.
- `-H` / `--help` — print a short help message and exit.

## Examples

Process all files of the datalist in the current directory, only those that are out of date:

    mbprocess -Idatalist.mb-1

Set an SVP file, a roll bias and a constant draft for one Kongsberg file with `mbset`, then process it with progress output:

    mbset -I0051_20010829_223755.mb57 -PSVPFILE:0051_20010829_223755.mb57.svp -PROLLBIAS:0.5 -PDRAFT:1.95
    mbprocess -I0051_20010829_223755.mb57 -V

Check which files need processing without doing anything:

    mbprocess -Idatalist.mb-1 -T -S

Reprocess every file of a survey with four threads:

    mbprocess -Idatalist.mb-1 -P -C4

## Notes

- To work with the processed files afterwards, create a datalist that refers to them, for example with `mbdatalist -Idatalist.mb-1 -Z`, or add `$PROCESSED` to the datalist.
- Processing is never done in place: the raw file is left untouched and can always be processed again with different parameters.
- Files locked by another program (for example an editor) are skipped and reported.
- Datalists and parameter files store file names separated by spaces, so keep survey folders and file names free of spaces.
- The full list of parameter file commands is given in the manual pages mbprocess(1) and mbset(1).

## See also

`mbset`, `mbpreprocess`, `mbclean`, `mbvoxelclean`, `mbbackangle`, `mbsvplist`, `mbotps`, `mbdatalist`, `mbinfo`, `mbgrid`. The complete reference is the MB-System manual page mbprocess(1) (English).
