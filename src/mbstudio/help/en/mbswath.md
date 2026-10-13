# mbswath — Plot swath bathymetry, amplitude or sidescan as a color map

`mbswath` is a GMT module of the MB-System supplement that draws swath sonar data directly from the data files onto a GMT map, without gridding. Each beam or sidescan pixel is painted as a footprint, giving a color fill or shaded relief image of bathymetry, a color image of beam amplitude or sidescan, or bathymetry shaded by amplitude. Use it for quick-look maps of raw or processed survey lines and to check coverage and data quality before gridding with `mbgrid`.

## Synopsis

    gmt mbswath -I<inputfile> -J<parameters> -R<west>/<east>/<south>/<north> -C<cptfile>
        [-A<factor>/<mode>/<depth>] [-B<tickinfo>]
        [-b<year>/<month>/<day>/<hour>/<minute>/<second>]
        [-e<year>/<month>/<day>/<hour>/<minute>/<second>]
        [-Ei|<dpi>] [-F<format>] [-G<magnitude>/<azimuth | median>]
        [-L<lonflip>] [-N<cptfile>] [-p<pings>] [-S<speed>] [-T<timegap>]
        [-W] [-Z<mode>[F]] [-K] [-O] [-P] [-U] [-V] [-X<x-shift>] [-Y<y-shift>]

## Description

`mbswath` reads one swath data file or all files referenced by a datalist, and paints each good beam (or sidescan pixel) of each ping as a small polygon (its footprint) colored by the color palette given with `-C`. Beams flagged as bad are not plotted. The result is written as a 24-bit color PostScript image (8-bit if the color palette is grayscale), followed by the map frame if `-B` is given. Whether raw or processed files are read is controlled by the datalist (for example a `$PROCESSED` line).

The plot style is selected with `-Z`: color fill of bathymetry (1), color shaded relief bathymetry (2), bathymetry shaded with amplitude (3), color fill of beam amplitude (4) or of sidescan (5). Bathymetry values are depths in meters, positive downward (feet with `-W`), so the color palette must cover the depth range as positive numbers. For shaded relief, `-G` sets the illumination strength and direction.

Data are read inside the `-R` bounds extended by 25% on every side, so swaths crossing the map edge are drawn completely up to the edge. Before reading a file, `mbswath` looks for its ".inf" file (the `mbinfo` output); if the ".inf" file shows that the file has no data inside the bounds, the file is skipped. For bathymetry-only plots (`-Z1`, `-Z2`) a ".fbt" fast bathymetry file is read instead of the original file when it exists. Create these ancillary files with `mbdatalist -O` to speed up plotting of large surveys.

The swath image covers the whole map area and is opaque (white where there are no data). It therefore must be the first layer of a map, and only one `mbswath` image can be placed on a map; draw contours, tracks, coastlines and color scales on top of it with `-K`/`-O`.

## Common options

