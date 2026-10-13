# mbconfig — 显示 MB-System 版本和安装配置

`mbconfig` 显示已安装 MB-System 的信息：版本（多种形式）、安装前缀、基于 MB-System 库编译程序所需的编译器和链接器标志，以及 Levitus 声速数据库和 OTPS 潮汐预报软件的配置位置。可用它查看已安装的 MB-System 版本，例如记录在处理日志或批处理文件中。

## 语法

    mbconfig --verbose --help --prefix --cflags --libs
             --version --version-id --version-major --version-minor --version-archive
             --levitus --otps

## 说明

每个请求的值单独一行写到标准输出，不附加其他文字，便于捕获输出。如果未给出任何选项，`mbconfig` 显示版本字符串，相当于给出了 `--version`。

可以组合多个选项。无论选项在命令行中的顺序如何，各值总是按固定顺序输出（前缀、编译标志、链接标志、版本、版本号、主版本号、次版本号和归档版本号、Levitus 位置、OTPS 位置）。

使用 `--verbose` 时，每个值前面有一行以 `#` 开头的说明（例如 `# MB-System version:`），程序名和版本也会写到标准错误。

版本号（version id）是一个随新版本发布而递增的整数：version_id = 10000000 x major + 100000 x minor + archive，因此版本 5.5.2304 的版本号为 50502304。便于在脚本中比较版本。

前缀、标志和数据位置是构建 MB-System 时固定的值。它们反映构建时的配置，不一定与 MB-Studio 的安装文件夹一致（见“注意”）。

## 常用选项

- `--version` — 显示 MB-System 版本字符串（未给出选项时的默认行为）。
- `--version-id` — 以单个整数显示版本（见“说明”）。
- `--version-major` — 显示主版本号（三个数字中的第一个）。
- `--version-minor` — 显示次版本号（第二个数字）。
- `--version-archive` — 显示归档（修订）号（第三个数字）。
- `--prefix` — 显示构建时配置的安装前缀。
- `--cflags` — 显示用于定位 MB-System 头文件的编译器标志。
- `--libs` — 显示 MB-System 库的链接器标志。
- `--levitus` — 显示 `mblevitus` 使用的 Levitus 年平均声速数据库的配置路径。
- `--otps` — 显示 `mbotps` 使用的 OTPS 潮汐预报软件的配置位置。
- `--verbose` — 在每个值之前加一行以 `#` 开头的说明。给出两次或以上时输出调试信息。
- `--help` — 在标准错误上显示简短说明和用法。

## 示例

显示已安装的 MB-System 版本：

    mbconfig

带说明标签显示版本及版本号：

    mbconfig --version --version-id --verbose

将版本保存到文本文件，例如作为处理记录的一部分：

    mbconfig --version > mbsystem_version.txt

## 注意

- 只接受长选项（以 `--` 开头）；未知选项会显示用法并以错误退出。
- `--prefix`、`--levitus` 和 `--otps` 显示的是构建时固定的路径。Windows 版的 `mblevitus` 会相对于自身程序文件夹（安装文件夹中的 `share\mbsystem`）或在环境变量 `MBSYSTEM_SHAREDIR` 指定的文件夹中查找数据库，因此这些路径可能与实际使用的文件不同。
- `--cflags` 和 `--libs` 只对基于 MB-System 库编译自有程序的开发人员有用。

## 另请参阅

`mbdefaults`、`mblevitus`、`mbotps`、`mbformat`。完整参考见 MB-System 手册页 mbconfig(1)（英文）。
