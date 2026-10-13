# mbsvpselect — 为每个条带文件选择最合适的声速剖面

`mbsvpselect` 为测量中的每个条带文件，从一组可用的声速剖面（SVP）中选出最合适的一个，并启用基于该剖面的射线追踪水深重算。用户需提供条带文件的数据列表、SVP 文件列表以及选择方法（位置最近、时间最近或二者组合）。当一次测量包含大量投放、用 `mbset` 逐个文件手动指定剖面过于繁琐时，使用本程序。

## 语法

    mbsvpselect
        --check-zero-position {-N}
        --help {-H}
        --input=datalist {-Idatalist}
        --method=mode[/period_or_range[/seasonal]] {-Pmode[/period_or_range[/seasonal]]}
        --svplist=svplist {-Ssvplist}
        --verbose {-V}

## 说明

每个条带文件的位置和时间从其 `.inf` 文件读取，因此必须事先生成辅助文件 `.inf`、`.fbt` 和 `.fnv`（例如使用 `mbdatalist -N`）。距离计算（WGS84 椭球上的大地线）使用每个文件的中点位置，时间比较使用其开始时间。

SVP 文件列在一个与数据列表类似的 svplist 中：每行一个 SVP 文件，svplist 也可以引用其他 svplist。每个 SVP 文件是由 `深度 声速`（米，米/秒）数对组成的文本文件，并含有一行给出投放时间和位置的文件头。支持的文件头包括 `mbsvplist` 输出的格式（`## MB-SVP yyyy/mm/dd hh:mm:ss longitude latitude`）和 CARIS 格式（`Section yyyy-ddd hh:mm:ss latitude longitude`，位置为 度:分:秒）。

可通过 `-P` 选择以下 5 种方法：
- `-P0` — 位置最近（默认）。
- `-P1` — 时间最近。
- `-P2` 或 `-P2/period` — 在距文件指定时段（小时，默认 10）内的 SVP 中选位置最近者；若时段内没有 SVP，则使用位置最近者。
- `-P3`、`-P3/range` 或 `-P3/range/0` — 在距文件指定距离（米，默认 10000）内的 SVP 中选时间最近者。
- `-P3/range/1` — 在指定距离内的 SVP 中选月份最接近者（按季节选择，不考虑年份）。

对每个条带文件，程序运行 `mbset -I file -PSVPFILE:svpfile`，在该文件的参数（`.par`）文件中设置 SVP 文件并启用射线追踪。条带文件与所选 SVP 的对应关系还会写入当前目录下的 `result.txt`。只有在之后运行 `mbprocess` 时，水深才会真正重新计算。

## 常用选项

- `-Idatalist` — 待处理条带文件的数据列表（长格式 `--input=datalist`）。默认值：`datalist.mb-1`。
- `-Ssvplist` — SVP 文件列表（长格式 `--svplist=svplist`）。默认值：`svplist.mb-1`。
- `-Pmode[/period_or_range[/seasonal]]` — 上述 SVP 选择方法（长格式 `--method=...`）。时间单位为小时，距离单位为米。默认值：`-P0`。
- `-N` — 检查 `.inf` 中位置被错误记录为经度 0、纬度 0 的条带文件，并避免在距离计算中使用这些位置（长格式 `--check-zero-position`）。不影响 SVP 的位置。
- `-V` — 详细模式（长格式 `--verbose`）：显示文件和 SVP 信息以及计算出的距离或时间差。
- `-H` — 显示简要说明和用法后退出（长格式 `--help`）。

## 示例

数据列表 `datalist.mb-1` 列出条带文件，`svplist.mb-1` 每行列出一个 SVP 文件：

    svp_001.svp
    svp_002.svp
    svp_003.svp

先生成辅助文件，然后在检查零位置的同时，为每个条带文件指定 50 小时内采集的最近 SVP：

    mbdatalist -I datalist.mb-1 -N
    mbsvpselect -N -V -I datalist.mb-1 -S svplist.mb-1 -P2/50

在距每个文件 5 km 以内的 SVP 中指定时间最近者，然后重新处理：

    mbsvpselect -I datalist.mb-1 -S svplist.mb-1 -P3/5000
    mbprocess -I datalist.mb-1

## 注意

- 程序结束时会显示“End the program press ENTER”并等待输入；按 Enter 键返回提示符。
- 请在包含数据列表和条带文件的目录中运行，`result.txt` 也写在该目录。SVP 文件名按 svplist 中的写法传给 `mbset`，因此相对文件名从当前目录解析。
- `mbset` 命令行在拼接时不加引号，因此条带文件和 SVP 文件的名称及路径中不要包含空格。
- `.inf` 文件必须是最新的；没有 `.inf` 文件的数据文件不会被找到。

## 另请参阅

`mbset`、`mbprocess`、`mbsvplist`、`mbdatalist`、`mblevitus`。完整参考请见 MB-System 手册页 mbsvpselect(1)（英文）。
