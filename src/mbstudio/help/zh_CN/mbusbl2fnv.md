# mbusbl2fnv — 将 USBL 跟踪 CSV 文件转换为 fnv 导航

`mbusbl2fnv` 将 CSV 格式的 USBL（超短基线）ROV 跟踪文件转换为 MB-System 快速导航（fnv）文本文件。需要把 USBL 位置和 ROV 深度导入 MB-System 时使用，例如在处理 ROV 搭载声呐数据时作为 `mbpreprocess` 的导航和传感器深度来源。

## 语法

    mbusbl2fnv [-I usbl.csv] [-O output.fnv] [-V] [-H]
    mbusbl2fnv [--input=usbl.csv] [--output=output.fnv] [--verbose] [--help]

## 说明

输入 CSV 文件包含一行表头，其后是由四个逗号分隔字段组成的数据行：Unix 历元秒（UTC）时间、十进制度纬度（北为正）、十进制度经度（东为正）和 ROV 深度（米，向下为正）。只读取第一个非空白字符可以作为数字开头的行，因此表头和以 `#` 开头的注释行会被自动跳过。不恰好包含四个数字的行，以及时间不晚于上一条记录的记录，也会被跳过。

输出为制表符分隔的 fnv 文本文件。第一行是列出各列名称的 `##` 注释行；其后每行包含日期时间（`yyyy mm dd hh mm ss.ssssss`）、历元秒、经度、纬度、航向、航速（km/h）、吃水（米）、横摇、纵摇和升沉。ROV 深度写入吃水字段。航向、航速、横摇、纵摇和升沉不属于 USBL 跟踪数据，写为 0.0；部分 fnv 文件中的条带边缘列不会写出。

不使用 `-I` 时从标准输入读取，不使用 `-O` 时写到标准输出。

## 常用选项

- `-I usbl.csv` — 输入的 USBL 跟踪 CSV 文件（长格式 `--input=usbl.csv`）。默认为标准输入。
- `-O output.fnv` — 输出的 fnv 文件（长格式 `--output=output.fnv`）。默认为标准输出。
- `-V` — 详细模式（长格式 `--verbose`）：输出文件名、报告被跳过的行，并输出读取、写出和跳过的记录数。
- `-H` — 显示用法和预期的 CSV 字段布局后退出（长格式 `--help`）。

## 示例

将 USBL 跟踪文件转换为 fnv 文件：

    mbusbl2fnv -I usbl_track.csv -O rov.fnv

使用长格式选项并显示处理进度：

    mbusbl2fnv --verbose --input=usbl_track.csv --output=rov.fnv

在 `mbpreprocess` 中将该 fnv 文件（导航格式 9）用作导航和传感器深度来源：

    mbpreprocess --input=datalist.mb-1 --nav-file=rov.fnv --nav-file-format=9 --sensordepth-file=rov.fnv --sensordepth-file-format=9

## 注意

- 由于航向、横摇、纵摇和升沉都写为 0.0，请不要把该 fnv 文件用作航向或姿态来源；这些数据应取自载体自身的传感器。
- USBL 定位往往噪声较大；在用于处理之前请检查位置，必要时进行清理。
- 在命令提示符中同样可以使用重定向，例如 `mbusbl2fnv < usbl_track.csv > rov.fnv`。包含空格的路径请加引号。

## 另请参阅

`mbpreprocess`, `mbprocess`, `mbnavlab2fnv`, `mbfnv2navlab`, `mbminirovnav`, `mbnavlist`。完整参考资料为 MB-System 手册页 mbusbl2fnv(1)（英文）。
