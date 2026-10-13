# mbbackangle — make amplitude/sidescan vs grazing angle correction tables

`mbbackangle` calculates how beam amplitude and sidescan values vary with the grazing angle on the seafloor and writes these functions as correction tables. `mbprocess` then uses the tables to flatten the strong angular dependence of backscatter, so that mosaics made with `mbmosaic` show seafloor reflectivity rather than the swath geometry.

## Synopsis

    mbbackangle
        --altitude=altitude {-Zaltitude}
        --angle-bins=nangles/angle_max {-Nnangles/angle_max}
        --beam-pattern-mode=mode[/beamwidth/depression] {-Bmode[/beamwidth/depression]}
        --data-kind=kind {-Akind}
        --dump-output {-D}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --output-grid=gridmode/angle/min/max/n_columns/n_rows {-Ggridmode/angle/min/max/n_columns/n_rows}
        --pings-average=pings {-Ppings}
        --reference-angle=refangle {-Rrefangle}
        --skip-existing {-S}
        --slope-correction {-Q}
        --symmetric-correction {-C}
        --topography-grid=topogridfile {-Ttopogridfile}
        --verbose {-V}

## Description

The program reads swath files containing beam amplitude and/or sidescan data (one file or all files of a datalist), bins the values by grazing angle and averages them. The swath from `-angle_max` to `+angle_max` is divided into `nangles` bins; outside the nadir region (more than 15 degrees from vertical) three neighbouring bins are averaged for each value. Each table is the average over a number of pings (default 50) and is time-tagged with the mean ping time, so a file normally gets many tables.

By default the grazing angle is computed from the acrosstrack distance and depth of each beam or pixel, assuming a flat seafloor. With `-Q` the acrosstrack slope from the swath bathymetry is taken into account, and with `-T` the slope is taken from a topography grid instead. Where no bathymetry is available, a flat seafloor at the altitude given with `-Z` is assumed; without `-Z` such data are not used.

The tables are written next to each swath file, named after it with `.aga` (amplitude) and `.sga` (sidescan) appended. The `mbprocess` parameter file of the swath file is created or updated to enable amplitude and/or sidescan correction with these tables (`AMPCORRFILE`, `SSCORRFILE` and related parameters). When `mbprocess` runs, it interpolates the tables in time and grazing angle. For data in dB the correction is subtracted and the table value at the reference angle (`-R`) is added back; for linear data the value is divided by the correction and multiplied by the reference value.

With `-G` the program also writes, for each swath file, a GMT grid holding the normalised histogram of values versus grazing angle (named after the swath file with `_aga.grd` or `_sga.grd` appended), which is useful to check the tables.

## Common options

- `-I<file>` / `--input=<file>` — swath file or datalist. Default: `datalist.mb-1`.
- `-A<kind>` / `--data-kind=<kind>` — process beam amplitude (`1`) or sidescan (`2`). Give the option twice for both. Default: both.
- `-N<nangles>/<angle_max>` / `--angle-bins=...` — number of angle bins and maximum angle in degrees; use an odd number so that one bin is centred on 0. Default: `81/80.0` (2 degree bins).
- `-P<pings>` / `--pings-average=<pings>` — number of pings averaged for each table. Default: 50.
- `-Q` / `--slope-correction` — use acrosstrack slopes from the swath bathymetry in the grazing angle calculation.
- `-T<grid>` / `--topography-grid=<grid>` — use slopes from this topography grid (for example made with `mbgrid`) instead of the swath bathymetry.
- `-Z<altitude>` / `--altitude=<altitude>` — default sonar altitude in metres, assuming a flat seafloor, for data without bathymetry.
- `-R<refangle>` / `--reference-angle=<refangle>` — reference angle used by `mbprocess` when applying the correction. Default: 30 degrees.
- `-C` / `--symmetric-correction` — force the tables to be symmetric about vertical. Usually it is better to leave the tables asymmetric and set symmetry later with `mbset` (`AMPCORRSYMMETRY`, `SSCORRSYMMETRY`).
- `-B<mode>[/<beamwidth>/<depression>]` / `--beam-pattern-mode=...` — `0`: empirical tables of the observed averages (default); `1`: parametric model of a Gaussian beam pattern with spherical spreading, with effective beam width and depression angle in degrees (defaults 50 and 20).
- `-G<kind>/<angle>/<min>/<max>/<n_columns>/<n_rows>` / `--output-grid=...` — write a histogram grid for amplitude (`kind` 1) or sidescan (`kind` 2) from `-angle` to `+angle` and values from `min` to `max`; give it twice for both.
- `-S` / `--skip-existing` — skip swath files whose correction tables already exist and are up to date.
- `-D` / `--dump-output` — write the tables to the screen (stdout) instead of `.aga`/`.sga` files.
- `-F<format>` / `--format=<format>` — MBIO format id; normally inferred from the file name.
- `-V` / `--verbose` — report progress and the number of tables written.

## Examples

Make sidescan and amplitude tables for all files of a datalist with 2 degree bins averaged over 50 pings:

    mbbackangle -Idatalist.mb-1 -N81/80.0 -P50 -V

Make sidescan tables only, taking seafloor slope from a bathymetry grid, and skip files already done:

    mbbackangle -Idatalist.mb-1 -A2 -Tsurvey_bathy.grd -S -V

Also write histogram grids for checking the tables of one file:

    mbbackangle -I0003_20020425_022926.mb57 -G1/80/60/81/121 -G2/80/60/81/121 -V

Apply the corrections:

    mbprocess -Idatalist.mb-1

## Notes

- Formats without amplitude or sidescan data are skipped with a message.
- With `-G` the program also tries to run the `mbm_grdplot` script to make a plot; MB-System scripts are not available on Windows, so this step fails (the command prompt reports that the command is not recognized), but the grids themselves are written and can be plotted with GMT.
- In `-G`, `min` may be omitted (`kind/angle/max/n_columns/n_rows`); it then defaults to 0.
- Run it on the raw swath files (a datalist without the `$PROCESSED` directive): the tables are attached to the parameter files of the files actually read, and `mbprocess` uses the parameter files of the raw files.

## See also

`mbprocess`, `mbset`, `mbfilter`, `mbmosaic`, `mbgrid`, `mbpreprocess`. The complete reference is the MB-System manual page mbbackangle(1) (English).
