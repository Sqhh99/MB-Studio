# mbcontour — Plot bathymetry contours and ship tracks from swath data

`mbcontour` is a GMT module of the MB-System supplement that contours swath bathymetry directly from the data files, ping by ping, and plots the ship track with time, date, ping number and file name annotations. Use it to add contours or annotated track lines to a map, often on top of an `mbswath` image, or to make a quick track chart of a survey.

## Synopsis

    gmt mbcontour -I<inputfile> -J<parameters> -R<west>/<east>/<south>/<north>
        [-A<cont_int>/<col_int>/<tick_int>/<label_int>/<tick_len>/<label_hgt>/<label_spacing>]
        [-B<tickinfo>] [-b<year>/<month>/<day>/<hour>/<minute>/<second>]
        [-C<contourfile>] [-D<time_tick>/<time_annot>/<date_annot>/<time_tick_len>]
        [-e<year>/<month>/<day>/<hour>/<minute>/<second>]
        [-F<format>] [-G<name_hgt>[/<name_perp>]] [-L<lonflip>]
        [-M<pingnumber_tick>/<pingnumber_annot>/<pingnumber_tick_len>]
        [-N<nplot>] [-p<pings>] [-Q] [-S<speed>] [-T<timegap>] [-Z<algorithm>]
        [-K] [-O] [-P] [-U] [-V] [-X<x-shift>] [-Y<y-shift>]

## Description

`mbcontour` reads one swath data file or all files referenced by a datalist and contours the bathymetry of groups of pings as they are read. Depths are in meters, positive downward. Beams flagged as bad are not used.

Without `-C`, contours are drawn every cont_int meters (`-A`) in four colors (black, red, green and blue) that change every col_int meters; contours at multiples of tick_int carry tick marks pointing downhill, and contours at multiples of label_int are labelled. With `-C` a contour file gives the levels and their style and color explicitly; `-A` is then ignored.

Two contouring algorithms are available (`-Z`). The default ping-to-ping algorithm is fast, but contours look poor where beams of one ping lie behind those of the previous ping (sharp turns, pitch steering). The triangle algorithm builds a triangle network from the soundings and contours it; it gives better contours but its run time grows with the square of the number of soundings, so fewer pings are contoured at a time (`-N`).

The ship track is plotted with `-D` (time ticks and annotations), `-M` (ping number ticks) and `-G` (file names at the start of each file). If none of `-A`, `-C`, `-D`, `-M` and `-Q` is given, both contours (with the default intervals) and the track are plotted. With only `-A` or `-C`, no track is drawn.

Data are read inside the `-R` bounds extended by 25% on every side. Before reading a file, `mbcontour` checks its ".inf" file (the `mbinfo` output) and skips the file if it has no data inside the bounds.

## Common options

