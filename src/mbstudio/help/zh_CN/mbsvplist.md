# mbsvplist — 列出条带数据中嵌入的声速剖面

`mbsvplist` 从条带声呐数据文件中提取所记录的水体声速剖面（SVP）。在预处理之后，可用它查看测量数据中包含哪些 SVP、将其保存为 `.svp` 文件，或将每个文件的第一个 SVP 设置为 `mbprocess` 重新计算水深时使用的剖面。它还可以列出声呐波束形成所用的表层声速（SSV）。

## 语法

    mbsvplist
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --counts {-C}
        --duplicates {-D}
        --format=format_id {-Fformat_id}
        --help {-H}
        --input=file {-Ifile}
        --min-num-pairs=min_num_pairs {-Nmin_num_pairs}
        --mode=mode {-Mmode}
        --output {-O}
        --process {-P}
        --source=source {-Asource}
        --ssv {-S}
        --table {-T}
        --verbose {-V}
        --zero-depth {-Z}

## 说明

条带水深是根据原始波束角和传播时间，通过水中声速模型进行射线追踪计算得到的。许多数据格式允许在数据流中保存 SVP，其中常常包含采集系统计算时所用的剖面。`mbsvplist` 读取单个条带文件或数据列表引用的所有文件（默认输入为 `datalist.mb-1`），并输出这些剖面。

默认情况下，SVP 以文本形式写到标准输出。只列出每个文件的第一个 SVP 以及与前一个不同的 SVP，重复的剖面会被跳过（参见 `-M` 和 `-D`）。使用 `-O` 时，每个 SVP 会写入单独的文件，文件名为 `<swathfile>_NNN.svp`（NNN 为该文件内的 SVP 序号，从 `000` 开始）。这些文件以 `## MB-SVP` 形式的文件头开始，记录剖面的时间和位置，`mbsvpselect` 利用这些信息为每个条带文件选择最合适的 SVP。

`-P` 的输出与 `-O` 相同，并且会更新 `mbprocess` 参数文件（`<swathfile>.par`），使每个文件写出的第一个 SVP 在 `mbprocess` 重新计算水深时被使用。

`-T` 输出简洁的 CSV 表（SVP 序号、时间、经度、纬度、深度-声速对数量）而不是完整剖面，`-C` 仅输出每个文件中不同 SVP 的数量。`-S` 输出波束形成所用的表层声速，并与声呐深度配对，通常每个 ping 一个值；配合 `-R` 可将 SSV 输出限制在指定的经纬度范围内。

## 常用选项

- `-Ifile` — 输入条带文件或数据列表。默认值：`datalist.mb-1`。
- `-Fformat_id` — 输入数据的 MBIO 格式 ID。通常根据文件扩展名自动推断；负值表示输入为数据列表。
- `-O` — 将每个 SVP 写入单独的 `<swathfile>_NNN.svp` 文件，而不是标准输出。
- `-P` — 与 `-O` 相同，并在参数文件中将每个文件的第一个 SVP 设置为 `mbprocess` 使用的剖面。
- `-Mmode` — SVP 输出模式：`0`（默认）每个文件的第一个 SVP 以及所有与前一个不同的 SVP；`1` 每个不同的 SVP 只输出第一次出现（跨越数据列表中的多个文件）；`2` 输出全部 SVP，包括重复的。
- `-D` — 输出重复的 SVP（等同于 `-M2`）。
- `-Asource` — 作为 SVP 来源的记录类型：`S` 为声速剖面记录（默认），`C` 为 CTD 记录。
- `-Nmin_num_pairs` — 忽略深度-声速对少于该数量的 SVP（适用于把单个声速值存为单点“SVP”的数据）。默认值：`0`。
- `-Z` — 输出前将每个 SVP 的第一个深度替换为 0（不从水面开始的剖面会在 `mbprocess` 中引起问题）。
- `-T` — 输出概括每个 SVP 的 CSV 表，而不是完整剖面。
- `-C` — 仅输出每个文件中不同 SVP 的数量。
- `-S` — 输出每个 ping 的表层声速（SSV）和声呐深度，而不是 SVP。
- `-Rwest/east/south/north` — 与 `-S` 一起使用，只输出该范围内的 SSV 值。默认：全部输出。
- `-V` — 详细模式（在错误输出中显示进度和记录数）。
- `-H` — 显示帮助后退出。

## 示例

在屏幕上列出数据列表中所有文件包含的 SVP：

    mbsvplist -I datalist.mb-1 -V

将 SVP 写入 `.svp` 文件，并将每个文件的第一个 SVP 设为水深重算所用剖面，然后进行处理：

    mbsvplist -I datalist.mb-1 -P -V
    mbprocess -I datalist.mb-1

将所有 SVP 的概要表（时间、位置、点数）保存为 CSV 文件：

    mbsvplist -I datalist.mb-1 -T > svp_table.csv

列出单个 Kongsberg 文件在指定范围内的表层声速：

    mbsvplist -I survey\0001_20250101_000000.all -S -R139.5/139.8/35.2/35.5 > ssv.txt

## 注意

- `-O` 和 `-P` 生成的 `.svp` 文件写在条带数据文件所在位置（使用数据列表中的路径），而不是当前目录。
- `-P` 会修改 `.par` 文件；之后需运行 `mbprocess` 才能应用新的 SVP。
- `-S` 是另一种输出模式，不要与 `-O`、`-P`、`-C` 或 `-T` 同时使用。
- 含空格的文件名需加引号，例如 `-I "my survey\datalist.mb-1"`。

## 另请参阅

`mbsvpselect`、`mbprocess`、`mbset`、`mblevitus`、`mbctdlist`、`mbinfo`。完整参考为 MB-System 手册页 mbsvplist(1)（英文）。
