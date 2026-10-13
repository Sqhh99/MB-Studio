# mbsegylist — 列出 SEGY 文件的道头值

`mbsegylist` 读取 SEGY 格式的地震或浅地层剖面文件，按每道一行输出选定的道头
值。可用它获取浅地层测线的炮点时间和位置（例如绘制航迹或与其他数据合并），
或在用 `mbsegygrid` 生成网格前检查延迟和道长。

## 语法

    mbsegylist -Ifile
        --binary-output {-A}
        --decimate=value {-Dvalue}
        --delimiter=character {-Gcharacter}
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --output-format=list {-Olist}
        --segment-tag=tag {-Ztag}
        --use-feet {-W}
        --verbose {-V}

## 说明

输出以制表符分隔的文本列形式写到屏幕（标准输出），列的顺序由 `-O` 指定。用
`>` 重定向即可保存到文件。使用 `-A` 时，数值改为以双精度二进制形式输出。

`-O` 列表是一串字符，每个字符对应一列输出：

- `C` CMP（RP、CDP）号；`c` CMP 道集内的道号
- `D` 道起始延迟（秒）；`I` 采样间隔（秒）；`L` 道长（秒）；`N` 采样数
- `i` 距上一道的时间（秒）；`V` 炮或脉冲间隔（秒）
- `J` 时间字符串 `yyyy jd hh mm ss.ssssss`（jd = 年积日）；`j` 时间字符串
  `yyyy jd dm ss.ssssss`（dm = 当日分钟数）
- `T` 时间字符串 `yyyy/mm/dd/hh/mm/ss`；`t` 时间字符串 `yyyy mm dd hh mm ss`
- `M` Unix 时间（小数秒）；`m` 自首道起的小数秒；`U` Unix 时间（整数秒）；
  `u` 自首道起的整数秒
- `l` 文件头中的测线号；`n` 道计数器（从 0 开始）
- `R` 震源到接收器的距离（米）；`S` 炮号；`s` 炮内道号
- `X` 经度（十进制度）；`x` 经度（度、十进制分、E/W）；`Y` 纬度（十进制度）；
  `y` 纬度（度、十进制分、N/S）
- `Z` 震源或声呐深度（米，向下为正）；`z` 水深（米，向下为正）
- `/` 对下一个值取倒数；`-` 改变下一个值的符号（仅限简单数值）

默认列表为 `TiXYSsCcDINL`。

## 常用选项

- `-I<file>` — 输入 SEGY 文件。必需。
- `-O<list>` — 输出的值及其顺序（见上文）。默认：`TiXYSsCcDINL`。
- `-G<character>` — 文本输出的字段分隔符。默认：制表符。
- `-D<value>` — 每隔 n 道输出一道。默认：1（所有道）。
- `-A` — 以二进制（双精度值）而非文本输出。
- `-L<lonflip>` — 经度范围：`-1` = -360 到 0，`0` = -180 到 180，`1` = 0 到
  360。默认：由 `mbdefaults` 设置。
- `-W` — 深度 `Z` 和 `z` 以英尺而非米输出。
- `-Z<tag>` — 在数据前写出此标记行（仅限文本输出），例如用于 GMT 多段文件的
  `-Z">"`。
- `-V` — 详细模式。

## 示例

列出每道的炮号、时间和位置：

    mbsegylist -I sbp_0001_env.segy -OSTXY

将 Unix 时间、经度、纬度和水深保存为逗号分隔文件：

    mbsegylist -I sbp_0001_env.segy -OMXYz -G, > sbp_0001_nav.csv

每十道列出一次延迟、采样数和道长：

    mbsegylist -I sbp_0001_env.segy -OSDNL -D10

## 注意

- `mbsegylist` 每次运行读取一个 SEGY 文件，不接受数据列表。
- 在命令提示符中，`>` 会将输出写入当前目录下的文件。

## 另请参阅

`mbsegyinfo`、`mbextractsegy`、`mbsegygrid`、`mblist`。完整参考请见
MB-System 手册页 mbsegylist(1)（英文）。
