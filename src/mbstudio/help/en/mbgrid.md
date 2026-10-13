# mbgrid — Grid bathymetry, amplitude or sidescan data from swath files

`mbgrid` grids bathymetry, topography, beam amplitude or sidescan data from a set of swath sonar files listed in a datalist and writes a GMT grid (or another grid format). It is the standard way to make a digital terrain model from processed MB-System data, usually after editing and `mbprocess`, and the resulting grid is used for plotting, `gmt mbgrdtiff`, `mbgrd2gltf` or export to GIS.

## Synopsis

    mbgrid
        --algorithm=mode[/threshold] {-Fmode[/threshold]}
        --background-data=background {-Kbackground}
        --border=border {-Bborder}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --bounds=factor {-Rfactor}
        --clip=clip[/mode] {-Cclip[/mode]}
        --data-type=datatype {-Adatatype}
        --dimensions=xdim/ydim {-Dxdim/ydim}
        --extend=extend {-Xextend}
        --extra-grids {-M}
        --gaussian-scale=scale {-Wscale}
        --help {-H}
        --input=filelist {-Ifilelist}
        --longitude-domain=lonflip {-Llonflip}
        --output=root {-Oroot}
        --output-format=gridkind {-Ggridkind}
        --pings=pings {-Ppings}
        --projection=projection {-Jprojection}
        --shift=shiftx/shifty[/mode] {-Yshiftx/shifty[/mode]}
        --spacing=dx/dy/units[!] {-Edx/dy/units[!]}
        --speed-minimum=speed {-Sspeed}
        --tension=tension {-Ttension}
        --time-difference=time {-Utime}
        --use-feet {-Q}
        --use-nan {-N}
        --verbose {-V}

## Description

The input is a datalist (`-I`) that lists the swath files and their formats; for bathymetry, the compact `.fbt` file of each swath file is read instead when it exists. A datalist entry with format 0 is read as an ASCII file of longitude, latitude, value triples, so conventional soundings can be included. The output names are built from the root given with `-O`: the grid is `root.grd` (or `root.asc` for ASCII formats, `root.grd1` for the old binary format), and a datalist of all files that actually contributed to the grid is written as `root.mb-1`.

If the bounds (`-R`) or the grid size (`-D` or `-E`) are not given, `mbgrid` uses the region covering all data and a cell size of 0.02 times the maximum sonar altitude. These bounds come from the `.inf` file of each swath file; missing or outdated `.inf`, `.fbt` and `.fnv` files are generated automatically first (as `mbdatalist -N` would do). With explicit bounds, files whose `.inf` file shows no data inside the grid are skipped, which makes it cheap to keep one master datalist.

Eight gridding algorithms are available with `-F`: 1 Gaussian weighted mean (default), 2 median filter, 3 minimum filter, 4 maximum filter, 5 beam footprint with slope, 6 beam footprint, and the minimum or maximum weighted mean (`-F3/threshold`, `-F4/threshold`), which average only soundings within `threshold` meters of the shallowest or deepest sounding of a cell. Edited bathymetry is usually gridded with the weighted mean; unedited bathymetry, amplitude and sidescan with the median filter. The footprint algorithms (5, 6) apply to bathymetry only and allow cells smaller than the beam footprint. Hydrographers often use the minimum filter to keep the shallowest soundings.

Cells not filled by data can be filled by thin plate spline interpolation (`-C`, `-T`) and, for bathymetry, by a background grid (`-K`). Empty cells are written as 99999.9, or as NaN with `-N` (recommended for GMT and GIS use). By default grids are in geographic coordinates (WGS84 longitude and latitude); `-J` produces a grid in a projected coordinate system such as UTM, with eastings and northings in meters.

## Common options

