# mbroutetime — 列出测量数据到达计划航线各航点的时间

`mbroutetime` 将条带数据的导航与计划测线航线进行比较，写出测量经过每条测线起点和终点航点时的位置和时间。生成的航点时间列表供 `mbextractsegy`（选项 `-Q`）使用，可将浅地层剖面数据按计划测线拆分为多个文件。

## 语法

    mbroutetime  -Rroutefile [-Fformat -Ifile -Owaypointtimefile -Urangethreshold -H -V]

        --format=format_id {-Fformat_id}
        --help {-H}
        --input=file {-Ifile}
        --output=waypointtimefile {-Owaypointtimefile}
        --range-threshold=rangethreshold {-Urangethreshold}
        --route-file=routefile {-Rroutefile}
        --verbose {-V}

## 说明

航线文件是 MBgrdviz 格式的文本航线文件（`.rte`）。对于带版本头的航线文件，只使用测线起点和终点航点，忽略过渡航点；此类航点至少需要两个。没有版本头的旧航线文件则使用所有点。

程序按顺序读取条带数据（单个文件或数据列表，默认 `datalist.mb-1`），若条带文件存在 `.fnv` 导航文件则使用它。航点按顺序逐个搜索：当导航进入距航点的阈值距离（默认 25 米）以内、随后距离开始增大时，即认为到达该航点，记录的是最接近时的值。如果数据在到达最后一个航点前结束，则在最后的导航位置添加一个终点。

输出文本文件中每个找到的航点占一行，各列为：序号、航点类型（3 = 测线起点，4 = 测线终点）、经度、纬度、航向，以及以十进制历元秒（自 1970-01-01 UTC 起的秒数）表示的时间。默认文件名为输入名加 `_wpttime_d.txt`，例如 `datalist.mb-1_wpttime_d.txt`。

## 常用选项

- `-Rroutefile` — 含计划航点的 MBgrdviz 航线文件（长格式 `--route-file=routefile`）。必需。
- `-Ifile` — 要检查的条带文件或数据列表（长格式 `--input=file`）。默认为 `datalist.mb-1`。
- `-Fformat_id` — 输入数据的 MB-System 格式；通常由文件名识别，数据列表为负值（长格式 `--format=format_id`）。
- `-Owaypointtimefile` — 输出的航点时间列表（长格式 `--output=waypointtimefile`）。默认为输入名加 `_wpttime_d.txt`。
- `-Urangethreshold` — 判定到达航点的距离（米）（长格式 `--range-threshold=rangethreshold`）。默认为 25。
- `-V` — 输出状态信息，包括每个 ping 到当前航点的距离（长格式 `--verbose`）。
- `-H` — 显示说明和用法后退出（长格式 `--help`）。

## 示例

求出 `datalist.mb-1` 中的测量经过航线各航点的时间：

    mbroutetime -R Line_plan.rte -I datalist.mb-1 -O Line_plan_wpttime.txt

允许在距航点 50 米以内经过：

    mbroutetime -R Line_plan.rte -I datalist.mb-1 -U50 -O Line_plan_wpttime.txt

利用航点时间，将浅地层剖面数据按计划测线提取为单独的 SEGY 文件（名称基于 `Line_plan`，编号从 1 开始）：

    mbextractsegy -I datalist.mb-1 -Q Line_plan_wpttime.txt -L1/Line_plan

## 注意

- 必须按航线顺序经过各航点。如果漏过某个航点（从未进入阈值距离以内），其后的航点也都无法找到；此时请增大 `-U`。
- 航线文件通常用 `mbgrdviz` 制作，而 `mbgrdviz` 在 Windows 上不可用；可以使用在其他系统上制作的航线文件。
- 包含空格的路径请加引号。

## 另请参阅

`mbextractsegy`, `mbnavlist`, `mbinfo`, `mbdatalist`。完整参考资料为 MB-System 手册页 mbroutetime(1)（英文）。
