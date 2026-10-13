# mbauvloglist — List selected fields from MBARI AUV mission log files

`mbauvloglist` reads a binary MBARI AUV mission log file (for example `navigation.log`, `fastcatlog.log`, `kearfott.log`) and prints the fields you choose as a text or binary table. Use it to pull navigation, attitude, CTD or sound speed time series out of AUV logs, for instance to build a sound speed profile or an auxiliary navigation file for preprocessing.

## Synopsis

    mbauvloglist
        --altitude-file=file {-Afile}
        --angles-degrees {-S}
        --clip-to-nav {-C}
        --decimate=value {-Dvalue}
        --help {-H}
        --input=file {-Ifile}
        --lonflip=lonflip {-Llonflip}
        --nav-file=file {-Nfile}
        --output-field=name {-Oname}
        --output-mode=mode {-Mmode}
        --print-format=format {-Fformat}
        --print-header {-P}
        --recalculate-ctd=id {-Rid}
        --scale=value {-Xvalue}
        --verbose {-V}

## Description

MBARI AUV mission logs are binary files that start with an ASCII header. Each header line describes one value of the fixed-size binary record: its type, name, default print format, description and units, for example `# double calculated_sound_velocity %8.8e ,Calculated sound velocity ,M/s`.

To see which values a log contains, run the program with `-P` alone: it prints the header and exits. Then run it again with one `-O` option per value, in the column order you want. If no `-O` is given, every field in the log is printed. Output goes to standard output; redirect it to a file with `>`.

Values are printed with the format from the log header unless `-F` sets a printf-style format; `-F` applies to all following `-O` fields until it is changed again (`-Fdefault` returns to the header formats). For the record time field (named `time` in the logs), `-Ftime_i` prints year, month, day, hour, minute, second and `-Ftime_j` prints year, day of year, hour, minute, second. In the same way, `-X` sets a scale factor applied to the following floating point `-O` fields.

Besides the fields defined in the header, special values can be requested: `zero`, `timeTag`, `timeInterval` (time since the previous record), values interpolated from an external navigation file given with `-N` (`mergeLon`, `mergeLat`, `mergeHeading`, `mergeSpeed`, `mergeDraft`, `mergeSensordepth`, `mergeRoll`, `mergePitch`, `mergeHeave`), `mergeAltitude` from an altitude file given with `-A`, CTD values calculated from `fastcatlog.log` (`calcConductivity`, `calcTemperature`, `calcPressure`, `calcSalinity`, `calcSoundspeed`, `calcPotentialTemperature`, `calcDensity`), and Kearfott/Phins derived values (`calcKTime`, `calcKSpeed`, `calcPSpeed`).

## Common options

- `-Ifile` — Input MBARI AUV mission log file.
- `-P` — Print the log file header. Used alone, it lists the available fields and exits.
- `-Oname` — Output the named field. Repeat for each column, in the desired order.
- `-Fformat` — printf-style format for the following `-O` fields; `time_i` or `time_j` for the time field; `default` restores the header format.
- `-Mmode` — Output mode: `0` tab-delimited text (default), `1` comma-delimited text, `2` binary values with no separators.
- `-Nfile` — Navigation file in "fnv" format whose interpolated values are available as the `merge...` fields.
- `-C` — With `-N`, do not output records whose times are outside the time span of the navigation. By default such records are output with merged values of zero.
- `-Afile` — ASCII altitude file (one "time altitude" pair per line, seconds and meters) whose interpolated values are available as `mergeAltitude`.
- `-Dvalue` — Output only every value-th record. Default: `1` (all records).
- `-Xvalue` — Scale factor multiplied with the following floating point `-O` fields. Default: `1.0`.
- `-S` — Convert the angle fields of Kearfott, Crossbow and Phins logs from radians to degrees.
- `-Rid` — For `fastcatlog.log`, recalculate temperature, conductivity, salinity and pressure with the calibration coefficients identified by id (only `1` is defined).
- `-Llonflip` — Longitude range: `-1` for -360 to 0, `0` for -180 to 180, `1` for 0 to 360. Default: the value set with `mbdefaults`.
- `-V` — Verbose output.
- `-H` — Print help and exit.

## Examples

Show which fields are stored in a navigation log:

    mbauvloglist -I navigation.log -P

Extract pressure and sound speed from a CTD log into a text file:

    mbauvloglist -I fastcatlog.log -F%f -O pressure -O calculated_sound_velocity > auv_ssv.txt

List date and time, latitude, longitude and depth from a navigation log as CSV:

    mbauvloglist -I navigation.log -M1 -Ftime_i -O time -Fdefault -O latitude -O longitude -O mDepth > auv_nav.csv

Merge external navigation into a Kearfott log and keep only records covered by the navigation:

    mbauvloglist -I kearfott.log -N mission_nav.fnv -C -O time -O mergeLon -O mergeLat -O mHeadK -S

## Notes

- In a batch file (`.bat`/`.cmd`), write a percent sign in a format as `%%`, e.g. `-F%%f`; at the interactive prompt `-F%f` works as shown.
- The output is unsorted, in recording order. To build a sound speed profile from a CTD log, sort the output by pressure and resample it with your own tools.
- The `calc...` CTD values are only defined for `fastcatlog.log`, and `calcKTime`/`calcKSpeed` only for `kearfott.log`.

## See also

`mbpreprocess`, `mbnavlist`, `mblist`, `mbctdlist`, `mbsvplist`. The complete reference is the MB-System manual page mbauvloglist(1) (English).
