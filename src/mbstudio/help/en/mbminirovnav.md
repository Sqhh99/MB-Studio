# mbminirovnav — Merge MBARI MiniROV logs into format 165 ROV navigation

`mbminirovnav` reads the four log files written by the MBARI MiniROV during a dive (USBL navigation, CTD, DVL and ROV attitude) and combines them into a single ROV navigation file in the MBARI ROV format, which MB-System reads as format 165 (MBF_MBARIROV). Use it to prepare MiniROV navigation for editing, plotting or merging with swath data.

## Synopsis

    mbminirovnav
        --help
        --input-ctd-file=file
        --input-dvl-file=file
        --input-nav-file=file
        --input-rov-file=file
        --interpolate-position
        --interval=seconds
        --output=file
        --rov-dive-start=yyyy/mo/da/hr/mn/sc
        --rov-dive-end=yyyy/mo/da/hr/mn/sc
        --utm-zone=zone_id/NorS
        --verbose

## Description

The MiniROV logs four comma-separated text files per UTC day, named `CTD_*.txt`, `DVL_*.txt`, `NAV_*.txt` and `ROV_*.txt` (logging in this form began with dive 100 in September 2017). All start with a time stamp in epoch seconds (UTC). The program uses:

- NAV: USBL positions passed on from the Winfrog software as `$GPGLL` strings (latitude and longitude in degrees and decimal minutes).
- CTD: the depth from the CTD pressure sensor.
- DVL: the altitude above the seafloor (non-zero only when the DVL has bottom lock); the DVL velocities are not used.
- ROV: heading, pitch and roll.

The values are interpolated onto regular output times, every `--interval` seconds (default 1 s), from `--rov-dive-start` to `--rov-dive-end`; without these options the whole time span of the input files is used. Each output record contains date and time, latitude, longitude, UTM easting and northing, depth, heading, altitude, pitch, roll and validity flags. Values that are missing, because a file was not given or has no data at that time, are flagged as invalid.

The NAV and ROV files are required: without data from both, no output file is written. The CTD and DVL files are optional. The UTM zone is computed from the average input position unless `--utm-zone` is given; the format 165 file does not record which zone was used.

The MiniROV logger repeats the most recent USBL fix about every two seconds, so unchanged positions mean no new fix. By default these repeated positions are kept. `--interpolate-position` discards repeated positions and fills the output times from the distinct fixes only. This is not recommended when the USBL data contain bad fixes (outliers), because they then affect the positions between the fixes; it is better to edit the fixes and interpolate afterwards.

## Common options

- `--input-nav-file=file` — MiniROV NAV log (USBL positions). Required.
- `--input-rov-file=file` — MiniROV ROV log (heading, pitch, roll). Required.
- `--input-ctd-file=file` — MiniROV CTD log (pressure depth).
- `--input-dvl-file=file` — MiniROV DVL log (altitude).
- `--output=file` — Output format 165 navigation file. Required; there is no default.
- `--interval=seconds` — Time step of the output records. Default: 1.0.
- `--rov-dive-start=yyyy/mo/da/hr/mn/sc` — Start time (UTC) of the output, e.g. `2017/09/05/16/54/00`.
- `--rov-dive-end=yyyy/mo/da/hr/mn/sc` — End time (UTC) of the output.
- `--utm-zone=zone_id/NorS` — UTM zone for easting and northing, e.g. `8/N` or `8N`. Default: from the average position.
- `--interpolate-position` — Drop repeated USBL positions and fill the output from the distinct fixes only (see above).
- `--verbose` — Print the parameters and a summary of input and valid output records.
- `--help` — Print the usage message and exit.

## Examples

Concatenate the day files of a cruise first, so that dives crossing midnight UTC are complete (Windows `copy` joins the files in name order):

    copy /b NAV_*.txt MiniROV_NAV_all.txt
    copy /b CTD_*.txt MiniROV_CTD_all.txt
    copy /b DVL_*.txt MiniROV_DVL_all.txt
    copy /b ROV_*.txt MiniROV_ROV_all.txt

Make the navigation for the on-bottom part of one dive in UTM zone 8 north:

    mbminirovnav --input-nav-file=MiniROV_NAV_all.txt --input-ctd-file=MiniROV_CTD_all.txt --input-dvl-file=MiniROV_DVL_all.txt --input-rov-file=MiniROV_ROV_all.txt --output=MiniROVnav_100.mb165 --utm-zone=8N --rov-dive-start=2017/09/05/16/54/00 --rov-dive-end=2017/09/05/17/56/00 --verbose

Write the same dive at 0.5 s intervals using only NAV and ROV data:

    mbminirovnav --input-nav-file=MiniROV_NAV_all.txt --input-rov-file=MiniROV_ROV_all.txt --output=MiniROVnav_100_nav.mb165 --interval=0.5 --rov-dive-start=2017/09/05/16/54/00 --rov-dive-end=2017/09/05/17/56/00

## Notes

- Only long options are accepted. The times must be written with slashes (`yyyy/mo/da/hr/mn/sc`), even though the built-in usage text shows `yyyymmddhhmmss`. The `--input` option listed in the usage text has no effect.
- The output file is written to the current directory unless a path is given. Quote paths that contain spaces.
- The interactive navigation editor `mbnavedit` mentioned in the manual page is not available on Windows.

## See also

`mbusbl2fnv`, `mbnavlist`, `mbpreprocess`, `mbinfo`. The complete reference is the MB-System manual page mbminirovnav(1) (English).
