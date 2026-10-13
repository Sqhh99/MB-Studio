# mbmesh — Reconstruct a 3D triangle mesh (GLB) from swath bathymetry

`mbmesh` reads swath bathymetry through an MB-System datalist and reconstructs a continuous 3D triangle surface from the accepted soundings, written as a binary glTF (`mesh.glb`) file. Use it after the data have been cleaned and processed when you want a true 3D model of the seafloor (for example steep walls, overhangs or structures) rather than a 2.5D grid made with `mbgrid`.

## Synopsis

    mbmesh -I datalist [-O outputdir] [-R west/east/south/north] [-L meters]
           [--decimate meters] [--metadata] [output options] [-V] [-H]

## Description

`mbmesh` reads all valid (unflagged) bathymetry soundings referenced by the datalist, optionally restricted to a geographic box with `-R`. It estimates the typical point spacing, derives the reconstruction settings automatically, estimates oriented surface normals (using the sensor positions), reconstructs the surface with screened Poisson reconstruction, extracts it with marching cubes, and finally trims mesh regions that are not supported by data. If trimming would remove the entire mesh, the program stops with an error instead of writing an empty surface.

The level of detail `-L` is the smallest feature size to resolve, in meters. If it is not given, it is set to about twice the estimated average point spacing. The value is limited to the range 0.01 to 5 m. A warning is printed when the requested level of detail is finer than the data density supports. If the reconstruction grid would become too large for the internal memory budget, `mbmesh` stops with an error; request a coarser level of detail (larger `-L`) or a smaller area (`-R`).

The vertices of `mesh.glb` are in a local Cartesian east-north-up frame in meters, with its origin at the sensor position of the first accepted ping (X east, Y north, Z up). Longitude and latitude are converted to local meters with an equirectangular approximation. This is not the ECEF frame used by `mbgrd2gltf` (nor its `--geoorigin` frame), so outputs of the two programs do not line up unless they are transformed into a common frame first.

All files are written to the output directory (default `mbmesh_output`, created if needed). Only `mesh.glb` is written by default; point clouds, diagnostic GLB files and an HTML viewer page are written only when the corresponding output options are given. `--metadata` prints the input counts, the geographic bounds and the estimated point spacing, and exits without building a mesh; it is a quick way to choose `-R` and `-L`.

## Common options

- `-I datalist` — Input MB-System datalist (long form `--input=datalist`). Required; the datalist may also be given as the first non-option argument.
- `-O outputdir` — Output directory (long form `--output=outputdir`). Default: `mbmesh_output`.
- `-R west/east/south/north` — Use only soundings inside this longitude/latitude box in degrees (long form `--bounds=west/east/south/north`).
- `-L meters` — Smallest feature size to reconstruct (long forms `--lod=meters`, `--level-of-detail=meters`). Default: derived from the point spacing.
- `--decimate meters` — Thin the point cloud on a voxel grid with this cell size in meters before reconstruction.
- `--metadata` — Print dataset metadata (counts, bounds, point spacing) and exit (also `--info`).
- `--xyz` — Also write `pointcloud-local.xyz` (local frame, same as `mesh.glb`) and `pointcloud-ecef.xyz` (WGS84 ECEF). Use `--local-xyz` or `--ecef-xyz` to write only one of them.
- `--oriented-ply` — Also write `oriented-pointcloud-ecef.ply`, the point cloud with normals.
- `--raw-mesh-glb` — Also write `raw_mesh.glb`, the mesh before support trimming.
- `--diagnostics` — Also write `pointcloud.glb`, `normals.glb` and `origins.glb` (individually `--pointcloud-glb`, `--normal-glb`, `--origin-glb`); intended for smaller datasets.
- `--html` — Also write `mesh.html`, an X3DOM viewer page for `mesh.glb`, and try to start a local web server and open it in the browser.
- `--all-outputs` — Write all optional outputs (including `--html`).
- `-V` — Print progress and diagnostic messages (long form `--verbose`).
- `-H` — Print the usage message and exit (also `-h`, `--help`).

The long output options are also accepted with a single dash (for example `-xyz`, `-diagnostics`).

## Examples

Build a mesh from all processed data in the datalist and write `mbmesh_output\mesh.glb`:

    mbmesh -I datalist.mb-1

Check the extent, sounding count and point spacing before meshing:

    mbmesh -I datalist.mb-1 --metadata

Mesh one area with a 0.5 m level of detail, writing the result and both XYZ point clouds to the folder `wall_mesh`:

    mbmesh -I datalist.mb-1 -O wall_mesh -R-122.5/-121.8/36.5/37.2 -L 0.5 --xyz -V

Thin a dense dataset to 0.25 m voxels and also keep the untrimmed mesh for comparison:

    mbmesh -I datalist.mb-1 --decimate 0.25 --raw-mesh-glb -V

## Notes

- The output directory is relative to the current directory unless a full path is given. Quote paths that contain spaces, e.g. `-O "D:\Survey 2025\mesh"`.
- `mesh.glb` can be opened in any glTF viewer or 3D application. Its coordinates are local meters, not longitude/latitude.
- The automatic viewer launch of `--html` relies on `python3`; when it cannot be started, `mbmesh` prints a warning and `mesh.html` is still written to the output directory.
- Clean the data first (for example with `mbclean` or `mbareaclean` and `mbprocess`): flagged soundings are ignored, but remaining outliers become spikes in the mesh.
- Reconstruction of large or dense datasets needs a lot of memory and time; start with `--metadata`, then use `-R`, a larger `-L` or `--decimate`.

## See also

`mbgrid`, `mbgrd2gltf`, `mbdatalist`, `mbprocess`, `mbinfo`. The complete reference is the MB-System manual page mbmesh(1) (English).
