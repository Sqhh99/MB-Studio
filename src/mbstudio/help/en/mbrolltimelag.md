# mbrolltimelag — Estimate roll time lag by correlating roll with seafloor slope

`mbrolltimelag` estimates the time lag (time offset) between the ping time stamps of swath bathymetry and the roll data used to calculate it. It cross-correlates the roll time series with the apparent across-track seafloor slope; a clear correlation peak at a non-zero lag indicates an attitude timing problem. Use it during quality control when the outer swath shows "ruffles" that follow the vessel roll.

## Synopsis

    mbrolltimelag
        --correlation-threshold=value {-Cvalue}
        --format=format_id {-Fformat_id}
        --help {-H}
        --input=swathdata {-Iswathdata}
        --lag-range=nlag/lagmin/lagmax {-Tnlag/lagmin/lagmax}
        --nav-channel=navchannel {-Snavchannel}
        --npings=nping {-Nnping}
        --output=outputname {-Ooutputname}
        --roll-source=rollsource {-Krollsource}
        --verbose {-V}

## Description

If roll is applied with the wrong time stamps, each across-track profile is tilted by the roll error, so the bathymetry is too shallow on one side and too deep on the other. `mbrolltimelag` removes the applied roll from the apparent across-track bottom slope and cross-correlates the result with the roll signal for a range of time lags. Over a reasonably flat seafloor the slope signal is dominated by the roll error, and the cross correlation has a distinct peak at the actual time lag.

The input is a single swath file or a datalist. The roll time series is first extracted from the whole input with `mbnavlist`, and the slope of each ping is extracted with `mblist`; both programs are run automatically. The data are then processed in blocks of `nping` pings: for each block a cross correlation function is calculated for `nlag` lags evenly spaced from `lagmin` to `lagmax` seconds, and the lag of the correlation peak is kept as an estimate when the peak correlation exceeds the threshold set with `-C`.

By default the output file root is the input name without its MB-System suffix (for `datalist.mb-1` the root is `datalist`). The program writes:

- `root_timelagest.txt` — the accepted estimates: time (epoch seconds) and time lag (seconds).
- `root_timelaghist.txt` — all accepted time lag values, for a histogram.
- `root_timelagmodel.txt` — one time lag per input file (time and lag) when the file gives a well-defined histogram peak.
- `root_xcorr.txt` — all cross correlation functions (only when the input is a datalist).

For each swath file it also writes `_xcorr.txt` and `_timelaghist.txt` files named after, and next to, the swath file. A clear, consistent time lag can then be corrected, for example with the time latency options of `mbpreprocess`.

## Common options

- `-Iswathdata` — Input swath file or datalist (long form `--input`). Default: `datalist.mb-1`.
- `-Fformat_id` — MB-System format id of the input (`--format`); a negative value means a datalist. Default: inferred from the file name suffix (e.g. `.mb88`, `.mb-1`).
- `-Nnping` — Number of pings used for each cross correlation estimate (`--npings`). Default: 100.
- `-Tnlag/lagmin/lagmax` — Number of time lags and the lag range in seconds (`--lag-range`). Default: `41/-2.0/2.0`, i.e. a 0.1 s step.
- `-Cvalue` — Minimum peak correlation coefficient for an estimate to be accepted (`--correlation-threshold`). Default: 0.9.
- `-Krollsource` — Take roll from data records of this kind (`--roll-source`), e.g. 18 for asynchronous attitude records, 1 for survey records. By default roll is taken from the survey data records.
- `-Snavchannel` — Take roll from asynchronous navigation records instead, from navigation channel `navchannel` (1, 2 or 3) (`--nav-channel`). Run `mbinfo -N` to see which record types a file contains.
- `-Ooutputname` — Root of the output file names (`--output`).
- `-V` — Verbose output: prints the parameters and the peak correlation of every block (`--verbose`).
- `-H` — Print a short description and the usage, then exit (`--help`).

## Examples

Estimate the roll time lag for all files in a datalist, with lags from -2 to 2 s in 0.1 s steps and roll taken from navigation channel 2:

    mbrolltimelag -Idatalistp.mb-1 -T41/-2.0/2.0 -S2 -V

Check a single Reson 7k file using the asynchronous attitude records, 200 pings per estimate, a finer lag range of -0.5 to 0.5 s, and the output root `line0001`:

    mbrolltimelag -I0001_20250101_000000.mb88 -K18 -N200 -T51/-0.5/0.5 -Oline0001

## Notes

- On Windows the plots normally generated with the `mbm_xyplot` and `mbm_histplot` macros are skipped (the macros are not available); the text result files are still written and can be plotted with any spreadsheet or plotting tool.
- `mbnavlist` and `mblist` must be available; they are installed in the same `bin` folder.
- Results are only meaningful over a fairly flat seafloor and with good data coverage. Use a lag range comfortably wider than the expected lag.
- Write negative values attached to the option (`-T41/-2.0/2.0`) so they are not taken for another option.

## See also

`mbnavlist`, `mblist`, `mbinfo`, `mbpreprocess`, `mbprocess`. The complete reference is the MB-System manual page mbrolltimelag(1) (English).
