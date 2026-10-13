# mbgpstide — Make tide files from GPS ellipsoid heights in swath data

`mbgpstide` creates tide files from the GPS height (height above the ellipsoid) recorded in swath data files. The height can be shifted by a constant offset or corrected for a geoid model, so that the bathymetry is referenced to the ellipsoid or a geoid instead of the water level. Use it in place of a tide gauge or a tide model when the survey was run with RTK or PPK GPS heights, and let `mbprocess` apply the result.

## Synopsis

    mbgpstide [-Atideformat -Dinterval -Fformat -Idatalist -M -Ooutput -Roffset -S -Tgeoid -Usource -V -H]
    mbgpstide [--tideformat=tideformat --interval=interval --format=format --input=datalist
              --setparameters --output=output --offset=offset --skipexisting --geoid=geoid
              --use=source --verbose --help]

## Description

The program reads a single swath file or a datalist (default `datalist.mb-1`) and averages the GPS heights over fixed time intervals (default 300 s). Each value written to the tide file is the averaged height plus the `-R` offset, minus the geoid separation when `-T` is used.

The source of the height depends on the format. For Kongsberg (Simrad) EM formats 56, 57, 58 and 59 the height comes from the Height datagram. For GSF files (format 121) it is taken from the ping header; with `-U1` the ping's separation value (`sep`) is added, giving the height relative to the vertical datum of the GSF file. Other formats contain no usable height, and the resulting tide files stay empty.

Without `-O`, one tide file is written for each swath file, named like the swath file with `.gps.tde` appended and placed next to it. With `-O`, all values go into one file. `-M` updates the parameter file of each swath file so that `mbprocess` applies the new tide file; run `mbprocess` afterwards to make the correction take effect.

## Common options

- `-I datalist` — Input swath file or datalist (long form `--input=datalist`). Default: `datalist.mb-1`.
- `-F format` — MB-System format id of the input; a negative value means the input is a datalist (long form `--format=format`). Normally detected from the file name.
- `-D interval` — Time interval between tide values in seconds (long form `--interval=interval`). Default: 300. With `-D0` every height is written; then use `-A1` to avoid duplicate time stamps.
- `-A tideformat` — Output format (long form `--tideformat=tideformat`): 2 = `year month day hour minute second tide` (default), 1 = `time_d tide` (epoch seconds), 5 = CARIS style `yyyy/mm/dd hh:mm:ss.sss tide`. Do not combine 5 with `-M`, because `mbprocess` cannot read it.
- `-O output` — Write all tide values to this single file (long form `--output=output`); `-O-` writes to standard output (not with `-M`).
- `-M` — Set the swath files' processing parameters so that `mbprocess` applies the tide files (long form `--setparameters`).
- `-R offset` — Add this constant (m) to every tide value (long form `--offset=offset`).
- `-S` — Skip swath files whose `.gps.tde` file is already newer than the data (long form `--skipexisting`); only without `-O`.
- `-T geoid` — GMT grid of the geoid height above the ellipsoid (for example converted from EGM2008); the value along the track is subtracted (long form `--geoid=geoid`).
- `-U source` — Height source (long form `--use=source`): 0 = Kongsberg Height datagram or GSF ellipsoid height (default), 1 = GSF height above the vertical datum.
- `-V` — Verbose output (long form `--verbose`).
- `-H` — Print the usage message and exit (long form `--help`).

## Examples

Make a tide file for every file in the datalist and set them up for `mbprocess`:

    mbgpstide -I datalist.mb-1 -M
    mbprocess -I datalist.mb-1

Use 60 s averages and add a constant offset of -35.20 m, skipping files that already have up-to-date tide files:

    mbgpstide -I datalist.mb-1 -D60 -R-35.20 -S -M

Write a single tide file for the whole survey in epoch-seconds format:

    mbgpstide -I datalist.mb-1 -A1 -O survey_gps_tide.txt

Use GSF heights relative to the vertical datum for one file:

    mbgpstide -I line_0001.gsf -F121 -U1 -M

## Notes

- The tide files are written next to the swath files listed in the datalist, not in the current directory (unless `-O` is used).
- `-T` extracts the geoid values by running `awk` or `mblist` piped into GMT `grdtrack` through the system shell. These commands are not normally available under these names in the Windows command prompt, so `-T` may fail there; in that case apply the geoid separation as a constant with `-R` if it is nearly constant over the survey area.
- Quote paths that contain spaces.

## See also

`mbprocess`, `mbset`, `mbotps`, `mbdatalist`, `mblist`. The complete reference is the MB-System manual page mbgpstide(1) (English).
