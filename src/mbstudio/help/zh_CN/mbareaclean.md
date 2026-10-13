# mbareaclean — 按区域网格单元的统计检验标记或取消标记测深点

`mbareaclean` 按区域而不是逐 ping 地清理条带水深。它将区域划分为正方形网格单元，收集所有输入文件中落入每个单元的测深点，并对其进行统计检验（中值滤波或标准差滤波）。未通过检验的测深点被标记为坏点，也可以选择恢复已被标记但通过检验的测深点。通常在 `mbclean` 之后使用，尤其适用于多条测线重叠覆盖同一海底的情况。

## 语法

    mbareaclean
        --beam-range=[-]minbeam[/maxbeam] {-N[-]minbeam[/maxbeam]}
        --bin-size=binsize {-Sbinsize}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --detection-type=type {-Ttype}
        --flag-bad {-B}
        --format=format {-Fformat}
        --help {-H}
        --input=infile {-Iinfile}
        --median-filter=threshold[/nmin[/nmax]] {-Mthreshold[/nmin[/nmax]]}
        --plane-fit=arg {-Parg}
        --std-dev-filter=threshold[/nmin] {-Dthreshold[/nmin]}
        --unflag-good {-G}
        --verbose {-V}

## 说明

输入为单个条带文件或数据列表（默认为 `datalist.mb-1`）。工作区域用 `-R` 设置，单元大小（米）用 `-S` 设置。未给出 `-R` 时，区域取自输入数据的 `.inf` 文件，并且除非指定 `-S`，单元大小为最大声呐高度的 0.2 倍。在检验之前会从每个文件的编辑保存文件中读取已有编辑，因此已被标记的测深点也会被考虑在内。

提供两种检验方法。中值滤波（`-M`）将与所在单元中值水深之差大于 threshold 倍声呐高度的测深点视为坏点；当 `threshold` = 0.05 时，比中值浅或深 5% 以上的测深点不通过。如果单元中的有效测深点超过 nmax 个，只有最接近中值的 nmax 个保持未标记。标准差滤波（`-D`）将与单元均值之差大于 threshold 倍标准差的测深点视为坏点。两种检验都只应用于测深点数不少于 nmin 的单元。如果未给出 `-M`、`-D` 或 `-P`，则使用中值滤波。

使用 `-B`（默认）时，标记未通过检验的有效测深点；使用 `-G` 时，取消通过全部检验的已标记测深点的标记；两者可同时使用。`-N` 和 `-T` 按波束号和海底检测方式限制可被修改的测深点，而统计量仍使用全部有效测深点计算。例如 `-N-30 -T1 -D1` 会标记两侧最外侧 30 个波束中、偏离均值超过一倍标准差的振幅检测测深点。

修改结果作为编辑事件写入每个条带文件的编辑保存文件（条带文件名加 `.esf`），必要时会新建该文件，同时更新 `mbprocess` 参数文件。运行 `mbprocess` 以应用这些编辑。

## 常用选项

- `-Iinfile` — 输入条带文件或数据列表。默认值：`datalist.mb-1`。
- `-Rwest/east/south/north` — 进行分格和检验的区域，区域外的测深点被忽略。默认：全部输入数据的范围（取自 `.inf` 文件）。
- `-Sbinsize` — 单元大小（米）。默认：最大声呐高度的 0.2 倍（仅在未给出 `-R` 时）。
- `-Mthreshold/nmin/nmax` — 中值滤波：以声呐高度比例表示的阈值、每个单元的最少测深点数、每个单元的最多有效测深点数。默认值：`0.25`、`10`、无上限。
- `-Dthreshold/nmin` — 标准差滤波：以标准差倍数表示的阈值，以及每个单元的最少测深点数。默认值：`2.0`、`10`。
- `-B` — 标记未通过检验的有效测深点（默认动作）。
- `-G` — 取消通过全部检验的已标记测深点的标记。
- `-Nminbeam/maxbeam` — 只修改 minbeam 到 maxbeam 号波束的测深点；前面加 `-` 时只修改该范围之外的波束。省略 maxbeam 时会对称设置，例如 `-N-30` 作用于两侧最外侧的 29 个波束。
- `-Ttype` — 只修改采用该海底检测方式的测深点：`0` 未知，`1` 振幅，`2` 相位，`3` 激光雷达，`4` 摄影测量。
- `-Parg` — 平面拟合检验（参数必须给出但不会被使用）；单独使用时，不进行中值滤波和标准差滤波。
- `-Fformat` — MBIO 格式 ID；负值表示数据列表。默认：根据文件扩展名推断。
- `-V` — 详细模式，包括每个文件被标记和取消标记的测深点数。
- `-H` — 显示帮助后退出。

## 示例

在已知区域内，用 2.5 米单元和 10% 阈值进行中值滤波：

    mbareaclean -I datalist.mb-1 -B -M0.1/10 -S2.5 -R-122.42556/-122.41974/47.67111/47.67529 -V

在整个数据集上用 5 米单元，标记偏离均值超过两倍标准差的测深点：

    mbareaclean -I datalist.mb-1 -S5 -D2/10

标记最外侧 30 个波束中偏离均值超过一倍标准差的振幅检测测深点，然后应用编辑：

    mbareaclean -I datalist.mb-1 -S5 -N-30 -T1 -D1
    mbprocess -I datalist.mb-1

## 注意

- 不使用 `-R` 时，区域范围取自 `.inf` 文件；如果缺少这些文件，请先用 `mbdatalist -I datalist.mb-1 -O` 生成（`mbpreprocess` 也会生成）。
- 使用 `-R` 时也请给出 `-S`，否则区域只会在东西方向上被简单地划分为约 100 个单元。
- 请使用大写的 `-S`；小写的 `-s` 不接受数值。
- 区域越大、单元越小，单元数量增长越快；非常大的测区请分成多个区域处理。
- 含空格的文件名需加引号。

## 另请参阅

`mbclean`、`mbvoxelclean`、`mbprocess`、`mbdumpesf`、`mbdatalist`、`mbinfo`。完整参考为 MB-System 手册页 mbareaclean(1)（英文）。
