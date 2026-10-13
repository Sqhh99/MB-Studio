# mbctdlist — 列出条带数据中的 CTD 记录

`mbctdlist` 列出条带数据文件中包含的 CTD（电导率、温度、深度）记录，以及时间和导航数值。一些 ROV 和 AUV 测深平台会将 CTD 数据记录在声呐数据流中；可用本程序提取这些数据，例如检查水体性质或推导声速信息。

## 语法

    mbctdlist
        --binary {-A}
        --decimate=decimate {-Ddecimate}
        --delimiter=delimiter {-Gdelimeter}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --output-format=output_format {-Ooutput_format}
        --segment=segment {-Zsegment}
        --verbose {-V}

## 说明

对找到的每条 CTD 记录，`mbctdlist` 输出一行，包含 `-O` 所选的数值。可用的数据有电导率 (S/m)、温度 (°C)、盐度和水中声速，以及最多 8 个辅助传感器的原始电压。时间、位置、艏向、航速、声呐深度和离底高度取自测深记录，并内插到每条 CTD 记录的时刻。如果文件中没有测深记录，则不列出 CTD 记录。

默认输出以制表符分隔的文本；`-G` 可指定其他分隔符，`-A` 输出双精度二进制数值，`-D` 对输出进行抽稀。只有存储 CTD 记录的数据格式才会产生输出（最初仅支持格式 88，Reson 7k）。

`-O` 可用字符：`T` 时间 yyyy/mm/dd/hh/mm/ss，`t` 时间 yyyy mm dd hh mm ss，`J` 和 `j` 以年和年积日表示的时间字符串，`M` Unix 时间（十进制秒），`m` 自第一条记录起的秒数，`U` Unix 时间（整数秒），`u` 自第一条记录起的整数秒，`V` 记录间隔 (s)，`X` 经度，`x` 经度（度 + 十进制分），`Y` 纬度，`y` 纬度（度 + 十进制分），`H` 艏向，`h` 对地航向，`L` 沿航迹距离 (km)，`l` 沿航迹距离 (m)，`C` 电导率，`c` 温度，`S` 盐度，`s` 声速 (m/s)，`P` 位温，`1` 至 `8` 传感器电压。加前缀 `^` 时：`^C` 声呐离底高度 (m)，`^c` 声呐深度 (m)，`^S` 航速 (km/h)，`^s` 对地航速 (km/h)。前缀 `-`（取反号）和 `/`（取倒数）修饰其后的数值。默认值为 `TXYHCc^cSs`（时间、经度、纬度、艏向、电导率、温度、声呐深度、盐度、声速）。

## 常用选项

- `-Ifile` (`--input=file`) — 输入条带文件或数据列表。默认：datalist.mb-1。
- `-Fformat` (`--format=format`) — MBIO 格式 ID；数据列表为 -1。默认：根据文件名后缀推断。
- `-Ooutput_format` (`--output-format=output_format`) — 选择输出列的字符串（见上文和"注意"）。默认：`TXYHCc^cSs`。
- `--decimate=decimate` — 每 decimate 条记录只输出一条。默认：1。
- `-Gdelimiter` (`--delimiter=delimiter`) — 文本输出的列分隔符。默认：制表符。
- `-A` (`--binary`) — 输出双精度二进制数值而不是文本。
- `-Zsegment` (`--segment=segment`) — 在每个输入文件的输出之前写一行 segment（GMT 多段文件使用 `>`）。仅用于文本输出。
- `-Llonflip` (`--longitude-domain=lonflip`) — 经度范围：-1 = -360 至 0，0 = -180 至 180，1 = 0 至 360。默认：0。
- `-V` (`--verbose`), `-H` (`--help`) — 输出详细信息；打印用法后退出。

## 示例

以默认列输出 Reson 7k 文件的 CTD 记录：

    mbctdlist -I 20080904_231809p.mb88 > ctd.txt

输出数据列表中所有文件的 CTD 记录，每 10 条只取 1 条，并以逗号分隔：

    mbctdlist -I datalist.mb-1 --decimate=10 -G, > ctd.csv

用自定义的 10 个字符替换默认列（Unix 时间、经度、纬度、声呐深度、温度、盐度、声速、电导率、艏向；由于 `^` 在 cmd.exe 中有特殊含义，参数需加引号）：

    mbctdlist -I 20080904_231809p.mb88 "-OMXY^ccSsCH" > ctd_custom.txt

## 注意

- 在此版本中，自定义的 `-O` 字符串始终按 10 个字段代码处理：所给字符替换默认值 `TXYHCc^cSs`（其中 `^c` 计为两个字符）的开头部分，第 10 个之后的字符被忽略。要完全控制所有列，请正好给出 10 个字符。
- 抽稀请使用长格式 `--decimate=N`；在此版本中短格式 `-D` 无法正确接收数值。
- 输出显示在屏幕上；要保存，请用 `> file.txt` 重定向。在 cmd.exe 中，包含 `^` 或 `>` 的参数请用双引号括起。
- 不含 CTD 记录的文件不会产生输出。

## 另请参阅

`mblist`, `mbnavlist`, `mbsvplist`, `mbinfo`, `mbdatalist`, `mblevitus`。完整参考请见 MB-System 手册页 mbctdlist(1)（英文）。
