# mblevitus — Create a mean sound velocity profile from the Levitus atlas

`mblevitus` creates a mean annual water sound velocity profile for a location, using temperature and salinity from the Levitus (1982) Climatological Atlas of the World Ocean. Use it when no measured sound velocity profile (CTD, XBT or SVP cast) is available and bathymetry must be recalculated by raytracing, or as a reasonable reference profile to compare against measured casts.

## Synopsis

    mblevitus
        --help {-H}
        --location=lon/lat {-Rlon/lat}
        --output-file=file {-Ooutfile}
        --verbose {-V}

## Description

The Levitus database has a resolution of 1 degree by 1 degree. `mblevitus` selects the cell containing the requested location and reports both the requested position and the cell center actually used (for example, a request for -95/2 uses the cell centered at 265.5 longitude, 2.5 latitude). The profile represents the mean annual water column of that cell.

Sound velocities are calculated from temperature and salinity using the DelGrosso equation, with pressure computed from depth and latitude. Velocities are given at standard depths from 0 to 12000 m. Where the database stops above 12000 m (it reflects regional water depths), the profile is extended to 12000 m using the deepest available temperature and salinity values. If the location is on land, no profile exists and no output file is created.

The output file is plain text: a block of `#` comment lines describing the origin of the profile, followed by one `depth velocity` pair per line (meters, meters/second). This is the sound velocity profile format read by `mbset`/`mbprocess` (parameter `SVPFILE`) for raytracing.

The program prints the requested and used location, the number of values taken directly from the database and the number extrapolated, and the output file name. With `-V` it also prints the complete depth, temperature, salinity and velocity table.

## Common options

- `-Rlon/lat` — Longitude and latitude of the profile in degrees (long form `--location=lon/lat`). West longitudes and south latitudes are negative. Whole degrees are sufficient because of the 1 degree resolution. Default: 0/0.
- `-Ooutfile` — Output file for the sound velocity profile (long form `--output-file=file`). Default: `velocity` in the current directory.
- `-V` — Verbose mode (long form `--verbose`): also prints the program version and the full water column table.
- `-H` — Prints a short description and the usage, then exits (long form `--help`).

## Examples

Create a mean annual profile for 95 W, 2 N and write it to `velocity_profile.svp`, printing the full table:

    mblevitus -R-95/2 -Ovelocity_profile.svp -V

Create a profile for a survey in the North Sea (3 E, 56 N) using the long option forms:

    mblevitus --location=3/56 --output-file=northsea_levitus.svp

Use the profile to recalculate bathymetry by raytracing for all files in a datalist, then reprocess:

    mbset -I datalist.mb-1 -PSVPFILE:northsea_levitus.svp
    mbprocess -I datalist.mb-1

## Notes

- The database file `LevitusAnnual82.dat` is installed in the `share\mbsystem` folder of the MB-Studio installation and is found automatically from the location of `mblevitus.exe`. If the environment variable `MBSYSTEM_SHAREDIR` is set, the database is read from that folder instead.
- A climatological mean profile is only an approximation; measured profiles collected during the survey should be preferred whenever available.
- Write the location with the value attached to the option (`-R-95/2`) so that a negative longitude is not mistaken for another option.

## See also

`mbset`, `mbprocess`, `mbsvplist`, `mbsvpselect`, `mbabsorption`. The complete reference is the MB-System manual page mblevitus(1) (English).
