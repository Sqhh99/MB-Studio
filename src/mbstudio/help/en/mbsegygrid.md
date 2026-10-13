# mbsegygrid — Grid SEGY trace data into a seismic section

`mbsegygrid` turns the traces of a SEGY file into a GMT netCDF grid that shows
a seismic or subbottom profiler section: trace number (or distance) along the
x axis and two-way travel time (or depth) along the y axis. Use it after
`mbextractsegy` to make section images of subbottom lines that can then be
plotted with `gmt`.

## Synopsis

    mbsegygrid -Ifile -Oroot
        --agc=agcmaxvalue/agcwindow {-Bagcmaxvalue/agcwindow}
        --decimation=decimatex/decimatey {-Ddecimatex/decimatey}
        --distance-plot=distancebin/startlon/endlon/startlat/endlat {-Rdistancebin/startlon/endlon/startlat/endlat}
        --filter=filtermode/filterwindow {-Ffiltermode/filterwindow}
        --gain=gainmode/gain[/gainwindow[/gaindelay]] {-Ggainmode/gain[/gainwindow[/gaindelay]]}
        --geometry=geometrymode {-Cgeometrymode}
        --help {-H}
        --input=file {-Ifile}
        --output=fileroot {-Ofileroot}
        --scale-to-distance=shotscale/timescale {-Ashotscale/timescale}
        --time-sweep=timesweep[/timedelay] {-Ttimesweep[/timedelay]}
        --trace-mode=tracemode[/tracestart/traceend[/chanstart/chanend]] {-Stracemode[/tracestart/traceend[/chanstart/chanend]]}
        --verbose {-V}
        --window=windowmode/windowstart/windowend {-Wwindowmode/windowstart/windowend}

## Description

The output grid is written to `<root>.grd`. Each grid column holds one trace
(after decimation with `-D`), each row one time sample. Travel time is stored as
negative values, so that later arrivals lie lower in the section. Cells without
data are set to NaN.

If the range of traces (`-S`) or the time sweep (`-T`) is not given, the
program takes them from the `.sinf` file of the SEGY file so that all traces and
all samples are gridded. A missing or outdated `.sinf` file is created
automatically by running `mbsegyinfo -O`.

Before gridding, the traces can be processed: a time varying gain (`-G`), a
cosine low-pass filter (`-F`), automatic gain control (`-B`) and a time window
(`-W`) that keeps only the samples of interest. With `-R` the traces are
projected onto a straight line between two positions and binned by distance
along that line instead of by trace number. With `-A` the axes are rescaled to
distance and depth in meters.

## Common options

- `-I<file>` — Input SEGY file. Required.
- `-O<root>` — Output name root; the grid is written to `<root>.grd`. Required.
- `-S<mode>[/<start>/<end>[/<schan>/<echan>]]` — Traces to grid. Mode `0`:
  `start` and `end` are shot numbers; mode `1`: CMP (RP, CDP) numbers; mode
  `2`: shot numbers with all channels combined. `schan`/`echan` select
  channels of multichannel data. Default: all traces.
- `-T<sweep>[/<delay>]` — Length of the time range to grid in seconds, and its
  start time. Default: the full trace length.
- `-D<decimatex>/<decimatey>` — Reduce the number of traces and samples by
  averaging within each grid cell. Default: `1/1`.
- `-G<mode>/<gain>[/<window>[/<delay>]]` — Gain: mode `0` off, `1` from the
  start of the trace, `2` from the seafloor arrival. Each sample is multiplied
  by (1 + gain * t); `window` limits the gained interval (samples after it are
  set to zero) and `delay` shifts the gain start (seconds). Mode `3` scales each
  trace so that its maximum within `window` around the seafloor equals `gain`.
- `-B<maxvalue>[/<window>]` — Automatic gain control: scale each sample so that
  the maximum within a moving window of `window` seconds equals `maxvalue`.
  Without `window` the whole trace is normalized.
- `-F<mode>/<window>` — Low-pass filter; mode `1` is a cosine taper filter of
  `window` seconds.
- `-W<mode>/<start>/<end>` — Grid only part of each trace: mode `1` absolute
  times, `2` times relative to the seafloor arrival (start is often negative),
  `3` times relative to the sonar depth.
- `-R<distancebin>/<startlon>/<endlon>/<startlat>/<endlat>` — Grid by distance
  along the line between the two positions, in bins of `distancebin` meters.
- `-A<shotscale>/<timescale>` — Rescale x from shots to meters (shot spacing in
  m) and y from seconds to meters (typically 750 m/s for water).
- `-C<mode>` — Geometry: `0` vertical traces (default), `1` correct sample
  positions for the recorded pitch.
- `-V` — Verbose output with more progress messages.

## Examples

Grid all traces and samples of a SEGY file into `sbp_0001.grd`:

    mbsegygrid -I sbp_0001_env.segy -O sbp_0001

Grid shots 1500 to 2000 and 0.1 s of data starting 0.25 s after the shot:

    mbsegygrid -I sbp_0001_env.segy -O sbp_0001_part -S0/1500/2000 -T0.1/0.25

Apply a gain starting at the seafloor and keep only 0.02 s above to 0.15 s
below the seafloor:

    mbsegygrid -I sbp_0001_env.segy -O sbp_0001_gain -G2/5.0 -W2/-0.02/0.15

Show the section in meters, with 0.5 m shot spacing and 750 m/s:

    mbsegygrid -I sbp_0001_env.segy -O sbp_0001_m -A0.5/750

## Notes

- The program finishes by calling the `mbm_grdplot` plot macro, which is not
  available on Windows; this step is skipped with a notice and the grid is
  still written. Plot the grid with `gmt grdimage`.
- Output files are written to the current directory unless `-O` contains a
  path. Quote paths with spaces.

## See also

`mbextractsegy`, `mbsegyinfo`, `mbsegylist`, `mbsegypsd`, `gmt`. The complete
reference is the MB-System manual page mbsegygrid(1) (English).
