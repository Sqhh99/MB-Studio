# mbhistogram — 水深、振幅或侧扫声呐数值的直方图

`mbhistogram` 读取条带数据，计算水深、波束振幅或侧扫声呐数值的直方图。它也可以不输出直方图，而输出一组将数据分布划分为等量区间的数值，可用于为振幅和侧扫声呐图像构建直方图均衡化色表。

## 语法

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

## 说明

数据类型由 `-A` 选择：0 = 水深，1 = 波束振幅，2 = 侧扫声呐（默认）。直方图覆盖 `-D` 给定的范围，并划分为 `-N` 设定数量的格元。min 值为第一个格元的中心，max 值为最后一个格元的中心；落在格元范围之外的数值被忽略。每行输出给出格元中心值及落入该格元的数值个数。结果输出到屏幕。

使用 `-M` 时，程序不输出直方图，而是输出指定个数的数值，将分布划分为等量区间（第一个和最后一个值为限制在 `-D` 范围内的数据最小值和最大值）。将这些数值用作色表的分界，可使每种颜色覆盖相同数量的数据值（直方图均衡化）。使用 `-G` 时，在输出或计算区间之前先将直方图变换为高斯分布，类似于 GMT 程序 grdhisteq 的高斯选项；这适合图像的晕渲，而线性形式适合灰度振幅图像和侧扫声呐图像。

## 常用选项

- `-Ifile` (`--input=file`) — 输入条带文件或数据列表。默认：标准输入。
- `-Fformat_id` (`--format=format_id`) — MBIO 格式 ID；数据列表为 -1。默认：根据文件名后缀推断。
- `-Akind` (`--data-type=kind`) — 使用的数据：0 = 水深，1 = 波束振幅，2 = 侧扫声呐。默认：2。
- `-Dmin/max` (`--value-range=min/max`) — 第一个和最后一个格元的中心值。默认：0/128。
- `-Nnbins` (`--bins=nbins`) — 格元数。默认：16；使用 `-M` 时为区间数的 50 倍。
- `-Mnintervals` (`--intervals=nintervals`) — 输出该数量的将分布等分的数值，而不是直方图。
- `-G` (`--gaussian`) — 将直方图变换为高斯分布。
- `-Ppings` (`--pings=pings`) — 将该数量的 ping 平均为一个。默认：1。
- `-Rwest/east/south/north` (`--bounds=...`) — 只使用该范围内的数据。
- `-Byr/mo/da/hr/mn/sc`, `-Eyr/mo/da/hr/mn/sc` (`--begin-time=...`, `--end-time=...`) — 只使用这两个时间之间的数据。
- `-Llonflip` (`--longitude-domain=lonflip`) — 经度范围：-1 = -360 至 0，0 = -180 至 180，1 = 0 至 360。
- `-Sspeed` (`--speed-minimum=speed`), `-Ttimegap` (`--time-gap=timegap`) — 读取时的最低航速 (km/h) 和最大时间间断（分钟）。
- `-V` (`--verbose`), `-H` (`--help`) — 输出详细信息；打印用法后退出。

## 示例

计算一个文件侧扫声呐数值从 0 到 45000、共 25 个格元的直方图：

    mbhistogram -I line1.mb41 -D0/45000 -N25

计算数据列表中所有文件从 0 到 500 m、格元宽 10 m 的水深直方图：

    mbhistogram -I datalist.mb-1 -A0 -D0/500 -N51 > depth_hist.txt

为色表计算将振幅分布等分的 16 个数值：

    mbhistogram -I datalist.mb-1 -A1 -D0/255 -M16

## 注意

- `-D` 应覆盖实际数据范围（参见 `mbinfo` 报告的最小值和最大值）；使用默认的 0/128 时，可能有许多数值落在格元之外。
- 请始终用 `-I` 指定输入；默认从标准输入读取，但实际很少有用。

## 另请参阅

`mbinfo`, `mblist`, `mbdatalist`, `mbgrid`, `mbmosaic`, `mbbackangle`。完整参考请见 MB-System 手册页 mbhistogram(1)（英文）。
