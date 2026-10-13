# mbclean — 自动标记条带水深中的坏波束

`mbclean` 识别条带声呐水深数据中的伪影，并标记受影响的波束。它提供多种检测方法（坡度过大、尖峰、水深上下限、偏离局部中值、外侧波束、横向距离或角度、航速、位置等），一次运行可任意组合使用。通常在 `mbprocess` 之前对预处理后的数据运行，作为自动的初步编辑。

## 语法

    mbclean
        --bounds=west/east/south/north {-Wwest/east/south/north}
        --depth-range=low/high {-Blow/high}
        --deviation-max=max {-Amax}
        --distance-range=min/max {-Dmin/max}
        --flag-distance-angle=distanceleft/distanceright[/mode] {-Ydistanceleft/distanceright[/mode]}
        --format=format {-Fformat}
        --fraction-range=fraction_low/fraction_high {-Gfraction_low/fraction_high}
        --help {-H}
        --input=infile {-Iinfile}
        --lonflip=lonflip {-Llonflip}
        --max-acrosstrack=max_acrosstrack {-Emax_acrosstrack}
        --max-heading-rate=maxheadingrate {-Rmaxheadingrate}
        --max-slope=slope/unit {-Cslope/unit}
        --min-good-beams=num_good_min {-Unum_good_min}
        --mode=mode {-Mmode}
        --ping-deviation-tolerance=tolerance {-Ntolerance}
        --range-min=range_min {-Krange_min}
        --speed-range=min_speed/max_speed {-Pmin_speed/max_speed}
        --spike-max=spike_slope/mode/format {-Sspike_slope/mode/format}
        --timestamp-tolerance=tolerance {-Ttolerance}
        --verbose {-V}
        --zap-beams=beamsleft/beamsright {-Xbeamsleft/beamsright}
        --zap-rails=backup_dist {-Qbackup_dist}
        --zero-position {-Z}

## 说明

`mbclean` 读取单个条带文件或数据列表中的全部文件（默认为 `datalist.mb-1`）。它不修改数据文件本身：波束标记作为编辑事件写入每个条带文件的编辑保存文件（条带文件名加 `.esf`），与交互式编辑和 `mbareaclean` 生成的文件属于同一类。如果编辑保存文件已经存在，会先加载并应用其中的编辑，再追加新的标记。`mbclean` 还会更新 `mbprocess` 参数文件，使 `mbprocess` 在写出处理后的文件时应用这些编辑。

各项检测按以下顺序执行：外侧波束（`-X`）、横向距离或角度（`-Y`）、ping 航速（`-P`）、位置范围（`-W`）、零位置（`-Z`）、绝对水深范围（`-B`）、最小斜距（`-K`）、航向变化率（`-R`）、“轨道”伪影（`-Q`）、最大横向距离（`-E`）、局部中值水深的比例（`-G`）、偏离局部中值水深（`-A`）、尖峰（`-S`）、坡度过大（`-C`）、有效波束过少（`-U`），以及整个 ping 与前后 ping 的偏差（`-N`）。

坡度检测（`-C`）是最常用的方法，也是默认检测：在未指定其他检测时使用。只要给出了任何其他检测选项，除非同时给出 `-C`，否则不再进行坡度检测。局部中值水深由当前 ping 及其前后 ping 中距离小于 `-D` 最大距离的波束计算得到。

`mbclean` 在每个文件结束和整个运行结束时，打印每种检测标记的波束数量。自动清理不可能发现所有伪影，之后请务必目视检查数据。

## 常用选项

