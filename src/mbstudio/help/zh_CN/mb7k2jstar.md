# mb7k2jstar — 从 Reson 7k 提取 Edgetech 侧扫声呐和浅地层数据

`mb7k2jstar` 提取嵌入在 Reson 7k 文件（格式 88）中的 Edgetech 浅地层剖面仪和
侧扫声呐记录，并以 Edgetech Jstar 格式（格式 132 和 133）写出。当 Edgetech
声呐通过 Reson 7k 系统记录（例如在 AUV 上）且需要将侧扫声呐或浅地层数据与
多波束数据分开处理时使用。

## 语法

    mb7k2jstar
        --bottom-pick=mode[/threshold] {-Bmode[/threshold]}
        --check-route-bearing {-M}
        --comments {-C}
        --extract-mode=type {-Atype}
        --flip-sidescan {-X}
        --format=format {-Fformat}
        --gain=mode[/factor] {-Gmode[/factor]}
        --help {-H}
        --input=file {-Ifile}
        --line-root=startline/lineroot {-Lstartline/lineroot}
        --output=file {-Ofile}
        --route-file=file {-Rfile}
        --smooth=value {-Svalue}
        --time-shift=timeshift {-Ttimeshift}
        --verbose {-V}

## 说明

输入为单个 Reson 7k 文件或 7k 文件的数据列表（`-I`）。默认提取所有 Edgetech
通道（浅地层、低频侧扫声呐和高频侧扫声呐）；可用 `-A` 加以限制。每个文件处理
结束时，程序会显示各类记录的读取数和写出数。

输出有以下三种组织方式：

- 不使用 `-O` 或 `-R` 时，每个输入文件生成各自的输出文件，文件名为去掉扩展名
  （以及已处理文件末尾的 `p`）的输入文件名加上新扩展名：提取全部记录类型或
  仅浅地层数据时为 `.jsf`，仅低频侧扫声呐为 `.mb132`，仅高频侧扫声呐为
  `.mb133`。
- 使用 `-O` 时，所有输入文件的数据写入该单个文件。
- 使用 `-R`（`mbgrdviz` 航线文件）时，数据按计划测线分割成多个文件，命名为
  `<lineroot>_<number>` 加上上述扩展名。

首个海底回波的时间会写入 Jstar 道头，可在将侧扫声呐图像归位到海底时使用；
`-B` 选择其获取方式。可对提取的道进行平滑（`-S`）、随距离变化的增益（`-G`）、
时间偏移（`-T`）以及左右舷交换（`-X`）。

## 常用选项

- `-I<file>` — 输入 Reson 7k 文件或数据列表。默认：`datalist.mb-1`。
- `-F<format>` — 输入格式；7k 文件为 `88`，数据列表为负值。默认：根据文件
  扩展名推断。
- `-O<file>` — 将所有提取的数据写入此单个输出文件。
- `-A<type>` — 要提取的数据：`1`（或 `sslow`）低频侧扫声呐，`2`（或 `sshigh`）
  高频侧扫声呐，`3`（或 `sbp`）浅地层，`4`（或 `all`）全部。可重复使用以组合
  多种类型。默认：全部。
- `-B<mode>[/<threshold>]` — 海底到达时间的来源：`1` 多波束水深，`2` 载体导航
  中的高度（默认），`3` 在侧扫声呐数据中拾取第一个超过道最大值 `threshold`
  倍的采样（默认 threshold 0.5）。
- `-G<mode>[/<factor>]` — 侧扫声呐增益：`0` 无（默认），`1` 海底回波之后施加
  与距离平方成正比的增益，并乘以 `factor`（默认 1.0）。
- `-S<value>` — 平滑道采样：正值取两侧各 `value` 个采样的平均，负值改用
  均方根。默认：0（不平滑）。
- `-T<timeshift>` — 将提取记录的时间戳偏移此秒数。默认：0。
- `-X` — 交换侧扫声呐的左舷和右舷通道。
- `-R<routefile>` — 使用 `mbgrdviz` 航线文件将输出按测线分割；距航点 50 米以内
  即视为到达。
- `-L<startline>/<lineroot>` — 航线输出的起始测线号和文件名根。默认：
  `1/jstar`。
- `-M` — 航线模式下跳过转弯期间的数据（艏向与测线方位相差超过 15 度）。
- `-C` — 显示输入中的注释记录。
- `-V` — 详细输出。

## 示例

将一个 7k 文件中的全部 Edgetech 数据提取到 `20050418_150155.jsf`：

    mb7k2jstar -I 20050418_150155.s7k

只将低频侧扫声呐提取到 `20050418_150155.mb132`：

    mb7k2jstar -I 20050418_150155.s7k -A1

将数据列表中所有文件的低频侧扫声呐汇集到一个文件：

    mb7k2jstar -A1 -I datalistp.mb-1 -O 20050418_sslow.mb132

将低频侧扫声呐分割为测线文件 `line_0001.mb132`、`line_0002.mb132`……并去除
转弯部分：

    mb7k2jstar -I datalist.mb-1 -A1 -R survey_route.rte -L1/line -M

## 注意

- 不使用 `-O` 或 `-R` 时，每个输出文件写在其输入文件所在位置；使用 `-R` 时，
  测线文件写入当前目录。路径含空格时请加引号。
- 每个输出文件都会生成一个 `.inf` 文件。
- 只能读取 Reson 7k 数据（格式 88）。

## 另请参阅

`mbextractsegy`、`mbsslayout`、`mbformat`、`mbinfo`、`mbdatalist`。完整参考
请见 MB-System 手册页 mb7k2jstar(1)（英文）。