- `-Ifilelist` — Datalist of the input files (`--input`). Default: `datalist.mb-1`.
- `-Oroot` — Root of the output file names (`--output`). Default: `grid`.
- `-Adatatype` — Data to grid (`--data-type`): 1 bathymetry (positive down, default), 2 topography (positive up), 3 beam amplitude, 4 sidescan.
- `-Edx/dy/units[!]` — Cell size (`--spacing`); units `meters` (default), `km`, `feet`, `degrees`, `arcmin` or `arcsec`. `dy` = 0 makes the cells square. Append `!` to keep the exact cell size by adjusting the bounds.
- `-Dxdim/ydim` — Number of grid nodes, used instead of a cell size (`--dimensions`). Default: 101/101.
- `-Rwest/east/south/north` — Grid bounds in degrees, or in eastings/northings when `-J` gives a fully specified projection (`--bounds`). `-Rfactor` (factor greater than 1) enlarges the automatically determined bounds, e.g. `-R1.1`.
- `-Fmode[/threshold]` — Gridding algorithm (`--algorithm`), see above. Default: 1.
- `-Cclip[/mode]` — Spline interpolation of empty cells up to `clip` cells from data (`--clip`). mode 1 (default) fills gaps only where data exist on opposite sides, mode 2 fills all cells within `clip` cells of data, mode 3 fills the whole grid. Default: no interpolation.
- `-Ttension` — Tension of the spline interpolation (`--tension`); 0 is a minimum curvature solution. Default: 0.
- `-Wscale` — Width of the Gaussian weighting function relative to the cell size (`--gaussian-scale`). Default: 1.0.
- `-N` — Write empty cells as NaN instead of 99999.9 (`--use-nan`).
- `-Jprojection` — Projected output grid (`--projection`): `-JU` chooses the UTM zone automatically, `-JUTM32N` / `-JUTM56S` give the zone, `-Jepsg32632` uses an EPSG code.
- `-Ggridkind` — Output grid format (`--output-format`). Default: GMT netCDF float (`nf`). Other GMT ids such as `bf`, `nd`, `ef` (ESRI ASCII), or the full form `=id/scale/offset/nan`; the old numeric ids 1 (ASCII), 2 (old GMT binary), 3 (netCDF) and 4 (Arc/Info ASCII grid) are still accepted. ESRI ASCII grids need equal cell sizes in x and y.
- `-M` — Also write `root_num.grd` (number of data per cell) and `root_sd.grd` (standard deviation per cell) (`--extra-grids`); ignored with the minimum or maximum filters.
- `-Utime` — Do not average overlapping swaths (`--time-difference`): in each cell, data more than `time` minutes from the first data are ignored; a negative value keeps the last data instead. Mainly for amplitude and sidescan.
- `-Xextend` — Enlarge the internal working grid by `extend` times the grid size on each side, so that data outside the grid guide the interpolation at the edges (`--extend`). Default: 0.
- `-Kbackground` — Fill remaining empty cells from a background GMT grid file (`--background-data`).
- `-Bborder` — Set empty border cells of an interpolated grid to the value `border` (`--border`).
- `-Q` — Grid bathymetry or topography in feet instead of meters (`--use-feet`).
- `-Llonflip` — Longitude range: -1, 0 or 1 (`--longitude-domain`).
- `-Ppings`, `-Sspeed` — Ping averaging and minimum speed in km/h of the input data (`--pings`, `--speed-minimum`).
- `-Yshiftx/shifty[/mode]` — Shift the grid bounds (mode 1, default) or all input data (mode 2) by the given meters east and north (`--shift`).
- `-V` — Verbose output (`--verbose`). `-H` prints the usage (`--help`).

## Examples

Grid the bathymetry in a datalist with 5 m cells over the full data extent, marking empty cells as NaN:

    mbgrid -Idatalist.mb-1 -Osurvey_bathy -E5/5/meters -N -V

Make a UTM topography grid with 2 m cells, fill small gaps by spline interpolation, and enlarge the bounds by 10 %:

    mbgrid -Idatalist.mb-1 -Osurvey_utm -A2 -JU -E2/2/meters -R1.1 -C3/1 -N

Grid a fixed area with the minimum filter, keeping the shallowest sounding of each 10 m cell:

    mbgrid -Idatalistp.mb-1 -Ominimum_depth -R139.65/139.90/35.20/35.45 -E10/10/meters -F3 -N

Grid sidescan with the median filter, using only the first data in each cell from overlapping lines:

    mbgrid -Idatalistp.mb-1 -Osidescan -A4 -F2 -E1/1/meters -U10 -N

## Notes

- On Windows the plotting scripts (`root.grd.cmd`) normally produced with the `mbm_grdplot` macro are not created; view the grid in MB-Studio or plot it with GMT, e.g. `gmt mbgrdtiff`.
- Output files are written relative to the current directory unless `-O` contains a path. Quote paths that contain spaces, e.g. `-O"D:\Survey 2025\grids\area1"`.
- Grid the processed data: list the processed files (or use a datalist with `$PROCESSED`, e.g. one made by `mbdatalist -Z`) so that edits and corrections are included.
- Large clip values combined with clip mode 1 are very slow.

## See also

`mbdatalist`, `mbinfo`, `mbprocess`, `mbmosaic`, `gmt mbgrdtiff`, `gmt mbswath`, `mbgrd2gltf`, `mbmakedatalist`. The complete reference is the MB-System manual page mbgrid(1) (English).
