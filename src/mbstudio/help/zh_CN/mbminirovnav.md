# mbminirovnav — 将 MBARI MiniROV 日志合并为格式 165 ROV 导航

`mbminirovnav` 读取 MBARI MiniROV 在下潜过程中记录的四种日志文件（USBL 导航、CTD、DVL 和 ROV 姿态），将其合并为一个 MBARI ROV 格式的 ROV 导航文件，MB-System 将该格式作为格式 165（MBF_MBARIROV）读取。可用它为 MiniROV 导航的编辑、绘图或与条带数据合并做准备。

## 语法

    mbminirovnav
        --help
        --input-ctd-file=file
        --input-dvl-file=file
        --input-nav-file=file
        --input-rov-file=file
        --interpolate-position
        --interval=seconds
        --output=file
        --rov-dive-start=yyyy/mo/da/hr/mn/sc
        --rov-dive-end=yyyy/mo/da/hr/mn/sc
        --utm-zone=zone_id/NorS
        --verbose

## 说明

MiniROV 每个 UTC 日记录四个逗号分隔的文本文件，名称为 `CTD_*.txt`、`DVL_*.txt`、`NAV_*.txt` 和 `ROV_*.txt`（这种记录方式始于 2017 年 9 月的第 100 次下潜）。每行都以历元秒（UTC）时间戳开头。程序使用以下数值：

- NAV：由 Winfrog 软件以 `$GPGLL` 语句传来的 USBL 位置（纬度和经度以度和十进制分表示）。
- CTD：CTD 压力传感器给出的深度。
- DVL：距海底高度（仅在 DVL 底锁时非零）；不使用 DVL 速度。
- ROV：航向、纵摇和横摇。

各数值被插值到规则的输出时刻上，时间间隔为 `--interval` 秒（默认 1 秒），范围从 `--rov-dive-start` 到 `--rov-dive-end`；不指定这些选项时，使用输入文件的整个时间范围。每条输出记录包含日期时间、纬度、经度、UTM 东坐标和北坐标、深度、航向、高度、纵摇、横摇以及有效性标记。由于未给出相应文件或该时刻没有数据而缺失的值会被标记为无效。

NAV 和 ROV 文件是必需的：缺少其中任一数据时不会写出输出文件。CTD 和 DVL 文件是可选的。除非指定 `--utm-zone`，UTM 分带由输入位置的平均值计算；格式 165 文件不记录所用的分带。

MiniROV 记录器约每两秒重复记录最近一次 USBL 定位值，因此位置不变表示没有新的定位。默认保留这些重复位置。`--interpolate-position` 会舍弃重复位置，只根据不同的定位值求出输出时刻的位置。当 USBL 数据含有错误定位（离群值）时不推荐使用该选项，因为这些错误会影响定位之间的位置；更好的做法是先编辑定位值，再进行插值。

## 常用选项

- `--input-nav-file=file` — MiniROV NAV 日志（USBL 位置）。必需。
- `--input-rov-file=file` — MiniROV ROV 日志（航向、纵摇、横摇）。必需。
- `--input-ctd-file=file` — MiniROV CTD 日志（压力深度）。
- `--input-dvl-file=file` — MiniROV DVL 日志（高度）。
- `--output=file` — 输出的格式 165 导航文件。必需，没有默认值。
- `--interval=seconds` — 输出记录的时间步长。默认为 1.0。
- `--rov-dive-start=yyyy/mo/da/hr/mn/sc` — 输出的开始时间（UTC），例如 `2017/09/05/16/54/00`。
- `--rov-dive-end=yyyy/mo/da/hr/mn/sc` — 输出的结束时间（UTC）。
- `--utm-zone=zone_id/NorS` — 东坐标和北坐标使用的 UTM 分带，例如 `8/N` 或 `8N`。默认由平均位置确定。
- `--interpolate-position` — 舍弃重复的 USBL 位置，只根据不同的定位值生成输出（见上文）。
- `--verbose` — 输出参数，以及输入记录和有效输出记录的统计。
- `--help` — 显示用法后退出。

## 示例

先将航次中的每日文件连接起来，使跨越 UTC 零点的下潜数据完整（Windows 的 `copy` 按文件名顺序合并文件）：

    copy /b NAV_*.txt MiniROV_NAV_all.txt
    copy /b CTD_*.txt MiniROV_CTD_all.txt
    copy /b DVL_*.txt MiniROV_DVL_all.txt
    copy /b ROV_*.txt MiniROV_ROV_all.txt

在 UTM 8 带（北半球）中生成一次下潜着底阶段的导航：

    mbminirovnav --input-nav-file=MiniROV_NAV_all.txt --input-ctd-file=MiniROV_CTD_all.txt --input-dvl-file=MiniROV_DVL_all.txt --input-rov-file=MiniROV_ROV_all.txt --output=MiniROVnav_100.mb165 --utm-zone=8N --rov-dive-start=2017/09/05/16/54/00 --rov-dive-end=2017/09/05/17/56/00 --verbose

只使用 NAV 和 ROV 数据，以 0.5 秒间隔写出同一次下潜：

    mbminirovnav --input-nav-file=MiniROV_NAV_all.txt --input-rov-file=MiniROV_ROV_all.txt --output=MiniROVnav_100_nav.mb165 --interval=0.5 --rov-dive-start=2017/09/05/16/54/00 --rov-dive-end=2017/09/05/17/56/00

## 注意

- 只接受长选项。尽管内置用法说明写的是 `yyyymmddhhmmss`，时间必须用斜杠分隔（`yyyy/mo/da/hr/mn/sc`）。用法说明中列出的 `--input` 选项不起作用。
- 未给出路径时，输出文件写入当前目录。包含空格的路径请加引号。
- 手册页中提到的交互式导航编辑器 `mbnavedit` 在 Windows 上不可用。

## 另请参阅

`mbusbl2fnv`, `mbnavlist`, `mbpreprocess`, `mbinfo`。完整参考资料为 MB-System 手册页 mbminirovnav(1)（英文）。
