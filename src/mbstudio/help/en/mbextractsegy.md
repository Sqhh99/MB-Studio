# mbextractsegy — Extract subbottom profiler data from swath files to SEGY

`mbextractsegy` reads swath sonar data files that contain subbottom profiler,
center beam reflection or seismic reflection records and writes those traces
to SEGY files. Use it after data acquisition to pull the subbottom data out of
Reson 7k or Edgetech Jstar files so they can be inspected with `mbsegyinfo`,
listed with `mbsegylist` and gridded as sections with `mbsegygrid`.

## Synopsis

    mbextractsegy
        --begin-time=yr/mo/da/hr/mn/sc/us {-Byr/mo/da/hr/mn/sc/us}
        --check-route-bearing {-M}
        --end-time=yr/mo/da/hr/mn/sc/us {-Eyr/mo/da/hr/mn/sc/us}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --output=segyfile {-Osegyfile}
        --plot-max=plotmax {-Zplotmax}
        --plot-scale=xscale/yscale/maxwidth {-Jxscale/yscale/maxwidth}
        --range-threshold=rangethreshold {-Urangethreshold}
        --route=routefile {-Rroutefile}
        --sample-format=sampleformat {-Ssampleformat}
        --start-line=startline/lineroot {-Lstartline/lineroot}
        --time-list=timelistfile {-Qtimelistfile}
        --time-shift=timeshift {-Ttimeshift}
        --verbose {-V}

## Description

The input is a single swath file or a datalist (`-I`). Subbottom traces can
only be extracted from formats that store such records, for example Reson 7k
(format 88) and Edgetech Jstar (formats 132 and 133). Sonar depth, seafloor
depth, position, heading, roll and pitch are written into the SEGY trace
headers. The SEGY files follow the conventions of the SIOSEIS seismic
processing package, with 32 bit IEEE float samples.

The output is organized in one of three ways:

- Without `-O`, `-R` or `-Q`, each input swath file gets its own SEGY output.
  The files are named from the line root and a running line number set with
  `-L` (default root `sbp`, first number 1), e.g. `sbp_0001_env.segy`,
  `sbp_0002_env.segy`, ...
- With `-O`, all traces from all input files are written to one output named
  from the given root, e.g. `-O survey` gives `survey_env.segy`.
- With a route file (`-R`, an `mbgrdviz` route file) or a waypoint time list
  (`-Q`, as made by `mbroutetime`), the data are split into one file per
  planned survey line, named `<lineroot>_<number>`.

The name ending depends on the sample format chosen with `-S`: envelope output
(the default) adds `_env`; trace output adds nothing; analytic output writes
three files ending in `_cor` (real part), `_hil` (imaginary part) and `_env`
(envelope). The suffix is `.segy` unless `-Osgy` selects `.sgy`.

For every SEGY file written, `mbextractsegy` runs `mbsegyinfo -O` to create a
statistics file named by adding `.sinf` to the SEGY file name. It also writes a
"first cut" plot script named `..._section.cmd` that contains `mbsegygrid`
commands for section grids (see Notes).

## Common options

- `-I<file>` — Input swath file or datalist. Default: `datalist.mb-1`.
- `-F<format>` — MB-System format id of the input; a negative value means the
  input is a datalist. Default: guessed from the file name suffix.
- `-O<root>` — Write all output to a single SEGY file set with this name root.
  The special values `-Osegy` and `-Osgy` only choose the file suffix.
- `-S<sampleformat>` — Output sample form: `1` = as stored (no
  transformation), `2` = single trace (real part of analytic data),
  `3` = envelope (default), `4` = analytic (three files).
- `-R<routefile>` — Split the output into survey lines using the waypoints of
  an `mbgrdviz` route file.
- `-Q<timelistfile>` — Split the output into survey lines using a list of the
  times at which each waypoint was reached (made with `mbroutetime`).
- `-L<startline>/<lineroot>` — First line number and file name root for the
  output files. Default: `1/sbp`.
- `-M` — In route mode, skip traces collected during turns; output starts when
  the heading is within 15 degrees of the next line's bearing.
- `-U<rangethreshold>` — Distance in meters at which a waypoint counts as
  reached. Default: 25.
- `-T<timeshift>` — Add this many seconds to the trace time stamps. Default: 0.
- `-B<yr/mo/da/hr/mn/sc>` / `-E<yr/mo/da/hr/mn/sc>` — Begin and end of the time
  window of data to extract.
- `-J<xscale>/<yscale>/<maxwidth>` — Scale of the section plots in the plot
  script (inches per shot, inches per second, maximum width in inches).
  Default: `0.01/50.0/30.0`.
- `-Z<plotmax>` — Trace value shown as full black in the section plots.
  Default: 50.
- `-V` — Verbose output; reports every tenth trace written.

## Examples

Extract the subbottom data of one Reson 7k file (writes `sbp_0001_env.segy`
and `sbp_0001_env.segy.sinf`):

    mbextractsegy -I 20040722_152111.s7k

Extract all files of a datalist into one SEGY file `survey_sbp_env.segy`:

    mbextractsegy -I datalist.mb-1 -O survey_sbp

Split the data into planned survey lines `line_0001_env.segy`,
`line_0002_env.segy`, ... and leave out the turns:

    mbextractsegy -I datalist.mb-1 -R survey_route.rte -L1/line -M

Write the raw trace instead of the envelope:

    mbextractsegy -I 20040722_152111.s7k -S2

## Notes

- Output files are written to the current directory.
- Give `-O` a name root without a suffix; the program appends `_env.segy` etc.
  itself.
- The `_section.cmd` plot script is a Unix shell script that calls
  `mbm_grdplot`, which is not available on Windows. Use the `mbsegygrid`
  command lines in it as a starting point and plot the grids with `gmt`.
  A message that `chmod` is not recognized can be ignored.

## See also

`mbsegyinfo`, `mbsegylist`, `mbsegygrid`, `mbsegypsd`, `mbroutetime`,
`mb7k2jstar`, `mbdatalist`. The complete reference is the MB-System manual page
mbextractsegy(1) (English).
