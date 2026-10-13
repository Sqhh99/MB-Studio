# mbroutetime — List the times when survey data reached route waypoints

`mbroutetime` compares the navigation of swath data with a planned survey route and writes the position and time at which the survey passed each line start and line end waypoint. The resulting waypoint time list is used by `mbextractsegy` (option `-Q`) to split subbottom data into files that correspond to the planned survey lines.

## Synopsis

    mbroutetime  -Rroutefile [-Fformat -Ifile -Owaypointtimefile -Urangethreshold -H -V]

        --format=format_id {-Fformat_id}
        --help {-H}
        --input=file {-Ifile}
        --output=waypointtimefile {-Owaypointtimefile}
        --range-threshold=rangethreshold {-Urangethreshold}
        --route-file=routefile {-Rroutefile}
        --verbose {-V}

## Description

The route file is a text route file in the MBgrdviz format (`.rte`). For route files with a version header, only the line start and line end waypoints are used and transit waypoints are ignored; at least two such waypoints are required. Older route files without a version header use every point.

The program reads the swath data (a single file or a datalist, default `datalist.mb-1`) in order, using the `.fnv` navigation file of each swath file when it exists. The waypoints are searched one after the other: a waypoint counts as reached when the navigation comes within the range threshold (default 25 m) of it and the distance then starts to increase again, so the closest approach is recorded. If the data end before the last waypoint is reached, a final point is added at the last navigation position.

The output text file has one line per waypoint found, with the columns: sequence number, waypoint type (3 = line start, 4 = line end), longitude, latitude, heading and time in decimal epoch seconds (seconds since 1970-01-01 UTC). By default it is named like the input with `_wpttime_d.txt` appended, e.g. `datalist.mb-1_wpttime_d.txt`.

## Common options

- `-Rroutefile` — MBgrdviz route file with the planned waypoints (long form `--route-file=routefile`). Required.
- `-Ifile` — Swath file or datalist to check (long form `--input=file`). Default: `datalist.mb-1`.
- `-Fformat_id` — MB-System format of the input; normally detected from the file name, negative for a datalist (long form `--format=format_id`).
- `-Owaypointtimefile` — Output waypoint time list (long form `--output=waypointtimefile`). Default: the input name with `_wpttime_d.txt` appended.
- `-Urangethreshold` — Distance in meters within which a waypoint counts as reached (long form `--range-threshold=rangethreshold`). Default: 25.
- `-V` — Print status messages, including the distance to the active waypoint for every ping (long form `--verbose`).
- `-H` — Print the description and usage and exit (long form `--help`).

## Examples

Find the times when the survey in `datalist.mb-1` passed the waypoints of a route:

    mbroutetime -R Line_plan.rte -I datalist.mb-1 -O Line_plan_wpttime.txt

Allow the vehicle to pass up to 50 m from the waypoints:

    mbroutetime -R Line_plan.rte -I datalist.mb-1 -U50 -O Line_plan_wpttime.txt

Use the waypoint times to extract subbottom data into one SEGY file per planned line, named from `Line_plan` and numbered from 1:

    mbextractsegy -I datalist.mb-1 -Q Line_plan_wpttime.txt -L1/Line_plan

## Notes

- The waypoints must be passed in the order of the route. If one waypoint is missed (never approached within the threshold), the following waypoints are not found either; increase `-U` in that case.
- The route file is normally made with `mbgrdviz`, which is not available on Windows; route files made on another system can be used.
- Quote paths that contain spaces.

## See also

`mbextractsegy`, `mbnavlist`, `mbinfo`, `mbdatalist`. The complete reference is the MB-System manual page mbroutetime(1) (English).
