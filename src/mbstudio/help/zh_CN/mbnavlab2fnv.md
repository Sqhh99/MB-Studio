# mbnavlab2fnv — 将 Navlab 二进制文件转换为 fnv 导航文件

`mbnavlab2fnv` 将 Kongsberg Navlab 平滑导航二进制文件（通常为 `navlab_smooth.bin`）转换为 MB-System 快速导航（fnv）文本文件。Navlab 是为 Kongsberg Hugin AUV 计算平滑导航和姿态解的后处理软件。需要将该结果引入 MB-System 时使用本程序，例如作为 `mbpreprocess` 的导航、传感器深度、姿态和航向数据源。

## 语法

    mbnavlab2fnv [-I navlab_smooth.bin] [-O output.fnv] [-V] [-H]
    mbnavlab2fnv [--input=file] [--output=file] [--verbose] [--help]

## 说明

输入文件没有文件头，由 168 字节的记录组成，每条记录包含 21 个小端序 IEEE 754 双精度值，标称采样率为 100 Hz。程序使用每条记录中的时间（Unix 纪元秒）、纬度、经度、深度、航向、横摇、纵摇和对水速度，忽略其他字段（不确定度、协方差项、垂向速度）。

输出为制表符分隔的 fnv 文本文件。第一行是以 `##` 开头、列出各列名称的注释行。其后每行包含日期时间（第一个字段内以空格分隔的 `yyyy mm dd hh mm ss.ssssss`）、纪元秒、经度、纬度、航向、速度（km/h）、吃水（m）、横摇、纵摇和升沉。Navlab 的深度写为吃水，速度从 m/s 换算为 km/h，日期时间按 UTC 计算。

Navlab 格式不包含升沉，因此升沉始终写为 0.0；如需升沉，请从声呐数据流中获取。由于没有条带几何信息，部分 fnv 文件中的左舷和右舷条带边缘列不会输出。

时间戳不晚于前一条记录的记录会被跳过。使用 `-V` 时，会报告每条被跳过的记录，并在最后输出读取和写入的记录数。

## 常用选项

- `-I navlab_smooth.bin` — 输入 Navlab 二进制文件（长格式 `--input=file`）。省略时从标准输入读取。
- `-O output.fnv` — 输出 fnv 文本文件（长格式 `--output=file`）。省略时写入标准输出。
- `-V` — 详细模式（长格式 `--verbose`）：向标准错误输出文件名、被跳过的记录和最终汇总。
- `-H` — 显示程序名、用法以及 Navlab 字段布局后退出（长格式 `--help`）。

## 示例

将 Navlab 二进制文件转换为 fnv 文件：

    mbnavlab2fnv -I navlab_smooth.bin -O mission.fnv

使用长格式选项并显示处理进度：

    mbnavlab2fnv --verbose --input=navlab_smooth.bin --output=mission.fnv

在预处理 Kongsberg KMALL 数据（格式 261）时，将 fnv 文件（导航格式 9）用作导航、传感器深度、姿态和航向的数据源：

    mbpreprocess --format=261 --input=datalist.mb-1 --nav-file=mission.fnv --nav-file-format=9 --sensordepth-file=mission.fnv --sensordepth-file-format=9 --attitude-file=mission.fnv --attitude-file-format=9 --heading-file=mission.fnv --heading-file-format=9

## 注意

- 在 Windows 上请始终用 `-I` 指定二进制输入。在命令提示符中通过标准输入提供二进制文件（`< navlab_smooth.bin`）并不可靠。
- `mbpreprocess` 也可以用格式 12 直接读取 Navlab 二进制文件；转换为 fnv 便于以文本形式查看、编辑或绘制导航。
- 含空格的路径需加引号，例如 `-I "D:\AUV dive 12\navlab_smooth.bin"`。

## 另请参阅

`mbfnv2navlab`、`mbpreprocess`、`mbprocess`、`mbnavlist`。完整参考请见 MB-System 手册页 mbnavlab2fnv(1)（英文）。
