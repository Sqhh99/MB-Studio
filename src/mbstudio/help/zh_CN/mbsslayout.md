# mbsslayout — 将时间序列侧扫铺设到海底（格式 71）

`mbsslayout` 将基于时间的原始侧扫声呐数据（左舷和右舷时间序列，例如 Edgetech 的 `.jsf` 文件）投影到平坦海底或地形网格上，转换为横向像素等间距的条带侧扫数据。结果写入 MB-System 格式 71 文件，之后可用 `mbbackangle` 改正、用 `mbprocess` 处理，并用 `mbmosaic` 做镶嵌。

## 语法

    mbsslayout [
        --verbose
        --help
        --input=datalist
        --format=format
        --platform-file=FILE
        --platform-target-sensor=SENSORID
        --output-source=record_kind
        --line-time-list=filename
        --line-route=filename
        --line-position-list=filename
        --line-name1=name
        --line-name2=name
        --line-range-threshold=value
        --topo-grid-file=filename
        --altitude-altitude
        --altitude-bottompick
        --altitude-bottompick-threshold=threshold[/blank]
        --altitude-topo-grid
        --channel-swap
        --swath-width=value
        --gain=value
        --interpolation=value
        --nav-file=filename
        --nav-file-format=format_id
        --nav-async=record_kind
        --sensordepth-file=filename
        --sensordepth-file-format=format_id
        --sensordepth-async=record_kind
        --altitude-file=filename
        --altitude-file-format=format_id
        --altitude-async=record_kind
        --heading-file=filename
        --heading-file-format=format_id
        --heading-async=record_kind
        --attitude-file=filename
        --attitude-file-format=format_id
        --attitude-async=record_kind
        --soundspeed-constant=value
        --soundspeed-file=filename
        --soundspeed-file-format=format_id
        --soundspeed-async=record_kind
        --time-latency-file=filename
        --time-latency-constant=value
        --time-latency-apply-nav
        --time-latency-apply-sensordepth
        --time-latency-apply-altitude
        --time-latency-apply-heading
        --time-latency-apply-attitude
        --time-latency-apply-all-ancilliary
        --time-latency-apply-survey
        --time-latency-apply-all
        --filter=value
        --filter-apply-nav
        --filter-apply-sensordepth
        --filter-apply-altitude
        --filter-apply-heading
        --filter-apply-attitude
        --filter-apply-all-ancilliary
        ]

## 说明

每个侧扫样本根据其双程旅行时、所在舷侧（左舷或右舷）、声速以及声呐的位置和姿态放置到海底上。样本被归入以声呐正下方为中心的等间距横向数组，同一像素中有多个样本时取平均，较小的空隙可用插值填充。输出条带最多 4001 个像素。默认情况下条带宽度随 ping 变化（W = 2.2 × sqrt(R² − A²)，R 为最大斜距，A 为高度），像素尺寸为 W / 4000；`--swath-width` 可设为固定宽度。

没有地形模型时，侧扫数据按声呐高度铺设在平坦海底上，高度默认取数据中记录的高度；`--altitude-bottompick` 则在左右舷时间序列中拾取首次海底回波，并取两者的平均值作为高度。使用 `--topo-grid-file` 时，每个样本被投影到 GMT 地形网格上（例如由同步或以往的多波束数据经 `mbgrid` 生成的网格），在起伏较大的海底也能得到正确位置。

导航、传感器深度、高度、艏向、姿态和声速默认取自输入文件。每一项都可以从外部文件（`--nav-file` 等）或数据中的其他记录类型（`--nav-async` 等）替换，也可以施加时间延迟改正和高斯时间域平滑；导航参考点与侧扫之间的偏移可用平台文件（`mbmakeplatform`）描述。

默认每个输入文件生成一个输出文件，文件名为去掉后缀的输入名加上 `_sidescan.mb71`（其中 `sidescan` 可用 `--line-name2` 设置）。当用 `--line-time-list`、`--line-route` 或 `--line-position-list` 定义测线时，输出按测线分割，文件名为 `Survey_sidescan_0000.mb71`、`Survey_sidescan_0001.mb71` 等（`Survey` 可用 `--line-name1` 设置）。

## 常用选项

