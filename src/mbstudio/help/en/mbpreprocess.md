# mbpreprocess — Preprocess raw swath data and merge ancillary data

`mbpreprocess` is the first processing step for raw swath sonar data in MB-System. It reads the raw files (usually through a datalist), writes them in the MB-System processing format together with the ancillary files the other programs need, and can merge navigation, sensor depth, heading, attitude, altitude and sound speed from the data stream or from external files. Run it after `mbmakedatalist` and before editing (`mbclean`) and `mbprocess`.

## Synopsis

    mbpreprocess --input=datalist [--format=format_id]
        --verbose --help
        --output-directory=directory
        --platform-file=platform_file --platform-target-sensor=sensor_id
        --output-sensor-fnv --skip-existing
        --nav-file=file --nav-file-format=format_id
        --nav-async=record_kind --nav-sensor=sensor_id
        --sensordepth-file=file --sensordepth-file-format=format_id
        --sensordepth-async=record_kind --sensordepth-sensor=sensor_id
        --heading-file=file --heading-file-format=format_id
        --heading-async=record_kind --heading-sensor=sensor_id
        --altitude-file=file --altitude-file-format=format_id
        --altitude-async=record_kind --altitude-sensor=sensor_id
        --attitude-file=file --attitude-file-format=format_id
        --attitude-async=record_kind --attitude-sensor=sensor_id
        --attitude-zero-heave
        --soundspeed-file=file --soundspeed-file-format=format_id
        --soundspeed-async=record_kind --soundspeed-sensor=sensor_id
        --time-latency-file=file --time-latency-file-format=format_id
        --time-latency-constant=value
        --time-latency-apply-nav --time-latency-apply-sensordepth
        --time-latency-apply-heading --time-latency-apply-attitude
        --time-latency-apply-altitude --time-latency-apply-all-ancilliary
        --time-latency-apply-survey --time-latency-apply-all
        --filter=value
        --filter-apply-nav --filter-apply-sensordepth
        --filter-apply-heading --filter-apply-attitude
        --filter-apply-altitude --filter-apply-all-ancilliary
        --recalculate-bathymetry --no-change-survey
        --multibeam-sidescan-source=recordid
        --sounding-amplitude-filter=value --sounding-altitude-filter=value
        --ignore-water-column
        --head1-offsets=x/y/z/heading/roll/pitch
        --head2-offsets=x/y/z/heading/roll/pitch
        --kluge-time-jumps=threshold --kluge-fix-7k-timestamps=targetoffset
        --kluge-ancilliary-time-jumps=threshold
        --kluge-mbaripressure-time-jumps=threshold
        --kluge-beam-tweak=factor --kluge-soundspeed-tweak=factor
        --kluge-zero-attitude-correction --kluge-zero-alongtrack-angles
        --kluge-fix-wissl-timestamps --kluge-auv-sentry-sensordepth
        --kluge-ignore-snippets --kluge-sensordepth-from-heave
        --kluge-early-MBARI-Mapping-AUV
        --kluge-flipsign-roll --kluge-flipsign-pitch
        --kluge-set-beamwidths=beamwidth-acrosstrack/beamwidth-alongtrack
        --kluge-set-beamwidth-acrosstrack=beamwidth-acrosstrack
        --kluge-set-beamwidth-alongtrack=beamwidth-alongtrack
        --kluge-ignore-duplicate-pings
        --kluge-xducer-depth-from-heave
        --kluge-xducer-depth-from-sensordepth
        --kluge-xducer-depth-from-heave-and-sensordepth
        --kluge-rangescale=factor --kluge-fix-wissl2-ranges

## Description

`mbpreprocess` replaces the format-specific preprocessing programs of older MB-System versions. For each input file it works in two passes. Pass 1 reads the asynchronous ancillary data (navigation, sensor depth, heading, attitude, altitude, sound speed) from the data stream and from any external files. Pass 2 writes the survey data to a new file, merging the selected ancillary data with each ping, and creates the ancillary files used by later programs: the `.inf` statistics file (by running `mbinfo`), the `.fnv` navigation file, usually a `.fbt` bathymetry file, and time series files of attitude and sensor depth.

The output file has the input file name root with the suffix `.mbXX`, where XX is the output format id. Some formats are translated into an extended processing format: for example Kongsberg `.all` data (format 58) become format 59 (`0001_20250101_000000.mb59`), older Kongsberg data (56) become format 57, and Imagenex 83P data (191) become format 192. Other formats keep their format id (e.g. `.kmall` data become `.mb261`); if the output name would equal the input name, the letter `r` is added before the suffix. Output files are written next to the input files unless `--output-directory` is given.

By default the ancillary values already in the data are used. A different source can be selected for each kind of data, either an external time series file (`--nav-file` with `--nav-file-format`, and the same for sensordepth, heading, altitude, attitude and soundspeed) or another record type in the data stream (`--nav-async=record_kind` and so on). Merging navigation, sensor depth, heading, attitude or sound speed, or giving any `...-apply-...` option, also turns on recalculation of the bathymetry. For Kongsberg `.kmall` data the installation datagram is used automatically to choose the active navigation and attitude system and to build the sensor geometry.

A platform file (made with `mbmakeplatform`) is only needed when the sensor offsets are not already applied in the data, typically for AUV or ROV surveys; it is supported for a few formats such as 88, 89, 261 and the 3D at Depth lidar formats. The `--time-latency-...` options correct time offsets of the ancillary or survey data, and `--filter` smooths ancillary data with a Gaussian filter; if no `...-apply-...` option is given, both are applied to all ancillary data. The `--kluge-...` options are fixes for problems in particular datasets; read the manual page before using them.

