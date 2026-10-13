# mblist — 将条带数据中的指定数值输出为文本或二进制表

`mblist` 读取条带数据文件，并将所选数值（时间、位置、水深、姿态、振幅、侧扫声呐等多种数据）以表格形式输出到屏幕或文件。可用于提取中央波束剖面、导航数据或供其他软件使用的"经度 纬度 水深"测深点，也可在处理过程中检查单个波束和像素。

## 语法

    mblist
        --beam-range=beam_start/beam_end|A|Xexcludepercent {-Mbeam_start/beam_end}
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --binary {-A}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --check-mode=check {-Ucheck}
        --decimate=decimate {-Kdecimate}
        --delimiter=delimiter {-Gdelimiter}
        --dump-mode=dumpmode {-Ddumpmode}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --netcdf {-C}
        --no-value-check {-Q}
        --output-file=outfile {-Xoutfile}
        --output-format=output_format {-Ooutput_format}
        --ping-average=pings {-Ppings}
        --pixel-range=pixel_start/pixel_end|A {-Npixel_start/pixel_end}
        --projection=projection {-Jprojection}
        --secondary-file=secondaryfile {-Ysecondaryfile}
        --segment=segment {-Zsegment}
        --speed-minimum=speed {-Sspeed}
        --time-gap=timegap {-Ttimegap}
        --use-feet {-W}
        --verbose {-V}

## 说明

默认情况下，`mblist` 每个 ping 输出一行，各列以制表符分隔。输出列由 `-O` 选择，它是一个字符串，每个字符选择一个数值；默认值为 `TXYHSLZ`（时间字符串、经度、纬度、艏向、航速、累计沿航迹距离、地形高程）。可用 `-G` 指定其他分隔符，用 `-A` 输出二进制（双精度），用 `-C` 输出 netCDF CDL。

在默认模式下，水深、振幅和侧扫声呐值取自最接近声呐正下方的波束和像素，位置为声呐的位置。若用 `-M` 或 `-N` 选择波束或像素范围，则每个所选波束或像素输出一行，位置为波束或像素在海底的位置。`-D` 是输出全部波束或像素"经度 纬度 数值"的快捷方式，便于作为网格化程序的输入。

默认不输出已标记值和空值。使用 `-Q` 或 `-U` 可输出已标记值（以及可选的空值）。

常用的 `-O` 字符：`T` 时间 yyyy/mm/dd/hh/mm/ss，`t` 时间 yyyy mm dd hh mm ss，`M` Unix 时间（十进制秒），`X` 经度，`Y` 纬度，`Z` 地形高程（向上为正），`z` 水深（向下为正），`H` 艏向，`S` 航速 (km/h)，`L` 沿航迹距离 (km)，`l` 沿航迹距离 (m)，`D` 横向距离 (m)，`E` 波束的纵向距离 (m)，`B` 振幅，`b` 侧扫声呐，`C` 声呐离底高度，`c` 声呐深度，`R` 横摇，`P` 纵摇，`r` 升沉，`F` 数值型波束标记，`f` 字符型波束标记，`#` 波束或像素编号，`N` ping 计数。前缀字符修饰其后的数值：`-` 取反号，`/` 取倒数，`^`（位于 `X` 或 `Y` 之前）给出 `-J` 所设投影（默认 UTM）中的东坐标或北坐标，`=` 和 `+` 分别取左舷端和右舷端有效波束的值。以 `,` 开头的代码（双程旅行时、角度、波束斜距）和以 `.` 开头的代码（格式特定的原始值、文件名、辅助文件中的列）请参阅手册页。特殊字符串 `%fnv` 选择用于 ".fnv" 快速导航文件的列（`tMXYHScRPr=X=Y+X+Y`）。

## 常用选项

