# mbmosaic — Mosaic amplitude or sidescan data from swath files

`mbmosaic` builds a gridded mosaic of beam amplitude or sidescan backscatter from the swath files listed in a datalist. Samples can be prioritized by apparent grazing angle, look azimuth or platform heading, so that, for example, the specular nadir region is suppressed or a mosaic looking in one direction is produced. It is normally run after the backscatter has been corrected with `mbbackangle` and `mbprocess`.

## Synopsis

    mbmosaic
        --altitude-default=bathdef {-Zbathdef}
        --border=border {-Bborder}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --bounds=factor {-Rfactor}
        --data-type=datatype {-Adatatype}
        --directional-priority=bearing/factor[/mode] {-Ubearing/factor[/mode]}
        --extend=extend {-Xextend}
        --extra-grids {-M}
        --grid-dimensions=xdim/ydim {-Dxdim/ydim}
        --grid-format=gridkind {-Ggridkind}
        --grid-spacing=dx/dy/units {-Edx/dy/units}
        --help {-H}
        --input=filelist {-Ifilelist}
        --longitude-domain=lonflip {-Llonflip}
        --output=root {-Oroot}
        --pings=pings {-Ppings}
        --priority-range=priority_range[/weight] {-Fpriority_range[/weight]}
        --priority-source=priority_source {-Ypriority_source}
        --projection=projection {-Jprojection}
        --speed-minimum=speed {-Sspeed}
        --spline-interpolation=clip/mode/tension {-Cclip/mode/tension}
        --topography-grid=topogridfile {-Ttopogridfile}
        --use-nan {-N}
        --verbose {-V}
        --weighting-scale=scale {-Wscale}

## Description

The input datalist (`-I`) and the output root (`-O`) work as in `mbgrid`: the mosaic is written to `root.grd` (or `root.asc` / `root.grd1` for the ASCII and old binary formats), and a datalist of the contributing files to `root.mb-1`. If the bounds or grid size are not given, the region covering all data and a cell size of 0.02 times the maximum sonar altitude are used; the bounds are taken from the `.inf` files, which are generated automatically when missing or outdated. Bathymetry cannot be mosaicked; grid it with `mbgrid`.

Each sample gets a priority that is the product of up to three factors: a priority as a function of apparent grazing angle (`-Y`, from a predefined table or a file), a look azimuth priority and a heading priority (`-U`). By default each cell takes the value of the single highest priority sample. With `-F` a Gaussian weighted mean of all samples whose priority is within `priority_range` of the highest priority in the cell is used instead; the optional `weight` additionally weights each sample by its priority (1) or its squared priority (2).

The grazing angle is the angle arctan(x/z) from the across-track distance x and the sonar altitude z: zero at nadir, negative to port and positive to starboard. A priority file has two columns, angle in degrees and priority between 0 and 1, from the port-most to the starboard-most angle; priorities are interpolated linearly in between and are zero outside the table.

Empty cells can be filled by thin plate spline interpolation (`-C`). They are written as 99999.9, or as NaN with `-N`. As with `mbgrid`, `-J` produces a mosaic in a projected coordinate system such as UTM.

## Common options

