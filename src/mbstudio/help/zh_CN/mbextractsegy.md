# mbextractsegy — 从条带文件中提取浅地层剖面数据为 SEGY

`mbextractsegy` 读取包含浅地层剖面仪、中央波束反射或地震反射记录的条带声呐
数据文件，并将这些道写入 SEGY 文件。数据采集完成后，可用它从 Reson 7k 或
Edgetech Jstar 文件中取出浅地层数据，然后用 `mbsegyinfo` 检查、用
`mbsegylist` 列出、用 `mbsegygrid` 生成剖面网格。

## 语法

    mbextractsegy
        --begin-time=yr/mo/da/hr/mn/sc/us {-Byr/mo/da/hr/mn/sc/us}
        --check-route-bearing {-M}
        --end-time=yr/mo/da/hr/mn/sc/us {-Eyr/mo/da/hr/mn/sc/us}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --output=segyfile {-Osegyfile}
        --plot-max=plotmax {-Zplotmax}
        --plot-scale=xscale/yscale/maxwidth {-Jxscale/yscale/maxwidth}
        --range-threshold=rangethreshold {-Urangethreshold}
        --route=routefile {-Rroutefile}
        --sample-format=sampleformat {-Ssampleformat}
        --start-line=startline/lineroot {-Lstartline/lineroot}
        --time-list=timelistfile {-Qtimelistfile}
        --time-shift=timeshift {-Ttimeshift}
        --verbose {-V}

## 说明

输入为单个条带文件或数据列表（`-I`）。只有存储此类记录的格式才能提取浅地层
道，例如 Reson 7k（格式 88）和 Edgetech Jstar（格式 132 和 133）。声呐深度、
海底深度、位置、艏向、横摇和纵摇会写入 SEGY 道头。SEGY 文件遵循地震处理软件
包 SIOSEIS 的约定，采样值为 32 位 IEEE 浮点数。

输出有以下三种组织方式：

- 不使用 `-O`、`-R` 或 `-Q` 时，每个输入条带文件生成各自的 SEGY 输出。文件名由
  `-L` 设置的测线根名和顺序测线号组成（默认根名 `sbp`，起始号 1），例如
  `sbp_0001_env.segy`、`sbp_0002_env.segy`……
- 使用 `-O` 时，所有输入文件的道都写入以给定根名命名的同一输出，例如
  `-O survey` 生成 `survey_env.segy`。
- 使用航线文件（`-R`，`mbgrdviz` 航线文件）或航点时间列表（`-Q`，由
  `mbroutetime` 生成）时，数据按计划测线分割成多个文件，命名为
  `<lineroot>_<number>`。

文件名结尾取决于 `-S` 选择的采样形式：包络输出（默认）加 `_env`；道输出不加；
解析信号输出生成三个文件，分别以 `_cor`（实部）、`_hil`（虚部）和 `_env`
（包络）结尾。扩展名为 `.segy`，使用 `-Osgy` 时为 `.sgy`。

对每个写出的 SEGY 文件，`mbextractsegy` 都会运行 `mbsegyinfo -O`，生成在
SEGY 文件名后加 `.sinf` 的统计文件。它还会写出一个名为 `..._section.cmd` 的
初步绘图脚本，其中包含生成剖面网格的 `mbsegygrid` 命令（见“注意”）。

## 常用选项

- `-I<file>` — 输入条带文件或数据列表。默认：`datalist.mb-1`。
- `-F<format>` — 输入的 MB-System 格式编号；负值表示输入为数据列表。默认：
  根据文件扩展名推断。
- `-O<root>` — 将所有输出写入以此为根名的一组 SEGY 文件。特殊值 `-Osegy` 和
  `-Osgy` 仅用于选择扩展名。
- `-S<sampleformat>` — 输出采样形式：`1` = 保持原样（不转换），`2` = 单道
  （解析信号取实部），`3` = 包络（默认），`4` = 解析信号（三个文件）。
- `-R<routefile>` — 使用 `mbgrdviz` 航线文件中的航点将输出按测线分割。
- `-Q<timelistfile>` — 使用到达各航点的时间列表（由 `mbroutetime` 生成）将输出
  按测线分割。
- `-L<startline>/<lineroot>` — 输出文件的起始测线号和文件名根。默认：`1/sbp`。
- `-M` — 航线模式下跳过转弯期间的道；当艏向与下一条测线方位相差 15 度以内时
  才开始输出。
- `-U<rangethreshold>` — 判定到达航点的距离（米）。默认：25。
- `-T<timeshift>` — 在道时间戳上加上此秒数。默认：0。
- `-B<yr/mo/da/hr/mn/sc>` / `-E<yr/mo/da/hr/mn/sc>` — 提取数据的时间窗口的
  开始和结束。
- `-J<xscale>/<yscale>/<maxwidth>` — 绘图脚本中剖面图的比例（每炮英寸、每秒
  英寸、最大宽度英寸）。默认：`0.01/50.0/30.0`。
- `-Z<plotmax>` — 剖面图中显示为全黑的道值。默认：50。
- `-V` — 详细输出；每写出十道报告一次。

## 示例

提取一个 Reson 7k 文件中的浅地层数据（生成 `sbp_0001_env.segy` 和
`sbp_0001_env.segy.sinf`）：

    mbextractsegy -I 20040722_152111.s7k

将数据列表中的所有文件提取到一个 SEGY 文件 `survey_sbp_env.segy`：

    mbextractsegy -I datalist.mb-1 -O survey_sbp

将数据按计划测线分割为 `line_0001_env.segy`、`line_0002_env.segy`……并去除
转弯部分：

    mbextractsegy -I datalist.mb-1 -R survey_route.rte -L1/line -M

输出原始道而不是包络：

    mbextractsegy -I 20040722_152111.s7k -S2

## 注意

- 输出文件写入当前目录。
- `-O` 请给出不带扩展名的根名；程序会自行添加 `_env.segy` 等。
- `_section.cmd` 绘图脚本是调用 `mbm_grdplot` 的 Unix shell 脚本，在 Windows
  上不可用。可参考其中的 `mbsegygrid` 命令行生成网格，再用 `gmt` 绘图。出现
  无法识别 `chmod` 的提示可以忽略。

## 另请参阅

`mbsegyinfo`、`mbsegylist`、`mbsegygrid`、`mbsegypsd`、`mbroutetime`、
`mb7k2jstar`、`mbdatalist`。完整参考请见 MB-System 手册页
mbextractsegy(1)（英文）。
