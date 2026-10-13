# mbmapscale — Show meters per degree of longitude and latitude

`mbmapscale` prints the local scaling between geographic coordinates (longitude and latitude) and meters east and north at a given latitude. Use it when you need to convert between degrees and meters for a local Cartesian approximation, for example to choose a grid cell size in degrees that matches a spacing in meters, or to interpret offsets in a survey area.

## Synopsis

    mbmapscale
        --alvinxy {-A}
        --help {-H}
        --latitude=latitude {-Llatitude}
        --verbose {-V}

## Description

The scale is calculated at the latitude given with `-L` (decimal degrees, positive north, negative south). If no latitude is given, the scale is calculated at the equator (latitude 0.0).

By default the WGS72 ellipsoid is used, as in the local scaling routines of other MB-System programs. With `-A` the scale is instead calculated on the 1866 Clarke spheroid, as used by the AlvinXY local coordinate convention.

The result is written to standard output as four values: meters per degree of longitude, meters per degree of latitude, and the inverse values (labelled "Meters to degree longitude" and "Meters to degree latitude"), which are the number of degrees corresponding to one meter east and one meter north.

## Common options

- `-Llatitude` — Latitude in decimal degrees at which the scale is calculated (long form `--latitude=latitude`). Default: 0.0 (equator).
- `-A` — Use the 1866 Clarke spheroid of the AlvinXY convention instead of the WGS72 ellipsoid (long form `--alvinxy`).
- `-V` — Verbose mode (long form `--verbose`): also prints the program name and MB-System version.
- `-H` — Prints a short description and the usage, then exits (long form `--help`).

## Examples

Show the scale at 36.8 degrees north using the default WGS72 ellipsoid:

    mbmapscale -L36.8

The output has the form:

    Local scaling between degrees longitude and latitude and meters east and north:
        Using WGS72 ellipsoid
        Meters per degree longitude: 89244.230
        Meters per degree latitude:  110973.870
        Meters to degree longitude:  0.000011205
        Meters to degree latitude:   0.000009011

Show the scale at the same latitude using the AlvinXY (1866 Clarke spheroid) convention:

    mbmapscale -L36.8 -A

Show the scale at a southern latitude (negative value):

    mbmapscale --latitude=-41.5

## Notes

- Only latitude affects the result; no longitude is needed.
- Options may also be given in lower case (`-l`, `-a`, `-v`, `-h`).

## See also

`mbtime`, `mbgrid`, `mbmosaic`, `mbinfo`. The complete reference is the MB-System manual page mbmapscale(1) (English).
