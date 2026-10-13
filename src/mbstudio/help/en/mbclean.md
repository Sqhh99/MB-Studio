# mbclean — Automatically flag bad beams in swath bathymetry

`mbclean` identifies artifacts in swath sonar bathymetry and flags the affected beams. Several tests are available (excessive slopes, spikes, depth limits, deviation from the local median, outer beams, acrosstrack distance or angle, speed, position, ...) and any combination can be applied in one run. It is normally run on the preprocessed data before `mbprocess`, as an automatic first editing pass.

## Synopsis

    mbclean
        --bounds=west/east/south/north {-Wwest/east/south/north}
        --depth-range=low/high {-Blow/high}
        --deviation-max=max {-Amax}
        --distance-range=min/max {-Dmin/max}
        --flag-distance-angle=distanceleft/distanceright[/mode] {-Ydistanceleft/distanceright[/mode]}
        --format=format {-Fformat}
        --fraction-range=fraction_low/fraction_high {-Gfraction_low/fraction_high}
        --help {-H}
        --input=infile {-Iinfile}
        --lonflip=lonflip {-Llonflip}
        --max-acrosstrack=max_acrosstrack {-Emax_acrosstrack}
        --max-heading-rate=maxheadingrate {-Rmaxheadingrate}
        --max-slope=slope/unit {-Cslope/unit}
        --min-good-beams=num_good_min {-Unum_good_min}
        --mode=mode {-Mmode}
        --ping-deviation-tolerance=tolerance {-Ntolerance}
        --range-min=range_min {-Krange_min}
        --speed-range=min_speed/max_speed {-Pmin_speed/max_speed}
        --spike-max=spike_slope/mode/format {-Sspike_slope/mode/format}
        --timestamp-tolerance=tolerance {-Ttolerance}
        --verbose {-V}
        --zap-beams=beamsleft/beamsright {-Xbeamsleft/beamsright}
        --zap-rails=backup_dist {-Qbackup_dist}
        --zero-position {-Z}

## Description

`mbclean` reads one swath file or all files of a datalist (default `datalist.mb-1`). It does not change the data files: the beam flags are written as edit events to the edit save file of each swath file (the swath file name with `.esf` appended), the same kind of file produced by interactive editing and by `mbareaclean`. If an edit save file already exists, its edits are loaded and applied first, and the new flags are added. `mbclean` also updates the `mbprocess` parameter file so that `mbprocess` applies the edits when it writes the processed file.

The tests are applied in this order: outer beams (`-X`), acrosstrack distance or angle (`-Y`), ping speed (`-P`), position bounds (`-W`), zero positions (`-Z`), absolute depth range (`-B`), minimum range (`-K`), heading rate (`-R`), rails (`-Q`), maximum acrosstrack distance (`-E`), fraction of the local median depth (`-G`), deviation from the local median depth (`-A`), spikes (`-S`), excessive slopes (`-C`), too few good beams (`-U`), and deviation of whole pings from their neighbours (`-N`).

The slope test (`-C`) is the most commonly used method and is the default: it is applied when no other test is specified. As soon as any other test option is given, the slope test is only used if `-C` is also given. The local median depth is computed from the current ping and the pings before and after it, using beams closer than the maximum distance of `-D`.

At the end of each file and of the run, `mbclean` prints how many beams each test flagged. Automatic cleaning never finds every artifact; follow it with a visual check of the data.

## Common options

- `-Iinfile` — Input swath file or datalist. Default: `datalist.mb-1`.
- `-Fformat` — MBIO format id; a negative value means the input is a datalist. Default: inferred from the file name suffix.
- `-Cslope/unit` — Flag beams associated with slopes greater than slope. unit: `0` tangent (default), `1` radians, `2` degrees. Default slope: `1.0`.
- `-Mmode` — How bad slopes are handled: `1` flag the one beam furthest from the local median depth (default); `2` flag both beams.
- `-Dmin/max` — Minimum and maximum distance between beams used by the slope and median tests, as fractions of the local median depth. Default: `0.01/0.25`.
- `-Sspike_slope/mode/format` — Flag spikes: beams with slopes greater than spike_slope on both sides with opposite signs. mode: `1` acrosstrack (default), `2` alongtrack, `3` both. The third value is the slope unit as for `-C`.
- `-Blow/high` — Flag depths shallower than low or deeper than high (meters).
- `-Gfraction_low/fraction_high` — Flag depths less than fraction_low or more than fraction_high times the local median depth (e.g. `-G0.8/1.2`).
- `-Amax` — Flag depths deviating from the local median depth by more than max meters.
- `-Xbeamsleft/beamsright` — Flag this number of outermost beams on the port and starboard sides (one value applies to both sides).
- `-Ydistanceleft/distanceright/mode` — Flag or unflag by acrosstrack position. mode: `1` (default) flag beams outside the distance range in meters; `2` unflag beams inside it; `3` flag beams outside the beam angle range in degrees; `4` unflag beams inside it. Port values are negative.
- `-Emax_acrosstrack` — Flag beams with an acrosstrack distance greater than this value in meters.
- `-Qbackup_dist` — Flag "rails": outer beams lying more than backup_dist meters inboard of a more inner beam.
- `-Unum_good_min` — Flag all beams of the port or starboard half of a ping that has fewer than this number of good beams.
- `-Ntolerance` — Flag all beams of pings whose rms deviation from the pings before and after exceeds tolerance (e.g. interference from other sonars).
- `-Pmin_speed/max_speed` — Flag pings with speeds outside this range (km/hour).
- `-Wwest/east/south/north` — Flag pings with navigation outside these bounds.
- `-Z` — Flag pings with zero longitude and latitude.
- `-Rmaxheadingrate` — Flag pings during heading changes faster than this rate (degrees/second).
- `-Krange_min` — Flag beams with ranges less than range_min (meters).
- `-Ttolerance` — Reset the times of existing edits to match the ping times within tolerance seconds (for edits transferred with `mbgetesf`).
- `-V` — Verbose output.

## Examples

Run the default slope test on all files of a datalist:

    mbclean -I datalist.mb-1

Flag beams producing slopes steeper than 3.5 (about 74 degrees), or deviating by more than 20 % from the local median depth:

    mbclean -I datalist.mb-1 -M1 -C3.5 -D0.01/0.20 -G0.80/1.20

Flag the five outermost beams on each side and every depth outside 5 to 250 m, keeping the slope test:

    mbclean -I datalist.mb-1 -X5 -B5/250 -C1.0

Flag beams more than 65 degrees from vertical, then apply all edits:

    mbclean -I datalist.mb-1 -Y-65/65/3
    mbprocess -I datalist.mb-1

## Notes

- The flags only reach the processed data when `mbprocess` is run.
- Running `mbclean` again adds to the existing edits; to start over, delete or rename the `.esf` files first.
- Depths are positive down in meters; `-B` and `-A` values are in meters.
- Quote file names that contain spaces, e.g. `-I "my survey\datalist.mb-1"`.

## See also

`mbareaclean`, `mbvoxelclean`, `mbprocess`, `mbdumpesf`, `mbgetesf`, `mbinfo`, `mbpreprocess`. The complete reference is the MB-System manual page mbclean(1) (English).
