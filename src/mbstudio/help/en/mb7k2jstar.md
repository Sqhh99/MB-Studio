# mb7k2jstar — Extract Edgetech sidescan and subbottom data from Reson 7k files

`mb7k2jstar` extracts Edgetech subbottom profiler and sidescan records embedded
in Reson 7k files (format 88) and writes them in the Edgetech Jstar format
(formats 132 and 133). Use it when an Edgetech sonar was logged through a Reson
7k system (for example on an AUV) and the sidescan or subbottom data are to be
processed separately from the multibeam data.

## Synopsis

    mb7k2jstar
        --bottom-pick=mode[/threshold] {-Bmode[/threshold]}
        --check-route-bearing {-M}
        --comments {-C}
        --extract-mode=type {-Atype}
        --flip-sidescan {-X}
        --format=format {-Fformat}
        --gain=mode[/factor] {-Gmode[/factor]}
        --help {-H}
        --input=file {-Ifile}
        --line-root=startline/lineroot {-Lstartline/lineroot}
        --output=file {-Ofile}
        --route-file=file {-Rfile}
        --smooth=value {-Svalue}
        --time-shift=timeshift {-Ttimeshift}
        --verbose {-V}

## Description

The input is a single Reson 7k file or a datalist of 7k files (`-I`). By default
all Edgetech channels (subbottom, low frequency sidescan and high frequency
sidescan) are extracted; `-A` restricts this. At the end of each file the
program prints how many records of each kind were read and written.

The output is organized in one of three ways:

- Without `-O` or `-R`, each input file gets its own output file, named from
  the input file name with its suffix (and a trailing `p` of processed files)
  removed and a new suffix added: `.jsf` when all record types or only
  subbottom data are extracted, `.mb132` for low frequency sidescan only,
  `.mb133` for high frequency sidescan only.
- With `-O`, the data from all input files go to that single file.
- With `-R` (an `mbgrdviz` route file), the data are split into one file per
  planned survey line, named `<lineroot>_<number>` plus the suffix above.

The time of the first seafloor return is written to the Jstar trace headers,
where it can be used when laying out the sidescan on the seafloor; `-B` selects
how it is obtained. Optional processing of the extracted traces includes smoothing
(`-S`), a range-dependent gain (`-G`), a time shift (`-T`) and swapping port and
starboard (`-X`).

## Common options

- `-I<file>` — Input Reson 7k file or datalist. Default: `datalist.mb-1`.
- `-F<format>` — Input format; `88` for a 7k file, a negative value for a
  datalist. Default: guessed from the file name suffix.
- `-O<file>` — Write all extracted data to this single output file.
- `-A<type>` — Data to extract: `1` (or `sslow`) low frequency sidescan,
  `2` (or `sshigh`) high frequency sidescan, `3` (or `sbp`) subbottom,
  `4` (or `all`) everything. The option may be repeated to combine types.
  Default: all.
- `-B<mode>[/<threshold>]` — Source of the bottom arrival time: `1` multibeam
  bathymetry, `2` altitude from the vehicle navigation (default), `3` pick in
  the sidescan data at the first sample above `threshold` times the trace
  maximum (default threshold 0.5).
- `-G<mode>[/<factor>]` — Sidescan gain: `0` none (default), `1` gain
  proportional to range squared beyond the bottom return, scaled by `factor`
  (default 1.0).
- `-S<value>` — Smooth the trace samples: a positive value averages over
  `value` samples on each side, a negative value uses the RMS instead.
  Default: 0 (no smoothing).
- `-T<timeshift>` — Shift the time stamps of the extracted records by this many
  seconds. Default: 0.
- `-X` — Swap port and starboard sidescan channels.
- `-R<routefile>` — Split the output into survey lines using an `mbgrdviz`
  route file; a waypoint counts as reached within 50 m.
- `-L<startline>/<lineroot>` — First line number and file name root for route
  output. Default: `1/jstar`.
- `-M` — In route mode, skip data collected during turns (heading more than 15
  degrees off the line bearing).
- `-C` — Print the comment records found in the input.
- `-V` — Verbose output.

## Examples

Extract all Edgetech data of one 7k file into `20050418_150155.jsf`:

    mb7k2jstar -I 20050418_150155.s7k

Extract only the low frequency sidescan into `20050418_150155.mb132`:

    mb7k2jstar -I 20050418_150155.s7k -A1

Collect the low frequency sidescan of all files in a datalist into one file:

    mb7k2jstar -A1 -I datalistp.mb-1 -O 20050418_sslow.mb132

Split the low frequency sidescan into survey line files named `line_0001.mb132`,
`line_0002.mb132`, ... and leave out the turns:

    mb7k2jstar -I datalist.mb-1 -A1 -R survey_route.rte -L1/line -M

## Notes

- Without `-O` or `-R`, each output file is written next to its input file;
  with `-R` the line files are written to the current directory. Quote paths
  with spaces.
- An `.inf` file is generated for every output file.
- Only Reson 7k data (format 88) can be read.

## See also

`mbextractsegy`, `mbsslayout`, `mbformat`, `mbinfo`, `mbdatalist`. The complete
reference is the MB-System manual page mb7k2jstar(1) (English).
