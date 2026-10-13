# mbotps — Predict tides with OSU TPXO tidal models (OTPS methods)

`mbotps` predicts ocean tides from the global TPXO tidal models of Oregon State University (OSU), using the methods of the OSU Tidal Prediction Software (OTPS). It produces either a tide time series for one position and time range, or a tide file for every swath file along its navigation, optionally set up so that `mbprocess` applies the tide correction. Use it when no tide gauge or GPS tide is available for the survey.

## Synopsis

    mbotps
        [
        --input=datalist {-Idatalist}
        --format=format_id {-Fformat_id}
        --use-mbprocess {-M}
        --skip-existing {-S}
        --tide-output=file {-Ooutput}
        --tide-position=lon/lat {-Rlon/lat}
        --tide-format=format_id {-Atideformat}
        --interval=interval {-Dinterval}
        --start-time=year/month/day/hour/minute/second {-Byear/month/day/hour/minute/second}
        --end-time=year/month/day/hour/minute/second {-Eyear/month/day/hour/minute/second}
        --tide-station-file=file {-Ntidestationfile}
        --tide-station-position=lon/lat {-Ulon/lat}
        --tide-station-format=format_id {-Ctidestationformat}
        --otps-model=model_name {-Tmodel}
        --otps-path=path {-Ppath}
        --help {-H}
        --verbose {-V}
        ]

## Description

`mbotps` computes the tide itself (harmonic prediction with nodal corrections and inferred minor constituents, interpolated from the model grids); the OTPS Fortran programs are not needed. You must, however, obtain a TPXO tidal model from OSU (free for academic and other noncommercial use, licensed for commercial use) and install its data files under a `DATA` folder below the OTPS location given with `--otps-path`, for example `C:\otps\DATA\TPXO10_atlas_v2`. Only the `h_*` elevation files are read. Run `mbotps --help` to see the OTPS location in use and which models were found.

Position mode (no `--input`): the tide is predicted at the position given with `--tide-position` from `--start-time` to `--end-time` every `--interval` seconds (default 60) and written to `--tide-output` (default `tide_model.txt`).

Swath mode (`--input` given): for each swath file of the file or datalist, the tide is predicted along the navigation (the `.fnv` file is used when it exists) at the given interval and written to a file named like the swath file with `.tde` appended, next to the swath file. With `--use-mbprocess`, each file's processing parameters are set so that `mbprocess` applies this tide file. Positions outside the model grid or on land are skipped with a message.

Tide station correction: with `--tide-station-file`, `--tide-station-position` and `--tide-station-format`, observed tides from a nearby station are compared with the model at the station, and the difference is used as a time-varying correction of the predicted tides in either mode.

## Common options

- `--input=datalist` — Swath file or datalist; switches to swath mode (short form `-Idatalist`).
- `--format=format_id` — MB-System format of the input; normally detected from the file name, negative for a datalist (`-Fformat_id`).
- `--use-mbprocess` — Set the swath files' processing parameters so that `mbprocess` applies the `.tde` files (`-M`).
- `--skip-existing` — Skip swath files whose `.tde` file is already newer than the data (`-S`).
- `--tide-output=file` — Output file in position mode (`-Ooutput`). Default: `tide_model.txt`.
- `--tide-position=lon/lat` — Position for position mode, in decimal degrees (`-Rlon/lat`).
- `--start-time=year/month/day/hour/minute/second` — Start of the predicted time series (`-B...`).
- `--end-time=year/month/day/hour/minute/second` — End of the predicted time series (`-E...`).
- `--interval=interval` — Time step in seconds (`-Dinterval`). Default: 60.
- `--tide-format=format_id` — 2 = `yr mon day hour min sec tide` (default); 1, 3 or 4 = `time_d tide` with epoch seconds (`-Atideformat`).
- `--tide-station-file=file` — Observed tide station data used to correct the model (`-Ntidestationfile`).
- `--tide-station-position=lon/lat` — Position of the tide station (`-Ulon/lat`).
- `--tide-station-format=format_id` — Station file format: 1 = `time_d tide`, 2 = `yr mon day hour min sec tide` (default), 3 = `yr jday hour min sec tide`, 4 = `yr jday daymin sec tide` (`-Ctidestationformat`).
- `--otps-model=model_name` — Model to use: a model folder name under `DATA` (e.g. `TPXO10_atlas_v2`) or the name of a `Model_` control file (`-Tmodel`). Default: `tpxo10_atlas`; if the requested model is not installed, another installed model is used.
- `--otps-path=path` — OTPS location, the folder that contains `DATA` (`-Ppath`). The default is set when the program is built.
- `--verbose` — Verbose output, including the list of available models (`-V`).
- `--help` — Print the usage message, list the available models and exit (`-H`).

## Examples

Check where `mbotps` looks for tide models and which models it finds:

    mbotps --otps-path=C:\otps --help

Predict the tide at 121.9 W, 36.6 N for one day at 60 s intervals:

    mbotps --otps-path=C:\otps --tide-position=-121.9/36.6 --start-time=2025/02/21/00/00/00 --end-time=2025/02/21/23/59/59 --tide-output=tide_20250221.txt

Make tide files along the navigation of all files in a datalist and apply them with `mbprocess`:

    mbotps --otps-path=C:\otps --input=datalist.mb-1 --use-mbprocess --verbose
    mbprocess -I datalist.mb-1

Correct the model with observations from a nearby tide gauge (format 2 file):

    mbotps --otps-path=C:\otps --input=datalist.mb-1 --tide-station-file=gauge_2025.txt --tide-station-position=-121.89/36.61 --tide-station-format=2 --use-mbprocess

## Notes

- The options `--otps-path`, `--input`, `--tide-output` and `--tide-station-file` read their value only up to the first space, so use paths without spaces (e.g. `C:\otps`).
- If no usable model is found, the program stops with "Unable to find a valid OTPS tidal model"; check the `DATA` folder layout and the `--otps-path` value.
- The `.tde` files are written next to the swath files listed in the datalist, not in the current directory.
- The prediction contains only the tidal signal of the model constituents; local datums and non-tidal water level changes are not included (a tide station correction can reduce such differences).

## See also

`mbprocess`, `mbset`, `mbgpstide`, `mbdatalist`, `mbnavlist`. The complete reference is the MB-System manual page mbotps(1) (English).
