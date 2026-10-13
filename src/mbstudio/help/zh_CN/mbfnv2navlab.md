# mbfnv2navlab — 将 fnv 导航文件转换为 Navlab 二进制文件

`mbfnv2navlab` 将 MB-System 快速导航（fnv）文本文件转换为 Kongsberg Navlab 平滑导航二进制文件（`navlab_smooth.bin` 格式）。当需要把在 MB-System 中修正或平差后的导航以 Navlab 格式重新作为导航源用于处理时（例如以导航格式 12 提供给 `mbpreprocess`），使用本程序。它是 `mbnavlab2fnv` 的逆向转换。

## 语法

    mbfnv2navlab [-I input.fnv] [-O navlab_smooth.bin] [-V] [-H]
    mbfnv2navlab [--input=file] [--output=file] [--verbose] [--help]

## 说明

输入为制表符分隔的 fnv 文本文件，由 `mbpreprocess` 或 `mbnavlab2fnv` 等 MB-System 程序生成。以 `#` 开头的行和空行会被跳过。每个数据行至少需要 10 个制表符分隔的字段：日期时间（字段内以空格分隔的 `yyyy mm dd hh mm ss.ssssss`）、纪元秒、经度、纬度、航向、速度（km/h）、吃水（m）、横摇、纵摇和升沉。其后可能出现的 4 个条带边缘字段会被接受但不使用。

每个输入行输出一条记录，每条记录由 21 个小端序 IEEE 754 双精度值（168 字节）组成，文件没有文件头。时间取自纪元秒字段。纬度、经度、吃水（写为深度）、航向、横摇和纵摇直接复制；速度从 km/h 换算为 m/s，并同时写入两个速度字段（10 和 11）。由于 Navlab 格式没有升沉字段，升沉会被读取但丢弃。

Navlab 记录中的其他字段（4、5、9 和 12～20：位置偏移、垂向速度、不确定度和协方差项）均设为 0.0，因此输出无法完全再现原始 Navlab 文件。将该文件作为 `mbpreprocess` 或 `mbprocess` 的导航源时，只会使用有值的字段，置零的字段不产生影响。输出的采样率与输入 fnv 文件相同，通常远低于原始 Navlab 输出的 100 Hz。

制表符字段少于 10 个的行，以及时间戳为零、负值或不晚于前一条记录的记录会被跳过。使用 `-V` 时，会报告每个被跳过的行及其行号和原因，最后输出读取行数、写入记录数和跳过行数的汇总。

## 常用选项

- `-I input.fnv` — 输入 fnv 文本文件（长格式 `--input=file`）。省略时从标准输入读取。
- `-O navlab_smooth.bin` — 输出 Navlab 二进制文件（长格式 `--output=file`）。省略时写入标准输出。
- `-V` — 详细模式（长格式 `--verbose`）：向标准错误输出文件名、被跳过的行和最终汇总。
- `-H` — 显示程序名、用法以及 fnv 到 Navlab 的字段对应关系后退出（长格式 `--help`）。

## 示例

将 fnv 文件转换为 Navlab 二进制文件：

    mbfnv2navlab -I mission.fnv -O adjusted_nav.bin

使用长格式选项并显示处理进度：

    mbfnv2navlab --verbose --input=mission.fnv --output=adjusted_nav.bin

在预处理 Kongsberg KMALL 数据（格式 261）时，将转换结果用作导航、传感器深度、姿态和航向的数据源：

    mbpreprocess --format=261 --input=datalist.mb-1 --nav-file=adjusted_nav.bin --nav-file-format=12 --sensordepth-file=adjusted_nav.bin --sensordepth-file-format=12 --attitude-file=adjusted_nav.bin --attitude-file-format=12 --heading-file=adjusted_nav.bin --heading-file-format=12

## 注意

- 在 Windows 上请始终用 `-O` 指定二进制输出。在命令提示符中将标准输出重定向到文件（`> file.bin`）可能会损坏二进制数据。
- 输入必须以制表符分隔。仅以空格分隔各列的文件会被逐行跳过（可用 `-V` 查看原因）。
- 含空格的路径需加引号，例如 `-I "D:\Survey 1\mission.fnv"`。

## 另请参阅

`mbnavlab2fnv`、`mbpreprocess`、`mbprocess`、`mblist`、`mbnavlist`。完整参考请见 MB-System 手册页 mbfnv2navlab(1)（英文）。
