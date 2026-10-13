# mbprocess — 对条带文件应用编辑、导航、声速及各项改正

`mbprocess` 是 MB-System 的核心处理程序。它读取原始条带文件，一次性应用为该文件配置的水深编辑、导航和姿态合并、利用声速剖面射线追踪重新计算水深、偏差、吃水和潮汐改正以及振幅/侧扫声呐改正，并写出处理后的副本。通常在编辑（`mbclean`、`mbvoxelclean` 等）和使用 `mbset` 设置参数之后、使用 `mbgrid` 或 `mbmosaic` 生成网格之前运行。

## 语法

    mbprocess -Iinfile [-C -Fformat -N -Ooutfile -P -S -T -V -H]
        --format=value {-Fvalue}
        --force {-P}
        --help {-H}
        --input=file {-Ifile}
        --output=file {-Ofile}
        --print-status {-S}
        --strip-comments {-N}
        --test {-T}
        --threads=value {-Cvalue}
        --verbose {-V}

## 说明

对每个文件执行哪些处理，由一个文本参数文件控制，其名称为条带文件名加 `.par` 后缀（`survey.mb58` 对应 `survey.mb58.par`）。参数文件的每一行是一条命令，如 `EDITSAVEFILE`、`NAVFILE`、`SVPFILE`、`ROLLBIAS` 或 `TIDEFILE`。参数文件可用 `mbset` 创建和修改，也会被生成辅助文件的程序修改，例如 `mbclean`、`mbareaclean`、`mbvoxelclean`、`mbbackangle`、`mbsvplist` 和 `mbgpstide`。如果没有参数文件，`mbprocess` 会查找标准名称的导航文件和编辑保存文件，推断合理的处理流程。

可应用的处理包括：

- 来自编辑保存文件（`.esf`）的水深编辑（波束标记）；
- 合并编辑后的导航（`.nve`）、`mbnavadjust` 调整后的导航（`.na0`、`.na1` 等）以及单独的姿态和声呐深度文件；
- 利用声速剖面（`.svp`）射线追踪重新计算水深，或在已改正与未改正水深之间转换；
- 横摇、纵摇和艏向偏差、吃水、升沉和杠杆臂改正，以及逐波束静态偏移；
- 使用潮汐文件（例如由 `mbotps` 或 `mbgpstide` 生成）进行潮汐改正；
- 按波束号、横向距离或航速裁剪数据；
- 使用 `mbbackangle` 生成的掠射角表进行振幅和侧扫声呐改正，以及部分声呐的侧扫重算；
- 插入元数据（船名、机构、航次、首席科学家等）。

`-I` 指定的输入可以是单个条带文件或数据列表；数据列表会被递归读取，其中的每个条带文件都会被处理。如果省略 `-I` 且当前目录中存在 `datalist.mb-1`，则使用该文件。输出文件名取决于格式编号：`root.mbXX` 变为 `rootp.mbXX`（例如 `mydata.mb71` 变为 `mydatap.mb71`）；没有 `.mbXX` 后缀的文件名会追加 `p.mbXX`。输出文件写在输入文件所在的文件夹。对于每个输出文件，`mbprocess` 还会生成 `.inf`、`.fbt` 和 `.fnv` 辅助文件。

默认情况下，只有当输出文件不存在，或比输入文件、参数文件或其引用的任何辅助文件（导航、编辑保存、SVP 文件等）更旧时，才处理该文件。使用 `-P` 可强制处理所有文件。使用 `-C` 时，数据列表中的多个文件会以并行线程处理。

对于 Kongsberg kmall 数据（格式 261），水深是根据 `mbpreprocess` 写入的 #XMT 数据报中的双程时间和角度重新计算的，因此这些文件必须先经过预处理。

## 常用选项

- `-I<file>` / `--input=<file>` — 要处理的条带文件或数据列表。默认值：当前目录中存在时为 `datalist.mb-1`。
- `-C<n>` / `--threads=<n>` — 并行处理的文件数。默认值：1；上限为 CPU 核数。
- `-F<format>` / `--format=<format>` — 输入的 MBIO 格式编号。通常从参数文件获取或根据 `.mbXX` 后缀推断。仅在输入为单个文件时有效。
- `-O<file>` / `--output=<file>` — 单个输入文件的输出文件名，替代默认的 `rootp.mbXX`。输入为数据列表时被忽略。
- `-P` / `--force` — 即使输出已是最新，也处理所有文件。
- `-T` / `--test` — 测试模式：只报告将会处理哪些文件，不做实际处理。
- `-S` / `--print-status` — 显示每个文件的状态（最新、过期、已锁定）以及用于判断的修改时间。
- `-N` / `--strip-comments` — 不复制输入中的注释记录，也不嵌入描述处理参数的注释。
- `-V` / `--verbose` — 显示处理参数和记录数。
- `-H` / `--help` — 显示简短帮助信息后退出。

## 示例

处理当前目录数据列表中的所有文件，只处理需要更新的文件：

    mbprocess -Idatalist.mb-1

用 `mbset` 为一个 Kongsberg 文件设置 SVP 文件、横摇偏差和固定吃水，然后处理并显示进度：

    mbset -I0051_20010829_223755.mb57 -PSVPFILE:0051_20010829_223755.mb57.svp -PROLLBIAS:0.5 -PDRAFT:1.95
    mbprocess -I0051_20010829_223755.mb57 -V

不做任何处理，仅检查哪些文件需要处理：

    mbprocess -Idatalist.mb-1 -T -S

使用 4 个线程重新处理测量的所有文件：

    mbprocess -Idatalist.mb-1 -P -C4

## 注意

- 若要在之后使用处理后的文件，请创建引用这些文件的数据列表，例如运行 `mbdatalist -Idatalist.mb-1 -Z`，或在数据列表中加入 `$PROCESSED`。
- 处理从不原地修改文件：原始文件保持不变，随时可以用不同参数重新处理。
- 被其他程序（例如编辑器）锁定的文件会被跳过并报告。
- 数据列表和参数文件以空格分隔存储文件名，因此测量文件夹和文件名中不要包含空格。
- 参数文件命令的完整列表见手册页 mbprocess(1) 和 mbset(1)。

## 另请参阅

`mbset`, `mbpreprocess`, `mbclean`, `mbvoxelclean`, `mbbackangle`, `mbsvplist`, `mbotps`, `mbdatalist`, `mbinfo`, `mbgrid`。完整参考资料为 MB-System 手册页 mbprocess(1)（英文）。