- `-I<inputfile>` — Swath data file or datalist to plot (required). A file ending in ".mb-1" is read as a datalist.
- `-J<parameters>` — GMT map projection and size, for example `-JM15c`. Required.
- `-R<west>/<east>/<south>/<north>` — Map region in degrees. Required.
- `-A<cont_int>/<col_int>/<tick_int>/<label_int>/<tick_len>/<label_hgt>/<label_spacing>` — Contour interval, color change interval, tick interval and label interval in meters; tick length and label height in inches; minimum label spacing in inches (0 = derived from the label height). Missing trailing values keep their defaults. Default: 25/100/100/100/0.05/0.1/0.
- `-C<contourfile>` — Text file of contour levels, one per line: `level label tick red green blue`, where label is `a` (annotated) or `n`, tick is `t` (ticked) or `n`, and red, green, blue are 0-255. Lines without a valid label and tick are drawn in black without labels and ticks. If a GMT CPT file is given, its color boundaries become plain black contours.
- `-D<time_tick>/<time_annot>/<date_annot>/<time_tick_len>` — Plot the ship track with time marks every time_tick hours, time annotations (hh:mm) every time_annot hours and date annotations (hh:mm/day of year) every date_annot hours; time_tick_len is the mark size in inches. Default: 0.25/1/4/0.1.
- `-G<name_hgt>[/<name_perp>]` — Annotate the start of each file with its file name, lettering name_hgt inches high; name_perp = 1 writes the name perpendicular to the track. Implies track plotting. Default height: 0.1.
- `-M<pingnumber_tick>/<pingnumber_annot>/<pingnumber_tick_len>` — Tick the track every pingnumber_tick pings and annotate the ping number every pingnumber_annot pings; tick length in inches. Default: 50/100/0.1.
- `-Z<algorithm>` — Contouring algorithm: 0 = ping to ping (default), 1 = triangle network.
- `-N<nplot>` — Number of pings read before each contouring step. Default: 50, or 5 with `-Z1`.
- `-Q` — Also plot the triangles of the triangle algorithm (`-Z1`), to show the data distribution.
- `-p<pings>` — Ping averaging: 1 = none, n = average n pings, 0 = automatic. Default: from `mbdefaults`.
- `-F<format>` — MBIO format id of the input (`-f` is equivalent). Default: taken from the file name suffix; -1 means a datalist.
- `-b<year>/<month>/<day>/<hour>/<minute>/<second>`, `-e<year>/<month>/<day>/<hour>/<minute>/<second>` — Use only pings between these times. Default: from `mbdefaults`.
- `-S<speed>` — Skip pings with ship speed below this value (km/h). Default: from `mbdefaults`.
- `-T<timegap>` — Time gap in minutes between pings beyond which the data are considered to have a gap. Default: from `mbdefaults`.
- `-L<lonflip>` — Longitude range: -1 = -360 to 0, 0 = -180 to 180, 1 = 0 to 360. Default: chosen from `-R`.
- `-B<tickinfo>` — GMT map frame and annotation, for example `-Ba`.
- `-K`, `-O`, `-P`, `-X`, `-Y`, `-U`, `-V` — GMT classic mode options: more PostScript follows, overlay on an existing plot, portrait orientation, shift of the plot origin, time stamp, and verbose progress reports.

## Examples

Plot contours and the ship track with the default settings:

    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -Ba -P > contours.ps

Plot 10 m contours with color changes, ticks and labels every 50 m, using the triangle algorithm:

    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -A10/50/50/50/0.03/0.08 -Z1 -Ba -P > contours.ps

Plot only the ship track, with time marks every 15 minutes, hourly time annotations, date annotations every 4 hours and file names perpendicular to the track:

    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -D0.25/1/4/0.1 -G0.08/1 -Ba -P > track.ps

Plot contours at chosen levels read from levels.txt, then convert the plot to a PNG image:

    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -Clevels.txt -Ba -P > contours.ps
    gmt psconvert contours.ps -Tg -A

where levels.txt contains, for example:

    100 a t 0 0 0
    150 n t 0 0 255
    200 a t 0 0 0

## Notes

- `mbcontour` is not a separate program: it is a module of the MB-System GMT supplement and is always run through the GMT driver as `gmt mbcontour`. GMT finds the supplement automatically in the `bin\gmt_plugins` folder of the installation; no GMT_CUSTOM_LIBS setting is needed.
- Use GMT classic mode as in the examples (`-K`/`-O` and output redirected with `>` and `>>`). To overlay contours on an `mbswath` image, run `gmt mbswath ... -K > map.ps` first and then `gmt mbcontour ... -O -K >> map.ps` with the same `-J` and `-R`. The modern-mode one-liner form (for example `-png map`) is not recognised by this module, and `-p` means ping averaging here.
- Label, tick and name sizes are given in inches, independent of the GMT length unit setting.
- The `-W` option of the manual page (depths in feet) is not available in this version; `-W` is accepted but has no effect.
- Write option arguments directly after the option letter, without a space. Quote file names that contain spaces.

## See also

`mbswath`, `mbgrdtiff`, `mbgrid`, `mbdatalist`, `mbinfo`, `mbnavlist`. The complete reference is the MB-System manual page mbcontour(1) (English).
