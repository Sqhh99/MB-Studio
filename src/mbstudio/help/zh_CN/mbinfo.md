# mbinfo — 报告条带数据文件的统计信息与范围

`mbinfo` 读取单个条带数据文件，或数据列表引用的所有文件，并报告基本统计信息：数据计数、导航汇总、首末记录，以及各数值的最小值、最大值和地理范围。在处理流程初期可用它检查文件内容，也可用它生成其他 MB-System 程序读取的 ".inf" 元数据文件。

## 语法

    mbinfo
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --comments {-C}
        --debug-record-types
        --debug-record-contents=identifier
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format_id { -Fformat_id}
        --good-nav {-G}
        --help {-H}
        --input=[file | datalist] {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --mask-dimensions=londim/latdim[/west/east/south/north] {-Mlondim/latdim[/west/east/south/north]}
        --notices {-N}
        --output-file {-O}
        --output-format=format_id {-Xformat_id}
        --ping-variances=number {-Ppings}
        --quick {-Q}
        --speed-minimum=speed {-Sspeed}
        --time-gap=timegap {-Ttimegap}
        --use-feet {-W}
        --verbose {-V}

## 说明

报告内容包括文件名、MBIO 格式 ID 及格式说明、由 `mbprocess` 写入的元数据（声呐型号、船名、偏差参数等），以及水深、振幅和侧扫声呐数值的个数。对每种数据类型，分别给出有效值、零值和已标记值的个数与百分比；"有效值"是指既不为零、也未被标记为坏值的数据。报告还给出总时长、航迹长度、平均航速、首末记录的时间和位置，以及经度、纬度、水深、高度、振幅和侧扫声呐值的最小值与最大值。

`mbinfo` 的一个重要用途是生成 ".inf" 文件。`mbgrid`、`mbmosaic` 等程序会读取每个条带文件对应的 ".inf" 文件（如果存在），从而跳过在目标区域内没有数据的文件。使用 `-O` 选项时，报告不输出到屏幕，而是写入以输入文件名加 ".inf" 命名的文件。要为数据列表中的所有文件生成 ".inf" 文件，请使用 `mbdatalist -N` 或 `mbdatalist -O`。

`-X` 选项用于更改报告格式：`-X1` 输出 JSON，`-X2` 输出 XML。与 `-O` 一起使用时，输出文件名的后缀分别为 "_inf.json" 或 "_inf.xml"。

如果输入是数据列表（格式 -1，例如以 ".mb-1" 结尾的文件），则统计结果是所有被引用文件的合计。未指定格式时，`mbinfo` 会根据文件名后缀推断格式（例如 ".mb58" 表示格式 58）。

`-P` 用于开启方差计算：按指定数量的 ping 分组读取，并输出每个波束（水深先去趋势）和每个像素的方差，可粗略反映整个条带宽度上的噪声水平。`-M` 会附加一个粗略的覆盖掩膜（由 0 和 1 组成的表），显示数据范围内哪些位置有数据。读取数据列表或使用 `-Q` 时不进行方差计算。

## 常用选项

- `-Ifile` (`--input=file`) — 输入条带数据文件或数据列表。默认：标准输入。
- `-Fformat_id` (`--format=format_id`) — 输入的 MBIO 格式 ID。负值 (-1) 表示输入为数据列表。默认：根据文件名后缀推断。
- `-O` (`--output-file`) — 将报告写入以输入文件名加 ".inf" 命名的文件，而不是屏幕（使用 `-X1` / `-X2` 时 ".inf" 变为 "_inf.json" / "_inf.xml"）。
- `-Xformat_id` (`--output-format=format_id`) — 报告格式：0 = 文本（默认），1 = JSON，2 = XML。
- `-N` (`--notices`) — 附加通知信息：发现的数据记录类型、非致命读取错误，以及零值导航或水深过大等数据问题。
- `-G` (`--good-nav`) — 计算经纬度范围时忽略零值位置以及意味着航速过大的位置。便于得到合理的绘图范围。
- `-Mlondim/latdim[/west/east/south/north]` (`--mask-dimensions=...`) — 附加 londim × latdim 个格元的覆盖掩膜。不指定范围时数据会被读取两次。
- `-Ppings` (`--ping-variances=number`) — 以该数量的 ping 为一组计算波束和像素的方差。默认：1（不计算方差）。
- `-Q` (`--quick`) — 从已有的 ".inf" 文件读取统计信息，而不读取条带文件。此模式下无法输出方差、通知和掩膜。
- `-C` (`--comments`) — 打印数据中的注释记录。
- `-Rwest/east/south/north` (`--bounds=...`) — 只使用该经纬度范围内的数据。默认：-360/360/-90/90。
- `-Byr/mo/da/hr/mn/sc`, `-Eyr/mo/da/hr/mn/sc` (`--begin-time=...`, `--end-time=...`) — 只使用这两个时间之间的数据。若开始时间晚于结束时间，则改为排除这两个时间之间的数据。
- `-Llonflip` (`--longitude-domain=lonflip`) — 输出经度的范围：-1 = -360 至 0，0 = -180 至 180，1 = 0 至 360。默认：根据第一个数据自动选择。
- `-Sspeed` (`--speed-minimum=speed`) — 忽略船速低于该值 (km/h) 的 ping。默认：0。
- `-Ttimegap` — 相邻 ping 之间的时间间隔超过该值（分钟）时视为数据间断。默认：1。
- `-W` (`--use-feet`) — 以英尺而不是米报告水深。
- `-V` (`--verbose`) — 详细输出，包括程序版本和读取错误信息。
- `-H` (`--help`) — 打印简要说明和用法后退出。

## 示例

报告单个 Kongsberg 文件的统计信息（格式由后缀确定）：

    mbinfo -I 0001_20250101_000000.mb58

在数据文件旁生成 ".inf" 文件，包含记录类型通知和 10 × 10 的覆盖掩膜：

    mbinfo -I 0001_20250101_000000.mb58 -N -M10/10 -O

报告数据列表中所有文件的合计统计，并使用合理的范围：

    mbinfo -I datalist.mb-1 -G

将文件的元数据以 JSON 格式写出（生成 "line1.mb88_inf.json"）：

    mbinfo -I line1.mb88 -X1 -O

## 注意

- 含空格的路径请加引号，例如 `-I "D:\Survey 2025\line1.mb58"`。
- 使用 `-O` 时，".inf" 文件写在输入文件旁边（输入路径加后缀），而不是当前目录。
- 请使用短格式 `-T` 设置时间间断；此版本无法正确识别长格式 `--time-gap`。
- `--debug-record-types` 和 `--debug-record-contents=identifier` 是面向开发者的选项，用于列出记录类型和转储记录内容。

## 另请参阅

`mbdatalist`, `mbformat`, `mblist`, `mbprocess`, `mbgrid`, `mbmosaic`, `mbset`。完整参考请见 MB-System 手册页 mbinfo(1)（英文）。
