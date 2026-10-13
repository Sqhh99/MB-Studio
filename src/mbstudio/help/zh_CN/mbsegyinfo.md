# mbsegyinfo — 汇总 SEGY 文件的内容

`mbsegyinfo` 读取 SEGY 格式的地震或浅地层剖面文件并报告基本统计信息。可用它
检查 `mbextractsegy`（或其他来源）生成的 SEGY 文件，并生成 `mbsegygrid` 和
`mbsegypsd` 用来确定网格大小的 `.sinf` 信息文件。

## 语法

    mbsegyinfo -Ifile [-Llonflip -O -H -V]
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --output-file {-O}
        --verbose {-V}

## 说明

报告内容包括：文件头的值（通道数、采样间隔、每道采样数、道长、采样数据格式、
CDP 覆盖次数）、道数，炮号、道号、RP（CMP）号、延迟、距离、震源和接收器高程
以及水深的最小值、最大值和范围，首道和末道的时间与位置，以及数据的地理范围。

默认情况下报告显示在屏幕上。使用 `-O` 时，报告改为写入在输入文件名后加
`.sinf` 的文件，例如 `line_0001_env.segy.sinf`。其他 MB-System 程序会读取这些
`.sinf` 文件；`mbextractsegy` 会为写出的每个 SEGY 文件自动生成它们，当 `.sinf`
文件缺失或比 SEGY 文件旧时，`mbsegygrid` 和 `mbsegypsd` 会自行运行
`mbsegyinfo -O`。

`mbsegyinfo` 每次运行读取一个 SEGY 文件，不接受数据列表。

## 常用选项

- `-I<file>` — 输入 SEGY 文件。必需。
- `-O` — 将报告写入 `<file>.sinf`，而不是显示在屏幕上。
- `-L<lonflip>` — 报告的经度范围：`-1` = -360 到 0，`0` = -180 到 180，
  `1` = 0 到 360。默认：由 `mbdefaults` 设置。
- `-V` — 详细模式；同时显示程序版本和读取错误。
- `-H` — 显示简短帮助并退出。

## 示例

显示 SEGY 文件的统计信息：

    mbsegyinfo -I sbp_0001_env.segy

生成（或更新）`.sinf` 文件 `sbp_0001_env.segy.sinf`：

    mbsegyinfo -I sbp_0001_env.segy -O

通过重定向屏幕输出，以自选名称保存报告：

    mbsegyinfo -I sbp_0001_env.segy > sbp_0001_report.txt

## 注意

- 文件名含空格时请加引号，例如 `-I "Line 12.segy"`。
- `.sinf` 文件写在输入文件所在位置。

## 另请参阅

`mbextractsegy`、`mbsegylist`、`mbsegygrid`、`mbsegypsd`、`mbinfo`。完整参考
请见 MB-System 手册页 mbsegyinfo(1)（英文）。