## Common options

- `--input=file` — Swath file or datalist to preprocess. Always give it.
- `--format=format_id` — MBIO format of the input. Default: inferred from the file name suffix.
- `--output-directory=directory` — Write the output files to this directory instead of next to the input files.
- `--skip-existing` — Skip files whose output already exists and is newer and larger than the input.
- `--verbose` — Report progress (record counts of both passes).
- `--nav-file=file` — External navigation to merge, replacing the navigation in the data.
- `--nav-file-format=format_id` — Format of the navigation file: `1` epoch-seconds lon lat [speed]; `2` year month day hour minute second lon lat; `3` year julian_day hour minute second lon lat; `4` year julian_day day_minute second lon lat; `5` L-DEO nav; `6` or `7` NMEA 0183 (ZDA, GLL, GGA, ...); `8` Simrad 90; `9` MB-System fnv style (date and time, epoch seconds, lon, lat, heading, speed, sensordepth [, roll, pitch, heave]); `10` R2R; `11` RVDAS; `12` Kongsberg Navlab `navlab_smooth.bin`.
- `--nav-async=record_kind` — Record type in the data used for navigation, e.g. `12` (navigation), `29`, `30`, `31` (navigation systems 1-3), `1` (survey records).
- `--sensordepth-file=file`, `--heading-file=file`, `--attitude-file=file`, `--altitude-file=file`, `--soundspeed-file=file` — External time series to merge for these values; the matching `...-file-format=` option takes `1` (epoch seconds and values), `2` (year month day hour minute second and values), `3` (year julian_day hour minute second ...), `4` (year julian_day day_minute second ...), and for sensor depth, heading and attitude also `9`, `11` and `12` as for navigation. Attitude files contain roll, pitch and heave.
- `--sensordepth-async=`, `--heading-async=`, `--attitude-async=`, `--altitude-async=`, `--soundspeed-async=record_kind` — Record type used for these values, e.g. `59` sonar depth, `16` height, `17` heading, `18` attitude, `56`-`58` attitude systems 1-3, `60` altitude, `19` SSV, `34` CTD. Use `mbinfo -N` to see which record types a file contains.
- `--attitude-zero-heave` — Set all heave values to zero.
- `--platform-file=file` — Use this platform file for lever arm and angular offset corrections.
- `--platform-target-sensor=sensor_id` — Platform sensor that produced the survey data (with `--nav-sensor`, `--sensordepth-sensor`, `--heading-sensor`, `--attitude-sensor` for the ancillary sensors).
- `--time-latency-constant=value` — Constant time latency in seconds (or `--time-latency-file=` with `--time-latency-file-format=` 1-4 for a time-varying model), applied as selected with the `--time-latency-apply-...` options.
- `--filter=value` — Half width in seconds of a Gaussian smoothing filter for ancillary data, applied as selected with the `--filter-apply-...` options.
- `--recalculate-bathymetry` — Recalculate the bathymetry during preprocessing (format dependent).
- `--no-change-survey` — Do not modify the survey data.
- `--multibeam-sidescan-source=recordid` — Backscatter used for multibeam sidescan (formats 88, 89, 261): `S` snippets, `C` calibrated snippets, `B` wide beam backscatter, `W` calibrated wide beam backscatter. Default: the best available.
- `--ignore-water-column` — Do not copy water column records to the output (Reson/Teledyne 7k formats).
- `--help` — Print help and exit.

## Examples

Create a datalist of raw Kongsberg files and preprocess them, showing progress:

    mbmakedatalist -S .all
    mbpreprocess --input=datalist.mb-1 --verbose

Re-run on a growing dataset, skipping files that were already preprocessed, and put the output in an existing folder `preprocessed`:

    mbpreprocess --input=datalist.mb-1 --skip-existing --output-directory=preprocessed

Replace the navigation with a post-processed position file (lines of epoch seconds, longitude, latitude) and correct a constant 0.05 s navigation latency:

    mbpreprocess --input=datalist.mb-1 --nav-file=ppk_nav.txt --nav-file-format=1 --time-latency-constant=0.05 --time-latency-apply-nav

Preprocess Reson 7k data from an AUV with a platform file in which sensor 2 is the multibeam, sensor 0 the INS and sensor 1 the pressure sensor:

    mbpreprocess --input=datalist.mb-1 --platform-file=auv.plf --platform-target-sensor=2 --nav-sensor=0 --heading-sensor=0 --attitude-sensor=0 --sensordepth-sensor=1

## Notes

- `mbpreprocess` has only long options (`--name` or `--name=value`); there are no single-letter forms.
- The paths given with `--output-directory` and `--platform-file` must not contain spaces. Quote other file names with spaces, e.g. `--input="my survey\datalist.mb-1"`.
- The output directory must already exist.
- After preprocessing, make a datalist of the new files (for example `mbmakedatalist -S .mb59 -P -O datalistp.mb-1`) and continue with `mbclean`, `mbset` and `mbprocess`.
- The `.inf` files are created by calling `mbinfo`, so run the program from the MB-System Command Prompt or MB-Studio, where the MB-System `bin` folder is on the PATH.

## See also

`mbmakedatalist`, `mbdatalist`, `mbinfo`, `mbmakeplatform`, `mbclean`, `mbset`, `mbprocess`, `mbnavlist`. The complete reference is the MB-System manual page mbpreprocess(1) (English).
