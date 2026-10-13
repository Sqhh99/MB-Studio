# mbsegylist — List trace header values from a SEGY file

`mbsegylist` reads a SEGY seismic or subbottom profiler file and prints chosen
trace header values, one line per trace. Use it to get shot times and positions
of a subbottom line (for example to plot the track or to merge with other
data), or to check delays and trace lengths before gridding with `mbsegygrid`.

## Synopsis

    mbsegylist -Ifile
        --binary-output {-A}
        --decimate=value {-Dvalue}
        --delimiter=character {-Gcharacter}
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --output-format=list {-Olist}
        --segment-tag=tag {-Ztag}
        --use-feet {-W}
        --verbose {-V}

## Description

The output goes to the screen (standard output) as text columns separated by
tabs, in the order given with `-O`. Redirect it with `>` to save it to a file.
With `-A` the values are written instead as binary double precision numbers.

The `-O` list is a string of characters, one per output column:

- `C` CMP (RP, CDP) number; `c` trace number within the CMP gather
- `D` trace start delay (s); `I` sample interval (s); `L` trace length (s);
  `N` number of samples
- `i` time since the previous trace (s); `V` shot or ping interval (s)
- `J` time string `yyyy jd hh mm ss.ssssss` (jd = day of year); `j` time string
  `yyyy jd dm ss.ssssss` (dm = minute of day)
- `T` time string `yyyy/mm/dd/hh/mm/ss`; `t` time string `yyyy mm dd hh mm ss`
- `M` Unix time in decimal seconds; `m` decimal seconds since the first trace;
  `U` Unix time in integer seconds; `u` integer seconds since the first trace
- `l` line number from the file header; `n` trace counter (starts at 0)
- `R` source to receiver range (m); `S` shot number; `s` trace number within
  the shot
- `X` longitude (decimal degrees); `x` longitude (degrees, decimal minutes,
  E/W); `Y` latitude (decimal degrees); `y` latitude (degrees, decimal minutes,
  N/S)
- `Z` source or sonar depth (m, positive down); `z` water depth (m, positive
  down)
- `/` inverts the next value; `-` changes the sign of the next value (simple
  numeric values only)

The default list is `TiXYSsCcDINL`.

## Common options

- `-I<file>` — Input SEGY file. Required.
- `-O<list>` — Values to output and their order (see above). Default:
  `TiXYSsCcDINL`.
- `-G<character>` — Field separator for text output. Default: tab.
- `-D<value>` — Output only every n-th trace. Default: 1 (all traces).
- `-A` — Binary output (double precision values) instead of text.
- `-L<lonflip>` — Longitude range: `-1` = -360 to 0, `0` = -180 to 180,
  `1` = 0 to 360. Default: as set by `mbdefaults`.
- `-W` — Output the depths `Z` and `z` in feet instead of meters.
- `-Z<tag>` — Write this tag line before the data (text output only), e.g.
  `-Z">"` for a GMT multiple segment file.
- `-V` — Verbose mode.

## Examples

List shot number, time and position of every trace:

    mbsegylist -I sbp_0001_env.segy -OSTXY

Save Unix time, longitude, latitude and water depth as a comma separated file:

    mbsegylist -I sbp_0001_env.segy -OMXYz -G, > sbp_0001_nav.csv

List every tenth trace with its delay, number of samples and trace length:

    mbsegylist -I sbp_0001_env.segy -OSDNL -D10

## Notes

- `mbsegylist` reads one SEGY file per run; it does not accept datalists.
- In the command prompt, `>` writes the output to a file in the current
  directory.

## See also

`mbsegyinfo`, `mbextractsegy`, `mbsegygrid`, `mblist`. The complete reference is
the MB-System manual page mbsegylist(1) (English).
