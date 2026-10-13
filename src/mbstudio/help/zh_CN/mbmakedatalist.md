# mbmakedatalist — 为目录中的条带文件生成数据列表

`mbmakedatalist` 扫描目录中可识别为条带数据的文件，并写出一个 MB-System 数据列表（默认为 `datalist.mb-1`），列出每个文件及其格式 ID。在为新测量数据建立处理流程时，它通常是第一条命令：大多数其他 MB-System 程序都以该数据列表作为输入。

## 语法

    mbmakedatalist [-B size|--min-size=size]
                   [-F format|--format=format]
                   [-I directory|--input=directory]
                   [-L|--skip-latest]
                   [-O datalist|--output=datalist]
                   [-P|--ignore-processed]
                   [-S suffix|--suffix=suffix]
                   [-T|--no-time-sort]
                   [-H|--help] [-V|--verbose]

## 说明

数据列表是文本文件，每行一个条带文件及其 MBIO 格式 ID；数据列表还可以引用其他数据列表。`mbmakedatalist` 可自动生成数据列表。程序检查目标目录（未指定 `-I` 时为当前目录）中的每个文件是否符合 MB-System 文件命名约定，例如格式 88 的 `.mb88`，或 Kongsberg 厂商扩展名 `.all` 和 `.kmall`，并将每个可识别的文件连同根据文件名推断出的格式 ID 一起加入列表。无法识别的文件、目录以及小于 `-B` 阈值的文件会被跳过。文件按字母顺序排列。

通常最好用 `-S` 指定所需文件的扩展名，以免无关但恰好符合某种命名约定的文件被包含进来。使用 `-S` 时，可以用 `-P` 排除已处理文件（扩展名前带字母 `p`，例如由 `mbprocess` 生成的 `0001p.mb88`），这样即使目录中原始文件和已处理文件并存，也能生成只含原始文件的数据列表。`-F` 为所有文件指定同一个格式 ID，而不进行推断。

Kongsberg 文件名以测线号开头，每次重新开始记录时都会从 0001 重新编号，因此按字母顺序排列不一定是时间顺序。当 `-S` 指定的扩展名为 `.all`、`.ALL`、`.kmall`、`.KMALL`、`.mb56`、`.mb57`、`.mb58`、`.mb59` 或 `.mb261` 时，文件会按文件名中的日期和时间（`NNNN_YYYYMMDD_HHMMSS...`）排序；`-T` 可关闭此功能。名为 `9999.all`、`9999.ALL`、`9999.kmall` 或 `9999.KMALL` 的 Kongsberg 垃圾文件会被忽略。

数据列表写在当前目录（或 `-O` 指定的路径）。如果找不到任何条带文件，则不会生成数据列表。

## 常用选项

- `-S suffix` — 只考虑以该扩展名结尾的文件（例如 `.all`、`.kmall`、`.mb88`）。比较时区分大小写。
- `-I directory` — 要扫描的目录。默认：当前目录。
- `-O datalist` — 输出数据列表的名称。默认值：`datalist.mb-1`。
- `-P` — 与 `-S` 一起使用，排除已处理文件（扩展名前带 `p`）。
- `-F format` — 为所有文件指定该格式 ID，而不根据文件名推断。
- `-B size` — 忽略小于 size 千字节的文件。
- `-L` — 排除列表中的最后一个文件，例如采集时仍在记录中的文件。
- `-T` — 不将 Kongsberg 文件按时间排序。
- `-V` — 显示检查内容以及加入的每个文件。
- `-H` — 显示帮助后退出。

每个选项都有语法中所示的长格式（例如 `--suffix=.all`）。

## 示例

用当前目录中所有可识别的条带文件生成 `datalist.mb-1`，并显示加入的文件：

    mbmakedatalist -V

只将子目录 `rawdata` 中的 Kongsberg 原始 `.all` 文件按时间顺序写入 `kongsberg.mb-1`，并排除仍在记录的文件：

    mbmakedatalist -I rawdata -S .all -P -L -O kongsberg.mb-1

使用长格式选项完成同样的操作：

    mbmakedatalist --input=rawdata --suffix=.all --ignore-processed --skip-latest --output=kongsberg.mb-1

为大于 100 千字节的 `.kmall` 文件生成数据列表：

    mbmakedatalist -S .kmall -B 100

## 注意

- 使用 `-I` 时，条目以相对于当前目录的 `directory/filename` 形式写出，因此后续的 MB-System 程序应在同一目录中运行，或者给 `-I` 指定绝对路径。
- 扩展名匹配区分大小写：`-S .all` 不会匹配 `0001_20250101_000000.ALL`。
- 启用 Kongsberg 时间排序时，文件名中不含 `NNNN_YYYYMMDD_HHMMSS` 模式的文件会从列表中删除；如果文件命名方式不同，请使用 `-T`。
- 可用 `mbdatalist -I datalist.mb-1` 或 `mbinfo -I datalist.mb-1` 检查结果。

## 另请参阅

`mbdatalist`、`mbinfo`、`mbformat`、`mbpreprocess`、`mbprocess`。完整参考为 MB-System 手册页 mbmakedatalist(1)（英文）。
