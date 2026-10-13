# mbdatalist — 解析数据列表并生成辅助文件

`mbdatalist` 读取数据列表（条带文件及其格式 ID 的列表，也可引用其他数据列表），并输出找到的每个条带文件及其格式 ID 和网格权重。它也是生成 ".inf"、".fbt" 和 ".fnv" 辅助文件（用于加快网格化和绘图）、检查处理状态和问题以及解除残留处理锁的常用工具。

## 语法

    mbdatalist
        [
        --verbose {-V}
        --help {-H}
        --copy {-C}
        --report {-D}
        --format=format_id {-Fformat_id}
        --input=file {-Ifile}
        --make-ancillary {-N}
        --update-ancillary {-O}
        --processed {-P}
        --problem {-Q}
        --bounds=w/e/s/n {-Rw/e/s/n}
        --status
        --raw {-U}
        --unlock {-Y}
        --datalistp {-Z}

## 说明

数据列表是一个文本文件，每行包含文件名、MBIO 格式 ID 和可选的网格权重，例如 `0001_20250101_000000.mb58 58` 或 `survey2\datalist.mb-1 -1 10.0`。数据列表本身的格式 ID 为 -1，因此数据列表可以嵌套。`mbdatalist` 遍历整个结构，每个条带文件输出一行：路径、格式 ID 和权重（默认 1.0）。如果输入的是条带文件而不是数据列表，则只输出该文件。

`mbprocess` 会生成"已处理"文件（例如由 `line1.mb58` 生成 `line1p.mb58`）。当数据列表中出现 `$PROCESSED` 标记或使用 `-P` 时，只要已处理文件存在就报告其文件名；`$RAW` 或 `-U` 只报告原始文件。`-Z` 生成便捷数据列表 `datalistp.mb-1`（由 `datalist.mb-1` 生成），其中包含 `$PROCESSED` 和对输入的引用，使网格化程序读取已处理数据。

`-N` 为每个条带文件生成辅助文件：".inf" 统计文件（由 `mbinfo` 生成）、".fbt" 快速水深文件（由 `mbcopy` 生成，格式 71）和 ".fnv" 快速导航文件（由 `mblist` 生成）。`-O` 只在辅助文件不存在或比数据文件旧时才生成。`mbgrid`、`mbmosaic` 和 `gmt mbswath` 等程序利用这些文件跳过目标区域外的数据并加快读取。

`-R` 只报告 ".inf" 范围与给定区域重叠的文件（没有 ".inf" 文件的文件总会被报告），由此可知 `mbgrid` 将使用哪些文件。`-Q` 检查每个文件的 `mbprocess` 参数文件所引用的文件（编辑保存文件、导航、潮汐、声速剖面）是否缺失，并列出 ".inf" 文件中记录的数据问题。`-S` 报告每个文件是否为最新、是否需要重新处理以及是否被锁定；`-Y` 删除中断的 `mbprocess` 遗留的处理锁（".lck" 文件）。`-D` 列出数据列表文件本身及其递归层级。

## 常用选项

- `-Ifile` (`--input=file`) — 输入数据列表或条带文件。默认：datalist.mb-1。
- `-Fformat_id` (`--format=format_id`) — 输入的格式 ID；数据列表为 -1。默认：根据文件名后缀推断。
- `-N` (`--make-ancilliary`) — 为所有条带文件生成（覆盖）".inf"、".fbt" 和 ".fnv" 文件。
- `-O` (`--update-ancilliary`) — 仅在辅助文件缺失或过期时生成。
- `-P` (`--processed`) — 已处理文件存在时报告其文件名。
- `-U` (`--raw`) — 只报告原始（未处理）文件名。
- `-Z` (`--datalistp`) — 写出一个以输入根名加 "p.mb-1" 命名的数据列表（默认为 datalistp.mb-1），其中设置 `$PROCESSED` 并引用输入，然后退出。
- `-Rw/e/s/n` (`--bounds=w/e/s/n`) — 根据 ".inf" 文件判断，只报告在该范围内有数据的文件。
- `-Q` (`--problem`) — 报告缺失的处理文件和数据问题。
- `-S` (`--status`) — 报告处理状态和锁。
- `-Y` (`--unlock`) — 解除所列文件的处理锁。
- `-D` (`--report`) — 列出递归结构中的数据列表文件，而不是条带文件。
- `-C` (`--copy`) — 将所列条带文件及其辅助文件复制到当前目录，并在该处写出 datalist.mb-1。
- `-V` (`--verbose`), `-H` (`--help`) — 输出详细信息；打印用法后退出。

## 示例

列出当前目录中数据列表引用的所有条带文件：

    mbdatalist

为整个测量项目生成缺失或过期的 ".inf"、".fbt" 和 ".fnv" 文件：

    mbdatalist -I datalist.mb-1 -O

生成 datalistp.mb-1，使后续程序使用已处理文件：

    mbdatalist -I datalist.mb-1 -Z

显示在某区域内有数据的文件，已处理文件存在时使用已处理文件：

    mbdatalist -I datalist.mb-1 -P -R140.5/141.0/35.0/35.4

## 注意

- 在此版本中，`-N` 和 `-O` 的长格式拼写为 `--make-ancilliary` 和 `--update-ancilliary`；程序用法说明中显示的 `--make-ancillary` 和 `--update-ancillary` 不被接受。短格式 `-N` 和 `-O` 始终可用。
- 数据列表中的路径相对于数据列表所在的文件夹。在命令行中，含空格的路径请加引号。
- `-N` 和 `-O` 会对每个文件运行 `mbinfo`、`mbcopy` 和 `mblist`，大型测量项目可能需要较长时间。
- `-C` 复制功能在内部使用 Unix 的 `cp` 命令，在标准 Windows 命令提示符中可能无法工作；请改用资源管理器或 `copy` 复制文件。

## 另请参阅

`mbmakedatalist`, `mbinfo`, `mbprocess`, `mbgrid`, `mbmosaic`, `mbset`, `mbformat`。完整参考请见 MB-System 手册页 mbdatalist(1)（英文）。
