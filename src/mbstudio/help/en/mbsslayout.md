# mbsslayout — Lay out time series sidescan onto the seafloor (format 71)

`mbsslayout` converts raw, time-based sidescan sonar data (port and starboard time series, e.g. Edgetech `.jsf` files) into swath sidescan with uniformly spaced across-track pixels, by projecting each sample onto a flat bottom or onto a topography grid. The result is written to MB-System format 71 files that can then be corrected with `mbbackangle`, processed with `mbprocess` and mosaicked with `mbmosaic`.

## Synopsis

    mbsslayout [
        --verbose
        --help
        --input=datalist
        --format=format
        --platform-file=FILE
        --platform-target-sensor=SENSORID
        --output-source=record_kind
        --line-time-list=filename
        --line-route=filename
        --line-position-list=filename
        --line-name1=name
        --line-name2=name
        --line-range-threshold=value
        --topo-grid-file=filename
        --altitude-altitude
        --altitude-bottompick
        --altitude-bottompick-threshold=threshold[/blank]
        --altitude-topo-grid
        --channel-swap
        --swath-width=value
        --gain=value
        --interpolation=value
        --nav-file=filename
        --nav-file-format=format_id
        --nav-async=record_kind
        --sensordepth-file=filename
        --sensordepth-file-format=format_id
        --sensordepth-async=record_kind
        --altitude-file=filename
        --altitude-file-format=format_id
        --altitude-async=record_kind
        --heading-file=filename
        --heading-file-format=format_id
        --heading-async=record_kind
        --attitude-file=filename
        --attitude-file-format=format_id
        --attitude-async=record_kind
        --soundspeed-constant=value
        --soundspeed-file=filename
        --soundspeed-file-format=format_id
        --soundspeed-async=record_kind
        --time-latency-file=filename
        --time-latency-constant=value
        --time-latency-apply-nav
        --time-latency-apply-sensordepth
        --time-latency-apply-altitude
        --time-latency-apply-heading
        --time-latency-apply-attitude
        --time-latency-apply-all-ancilliary
        --time-latency-apply-survey
        --time-latency-apply-all
        --filter=value
        --filter-apply-nav
        --filter-apply-sensordepth
        --filter-apply-altitude
        --filter-apply-heading
        --filter-apply-attitude
        --filter-apply-all-ancilliary
        ]

## Description

Each sidescan sample is placed on the seafloor using its two-way travel time, its side (port or starboard), the sound speed, and the position and attitude of the sonar. The samples are binned into a regular across-track array centered below the sonar, averaged where several samples fall into one pixel, and small empty gaps can be filled by interpolation. The output swath has at most 4001 pixels. By default the swath width varies from ping to ping (W = 2.2 × sqrt(R² − A²), with R the maximum slant range and A the altitude) and the pixel size is W / 4000; `--swath-width` sets a constant width instead.

Without a topography model the sidescan is laid out on a flat bottom at the sonar altitude, which by default comes from the altitude recorded with the data; `--altitude-bottompick` instead picks the first bottom return in the port and starboard time series and uses their average. With `--topo-grid-file` each sample is projected onto a GMT topography grid (for example a grid made with `mbgrid` from simultaneous or earlier multibeam data), giving correct positions in rough terrain.

Navigation, sensor depth, altitude, heading, attitude and sound speed are taken from the input files by default. Each can be replaced from an external file (`--nav-file`, ...) or from another record type in the data (`--nav-async`, ...), time latency corrections and Gaussian time-domain smoothing can be applied, and a platform file (`mbmakeplatform`) can describe the offset between the navigation reference and the sidescan.

By default every input file gives one output file named after it: the input name without its suffix, followed by `_sidescan.mb71` (the part `sidescan` is set with `--line-name2`). When survey lines are defined with `--line-time-list`, `--line-route` or `--line-position-list`, the output is split by line into files named `Survey_sidescan_0000.mb71`, `Survey_sidescan_0001.mb71`, ... (`Survey` is set with `--line-name1`).

## Common options

