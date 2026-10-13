# mbnavlist — 从条带数据中列出导航与姿态数据

`mbnavlist` 读取条带数据文件，并将导航数值（时间、位置、艏向、航速、声呐深度、横摇、纵摇、升沉等）以表格形式输出到屏幕。可用于提取船舶或潜器的航迹以便绘图、检查导航质量，或列出数据文件中存储的异步导航记录和姿态记录。

## 语法

    mbnavlist
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --binary {-A}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --data-kind=kind {-Kkind}
        --decimate=decimate {-Ddecimate}
        --delimiter=delimiter {-Gdelimiter}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --nav-channel=navchannel {-Nnavchannel}
        --output-format=output_format {-Ooutput_format}
        --projection=projection {-Jprojection}
        --segment=segment {-Zsegment}
        --speed-minimum=speed {-Sspeed}
        --time-gap=timegap {-Ttimegap}
        --verbose {-V}

## 说明

默认输出以制表符分隔的文本；`-G` 可指定其他分隔符，`-A` 输出双精度二进制数值，`-D` 对输出进行抽稀。输出列由 `-O` 选择，默认值为 `tMXYHs`（时间字符串、Unix 时间、经度、纬度、艏向、对地航速）。

默认情况下，导航数据取自该数据格式的"导航源"记录类型。许多格式还会在单独的记录中保存异步导航，有时来自多个导航系统。`-N0` 列出主异步导航，`-N1`、`-N2` 或 `-N3` 列出辅助导航通道。如果文件中没有所请求的记录，则不输出任何内容；可运行 `mbinfo -N` 查看文件中包含哪些记录类型。

`-K` 使输出在另一种记录类型的时刻进行，例如 `-K18` 表示在姿态记录的时刻输出，这样横摇、纵摇和升沉以其完整采样率列出，而位置等其他数值内插到这些时刻。`-K` 优先于 `-N`。

`-O` 可用字符：`T` 时间 yyyy/mm/dd/hh/mm/ss，`t` 时间 yyyy mm dd hh mm ss，`J` 和 `j` 以年和年积日表示的时间字符串，`M` Unix 时间（十进制秒），`m` 自第一条记录起的秒数，`U` Unix 时间（整数秒），`u` 自第一条记录起的整数秒，`V` 记录间隔 (s)，`X` 经度，`x` 经度（度 + 十进制分），`Y` 纬度，`y` 纬度（度 + 十进制分），`H` 艏向，`h` 对地航向，`S` 航速 (km/h)，`s` 对地航速 (km/h)，`L` 沿航迹距离 (km)，`l` 沿航迹距离 (m)，`c` 声呐深度 (m)，`p` 吃水 (m)，`R` 横摇，`P` 纵摇，`r` 升沉。前缀 `-`（取反号）、`/`（取倒数）和 `^`（将其后的 `X` 或 `Y` 转为 `-J` 所设投影中的东坐标或北坐标）修饰其后的数值。

## 常用选项

- `-Ifile` (`--input=file`) — 输入条带文件或数据列表。默认：datalist.mb-1。
- `-Fformat` (`--format=format`) — MBIO 格式 ID；数据列表为 -1。默认：根据文件名后缀推断。
- `-Ooutput_format` (`--output-format=output_format`) — 选择输出列的字符串。默认：`tMXYHs`。
- `-Nnavchannel` (`--nav-channel=navchannel`) — 列出异步导航：0 = 主通道，1 至 3 = 辅助通道。
- `-Kkind` (`--data-kind=kind`) — 触发输出的记录类型，例如 12 = 导航，17 = 艏向，18 = 姿态，28 至 30 = 辅助导航 1 至 3，53 = NMEA GGA。完整列表请参阅手册页。
- `-Ddecimate` (`--decimate=decimate`) — 每 decimate 条记录只输出一条。默认：1。
- `-Gdelimiter` (`--delimiter=delimiter`) — 文本输出的列分隔符。默认：制表符。
- `-A` (`--binary`) — 输出双精度二进制数值而不是文本。
- `-Jprojection` (`--projection=projection`) — `^X` / `^Y` 使用的投影坐标系，例如 `UTM32N`、`epsg32632`，或 `U` 表示按第一个数据确定分带的 UTM。
- `-Zsegment` (`--segment=segment`) — 用包含 segment 的行分隔每个输入文件的输出（GMT 多段文件使用 `>`）。指定 `swathfile` 或 `datalist` 时，写出 "#" 加源文件或数据列表的路径。
- `-Rwest/east/south/north` (`--bounds=...`) — 只使用该范围内的数据。
- `-Byr/mo/da/hr/mn/sc`, `-Eyr/mo/da/hr/mn/sc` (`--begin-time=...`, `--end-time=...`) — 只使用这两个时间之间的数据。
- `-Llonflip` (`--longitude-domain=lonflip`) — 经度范围：-1 = -360 至 0，0 = -180 至 180，1 = 0 至 360。默认：0。
- `-Sspeed` (`--speed-minimum=speed`), `-Ttimegap` (`--time-gap=timegap`) — 读取时的最低航速 (km/h) 和最大时间间断（分钟）。
- `-V` (`--verbose`), `-H` (`--help`) — 输出详细信息；打印用法后退出。

## 示例

列出一个文件的 Unix 时间、经度和纬度，并保存到文本文件：

    mbnavlist -I 0001_20250101_000000.mb58 -OMXY > nav.txt

将数据列表中每个文件的航迹写成 GMT 多段文件（`>` 加引号，以免 cmd.exe 将其视为重定向）：

    mbnavlist -I datalist.mb-1 -OXY "-Z>" > tracks.txt

按姿态记录的完整采样率列出横摇、纵摇和升沉：

    mbnavlist -I line1.mb88 -K18 -OMRPr > attitude.txt

以 UTM 东坐标和北坐标列出主异步导航：

    mbnavlist -I line1.mb88 -N0 "-OtM^X^Y" -JU > nav_utm.txt

## 注意

- 输出显示在屏幕上；要保存，请用 `> file.txt` 重定向。本程序没有指定输出文件的选项。
- 在 cmd.exe 中，包含 `^`、`>`、`|`、`&` 或 `%` 的参数请用双引号括起。

## 另请参阅

`mblist`, `mbinfo`, `mbdatalist`, `mbctdlist`, `mbauvloglist`, `mbformat`。完整参考请见 MB-System 手册页 mbnavlist(1)（英文）。
