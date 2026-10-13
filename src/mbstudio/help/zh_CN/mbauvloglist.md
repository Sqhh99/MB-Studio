# mbauvloglist — 从 MBARI AUV 任务日志中列出指定字段

`mbauvloglist` 读取 MBARI AUV 的二进制任务日志文件（例如 `navigation.log`、`fastcatlog.log`、`kearfott.log`），并将所选字段输出为文本或二进制表格。可用它从 AUV 日志中提取导航、姿态、CTD 或声速等时间序列，例如用于建立声速剖面或制作预处理所需的辅助导航文件。

## 语法

    mbauvloglist
        --altitude-file=file {-Afile}
        --angles-degrees {-S}
        --clip-to-nav {-C}
        --decimate=value {-Dvalue}
        --help {-H}
        --input=file {-Ifile}
        --lonflip=lonflip {-Llonflip}
        --nav-file=file {-Nfile}
        --output-field=name {-Oname}
        --output-mode=mode {-Mmode}
        --print-format=format {-Fformat}
        --print-header {-P}
        --recalculate-ctd=id {-Rid}
        --scale=value {-Xvalue}
        --verbose {-V}

## 说明

MBARI AUV 任务日志是以 ASCII 文件头开始的二进制文件。文件头的每一行描述固定长度二进制记录中的一个值：类型、名称、默认输出格式、说明和单位，例如 `# double calculated_sound_velocity %8.8e ,Calculated sound velocity ,M/s`。

要查看日志中包含哪些值，可只使用 `-P` 运行程序：它会打印文件头后退出。然后按所需的列顺序，为每个值指定一个 `-O` 再次运行。如果没有给出任何 `-O`，则输出日志中的全部字段。结果写到标准输出，可用 `>` 重定向到文件。

除非用 `-F` 指定 printf 风格的格式，数值将按日志文件头中的格式输出；`-F` 对其后的所有 `-O` 字段有效，直到再次更改（`-Fdefault` 恢复文件头格式）。对于记录时间字段（日志中名为 `time`），`-Ftime_i` 输出年、月、日、时、分、秒，`-Ftime_j` 输出年、年积日、时、分、秒。同样，`-X` 设置一个比例因子，乘到其后的浮点 `-O` 字段上。

除文件头中定义的字段外，还可请求以下特殊值：`zero`、`timeTag`、`timeInterval`（距上一条记录的时间）、由 `-N` 指定的外部导航文件插值得到的值（`mergeLon`、`mergeLat`、`mergeHeading`、`mergeSpeed`、`mergeDraft`、`mergeSensordepth`、`mergeRoll`、`mergePitch`、`mergeHeave`）、由 `-A` 指定的高度文件得到的 `mergeAltitude`、根据 `fastcatlog.log` 计算的 CTD 值（`calcConductivity`、`calcTemperature`、`calcPressure`、`calcSalinity`、`calcSoundspeed`、`calcPotentialTemperature`、`calcDensity`），以及由 Kearfott/Phins 数据导出的值（`calcKTime`、`calcKSpeed`、`calcPSpeed`）。

## 常用选项

- `-Ifile` — 输入的 MBARI AUV 任务日志文件。
- `-P` — 打印日志文件头。单独使用时列出可用字段后退出。
- `-Oname` — 输出指定字段。每列指定一次，按所需顺序重复使用。
- `-Fformat` — 其后 `-O` 字段使用的 printf 风格格式；时间字段可用 `time_i` 或 `time_j`；`default` 恢复文件头格式。
- `-Mmode` — 输出模式：`0` 制表符分隔文本（默认），`1` 逗号分隔文本，`2` 无分隔的二进制值。
- `-Nfile` — "fnv" 格式的导航文件，其插值结果可作为 `merge...` 字段输出。
- `-C` — 与 `-N` 一起使用时，不输出时间超出导航数据时间范围的记录。默认会输出这些记录，且合并值为 0。
- `-Afile` — ASCII 高度文件（每行一对“时间 高度”，单位为秒和米），其插值结果可作为 `mergeAltitude` 输出。
- `-Dvalue` — 每 value 条记录只输出一条。默认值：`1`（全部记录）。
- `-Xvalue` — 乘到其后浮点 `-O` 字段上的比例因子。默认值：`1.0`。
- `-S` — 将 Kearfott、Crossbow 和 Phins 日志中的角度字段从弧度转换为度。
- `-Rid` — 对 `fastcatlog.log`，使用 id 指定的校准系数重新计算温度、电导率、盐度和压力（目前只定义了 `1`）。
- `-Llonflip` — 经度范围：`-1` 为 -360 到 0，`0` 为 -180 到 180，`1` 为 0 到 360。默认值：由 `mbdefaults` 设置的值。
- `-V` — 详细模式。
- `-H` — 显示帮助后退出。

## 示例

查看导航日志中保存了哪些字段：

    mbauvloglist -I navigation.log -P

从 CTD 日志中提取压力和声速并保存为文本文件：

    mbauvloglist -I fastcatlog.log -F%f -O pressure -O calculated_sound_velocity > auv_ssv.txt

从导航日志中以 CSV 格式输出日期时间、纬度、经度和深度：

    mbauvloglist -I navigation.log -M1 -Ftime_i -O time -Fdefault -O latitude -O longitude -O mDepth > auv_nav.csv

将外部导航插值合并到 Kearfott 日志中，并只保留导航覆盖时间内的记录：

    mbauvloglist -I kearfott.log -N mission_nav.fnv -C -O time -O mergeLon -O mergeLat -O mHeadK -S

## 注意

- 在批处理文件（`.bat`/`.cmd`）中，格式里的百分号要写成 `%%`，例如 `-F%%f`；在交互式命令提示符中按上例写 `-F%f` 即可。
- 输出按记录顺序排列，不做排序。若要用 CTD 日志建立声速剖面，请将输出按压力排序，并用其他工具重新采样。
- `calc...` 系列 CTD 值仅对 `fastcatlog.log` 有定义，`calcKTime`/`calcKSpeed` 仅对 `kearfott.log` 有定义。

## 另请参阅

`mbpreprocess`、`mbnavlist`、`mblist`、`mbctdlist`、`mbsvplist`。完整参考为 MB-System 手册页 mbauvloglist(1)（英文）。
