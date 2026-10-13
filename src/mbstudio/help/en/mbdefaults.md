# mbdefaults — Set or list the default MBIO control parameters

`mbdefaults` lists or changes the user's default MB-System (MBIO) control parameters, such as the longitude range convention, the time gap that marks a data gap, the I/O buffer size and the version of "fast bathymetry" (`.fbt`) files. Run it without options to see the current defaults; give options to store new values for all later MB-System programs.

## Synopsis

    mbdefaults
        --fbt-version=fbtversion {-Ffbtversion}
        --file-io-buffer=fileiobuffer {-Bfileiobuffer}
        --help {-H}
        --image-display=imagedisplay {-Iimagedisplay}
        --lonflip=lonflip {-Llonflip}
        --mbview-settings=mbviewsettings {-Mmbviewsettings}
        --project=mbproject {-Wmbproject}
        --ps-display=psdisplay {-Dpsdisplay}
        --time-gap=timegap {-Ttimegap}
        --use-lock-files=yes|no {-Uyes|no}
        --verbose {-V}

## Description

The defaults are stored in the text file `.mbio_defaults` in the user's home directory. On Windows this is the folder given by `HOME` if that variable is set, otherwise `%USERPROFILE%` (for example `C:\Users\name\.mbio_defaults`). If the file does not exist, the built-in MBIO defaults are used.

Without any setting option, `mbdefaults` only prints the current values. If any setting option is given, a new `.mbio_defaults` file is written that contains the new values together with all values that were not changed, and the new set of parameters is printed.

The defaults affect the programs that read swath data through MBIO, e.g. the longitude convention used by `mblist` or `mbgrid` and the gap detection in navigation. Older versions of MB-System also stored the format id, ping averaging, area and time windows here; these are no longer part of the defaults.

## Common options

- `-Llonflip` — Longitude range of returned values (`--lonflip`): `-1` gives -360 to 0, `0` gives -180 to 180, `1` gives 0 to 360 degrees. Default: 0.
- `-Ttimegap` — Maximum time between adjacent pings, in minutes, before the data are considered to have a gap (`--time-gap`). Default: 1.
- `-Bfileiobuffer` — File I/O buffer size in kilobytes for formats that support it (currently formats 58 and 88), e.g. `-B10000` (`--file-io-buffer`). Mainly useful when reading data over a network. Default: 0 (system default buffering).
- `-Ffbtversion` — Version of the `.fbt` files written by `mbdatalist` and `mbprocess` (`--fbt-version`): `3`, `new` or `NEW` for the current version, `2`, `old` or `OLD` for the pre-5.3.1897 version. Default: 3.
- `-Uyes|no` — Use lock files so that a swath file is processed or edited by only one program at a time (`--use-lock-files`); `yes`/`1` or `no`/`0`.
- `-Mmbviewsettings` — Default display settings of the 3D viewers (`--mbview-settings`): `-MPcolortable/mode/shade_mode`, `-MGcolortable/mode` (slope), `-MOcolortable/mode` (overlay), `-MImagnitude/elevation/azimuth` (illumination) or `-MSmagnitude` (slope shading). Colortables: 0 Haxby, 1 Bright, 2 Muted, 3 Grayscale, 4 Flat gray, 5 Sealevel 1, 6 Sealevel 2.
- `-Dpsdisplay` — Program used by plot scripts to display PostScript files (`--ps-display`). Default: `Default` (the system's default viewer).
- `-Iimagedisplay` — Program used to display image files (`--image-display`). Default: `Default`.
- `-Wmbproject` — Name of the default project (`--project`). Default: `none`.
- `-V` — Also print the program version (`--verbose`).
- `-H` — Print a short description and the usage, then exit (`--help`).

## Examples

List the current default parameters:

    mbdefaults

Return longitudes in the 0 to 360 degree range (useful for surveys crossing the 180 degree meridian):

    mbdefaults -L1

Set a 2 minute time gap and a 10 MB file I/O buffer:

    mbdefaults -T2 -B10000

## Notes

- The settings are personal: each Windows user account has its own `.mbio_defaults` file.
- Every change rewrites the whole file with the current values, so only the options you give are changed.
- In the Windows build the lock file mechanism is not used, so the `-U` setting has no practical effect there.
- The PostScript/image viewer and mbview settings are used only by tools that are not part of the Windows build (plot macros, 3D viewers).
- `-MS` must be given with an upper case `S`.

## See also

`mbinfo`, `mblist`, `mbgrid`, `mbdatalist`, `mbprocess`, `gmt mbswath`, `gmt mbcontour`, `gmt mbgrdtiff`. The complete reference is the MB-System manual page mbdefaults(1) (English).
