# mbgrdtiff — Make a georeferenced GeoTIFF image from a GMT grid

`mbgrdtiff` is a GMT module of the MB-System supplement that turns a GMT grid, such as a bathymetry grid from `mbgrid` or a sidescan mosaic from `mbmosaic`, into a GeoTIFF image. The grid values are colored with a GMT color palette (CPT) file and can be shaded with an intensity grid. Use it to deliver survey results as images that GIS and charting software can load in the right place.

## Synopsis

    gmt mbgrdtiff -I<grdfile> -C<cptfile> -J<parameters> -O<tiff_file>
        [-I<intensgrid>|<value>] [-M] [-N<nudge_x>/<nudge_y>]
        [-R<west>/<east>/<south>/<north>] [-V]
    gmt mbgrdtiff <grd_r> <grd_g> <grd_b> -J<parameters> -O<tiff_file> [options]

## Description

The first `-I` option names the grid to convert. Each grid cell becomes one image pixel whose color is taken from the CPT file given with `-C`. Cells without data (NaN) get the CPT's NaN color. If every color in the CPT is a gray (red = green = blue), the TIFF is an 8-bit grayscale image; otherwise it is a 24-bit color image. Instead of one grid and a CPT, three grids holding the red, green and blue components (0-255) can be given as plain file names.

A second `-I` option adds shading, either from an intensity grid (for example made with `gmt grdgradient`, with the same dimensions as the data grid) or from a constant intensity value.

The georeferencing is written into the TIFF using the GeoTIFF tags. `mbgrdtiff` recognizes the coordinate system recorded by `mbgrid` and `mbmosaic` in the grid: UTM zones and EPSG projected systems are written as the corresponding projected coordinate system; all other grids are treated as geographic coordinates (longitude and latitude, WGS84). A world file with the same name and the suffix ".tfw" is always written next to the TIFF for software that does not read the GeoTIFF tags.

`-R` limits the image to part of the grid. `-N` shifts the georeferenced position of the image by a given number of meters, for example to correct a known positioning offset.

## Common options

- `-I<grdfile>` — The grid to convert (the first `-I`). Required unless three r/g/b grids are given.
- `-I<intensgrid>|<value>` — The second `-I`: intensity grid used for shading, or a constant intensity.
- `-C<cptfile>` — GMT color palette file that converts grid values to colors. Required unless three r/g/b grids are given.
- `-O<tiff_file>` — Name of the output GeoTIFF file (required). Use a name ending in ".tif": the world file name is made by replacing the last four characters with ".tfw".
- `-J<parameters>` — GMT requires a projection; use a linear one such as `-JX15c`. The size does not matter: the image always has one pixel per grid cell.
- `-M` — Make a monochrome (8-bit grayscale) image, converting the colors to gray levels.
- `-N<nudge_x>/<nudge_y>` — Shift the image position by nudge_x meters east and nudge_y meters north.
- `-R<west>/<east>/<south>/<north>` — Use only this part of the grid. Default: the whole grid.
- `-V` — Verbose progress reports.

## Examples

Make a color palette from the range of a depth grid made by `mbgrid` (deep areas blue) and convert the grid to a GeoTIFF (also writes survey.tfw):

    gmt grd2cpt survey.grd -Chaxby -Z -I > survey.cpt
    gmt mbgrdtiff -Isurvey.grd -Csurvey.cpt -JX15c -Osurvey.tif

Make a shaded relief image using an intensity grid computed with illumination from the northwest:

    gmt grdgradient survey.grd -A315 -Ne0.6 -Gsurvey_int.grd
    gmt mbgrdtiff -Isurvey.grd -Isurvey_int.grd -Csurvey.cpt -JX15c -Osurvey_shaded.tif

Convert a sidescan mosaic made with `mbmosaic` to an 8-bit grayscale GeoTIFF:

    gmt makecpt -Cgray -T0/60/1 -Z > ss.cpt
    gmt mbgrdtiff -Imosaic.grd -Css.cpt -JX15c -Omosaic.tif

Shift the image 2.5 m east and 1 m south:

    gmt mbgrdtiff -Isurvey.grd -Csurvey.cpt -JX15c -N2.5/-1 -Osurvey_shifted.tif

## Notes

- `mbgrdtiff` is not a separate program: it is a module of the MB-System GMT supplement and is always run through the GMT driver as `gmt mbgrdtiff`. GMT finds the supplement automatically in the `bin\gmt_plugins` folder of the installation; no GMT_CUSTOM_LIBS setting is needed.
- Unlike the GMT plotting modules, `mbgrdtiff` writes no PostScript; `-O` here is the output file name, not the GMT overlay option.
- Do not use a nonlinear projection such as `-JM` with `mbgrdtiff`: the grid would be resampled for the map while the GeoTIFF still records the original coordinates.
- `mbgrid` grids contain depths (positive down) by default, or topography (positive up) with `-A2`. `gmt grd2cpt` adapts the color palette to the range of the grid; drop `-I` from the first example for topography grids.
- Write option arguments directly after the option letter, without a space. Quote file names that contain spaces.

## See also

`mbgrid`, `mbmosaic`, `mbswath`, `mbcontour`. The complete reference is the MB-System manual page mbgrdtiff(1) (English).
