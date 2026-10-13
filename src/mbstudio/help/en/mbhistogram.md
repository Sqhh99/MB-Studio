# mbhistogram — Histogram of bathymetry, amplitude or sidescan values

`mbhistogram` reads swath data and computes a histogram of the bathymetry, beam amplitude or sidescan values. Instead of the histogram it can output a set of values that split the data distribution into equal-sized parts, which is useful for building histogram-equalized color tables for amplitude and sidescan images.

## Synopsis

    mbhistogram [-Akind -Byr/mo/da/hr/mn/sc -Dmin/max -Eyr/mo/da/hr/mn/sc -Fformat -G -Ifile -Llonflip -Mnintervals -Nnbins -Ppings -Rw/e/s/n -Sspeed -V -H]
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --bins=nbins {-Nnbins}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --data-type=kind {-Akind}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format_id {-Fformat_id}
        --gaussian {-G}
        --help {-H}
        --input=[file | datalist] {-Ifile}
        --intervals=nintervals {-Mnintervals}
        --longitude-domain=lonflip {-Llonflip}
        --pings=pings {-Ppings}
        --speed-minimum=speed {-Sspeed}
        --time-gap=timegap {-Ttimegap}
        --value-range=min/max {-Dmin/max}
        --verbose {-V}

## Description

The kind of data is chosen with `-A`: 0 = bathymetry, 1 = beam amplitude, 2 = sidescan (default). The histogram covers the range given with `-D`, divided into the number of bins set with `-N`. The min value is the centre of the first bin and the max value the centre of the last bin; values outside the bins are ignored. Each output line gives the bin centre and the number of values in that bin. The results are written to the screen.

With `-M` the program instead writes the given number of values that break the distribution into equal-sized regions (the first and last values are the minimum and maximum of the data, limited to the `-D` range). These values can be used as the boundaries of a color table so that each color covers the same number of data values (histogram equalization). With `-G` the histogram is recast into a Gaussian distribution before output or before the intervals are calculated, similar to the Gaussian option of the GMT program grdhisteq; this suits shading of images, while the linear form suits grayscale amplitude and sidescan images.

## Common options

- `-Ifile` (`--input=file`) — Input swath file or datalist. Default: stdin.
- `-Fformat_id` (`--format=format_id`) — MBIO format id; -1 for a datalist. Default: inferred from the file name suffix.
- `-Akind` (`--data-type=kind`) — Data used: 0 = bathymetry, 1 = beam amplitude, 2 = sidescan. Default: 2.
- `-Dmin/max` (`--value-range=min/max`) — Centres of the first and last bins. Default: 0/128.
- `-Nnbins` (`--bins=nbins`) — Number of bins. Default: 16, or 50 times the number of intervals when `-M` is used.
- `-Mnintervals` (`--intervals=nintervals`) — Output this many values that divide the distribution into equal parts instead of the histogram.
- `-G` (`--gaussian`) — Recast the histogram into a Gaussian distribution.
- `-Ppings` (`--pings=pings`) — Average this many pings into one. Default: 1.
- `-Rwest/east/south/north` (`--bounds=...`) — Use only data within these bounds.
- `-Byr/mo/da/hr/mn/sc`, `-Eyr/mo/da/hr/mn/sc` (`--begin-time=...`, `--end-time=...`) — Use only data between these times.
- `-Llonflip` (`--longitude-domain=lonflip`) — Longitude range: -1 = -360 to 0, 0 = -180 to 180, 1 = 0 to 360.
- `-Sspeed` (`--speed-minimum=speed`), `-Ttimegap` (`--time-gap=timegap`) — Minimum speed (km/hr) and maximum time gap (minutes) used when reading.
- `-V` (`--verbose`), `-H` (`--help`) — Verbose messages; print usage and exit.

## Examples

Histogram of the sidescan values of a file in 25 bins from 0 to 45000:

    mbhistogram -I line1.mb41 -D0/45000 -N25

Depth histogram of all files in a datalist, 10 m bins from 0 to 500 m:

    mbhistogram -I datalist.mb-1 -A0 -D0/500 -N51 > depth_hist.txt

Sixteen values that split the amplitude distribution into equal parts, for a color table:

    mbhistogram -I datalist.mb-1 -A1 -D0/255 -M16

## Notes

- Choose `-D` to cover the actual data range (see the minimum and maximum reported by `mbinfo`); with the default 0/128 many values may fall outside the bins.
- Always give the input with `-I`; reading from standard input is the default but is rarely useful.

## See also

`mbinfo`, `mblist`, `mbdatalist`, `mbgrid`, `mbmosaic`, `mbbackangle`. The complete reference is the MB-System manual page mbhistogram(1) (English).
