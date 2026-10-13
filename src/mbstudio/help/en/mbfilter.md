# mbfilter — smoothing, high-pass and contrast filters for sidescan/amplitude

`mbfilter` applies simple image-processing filters to sidescan or beam amplitude data: low-pass smoothing to reduce speckle, high-pass filtering to emphasise fine structure, and contrast enhancement. It is typically run on processed (and angle-corrected) files before making backscatter mosaics with `mbmosaic`.

## Synopsis

    mbfilter
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --buffer-size=buffersize {-Nbuffersize}
        --contrast-filter=mode/xdim/ldim/iteration {-Cmode/xdim/ldim/iteration}
        --data-kind=kind {-Akind}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format {-Fformat}
        --help {-H}
        --hipass-filter=mode/xdim/ldim/iteration/offset {-Dmode/xdim/ldim/iteration/offset}
        --input=infile {-Iinfile}
        --smooth-filter=mode/xdim/ldim/iteration/threshold_lo/threshold_hi {-Smode/xdim/ldim/iteration/threshold_lo/threshold_hi}
        --threshold=threshold_lo/threshold_hi {-Tthreshold_lo/threshold_hi}
        --verbose {-V}

## Description

The swath data are treated as an image whose x axis is the pixel (or beam) number and whose y axis is the ping number. Each filter is an `xdim` by `ldim` window (acrosstrack by alongtrack) convolved with this image; missing and flagged values are ignored. Available filters:

- `-S1` boxcar mean, `-S2` Gaussian mean, `-S3` boxcar median, `-S4` inverse gradient (low-pass, smoothing);
- `-D1` boxcar mean, `-D2` Gaussian mean, `-D3` boxcar median subtraction (high-pass);
- `-C1` edge detection, `-C2` gradient magnitude subtraction (contrast enhancement).

Up to ten filters can be given; they are applied in the order they appear on the command line, each with its own number of iterations. A 3x3 or 5x5 window applied iteratively is typical for speckle reduction; elongated windows such as 3x10 are typical for high-pass filtering. The inverse gradient filter smooths flat areas more than edges and so keeps features sharper. The median filter can be limited to despiking with low/high ratio thresholds: a value is only replaced if its ratio to the median is below `threshold_lo` or above `threshold_hi`, which is useful to suppress stripes of bad pings.

High-pass filters subtract a low-pass version of the data and add an offset so that the result stays positive. Contrast filters amplify noise and generally only work after smoothing; edge detection needs a window of at least 5x5.

The swath files themselves are not changed. The filtered values are written to ancillary files next to each input file: the file name with `.ffs` appended for sidescan and `.ffa` for amplitude. To use the filtered data, append `F` to the data kind of the plotting or mosaicking program, for example `mbmosaic -A4F` (filtered sidescan) or `-A3F` (filtered amplitude), or `gmt mbswath -Z5F` / `-Z4F`.

## Common options

- `-I<file>` / `--input=<file>` — swath file or datalist. Default: `datalist.mb-1`.
- `-A<kind>` / `--data-kind=<kind>` — filter beam amplitude (`1`) or sidescan (`2`). Default: 2.
- `-S<mode>/<xdim>/<ldim>[/<iteration>[/<threshold_lo>/<threshold_hi>]]` / `--smooth-filter=...` — low-pass filter: mode 1 boxcar mean, 2 Gaussian mean, 3 boxcar median, 4 inverse gradient. The thresholds only affect the median filter. Default: 1 iteration.
- `-D<mode>/<xdim>/<ldim>[/<iteration>[/<offset>]]` / `--hipass-filter=...` — high-pass filter: mode 1 boxcar mean, 2 Gaussian mean, 3 boxcar median subtraction. `offset` is added to the result to keep it positive; choose it for the value range of the data. Defaults: 1 iteration, offset 1000.
- `-C<mode>/<xdim>/<ldim>[/<iteration>]` / `--contrast-filter=...` — contrast enhancement: mode 1 edge detection, 2 gradient magnitude subtraction. Default: 1 iteration.
- `-T<threshold_lo>/<threshold_hi>` / `--threshold=...` — ratio thresholds for median smoothing filters given after this option.
- `-B<yr/mo/da/hr/mn/sc>`, `-E<yr/mo/da/hr/mn/sc>` / `--begin-time`, `--end-time` — time window of the data to filter.
- `-R<west/east/south/north>` / `--bounds=...` — geographic window of the data to filter.
- `-N<buffersize>` / `--buffer-size=<buffersize>` — number of records held in the buffer (10 to 5000). Default: 5000.
- `-F<format>` / `--format=<format>` — MBIO format id; normally inferred from the file name.
- `-V` / `--verbose` — print the filter settings and progress.

## Examples

One iteration of a 3x3 boxcar mean filter on the sidescan of the processed files:

    mbfilter -Idatalistp.mb-1 -S1/3/3/1

Five iterations of the 3x3 inverse gradient filter:

    mbfilter -Idatalistp.mb-1 -S4/3/3/5

High-pass filter to emphasise fine-scale structure, then smoothing to reduce speckle:

    mbfilter -Idatalistp.mb-1 -D1/3/10/1/10000 -S4/3/3/5

Smooth with a 7x7 median filter, then apply 5x5 edge detection:

    mbfilter -Idatalistp.mb-1 -S3/7/7/1 -C1/5/5/1

## Notes

- Each run overwrites the `.ffs` or `.ffa` files; filters from earlier runs are not accumulated, so give the whole filter chain in one command.
- A filter needs at least `mode/xdim/ldim`; with fewer values it is ignored.
- `-T` only applies to `-S` filters that come after it on the command line; alternatively give the thresholds as the 5th and 6th values of `-S3`.
- A datalist of the processed files (`datalistp.mb-1`) can be made with `mbdatalist -Idatalist.mb-1 -Z`.

## See also

`mbbackangle`, `mbprocess`, `mbmosaic`, `mbdatalist`, `gmt mbswath`. The complete reference is the MB-System manual page mbfilter(1) (English).