- `-Ifile` (`--input=file`) — 输入条带文件或数据列表。默认：datalist.mb-1。
- `-Fformat` (`--format=format`) — MBIO 格式 ID；数据列表为 -1。默认：根据文件名后缀推断。
- `-Ooutput_format` (`--output-format=output_format`) — 选择输出列的字符串（见上文）。默认：`TXYHSLZ`。
- `-Mbeam_start/beam_end` (`--beam-range=...`) — 输出该范围内波束（从左舷侧以 0 开始编号）的记录。`-MA` 输出全部波束；`-MXpercent` 输出除外侧 percent % 以外的全部波束。
- `-Npixel_start/pixel_end` (`--pixel-range=...`) — 输出该范围内侧扫声呐像素的记录；`-NA` 输出全部像素。
- `-Ddumpmode` (`--dump-mode=dumpmode`) — 快捷转储：1 = 按 `-O` 格式（默认），2 = 经度 纬度 水深，3 = 经度 纬度 地形高程，4 = 经度 纬度 振幅，5 = 经度 纬度 侧扫声呐。优先于 `-O`、`-M` 和 `-N`。
- `-Ucheck` (`--check-mode=check`) — 已标记值和空值的处理方式：0 = 不输出（默认），1 = 输出已标记值、跳过空值，2 = 输出已标记值、空值输出为零，3 = 输出已标记值、空值输出为 NaN，4 = 已标记值和空值都输出为 NaN。
- `-Q` (`--no-value-check`) — 输出所有数值，已标记值保持不变，空值输出为零（与 `-U2` 相同）。
- `-Xoutfile` (`--output-file=outfile`) — 输出到该文件而不是屏幕。
- `-Gdelimiter` (`--delimiter=delimiter`) — 文本输出的列分隔符。默认：制表符。
- `-A` (`--binary`) — 输出双精度二进制数值而不是文本。
- `-Jprojection` (`--projection=projection`) — `^X` / `^Y` 使用的投影坐标系，例如 `UTM12N`、`epsg32612`，或 `U` 表示按第一个数据确定分带的 UTM。
- `-Kdecimate` (`--decimate=decimate`) — 每 decimate 条记录只输出一条。默认：1。
- `-Ppings` (`--ping-average=pings`) — 将该数量的 ping 平均为一个。默认：1。
- `-Rwest/east/south/north` (`--bounds=...`) — 只使用该范围内的数据。
- `-Byr/mo/da/hr/mn/sc`, `-Eyr/mo/da/hr/mn/sc` (`--begin-time=...`, `--end-time=...`) — 只使用这两个时间之间的数据。
- `-Zsegment` (`--segment=segment`) — 用包含 segment 的行分隔每个输入文件的文本输出（GMT 多段文件使用 `>`）。指定 `swathfile` 或 `datalist` 时，写出 "#" 加源文件或数据列表的路径。
- `-Ysecondaryfile` (`--secondary-file=secondaryfile`) — 文本文件，第一列为 Unix 时间，其后最多 19 列为数据；数值按 ping 时间内插，可在 `-O` 中用 `.1C` … `.19C` 输出。
- `-W` (`--use-feet`) — 以英尺输出水深和距离。
- `-C` (`--netcdf`) — 输出 netCDF CDL；与 `-A` 一起使用时，用 `ncgen` 转换为二进制 netCDF 文件。
- `-V` (`--verbose`), `-H` (`--help`) — 输出详细信息；打印用法后退出。

## 示例

将每个 ping 中央波束的时间、位置和水深写入文本文件：

    mblist -I 0001_20250101_000000.mb58 -OtXYz -X centre.txt

输出数据列表中所有有效测深点的经度、纬度和水深：

    mblist -I datalist.mb-1 -D2 -X soundings.xyz

列出一个文件中所有波束（包括已标记波束）的波束编号、波束标记字符和水深：

    mblist -I line1.mb88 -MA -O#fz -U1 -X beams.txt

以逗号分隔输出 UTM 东坐标、北坐标和水深（由于 `^` 在 cmd.exe 中有特殊含义，格式字符串需加引号）：

    mblist -I datalist.mb-1 -MA "-O^X^Yz" -JU -G, -X soundings_utm.csv

## 注意

- 在 cmd.exe 中，`^`、`>`、`|`、`&` 和 `%` 有特殊含义。包含这些字符的参数请用双引号括起，例如 `"-O^X^Yz"` 或 `"-Z>"`。在批处理文件中请写 `%%fnv` 而不是 `%fnv`。
- 不使用 `-X` 时输出显示在屏幕上；要保存，请使用 `-X` 或用 `> file.txt` 重定向。
- 二进制 netCDF 输出（`-C -A`）需要 PATH 中有 netCDF 的 `ncgen` 程序；结果写入 `-X` 指定的文件（未指定时为 mblist.nc）。

## 另请参阅

`mbinfo`, `mbnavlist`, `mbdatalist`, `mbformat`, `mbprocess`, `mbgrid`。完整参考请见 MB-System 手册页 mblist(1)（英文）。
