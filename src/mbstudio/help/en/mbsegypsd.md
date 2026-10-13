# mbsegypsd — Compute power spectral density of SEGY traces as a grid

`mbsegypsd` calculates the power spectral density (PSD) of every trace in a
SEGY file and writes the result as a GMT netCDF grid, with trace number along
the x axis and frequency along the y axis (a sonogram). Use it to look at the
frequency content of subbottom profiler or hydrophone data, for example to
identify noise sources or to check the source signature.

## Synopsis

    mbsegypsd
        --decimate=decimatex {-Ddecimatex}
        --fft-length=nfft {-Nnfft}
        --help {-H}
        --input=file {-Ifile}
        --log-scale {-L}
        --output=root {-Oroot}
        --shot-scale=shotscale[/frequencyscale] {-Ashotscale[/frequencyscale]}
        --time-sweep=sweep[/delay] {-Tsweep[/delay]}
        --trace-range=mode[/start/end[/schan/echan]] {-Smode[/start/end[/schan/echan]]}
        --verbose {-V}
        --window=mode/start/end {-Wmode/start/end}

## Description

For each trace the PSD is estimated by ensemble averaging: the trace is cut
into segments of `nfft` samples (`-N`, default 1024), each segment is
multiplied by a Hanning (cosine) taper, a fast Fourier transform is computed,
and the spectra of all segments are averaged. The estimates are normalized for
the signal lost by the taper. The frequency axis runs from 0 Hz to the Nyquist
frequency 1/(2 * sample interval), in `nfft`/2 + 1 steps.

Two files are written:

- `<root>.grd` — the PSD grid, in Intensity/Hz, or in dB/Hz
  (20 * log10 of the PSD) when `-L` is given;
- `<root>_psd.txt` — the PSD averaged over all traces, as text lines of
  frequency and PSD value.

If the range of traces (`-S`) or the time sweep (`-T`) is not given, all traces
and samples are used; the limits are read from the `.sinf` file of the SEGY
file, which is created automatically with `mbsegyinfo -O` when it is missing or
out of date.

## Common options

- `-I<file>` — Input SEGY file. Required.
- `-O<root>` — Output name root for the `.grd` and `_psd.txt` files. Required.
- `-N<nfft>` — FFT length in samples. Default: 1024.
- `-L` — Write the grid in dB/Hz instead of linear units.
- `-S<mode>[/<start>/<end>[/<schan>/<echan>]]` — Traces to process. Mode `0`:
  `start` and `end` are shot numbers; mode `1`: CMP (RP, CDP) numbers.
  `schan`/`echan` select channels of multichannel data. Default: all traces.
- `-T<sweep>[/<delay>]` — Length and start time of the time range used, in
  seconds.
- `-W<mode>/<start>/<end>` — Use only part of each trace: mode `1` absolute
  times, `2` times relative to the seafloor arrival, `3` times relative to the
  sonar depth.
- `-D<decimatex>` — Trace decimation for the output grid. Default: 1.
- `-A<shotscale>/<frequencyscale>` — Rescale the x axis from shot number to
  distance using the shot spacing in meters. Both values must be given.
- `-V` — Verbose output with more progress messages.

## Examples

Compute a sonogram in dB/Hz with a 1024 point FFT (writes `testpsd.grd` and
`testpsd_psd.txt`):

    mbsegypsd -I hydrophone_ch025.segy -N1024 -L -O testpsd -V

Use a longer FFT for finer frequency resolution and process shots 100 to 500
only:

    mbsegypsd -I sbp_0001_env.segy -N4096 -L -S0/100/500 -O sbp_0001_psd

## Notes

- The program finishes by calling the `mbm_grdplot` and `mbm_xyplot` plot
  macros, which are not available on Windows; these steps are skipped with a
  notice and the output files are still written. Plot the grid with
  `gmt grdimage` and the average spectrum with `gmt plot`.
- The `frequencyscale` value of `-A` is recorded but does not change the
  frequency axis.

## See also

`mbsegygrid`, `mbsegyinfo`, `mbsegylist`, `mbextractsegy`, `gmt`. The complete
reference is the MB-System manual page mbsegypsd(1) (English).
