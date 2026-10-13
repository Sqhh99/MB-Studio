# mbgetesf — 将条带文件中的波束标记提取为编辑保存文件

`mbgetesf` 读取条带数据文件中保存的波束标记，并将其写成编辑保存文件（`.esf`），与 `mbclean` 和交互式编辑器生成的文件属于同一类。用于把在某一版本数据（例如已编辑或已转换的文件）上完成的编辑转移到同一数据的另一版本，而无需重新编辑。

## 语法

    mbgetesf
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format_id {-Fformat_id}
        --help {-H}
        --input=infile {-Iinfile}
        --kluge=value {-Kvalue}
        --mode=value {-Mvalue}
        --output=esffile {-Oesffile}
        --verbose {-V}

## 说明

水深编辑会给坏波束加上标记。通常这些标记保存在编辑保存文件中，再由 `mbprocess` 应用到原始数据。但有时标记只存在于数据文件内部，例如在只保留部分原始数据流的格式中编辑过的文件，或在其他软件中处理过的数据。`mbgetesf` 将此类文件中的标记转换为编辑事件（时间、波束号和操作），并写入编辑保存文件。

如果输出文件命名为目标条带文件名加 `.esf`，`mbclean` 会自动将其作为起点读入。要直接应用这些编辑，请用 `mbset`（`EDITSAVEMODE`、`EDITSAVEFILE`）在目标文件的参数文件中启用编辑，然后运行 `mbprocess`。

默认只写出已标记的波束（`-M1`）。模式 2 和 3 还会写出空波束，或包括正常波束在内的所有波束；当之前的编辑把原本正常的波束置空，或取消了原本已标记波束的标记时，这很有用。模式 4 到 6 写出的文件对未列出的所有波束采用隐式默认状态（正常、空，或按格式选择）；这是 `mbpreprocess` 用于“全局编辑文件”（`.gef`）的形式。

程序读取单个条带文件（不是数据列表）。未指定输入文件时从标准输入读取，未指定输出文件时将编辑事件写到标准输出。

## 常用选项

- `-I<file>` / `--input=<file>` — 要读取的条带数据文件。默认值：标准输入。
- `-O<esffile>` / `--output=<esffile>` — 要写出的编辑保存文件。默认值：标准输出。
- `-F<format_id>` / `--format=<format_id>` — 输入的 MBIO 格式编号；通常根据文件名推断。
- `-M<mode>` / `--mode=<mode>` — 写出哪些波束：`1` 仅已标记波束（默认），`2` 已标记和空波束，`3` 所有波束；`4`、`5`、`6` 写出隐式默认的编辑文件，未列出的波束按格式为正常或空（`4`）、为空（`5`）或为正常（`6`）。
- `-B<yr/mo/da/hr/mn/sc>` / `--begin-time=...` — 时间范围起点。默认值：1962/2/21/10/30/0。
- `-E<yr/mo/da/hr/mn/sc>` / `--end-time=...` — 时间范围终点。默认值：2062/2/21/10/30/0。
- `-K<value>` / `--kluge=<value>` — `1` 用于修正部分 HDCS 格式（格式 151）EM300/EM3000 数据中一个波束的索引偏移；对其他格式无效。默认值：0。
- `-V` / `--verbose` — 显示模式和写出的波束数。
- `-H` / `--help` — 显示简短帮助信息后退出。

## 示例

从已编辑的格式 22 文件中提取标记，并以对应的格式 24 文件的名称保存：

    mbgetesf -F22 -Ihs_ew9103_143.mb22 -Ohs_ew9103_143.mb24.esf -V

为格式 24 文件启用提取出的编辑并进行处理：

    mbset -Ihs_ew9103_143.mb24 -PEDITSAVEMODE:1 -PEDITSAVEFILE:hs_ew9103_143.mb24.esf
    mbprocess -Ihs_ew9103_143.mb24

为一小时数据写出所有波束（正常、已标记和空）的编辑事件：

    mbgetesf -Iedited_line.mb57 -M3 -B2025/01/01/10/00/00 -E2025/01/01/11/00/00 -Oraw_line.mb57.esf

## 注意

- 两个文件必须包含相同的 ping：编辑事件按时间和波束号匹配。
- 与输出同名的现有编辑保存文件会被覆盖，因此不要覆盖需要保留原有编辑的文件的 `.esf` 文件。
- 输出为二进制文件；请务必指定 `-O`，不要输出到控制台。

## 另请参阅

`mbclean`, `mbset`, `mbprocess`, `mbvoxelclean`, `mbareaclean`, `mbdumpesf`。完整参考资料为 MB-System 手册页 mbgetesf(1)（英文）。