- `-I<inputfile>` — Swath data file or datalist to plot (required). A file ending in ".mb-1" is read as a datalist.
- `-J<parameters>` — GMT map projection and size, for example `-JM15c` (Mercator, 15 cm wide). Required.
- `-R<west>/<east>/<south>/<north>` — Map region in degrees. Required.
- `-C<cptfile>` — GMT color palette (CPT) file used to color the data, for example made with `gmt makecpt`. Required in practice.
- `-Z<mode>[F]` — Plot style: 1 = color fill of bathymetry, 2 = color shaded relief bathymetry, 3 = bathymetry shaded with amplitude, 4 = color fill of amplitude, 5 = color fill of sidescan. Append `F` (for example `-Z5F`) to plot amplitude or sidescan filtered by `mbfilter` (".ffa" / ".ffs" files); the plot fails if those files do not exist. Default: 1.
- `-G<magnitude>/<azimuth>` — With `-Z2`: shading strength (an effective vertical exaggeration, typically 1-5) and illumination azimuth in degrees. With `-Z3`: shading strength and the amplitude value that gives neutral shading (`-G<magnitude>/<median>`). Default: 1/270.
- `-N<cptfile>` — With `-Z3`: grayscale CPT file used to convert amplitudes to shading instead of a linear mapping.
- `-A<factor>/<mode>/<depth>` — Footprint size along track. Mode 1: from the sonar's fore-aft beam width times factor (footprints grow toward the outer swath); mode 2: the distance between pings times factor; mode 3: plot each sounding as a single point. depth is used for the footprint calculation when the data contain no depths (pure sidescan). factor smaller than 1 helps for densely sampled data, larger than 1 fills gaps. Default: 1/1.
- `-E<dpi>` — Resolution of the swath image in dots per inch. Default: 100. `-Ei` lets the PostScript device interpolate the image at its own resolution.
- `-p<pings>` — Ping averaging: 1 = none, n = average n pings, 0 = automatic (along-track spacing matched to beam spacing). Default: 1.
- `-F<format>` — MBIO format id of the input (`-f` is equivalent). Default: taken from the file name suffix; -1 means a datalist.
- `-b<year>/<month>/<day>/<hour>/<minute>/<second>`, `-e<year>/<month>/<day>/<hour>/<minute>/<second>` — Plot only pings between these times. Default: from `mbdefaults`.
- `-S<speed>` — Skip pings with ship speed below this value (km/h). Default: 0.
- `-T<timegap>` — Time gap in minutes between pings beyond which the swath is broken. Default: 1.
- `-L<lonflip>` — Longitude range: -1 = -360 to 0, 0 = -180 to 180, 1 = 0 to 360. Default: chosen from `-R`.
- `-W` — Use depths in feet instead of meters (the CPT must then be in feet).
- `-B<tickinfo>` — GMT map frame and annotation, for example `-Ba`.
- `-K`, `-O`, `-P`, `-X`, `-Y`, `-U`, `-V` — GMT classic mode options: more PostScript follows, overlay on an existing plot, portrait orientation, shift of the plot origin, time stamp, and verbose progress reports.

## Examples

Make a color palette for depths from 0 to 1500 m (shallow red, deep blue), plot the swaths as shaded relief, and convert the PostScript to a PNG image (swath.png):

    gmt makecpt -Chaxby -T0/1500/50 -Z -I > depth.cpt
    gmt mbswath -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -Cdepth.cpt -Z2 -G2/315 -Ba -P > swath.ps
    gmt psconvert swath.ps -Tg -A

Plot sidescan as a grayscale image (adjust the -T range to the sidescan values reported by `mbinfo`):

    gmt makecpt -Cgray -T0/60/1 -Z > ss.cpt
    gmt mbswath -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -Css.cpt -Z5 -Ba -P > sidescan.ps

Build a map in layers: color-filled bathymetry, contours from `mbcontour` on top, and a color scale below the map:

    gmt mbswath -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -Cdepth.cpt -Z1 -Ba -P -K > map.ps
    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -A50/250/250/250/0.04/0.1 -P -O -K >> map.ps
    gmt psscale -Cdepth.cpt -Dx7.5c/-1.5c+w10c/0.4c+h+jTC -Baf+l"Depth (m)" -O >> map.ps

## Notes

- `mbswath` is not a separate program: it is a module of the MB-System GMT supplement and is always run through the GMT driver as `gmt mbswath`. GMT finds the supplement automatically in the `bin\gmt_plugins` folder of the installation; no GMT_CUSTOM_LIBS setting is needed. `gmt mbswath` without arguments prints the usage.
- Use GMT classic mode as in the examples: `-K` on every command except the last, `-O` on every command except the first, and the PostScript written with `>` (first command) and appended with `>>`. The modern-mode one-liner form (for example `-png map`) is not recognised by this module, and `-p` means ping averaging here, not the GMT perspective option.
- Write the option arguments directly after the option letter, without a space (`-Idatalist.mb-1`). Quote file names that contain spaces.
- `gmt psconvert` needs Ghostscript to convert PostScript to PNG, JPEG or PDF.
- Create ".inf" and ".fbt" files with `mbdatalist -O` so that files outside the map region are skipped quickly.

## See also

`mbcontour`, `mbgrdtiff`, `mbgrid`, `mbdatalist`, `mbinfo`, `mbfilter`, `mbprocess`. The complete reference is the MB-System manual page mbswath(1) (English).
