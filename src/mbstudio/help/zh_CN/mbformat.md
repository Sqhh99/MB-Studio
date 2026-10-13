# mbformat — 识别并说明 MB-System 条带数据格式

`mbformat` 用于说明 MBIO 库支持的条带数据格式。它可以列出所有支持的格式、说明某个格式 ID，或根据文件名判断 MB-System 将采用的格式 ID。在需要为 `-F` 选项或数据列表各行确定格式 ID 时使用。

## 语法

    mbformat
        --format=format_id {-Fformat_id}
        --help {-H}
        --html {-W}
        --input=file {-Ifile}
        --list-ids {-L}
        --list-root {-K}
        --verbose {-V}

## 说明

不带选项运行时，`mbformat` 列出所有支持的格式及其 ID、名称、简要说明和属性（声呐、波束数、数据类型、文件类型）。使用 `-F` 时，打印该格式 ID 的说明。

使用 `-I` 时，程序按 MB-System 内置的文件名前缀和后缀规则（例如 ".mb58"、".all"、".s7k"、".xtf"）检查文件名，并报告推断出的格式 ID。同时还会确定文件的"根名"，即去掉已识别的后缀或前缀后的文件名。判断主要依据文件名；但对某些后缀（例如 ".all"），还会读取文件开头的几个字节以区分格式变体，因此文件应当存在。

`-L` 使输出只包含格式 ID 数字（若找不到有效 ID 则为 0），`-K` 输出文件根名和格式 ID。这些简短输出便于在脚本中使用。`-W` 将所有格式的列表输出为 HTML 页面。

## 常用选项

- `-Fformat_id` (`--format=format_id`) — 说明该格式 ID。若既未指定 `-F` 也未指定 `-I`，则列出所有格式。
- `-Ifile` (`--input=file`) — 根据该文件名推断格式 ID。
- `-L` (`--list-ids`) — 只打印格式 ID 数字（无法识别时为 0）。
- `-K` (`--list-root`) — 打印文件根名和格式 ID。
- `-W` (`--html`) — 以 HTML 输出所有格式的列表。
- `-V` (`--verbose`) — 同时打印程序版本。
- `-H` (`--help`) — 打印简要说明和用法后退出。

## 示例

列出所有支持的格式：

    mbformat

说明格式 58（Kongsberg 第三代多波束厂商格式，适用于 EM122/EM302/EM710 等）：

    mbformat -F58

查询 MB-System 对 Kongsberg ".all" 文件采用的格式 ID：

    mbformat -I 0001_20250101_000000.all

只打印文件名的根名和格式 ID：

    mbformat -K -I 0001_20250101_000000.mb58

将格式列表保存为 HTML 页面：

    mbformat -W > formats.html

## 注意

- 如果文件名没有可识别的后缀，`mbformat` 会报告无法推断格式；此时请在其他程序中用 `-F` 明确指定格式。
- 格式 ID 也用于数据列表：每行写文件名及其格式 ID。

## 另请参阅

`mbinfo`, `mbdatalist`, `mbmakedatalist`, `mbcopy`, `mbpreprocess`, `mbdefaults`。完整参考请见 MB-System 手册页 mbformat(1)（英文）。