- `--input=datalist` — 包含时间序列侧扫数据的数据列表或单个文件。默认值：`datalist.mb-1`。
- `--format=format` — 输入的格式编号；默认根据文件名推断。
- `--output-source=record_kind` — 包含要铺设的侧扫数据的记录类型：记录类型编号，如 1（测量数据，默认）或 38（第二侧扫），或 `low` / `sidescan`（类型 1）和 `high`（类型 38）。Edgetech `.jsf` 文件通常低频为类型 1、高频为类型 38；可用 `mbinfo -N` 列出文件中的记录类型。
- `--topo-grid-file=filename` — 将侧扫铺设到该 GMT 地形网格上。
- `--altitude-bottompick` — 用时间序列中的首次海底回波作为高度（平坦海底铺设）。`--altitude-bottompick-threshold=threshold[/blank]` 设置拾取阈值（相对于最大振幅的比例，默认 0.5）和可选的消隐时间（秒），并同时启用海底拾取。
- `--altitude-altitude` — 使用记录的高度（平坦海底铺设的默认方式）。
- `--swath-width=value` — 固定的输出条带宽度（米）。
- `--interpolation=value` — 用插值填充最多 `value` 个连续空像素。默认值：0（不插值）。
- `--gain=value` — 以该增益值进行增益改正。
- `--channel-swap` — 交换左舷和右舷通道（见“注意”）。
- `--line-time-list=filename` — 按该文件中的时间（历元秒，每行一个，例如由 `mbroutetime` 生成）将输出分割为测线。
- `--line-route=filename` — 按 MB-System 航线文件的航路点分割输出。`--line-position-list=filename` 则使用简单的 `longitude latitude` 航路点列表。
- `--line-range-threshold=value` — 判定到达航路点的距离（米）。默认值：50。
- `--line-name1=name`、`--line-name2=name` — 输出文件名的组成部分。默认值：`Survey` 和 `sidescan`。
- `--nav-file=filename`、`--nav-file-format=format_id` — 从外部文件合并导航（例如格式 1 = `time_d lon lat speed`，2 = `year month day hour minute second lon lat`，6 或 7 = NMEA 语句）。`--nav-async=record_kind` 从数据中的其他记录类型（例如 12、29、30、31）获取导航。
- `--sensordepth-file`、`--altitude-file`、`--heading-file`、`--attitude-file`、`--soundspeed-file` — 同样用于传感器深度、高度、艏向、姿态和声速，各自有对应的 `-file-format` 和 `-async` 选项。
- `--soundspeed-constant=value` — 固定水中声速（m/s）。默认值：1500。
- `--platform-file=FILE`、`--platform-target-sensor=SENSORID` — 描述传感器偏移的平台文件，以及其中侧扫传感器的编号。
- `--time-latency-constant=value` 或 `--time-latency-file=filename` — 时间延迟改正（秒），施加于由 `--time-latency-apply-nav`、`-sensordepth`、`-altitude`、`-heading`、`-attitude`、`-all-ancilliary`、`-survey` 或 `-all` 选定的数据。
- `--filter=value` — 高斯平滑滤波器的长度（秒），施加于由 `--filter-apply-nav`、`-sensordepth`、`-altitude`、`-heading`、`-attitude` 或 `-all-ancilliary` 选定的辅助数据。默认值：0（不滤波）。
- `--verbose` — 显示参数和处理进度。`--help` 显示用法。

## 示例

将数据列表中所有 Edgetech 文件的低频侧扫按记录的高度铺设在平坦海底上，并填充最多 5 个像素的空隙：

    mbsslayout --input=datalist.mb-1 --output-source=low --interpolation=5 --verbose

将高频侧扫以固定 150 m 条带宽度铺设到水深网格上：

    mbsslayout --input=datalist.mb-1 --output-source=high --topo-grid-file=survey_bathy.grd --swath-width=150 --line-name2=hf

用海底拾取得到高度，对姿态做 2 秒平滑，并按时间列表将输出分割为测线：

    mbsslayout --input=datalist.mb-1 --altitude-bottompick-threshold=0.5/0.01 --filter=2.0 --filter-apply-attitude --line-time-list=line_times.txt --line-name1=Area1

## 注意

- 所有选项均为 `--name=value` 长格式，没有单字母选项。
- 输出文件为格式 71；请为其建立数据列表（例如用 `mbdatalist` 或 `mbmakedatalist`）以便后续处理。
- 定义了测线时，输出文件写入当前目录；否则写在各输入文件旁边。
- 程序还会写出绘图脚本 `Survey_sidescan_ssswathplot.cmd`（按 `--line-name1` 和 `--line-name2` 命名），它调用 Windows 上不可用的 `mbm_plot` 宏；关于找不到 `chmod` 的提示可以忽略。
- 在当前源代码中 `--channel-swap` 需要一个值：请写成 `--channel-swap=1`（任何值都会启用交换）。

## 另请参阅

`mbbackangle`、`mbprocess`、`mbmosaic`、`mbgrid`、`mbmakeplatform`、`mbroutetime`、`mbinfo`、`mbdatalist`、`mb7k2jstar`。完整参考见 MB-System 手册页 mbsslayout(1)（英文）。
