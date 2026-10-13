# mbcopy — 带格式转换、范围截取和平均的条带文件复制

`mbcopy` 将条带声呐数据文件复制为新文件，可选择转换为其他 MBIO 格式、按时间或地理范围截取、对 ping 求平均，或从另一个文件合并水深。它是进行格式转换和提取测线子集的通用工具。

## 语法

    mbcopy
        --bathymetry-only {-D}
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --comment-file=commentfile {-Ccommentfile}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=iformat/oformat/mformat {-Fiformat/oformat/mformat}
        --help {-H}
        --input=infile {-Iinfile}
        --lonflip=lonflip {-Llonflip}
        --merge-file=mergefile {-Mmergefile}
        --output=outfile {-Ooutfile}
        --ping-average=pings {-Ppings}
        --sleep-factor=sleep_factor {-Qsleep_factor}
        --speed-minimum=speed {-Sspeed}
        --strip-comments {-N}
        --time-gap=timegap {-Ttimegap}
        --verbose {-V}

## 说明

`mbcopy` 读取一个条带文件（不是数据列表）并写出一个输出文件。当输入和输出格式属于同一声呐系统时，复制完整的数据流；属于不同系统时，只传递时间、导航和波束数值（水深、振幅、侧扫声呐）。使用 ping 平均时同样只进行部分复制。

数据可以按时间范围（`-B`、`-E`）、地理范围（`-R`）和最低航速（`-S`）进行限制。如果起始时间晚于结束时间，则改为排除两个时间之间的数据。

使用 `-M` 时，水深和波束标记取自合并文件，并与输入文件中的其他所有数据一起写出。这用于合并同一原始数据的两条处理流程，例如将其他软件导出的已改正水深与原始的姿态和反向散射记录相结合。

默认情况下，`mbcopy` 会添加记录运行情况的注释记录，并传递输入中的注释。未指定 `-I` 时从标准输入读取，未指定 `-O` 时写到标准输出。

## 常用选项

- `-I<file>` / `--input=<file>` — 输入条带文件。默认值：标准输入。
- `-O<file>` / `--output=<file>` — 输出条带文件。默认值：标准输出。
- `-F<iformat>[/<oformat>[/<mformat>]]` / `--format=...` — 输入、输出和合并文件的格式编号。只给出输入格式时，输出格式与之相同。省略时，输入格式根据输入文件名（例如 `.mb58`）推断，合并格式根据合并文件名推断。
- `-B<yr/mo/da/hr/mn/sc>` / `--begin-time=...` — 时间范围起点。默认值：1962/2/21/10/30/0。
- `-E<yr/mo/da/hr/mn/sc>` / `--end-time=...` — 时间范围终点。默认值：2062/2/21/10/30/0。
- `-R<west/east/south/north>` / `--bounds=...` — 只复制该经纬度范围内的数据。
- `-P<pings>` / `--ping-average=<pings>` — 将这么多个 ping 平均为一个输出 ping；`0` 表示自动确定，使沿航向间距等于横向波束间距。默认值：1（不平均）。
- `-S<speed>` / `--speed-minimum=<speed>` — 跳过航速低于该值（km/h）的 ping。默认值：0。
- `-T<timegap>` / `--time-gap=<timegap>` — 视为数据间断的 ping 间时间间隔（分钟）。默认值：1。
- `-M<mergefile>` / `--merge-file=<mergefile>` — 从该文件获取水深和波束标记。
- `-D` / `--bathymetry-only` — 只写出水深（不写振幅和侧扫声呐）；仅在输出格式为 71 时有效。快速水深（`.fbt`）文件就是这样生成的。
- `-N` / `--strip-comments` — 不写 `mbcopy` 的注释并丢弃输入中的注释。指定两次（`-N -N`）时，还只传递水深、导航、艏向和姿态记录（适用于区分这些记录类型的格式）。
- `-C<commentfile>` / `--comment-file=<commentfile>` — 将文本文件的各行作为注释插入到输出开头。
- `-L<lonflip>` / `--lonflip=<lonflip>` — 经度范围：`-1` 为 -360 到 0，`0` 为 -180 到 180，`1` 为 0 到 360。
- `-Q<sleep_factor>` / `--sleep-factor=<sleep_factor>` — 在每条记录之前暂停 ping 间隔乘以该系数的时间，以模拟数据采集系统。
- `-V` / `--verbose` — 显示记录数和消息。

## 示例

将原始 Hydrosweep 文件（格式 21）转换为格式 24，只保留西经 145 度至 140 度、南纬 10 度至 5 度之间的数据：

    mbcopy -F21/24 -R-145/-140/-10/-5 -Ihs_raw -Ohs_binary

从 Kongsberg 文件中提取一小时的数据到相同格式的新文件：

    mbcopy -I0001_20250101_000000.mb58 -B2025/01/01/10/00/00 -E2025/01/01/11/00/00 -Oline1_part.mb58

将原始 Simrad 数据（格式 56，以格式 57 写出）与 GSF 文件（格式 121）中的已改正水深合并：

    mbcopy -F56/57/121 -I0015_20040212_052714_raw.all -M20040212_052714.gsf -O20040212_052714.mb57

以格式 71 写出紧凑的纯水深副本：

    mbcopy -F58/71 -D -I0001_20250101_000000.mb58 -O0001_20250101_000000.mb71

## 注意

- 请始终指定 `-O`：条带数据是二进制的，不能输出到控制台。
- 并非所有输入与输出格式的组合都有意义；在不同声呐系统之间转换会丢失系统特有的记录。
- 包含空格的文件名请加引号，例如 `-I"D:\Survey 2025\line1.mb58"`。

## 另请参阅

`mbformat`, `mbinfo`, `mbdatalist`, `mbpreprocess`, `mbprocess`, `mblist`。完整参考资料为 MB-System 手册页 mbcopy(1)（英文）。