- `--input=datalist` — Datalist or single file with the time series sidescan. Default: `datalist.mb-1`.
- `--format=format` — Format id of the input; by default inferred from the file name.
- `--output-source=record_kind` — Record type holding the sidescan to lay out: a record kind number such as 1 (survey data, default) or 38 (second sidescan), or `low` / `sidescan` (kind 1) and `high` (kind 38). Edgetech `.jsf` files usually hold the low frequency as kind 1 and the high frequency as kind 38; `mbinfo -N` lists the record types in a file.
- `--topo-grid-file=filename` — Lay the sidescan out onto this GMT topography grid.
- `--altitude-bottompick` — Use the first bottom return in the time series as altitude (flat bottom layout). `--altitude-bottompick-threshold=threshold[/blank]` sets the pick threshold as a fraction of the maximum amplitude (default 0.5) and an optional blanking time in seconds, and also enables bottom picking.
- `--altitude-altitude` — Use the recorded altitude (default for the flat bottom layout).
- `--swath-width=value` — Constant output swath width in meters.
- `--interpolation=value` — Fill gaps of up to `value` empty pixels by interpolation. Default: 0 (no interpolation).
- `--gain=value` — Apply a gain correction with this gain value.
- `--channel-swap` — Swap the port and starboard channels (see Notes).
- `--line-time-list=filename` — Split the output into lines at the times (epoch seconds, one per line) in this file, e.g. made with `mbroutetime`.
- `--line-route=filename` — Split the output at the waypoints of an MB-System route file. `--line-position-list=filename` uses a plain list of `longitude latitude` waypoints instead.
- `--line-range-threshold=value` — Distance in meters within which a waypoint counts as reached. Default: 50.
- `--line-name1=name`, `--line-name2=name` — Name parts of the output files. Defaults: `Survey` and `sidescan`.
- `--nav-file=filename`, `--nav-file-format=format_id` — Merge navigation from an external file (e.g. format 1 = `time_d lon lat speed`, 2 = `year month day hour minute second lon lat`, 6 or 7 = NMEA strings). `--nav-async=record_kind` takes navigation from another record type in the data (e.g. 12, 29, 30, 31).
- `--sensordepth-file`, `--altitude-file`, `--heading-file`, `--attitude-file`, `--soundspeed-file` — Likewise for sensor depth, altitude, heading, attitude and sound speed, each with matching `-file-format` and `-async` options.
- `--soundspeed-constant=value` — Constant water sound speed in m/s. Default: 1500.
- `--platform-file=FILE`, `--platform-target-sensor=SENSORID` — Platform file with the sensor offsets, and the id of the sidescan sensor in it.
- `--time-latency-constant=value` or `--time-latency-file=filename` — Time latency correction in seconds, applied to the data selected with `--time-latency-apply-nav`, `-sensordepth`, `-altitude`, `-heading`, `-attitude`, `-all-ancilliary`, `-survey` or `-all`.
- `--filter=value` — Length in seconds of a Gaussian smoothing filter for the ancillary data selected with `--filter-apply-nav`, `-sensordepth`, `-altitude`, `-heading`, `-attitude` or `-all-ancilliary`. Default: 0 (no filtering).
- `--verbose` — Print the parameters and progress. `--help` prints the usage.

## Examples

Lay out the low frequency sidescan of all Edgetech files in a datalist on a flat bottom, using the recorded altitude and filling gaps of up to 5 pixels:

    mbsslayout --input=datalist.mb-1 --output-source=low --interpolation=5 --verbose

Lay out the high frequency sidescan onto a bathymetry grid with a constant 150 m swath width:

    mbsslayout --input=datalist.mb-1 --output-source=high --topo-grid-file=survey_bathy.grd --swath-width=150 --line-name2=hf

Use bottom picking for the altitude, smooth the attitude over 2 s, and split the output into survey lines from a time list:

    mbsslayout --input=datalist.mb-1 --altitude-bottompick-threshold=0.5/0.01 --filter=2.0 --filter-apply-attitude --line-time-list=line_times.txt --line-name1=Area1

## Notes

- All options use the long `--name=value` form; there are no single letter options.
- The output files are written in format 71; make a datalist of them (e.g. with `mbdatalist` or `mbmakedatalist`) for further processing.
- When lines are defined, the output files are written to the current directory; otherwise next to each input file.
- A plot script `Survey_sidescan_ssswathplot.cmd` (named after `--line-name1` and `--line-name2`) is also written; it calls the `mbm_plot` macro, which is not available on Windows, and a message that `chmod` was not found can be ignored.
- In the current source `--channel-swap` expects a value: write it as `--channel-swap=1` (any value enables swapping).

## See also

`mbbackangle`, `mbprocess`, `mbmosaic`, `mbgrid`, `mbmakeplatform`, `mbroutetime`, `mbinfo`, `mbdatalist`, `mb7k2jstar`. The complete reference is the MB-System manual page mbsslayout(1) (English).