- `-Ifilelist` — Datalist of the input files (`--input`). Default: `datalist.mb-1`.
- `-Oroot` — Root of the output file names (`--output`). Default: `grid`.
- `-Adatatype` — Data to mosaic (`--data-type`): 3 beam amplitude, 4 sidescan (default), 5 flat bottom grazing angle, 6 across-track grazing angle, 7 across-track slope. Append `F` (e.g. `-A4F`) to use data filtered with `mbfilter` (`.ffa`/`.ffs` files).
- `-Edx/dy/units` — Cell size (`--grid-spacing`); units `meters` (default), `km`, `feet`, `degrees`, `arcmin` or `arcsec`. Append `!` to keep the exact cell size by adjusting the bounds.
- `-Dxdim/ydim` — Number of grid nodes, used instead of a cell size (`--grid-dimensions`). Default: 101/101.
- `-Rwest/east/south/north` — Mosaic bounds (`--bounds`); `-Rfactor` (greater than 1) enlarges the automatic bounds.
- `-Ypriority_source` — Grazing angle priorities (`--priority-source`): a number 1 to 8 for a predefined table (1-4 favour the outer swath up to ±60, 67, 75 or 85 degrees, 5-8 favour nadir) or the name of a priority file.
- `-Ubearing/factor[/mode]` — Directional priority (`--directional-priority`): mode 0 (default) prefers the look azimuth `bearing`, mode 1 prefers the platform heading `bearing`. The priority is cos(factor × difference), zero beyond 90 degrees / factor.
- `-Fpriority_range[/weight]` — Use a Gaussian weighted mean of the samples within `priority_range` of the highest priority (`--priority-range`). Default: highest priority sample only.
- `-Wscale` — Width of the Gaussian weighting function relative to the cell size (`--weighting-scale`). Default: 1.0.
- `-Cclip/mode/tension` — Spline interpolation (`--spline-interpolation`): fill empty cells up to `clip` cells from data; mode 1 fills gaps, mode 2 all cells near data, mode 3 the whole grid; `tension` 0 is minimum curvature. Default: no interpolation.
- `-N` — Write empty cells as NaN instead of 99999.9 (`--use-nan`).
- `-Jprojection` — Projected output (`--projection`), e.g. `-JU`, `-JUTM32N`, `-Jepsg32632`.
- `-Ggridkind` — Output grid format (`--grid-format`), as for `mbgrid`. Default: GMT netCDF float.
- `-Ttopogridfile` — Use a topography grid instead of a flat bottom to calculate the footprint of each sample (`--topography-grid`).
- `-Zbathdef` — Depth used for grazing angles where no depth is available (`--altitude-default`). Default: 1000.
- `-M` — Also write the number of samples and the standard deviation per cell (`root_num.grd`, `root_sd.grd`); requires `-F` (`--extra-grids`).
- `-Xextend` — Enlarge the internal working grid by `extend` times the grid size on each side (`--extend`).
- `-Bborder`, `-Llonflip`, `-Ppings`, `-Sspeed` — Border value, longitude range, ping averaging and minimum speed in km/h, as in `mbgrid`.
- `-V` — Verbose output (`--verbose`). `-H` prints the usage (`--help`).

## Examples

Mosaic sidescan with 1 m cells, preferring the outer swath (predefined table 2) and averaging all samples within 0.1 of the best priority:

    mbmosaic -Idatalistp.mb-1 -Osidescan_mosaic -A4 -E1/1/meters -Y2 -F0.1 -N -V

Mosaic beam amplitude in UTM, using a priority file and filling small gaps by interpolation:

    mbmosaic -Idatalistp.mb-1 -Oamplitude_utm -A3 -JU -E2/2/meters -Yangle_priority.txt -F0.1 -C5/1/0 -N

Make an east-looking sidescan mosaic (preferred look azimuth 90 degrees):

    mbmosaic -Idatalistp.mb-1 -Ossmosaic_east -A4 -E2/2/meters -U90/2 -F0.05 -N

A priority file `angle_priority.txt` that gives low priority to the nadir region and the highest to 45 degrees could contain:

    -60.0 0.2
    -45.0 1.0
    -15.0 0.8
    -14.9 0.1
    14.9 0.1
    15.0 0.8
    45.0 1.0
    60.0 0.2

## Notes

- On Windows the plotting scripts (`root.grd.cmd`) normally produced with the `mbm_grdplot` macro are not created; view the mosaic in MB-Studio or with `gmt mbgrdtiff`.
- Correct the backscatter for grazing angle effects first (`mbbackangle`, then `mbprocess`) and mosaic the processed files.
- Quote paths that contain spaces.

## See also

`mbgrid`, `mbbackangle`, `mbprocess`, `mbfilter`, `mbdatalist`, `gmt mbgrdtiff`. The complete reference is the MB-System manual page mbmosaic(1) (English).
