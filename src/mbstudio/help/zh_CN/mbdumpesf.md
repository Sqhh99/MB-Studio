# mbdumpesf — 导出或筛选编辑保存文件（.esf）中的波束编辑

`mbdumpesf` 读取 MB-System 的编辑保存文件（`.esf`），该文件保存由 `mbclean`、`mbareaclean` 或交互式编辑生成的波束标记，并以文本形式列出每个编辑事件。它也可以把编辑复制到新的二进制 `.esf` 文件，同时排除选定类型的编辑。可用于检查文件中哪些波束被标记，或在运行 `mbprocess` 之前去除某些类型的编辑。

## 语法

    mbdumpesf --input=esffile
        [--output=esffile --ignore-unflag --ignore-flag
        --ignore-filter --ignore-zero
        --verbose --help]

## 说明

在 MB-System 中，波束标记不会写入原始条带文件，而是作为编辑事件保存在二进制的“编辑保存文件”中，文件名为条带文件名加上 `.esf`（例如 `0001_20250101_000000.all.esf`），之后由 `mbprocess` 应用。每个事件包含 ping 时间戳、波束号和动作：

    action=1: flag manually
    action=2: unflag
    action=3: zero - make beam null
    action=4: flag by autofilter

不使用 `--output` 时，每个事件以一行文本写到标准输出，例如 `EDITS READ: i:0 time: 947562291.799000 2008/04/22 22:26:36.799000 beam:21 action:1`，其中时间同时以纪元秒（自 1970-01-01 起的秒数）和日期时间给出。

使用 `--output=esffile` 时，事件改为以二进制形式写入新的编辑保存文件。`--ignore-...` 选项会将相应类型的事件从输出中排除；在文本模式下，被排除的事件仍会列出，并标注 `** EDITS READ BUT IGNORED **`。使用 `--verbose` 时，最后会打印各类编辑的总数。

## 常用选项

- `--input=esffile`（或 `-Iesffile`）— 要读取的编辑保存文件。必需。
- `--output=esffile` — 将（剩余的）编辑写入此二进制编辑保存文件，而不是以文本列出。
- `--ignore-flag` — 排除手动标记事件（action 1）。
- `--ignore-unflag` — 排除取消标记事件（action 2）。
- `--ignore-zero` — 排除置零（波束置空）事件（action 3）。
- `--ignore-filter` — 排除自动滤波标记事件（action 4）。
- `--verbose`（或 `-V`）— 打印读取和排除的编辑总数。
- `--help`（或 `-H`）— 显示帮助后退出。

## 示例

在屏幕上列出一个文件的全部编辑事件：

    mbdumpesf -I survey\0001_20250101_000000.all.esf

将列表保存到文本文件：

    mbdumpesf -I survey\0001_20250101_000000.all.esf > edits.txt

生成不含自动滤波标记的编辑副本，并显示总数：

    mbdumpesf --input=0001_20250101_000000.all.esf --output=manual_only.esf --ignore-filter --verbose

## 注意

- 只有 `-I`、`-V` 和 `-H` 有短格式；输出和排除选项必须使用以 `--` 开头的长格式。
- 若要使用筛选后的副本，请先备份原文件，再将副本重命名为条带文件名加 `.esf`，然后重新运行 `mbprocess`。
- 含空格的文件名需加引号。

## 另请参阅

`mbclean`、`mbareaclean`、`mbprocess`、`mbgetesf`、`mbvoxelclean`、`mbset`。完整参考为 MB-System 手册页 mbdumpesf(1)（英文）。