- `-Iinfile` — 输入条带文件或数据列表。默认值：`datalist.mb-1`。
- `-Fformat` — MBIO 格式 ID；负值表示输入为数据列表。默认：根据文件扩展名推断。
- `-Cslope/unit` — 标记与大于 slope 的坡度相关的波束。unit：`0` 正切（默认），`1` 弧度，`2` 度。slope 默认值：`1.0`。
- `-Mmode` — 坡度异常的处理方式：`1` 只标记离局部中值水深最远的一个波束（默认）；`2` 标记两个波束。
- `-Dmin/max` — 坡度检测和中值检测所用波束间的最小和最大距离，以局部中值水深的比例表示。默认值：`0.01/0.25`。
- `-Sspike_slope/mode/format` — 标记尖峰，即两侧坡度均大于 spike_slope 且符号相反的波束。mode：`1` 横向（默认），`2` 沿航迹方向，`3` 两者。第三个值为坡度单位，与 `-C` 相同。
- `-Blow/high` — 标记浅于 low 或深于 high 的水深（米）。
- `-Gfraction_low/fraction_high` — 标记小于局部中值水深 fraction_low 倍或大于 fraction_high 倍的水深（例如 `-G0.8/1.2`）。
- `-Amax` — 标记偏离局部中值水深超过 max 米的水深。
- `-Xbeamsleft/beamsright` — 标记左舷和右舷最外侧指定数量的波束（只给一个值时两侧相同）。
- `-Ydistanceleft/distanceright/mode` — 按横向位置标记或取消标记。mode：`1`（默认）标记距离范围（米）之外的波束；`2` 取消范围内波束的标记；`3` 标记波束角范围（度）之外的波束；`4` 取消范围内波束的标记。左舷值为负。
- `-Emax_acrosstrack` — 标记横向距离大于该值（米）的波束。
- `-Qbackup_dist` — 标记“轨道”伪影：比更内侧波束还靠内超过 backup_dist 米的外侧波束。
- `-Unum_good_min` — 如果 ping 的左舷或右舷半边有效波束少于该数量，则标记该半边的全部波束。
- `-Ntolerance` — 标记与前后 ping 的均方根偏差超过 tolerance 的 ping 中全部波束（例如受其他声呐干扰）。
- `-Pmin_speed/max_speed` — 标记航速超出此范围（公里/小时）的 ping。
- `-Wwest/east/south/north` — 标记导航位置超出此范围的 ping。
- `-Z` — 标记经纬度为零的 ping。
- `-Rmaxheadingrate` — 标记航向变化速率超过该值（度/秒）期间的 ping。
- `-Krange_min` — 标记斜距小于 range_min（米）的波束。
- `-Ttolerance` — 将已有编辑的时间重设为在 tolerance 秒内匹配的 ping 时间（用于通过 `mbgetesf` 转移的编辑）。
- `-V` — 详细模式。

## 示例

对数据列表中的所有文件执行默认的坡度检测：

    mbclean -I datalist.mb-1

标记坡度大于 3.5（约 74 度）或偏离局部中值水深超过 20% 的波束：

    mbclean -I datalist.mb-1 -M1 -C3.5 -D0.01/0.20 -G0.80/1.20

在保留坡度检测的同时，标记两侧最外侧 5 个波束以及 5 到 250 米范围之外的水深：

    mbclean -I datalist.mb-1 -X5 -B5/250 -C1.0

标记偏离垂直方向超过 65 度的波束，然后应用全部编辑：

    mbclean -I datalist.mb-1 -Y-65/65/3
    mbprocess -I datalist.mb-1

## 注意

- 只有运行 `mbprocess` 后，标记才会体现在处理后的数据中。
- 再次运行 `mbclean` 会在已有编辑的基础上追加；如需从头开始，请先删除或重命名 `.esf` 文件。
- 水深以米为单位、向下为正；`-B` 和 `-A` 的值以米为单位。
- 含空格的文件名需加引号，例如 `-I "my survey\datalist.mb-1"`。

## 另请参阅

`mbareaclean`、`mbvoxelclean`、`mbprocess`、`mbdumpesf`、`mbgetesf`、`mbinfo`、`mbpreprocess`。完整参考为 MB-System 手册页 mbclean(1)（英文）。
