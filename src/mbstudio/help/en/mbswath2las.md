# mbswath2las — Export swath bathymetry soundings to LAS 1.4 point clouds

`mbswath2las` reads swath bathymetry in any MB-System format and writes every
valid sounding as a point in a LAS 1.4 file. Use it to load multibeam soundings
into point cloud viewers, GIS packages and processing tools (for example
CloudCompare, QGIS or PDAL) that cannot read MB-System formats, typically after
the data have been cleaned and processed with `mbprocess`.

## Synopsis

    mbswath2las [-A -Bbegintime -Eendtime -Fformat -Iinputfile -Jprojection -Llonflip -Ooutputfile -P -Rwest/east/south/north -Sspeedmin -Ttimegap -V -H]

Long forms: `--use-amplitude` (`-A`), `--begin-time` (`-B`), `--end-time`
(`-E`), `--format` (`-F`), `--input` (`-I`), `--projection` (`-J`),
`--lonflip` (`-L`), `--output` (`-O`), `--write-prj` (`-P`), `--bounds`
(`-R`), `--speed-minimum` (`-S`), `--time-gap` (`-T`), `--verbose` (`-V`),
`--help` (`-H`).

## Description

Each sounding position is computed from the navigation, heading and the
acrosstrack and alongtrack distances of the beam, the same way as in `mblist`.
Beams flagged as bad are skipped. The LAS elevation (Z) is the negative of the
depth, because LAS elevations are positive up. By default positions are
geographic longitude and latitude (WGS84); with `-J` they are eastings and
northings in meters in the chosen projected coordinate system. The coordinate
system is stored in the LAS file as an OGC WKT record, and `-P` also writes an
Esri style `.prj` file next to each LAS file.

The input is one swath file or a datalist. Without `-O` a separate LAS file is
written for each input file, named by replacing the MB-System format suffix of
the input with `.las` (e.g. `0001_20250101_000000.mb59` gives
`0001_20250101_000000.las`; a name without a recognized suffix gets `.las`
appended). With `-O` all soundings go to one LAS file.

To save time, the program reads the bathymetry-only `.fbt` file of an input file
when one exists. These files have no amplitude, so the LAS intensity field is
then 0. With `-A` the original swath files are read and the per-beam amplitude
is scaled linearly from the amplitude range of the whole input into the LAS
range 0-65535 (relative values, not calibrated reflectivity). When `-J` or `-A`
is used, missing or outdated `.inf`, `.fbt` and `.fnv` files are generated
first, and the survey bounds and amplitude range are taken from the `.inf`
files.

The LAS files use point data record format 1. X/Y resolution is 1e-7 degree
(geographic) or 0.01 m (projected); Z resolution is 0.001 m. The GPS time field
holds the Unix time of each sounding, not true GPS time.

## Common options

- `-I<file>` — Input swath file or datalist. Default: `datalist.mb-1`.
- `-F<format>` — Input format id; a negative value means a datalist. Default:
  guessed from the file name suffix.
- `-O<file>` — Write all soundings to this single LAS file.
- `-J<projection>` — Write projected coordinates: `U` or `UTM` for the UTM
  zone of the survey center, `UTM31N`-style zone names, `L` or `LTM` for a local
  Transverse Mercator centered on the survey, `LTM<lon0>/<lat0>` for an
  explicit origin, or an EPSG code or PROJ definition. Default: geographic
  WGS84.
- `-P` — Also write a `.prj` file (the LAS name with `.las` replaced by
  `.prj`).
- `-A` — Fill the LAS intensity field from the beam amplitudes (reads the
  original files instead of the `.fbt` files).
- `-R<west>/<east>/<south>/<north>` — Only export data within these bounds.
- `-B<yr/mo/da/hr/mn/sc>` / `-E<yr/mo/da/hr/mn/sc>` — Only export data between
  these times.
- `-S<speed>` — Skip pings with a ship speed below this value (km/h).
  Default: 0.
- `-T<timegap>` — Time gap in minutes reported as a data gap. Default: 1.
- `-L<lonflip>` — Longitude range: `-1` = -360 to 0, `0` = -180 to 180,
  `1` = 0 to 360. Default: as set by `mbdefaults`.
- `-V` — Verbose output: prints the coordinate system actually used, each
  output file name and the number of points written.

## Examples

Convert one swath file to `0002_20180911_020351_EX1811_MB.las`:

    mbswath2las -I 0002_20180911_020351_EX1811_MB.mb88 -F88

Convert every file of a datalist to its own LAS file and report progress:

    mbswath2las -I datalist.mb-1 -V

Combine all soundings of a datalist into one UTM LAS file with a `.prj` file
for ArcGIS:

    mbswath2las -I datalist.mb-1 -JUTM -P -O survey.las

Use a local Transverse Mercator projection and fill intensity from the
amplitudes:

    mbswath2las -I datalist.mb-1 -JLTM -A -O survey.las

## Notes

- Without `-O` each LAS file is written next to its input file. Quote paths
  with spaces.
- Run `mbprocess` first and make sure the datalist selects the processed
  files, so that the exported soundings include your edits and corrections.

## See also

`mblist`, `mbdatalist`, `mbinfo`, `mbgrid`, `mbcopy`, `mbprocess`. The complete
reference is the MB-System manual page mbswath2las(1) (English).
