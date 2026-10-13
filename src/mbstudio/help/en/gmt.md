# gmt — GMT with the MB-System plotting modules

`gmt` is the driver of the Generic Mapping Tools (GMT 6), installed with MB-Studio together with the MB-System GMT supplement. Besides all the standard GMT modules (`grdimage`, `coast`, `basemap`, `grdcontour`, ...) it runs the MB-System modules `mbswath`, `mbcontour` and `mbgrdtiff`, which plot swath data and grids directly. Use it to make maps of grids made with `mbgrid` and `mbmosaic` and of the swath data themselves.

## Synopsis

    gmt MODULE [module options]
    gmt mbswath -I datalist -J... -R... [options]
    gmt mbcontour -I datalist -J... -R... [options]
    gmt mbgrdtiff -I grid -O geotiff [options]
    gmt --version | --help

## Description

Each GMT program is a module of the `gmt` driver: the first argument names the module, the following arguments are the module's options. Run `gmt MODULE` without options to print the usage of a module, for example `gmt mbswath`.

GMT 6 has two modes. In modern mode a plot is a session (`gmt begin map png`, plotting commands, `gmt end show`); the one-line form appends the graphics format and file name to a single command (`-png map`). In classic mode each module writes PostScript to standard output, which is redirected to a file (`> map.ps`), with `-K` and `-O` to combine several layers. Both modes work in the Windows command prompt.

The MB-System modules are found in the `bin\gmt_plugins` folder of the installation; no configuration is needed. GMT reads its own data (colour tables, fonts, coastlines when installed) from `share\gmt`.

## Common options

- `MODULE` — Name of the GMT or MB-System module to run, for example `grdimage` or `mbswath`.
- `--version` — Print the GMT version and exit.
- `--help` — Print the list of the GMT modules and exit.
- `--show-modules` — List the available modules, including the MB-System modules.
- `--show-plugindir` — Print the folder searched for supplement modules.
- `--show-sharedir` — Print the folder of the GMT data files.

## Examples

Shaded relief map of the swath bathymetry in a datalist, written to swath.png (one-line modern mode):

    gmt mbswath -I datalist.mb-1 -JM15c -R-122.1/-121.9/36.7/36.9 -Z2 -png swath

Colour image of a grid made with mbgrid:

    gmt grdimage bathymetry.grd -JM15c -Cgeo -B -png bathymetry

List the MB-System modules known to GMT:

    gmt --show-modules

## Notes

- Quote file names with spaces, for example `-I "D:\Survey 2025\datalist.mb-1"`.
- Classic-mode PostScript must be redirected to a file (`> map.ps`); in the MB-Studio Programs page use the modern one-line form (`-png name`) instead, because redirections are not available there.
- The complete GMT documentation is at docs.generic-mapping-tools.org.

## See also

`mbswath`, `mbcontour`, `mbgrdtiff`, `mbgrid`, `mbmosaic`.
