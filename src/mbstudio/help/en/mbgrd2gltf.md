# mbgrd2gltf — Convert a bathymetry grid to a glTF 3D model

`mbgrd2gltf` converts a GMT bathymetry grid (netCDF `.grd`, e.g. made with `mbgrid`) into a glTF 3D model (`.gltf` text or `.glb` binary) for 3D viewers, game engines, web pages and presentation tools. Options control vertical exaggeration, Draco mesh compression, a local coordinate origin for better rendering precision, and an optional HTML viewer page.

## Synopsis

    mbgrd2gltf --input FILE [--binary] [--output ROOT] [--exaggeration NUM]
               [--geoorigin[=LON,LAT,ELEV]] [--draco] [--quantize-position NUM]
               [--quantize-normal NUM] [--quantize-texcoord NUM] [--quantize-color NUM]
               [--html] [--verbose] [--help]

    mbgrd2gltf -IFILE [-B] [-OROOT] [-ENUM] [-G[LON,LAT,ELEV]] [-D]
               [-QpNUM] [-QnNUM] [-QtNUM] [-QcNUM] [-W] [-V] [-H]

## Description

The input is a GMT grid in netCDF format (COARDS longitude/latitude grids and old GMT netCDF grids are read) with longitude and latitude coordinates and elevation or depth values. The grid is converted into a triangle mesh; large grids are handled by tiling. Vertex positions are in an Earth-Centered, Earth-Fixed (ECEF) Cartesian coordinate system in meters, with the origin at the center of the Earth, Z toward the North Pole and X through the prime meridian on the equator.

Because ECEF coordinates are millions of meters large, they can lose precision in the single-precision arithmetic of graphics cards. With `--geoorigin` the vertices are instead written as offsets from a local origin, either given as longitude, latitude and elevation or computed automatically from the grid center and mean altitude. Without `--geoorigin` full ECEF coordinates are used.

The output is written as `ROOT.gltf`, or `ROOT.glb` with `--binary`. By default `ROOT` is the input file name without its extension, in the same folder as the input. Draco compression (`--draco`) can reduce the file size considerably; the quantization options set the number of bits used for positions, normals, texture coordinates and colors (fewer bits give smaller files and lower precision). With `--html` an HTML viewer page `ROOT.html` is also written, which shows the model with X3DOM together with processing information.

## Common options

- `--input FILE`, `-IFILE` — Input GMT grid file (required).
- `--output ROOT`, `-OROOT` — Output file root; `.gltf` or `.glb` is appended. Default: input file name without extension, in the input folder.
- `--binary`, `-B` — Write binary glTF (`.glb`), which is more compact and loads faster. Default: text glTF.
- `--exaggeration NUM`, `-ENUM` — Vertical exaggeration factor, greater than 0. Default: 1.0.
- `--geoorigin[=LON,LAT,ELEV]`, `-G[LON,LAT,ELEV]` — Write vertices relative to a local origin; without values the grid center and mean altitude are used. Default: full ECEF coordinates.
- `--draco`, `-D` — Enable Draco mesh compression.
- `--quantize-position NUM`, `-QpNUM` — Draco position quantization bits, 2-30. Default: 16.
- `--quantize-normal NUM`, `-QnNUM` — Draco normal quantization bits, 2-30. Default: 7.
- `--quantize-texcoord NUM`, `-QtNUM` — Draco texture coordinate quantization bits, 2-30. Default: 10.
- `--quantize-color NUM`, `-QcNUM` — Draco color quantization bits, 2-30. Default: 8.
- `--html`, `-W` — Also write an HTML viewer page `ROOT.html`.
- `--verbose`, `-V` — Print details about reading the grid and memory use.
- `--help`, `-H` — Print the help text and exit.

## Examples

Convert a grid to a text glTF model with default settings (writes `survey_bathy.gltf`):

    mbgrd2gltf --input survey_bathy.grd

Write a compact binary model with 5x vertical exaggeration and a local origin at the grid center:

    mbgrd2gltf --input survey_bathy.grd --binary --exaggeration 5 --geoorigin

Write a Draco-compressed binary model to another folder with higher position precision:

    mbgrd2gltf --input survey_bathy.grd --binary --draco --quantize-position 20 --output "D:\Models\survey area"

Use an explicit local origin and also create an HTML viewer page:

    mbgrd2gltf --input monterey.grd --geoorigin=-122.0,36.8,-1000 --binary --html

## Notes

- Give explicit origin values with `=` (`--geoorigin=-122.0,36.8,-1000`, legacy form `-G-122.0,36.8,-1000`); a value in a separate argument is not read for this option.
- With the legacy syntax, `-D` must come before the `-Q` options.
- Make the grid in geographic coordinates (no `-J` in `mbgrid`); preferably mark empty cells as NaN (`mbgrid -N`).
- The default output name is the input name up to its first dot, so `area.v2.grd` gives `area.gltf`; use `--output` to choose another name.
- Web browsers usually refuse to load the model from a local `.html` file opened directly; serve the folder with a local web server (for example `python -m http.server 8000`) and open the page through `http://localhost:8000/`.

## See also

`mbgrid`, `mbinfo`, `gmt mbgrdtiff`. The complete reference is the MB-System manual page mbgrd2gltf(1) (English).
