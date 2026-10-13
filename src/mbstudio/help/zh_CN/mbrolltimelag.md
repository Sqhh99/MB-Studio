# mbrolltimelag — 通过横摇与海底坡度相关估计横摇时延

`mbrolltimelag` 用于估计条带水深数据的 ping 时间戳与计算水深时所用横摇数据之间的时间延迟（时间偏差）。程序计算横摇时间序列与视横向海底坡度之间的互相关；如果在非零时延处出现明显的相关峰值，说明姿态数据的时间存在问题。当外侧条带出现随船体横摇变化的“波纹”时，可在质量控制中使用本程序。

## 语法

    mbrolltimelag
        --correlation-threshold=value {-Cvalue}
        --format=format_id {-Fformat_id}
        --help {-H}
        --input=swathdata {-Iswathdata}
        --lag-range=nlag/lagmin/lagmax {-Tnlag/lagmin/lagmax}
        --nav-channel=navchannel {-Snavchannel}
        --npings=nping {-Nnping}
        --output=outputname {-Ooutputname}
        --roll-source=rollsource {-Krollsource}
        --verbose {-V}

## 说明

如果横摇以错误的时间戳进行改正，每条横向剖面都会按横摇误差倾斜，使一舷偏浅、另一舷偏深。`mbrolltimelag` 从视横向海底坡度中减去已改正的横摇，然后在一系列时延下计算其与横摇信号的互相关。在较平坦的海底上，坡度信号主要由横摇误差构成，互相关会在真实时延处出现明显的峰值。

输入可以是单个条带文件或数据列表。程序先用 `mbnavlist` 从全部输入中提取横摇时间序列，再用 `mblist` 提取每个 ping 的坡度（两者均自动运行）。随后按每 `nping` 个 ping 分块处理：对每一块在 `lagmin` 到 `lagmax` 秒之间等间隔的 `nlag` 个时延上计算互相关函数；若相关峰值超过 `-C` 设定的阈值，则将峰值对应的时延作为一个估计值。

默认的输出文件名根为去掉 MB-System 后缀的输入名（对于 `datalist.mb-1`，根为 `datalist`）。程序写出以下文件：

- `root_timelagest.txt` — 被采用的估计值：时间（历元秒）和时延（秒）。
- `root_timelaghist.txt` — 所有被采用的时延值，用于直方图。
- `root_timelagmodel.txt` — 对直方图峰值明确的每个输入文件给出一个时延（时间和时延）。
- `root_xcorr.txt` — 所有互相关函数（仅当输入为数据列表时）。

此外，对每个条带文件还会在其所在位置写出以该文件命名的 `_xcorr.txt` 和 `_timelaghist.txt` 文件。如果得到明确且一致的时延，可以加以改正，例如使用 `mbpreprocess` 的时间延迟（time latency）选项。

## 常用选项

- `-Iswathdata` — 输入条带文件或数据列表（长格式 `--input`）。默认值：`datalist.mb-1`。
- `-Fformat_id` — 输入的 MB-System 格式编号（`--format`），负值表示数据列表。默认值：根据文件后缀（如 `.mb88`、`.mb-1`）推断。
- `-Nnping` — 每次互相关估计使用的 ping 数（`--npings`）。默认值：100。
- `-Tnlag/lagmin/lagmax` — 时延个数及时延范围（秒）（`--lag-range`）。默认值：`41/-2.0/2.0`，即步长 0.1 秒。
- `-Cvalue` — 估计值被采用所需的最小相关系数峰值（`--correlation-threshold`）。默认值：0.9。
- `-Krollsource` — 从该类数据记录中提取横摇（`--roll-source`），例如 18 为异步姿态记录，1 为测量数据记录。默认从测量数据记录中提取。
- `-Snavchannel` — 改为从异步导航记录的导航通道 `navchannel`（1、2 或 3）提取横摇（`--nav-channel`）。可用 `mbinfo -N` 查看文件中包含的记录类型。
- `-Ooutputname` — 输出文件名的根（`--output`）。
- `-V` — 详细输出，显示参数和每一块的相关峰值（`--verbose`）。
- `-H` — 显示简要说明和用法后退出（`--help`）。

## 示例

对数据列表中的所有文件估计横摇时延，时延范围 -2 至 2 秒、步长 0.1 秒，横摇取自导航通道 2：

    mbrolltimelag -Idatalistp.mb-1 -T41/-2.0/2.0 -S2 -V

检查单个 Reson 7k 文件：使用异步姿态记录，每次估计 200 个 ping，时延范围更细（-0.5 至 0.5 秒），输出根为 `line0001`：

    mbrolltimelag -I0001_20250101_000000.mb88 -K18 -N200 -T51/-0.5/0.5 -Oline0001

## 注意

- 在 Windows 上，通常由 `mbm_xyplot` 和 `mbm_histplot` 宏生成的绘图会被跳过（这些宏不可用）；文本结果文件仍会写出，可用任何电子表格或绘图工具作图。
- 需要 `mbnavlist` 和 `mblist`，它们安装在同一个 `bin` 文件夹中。
- 只有在海底较平坦且数据覆盖良好时结果才有意义。时延范围应明显大于预期的时延。
- 负值应紧跟在选项后书写（`-T41/-2.0/2.0`），以免被误认为另一个选项。

## 另请参阅

`mbnavlist`、`mblist`、`mbinfo`、`mbpreprocess`、`mbprocess`。完整参考见 MB-System 手册页 mbrolltimelag(1)（英文）。
