# mbpreprocess — 预处理原始条带数据并合并辅助数据

`mbpreprocess` 是 MB-System 中处理原始条带声呐数据的第一步。它读取原始文件（通常通过数据列表），将其以 MB-System 处理格式写出，同时生成其他程序所需的辅助文件，并可从数据流或外部文件合并导航、传感器深度、航向、姿态、高度和声速。应在 `mbmakedatalist` 之后、编辑（`mbclean`）和 `mbprocess` 之前运行。

## 语法

    mbpreprocess --input=datalist [--format=format_id]
        --verbose --help
        --output-directory=directory
        --platform-file=platform_file --platform-target-sensor=sensor_id
        --output-sensor-fnv --skip-existing
        --nav-file=file --nav-file-format=format_id
        --nav-async=record_kind --nav-sensor=sensor_id
        --sensordepth-file=file --sensordepth-file-format=format_id
        --sensordepth-async=record_kind --sensordepth-sensor=sensor_id
        --heading-file=file --heading-file-format=format_id
        --heading-async=record_kind --heading-sensor=sensor_id
        --altitude-file=file --altitude-file-format=format_id
        --altitude-async=record_kind --altitude-sensor=sensor_id
        --attitude-file=file --attitude-file-format=format_id
        --attitude-async=record_kind --attitude-sensor=sensor_id
        --attitude-zero-heave
        --soundspeed-file=file --soundspeed-file-format=format_id
        --soundspeed-async=record_kind --soundspeed-sensor=sensor_id
        --time-latency-file=file --time-latency-file-format=format_id
        --time-latency-constant=value
        --time-latency-apply-nav --time-latency-apply-sensordepth
        --time-latency-apply-heading --time-latency-apply-attitude
        --time-latency-apply-altitude --time-latency-apply-all-ancilliary
        --time-latency-apply-survey --time-latency-apply-all
        --filter=value
        --filter-apply-nav --filter-apply-sensordepth
        --filter-apply-heading --filter-apply-attitude
        --filter-apply-altitude --filter-apply-all-ancilliary
        --recalculate-bathymetry --no-change-survey
        --multibeam-sidescan-source=recordid
        --sounding-amplitude-filter=value --sounding-altitude-filter=value
        --ignore-water-column
        --head1-offsets=x/y/z/heading/roll/pitch
        --head2-offsets=x/y/z/heading/roll/pitch
        --kluge-time-jumps=threshold --kluge-fix-7k-timestamps=targetoffset
        --kluge-ancilliary-time-jumps=threshold
        --kluge-mbaripressure-time-jumps=threshold
        --kluge-beam-tweak=factor --kluge-soundspeed-tweak=factor
        --kluge-zero-attitude-correction --kluge-zero-alongtrack-angles
        --kluge-fix-wissl-timestamps --kluge-auv-sentry-sensordepth
        --kluge-ignore-snippets --kluge-sensordepth-from-heave
        --kluge-early-MBARI-Mapping-AUV
        --kluge-flipsign-roll --kluge-flipsign-pitch
        --kluge-set-beamwidths=beamwidth-acrosstrack/beamwidth-alongtrack
        --kluge-set-beamwidth-acrosstrack=beamwidth-acrosstrack
        --kluge-set-beamwidth-alongtrack=beamwidth-alongtrack
        --kluge-ignore-duplicate-pings
        --kluge-xducer-depth-from-heave
        --kluge-xducer-depth-from-sensordepth
        --kluge-xducer-depth-from-heave-and-sensordepth
        --kluge-rangescale=factor --kluge-fix-wissl2-ranges

## 说明

`mbpreprocess` 取代了旧版 MB-System 中针对各格式的预处理程序。对每个输入文件分两遍处理。第 1 遍从数据流及指定的外部文件中读取异步辅助数据（导航、传感器深度、航向、姿态、高度、声速）。第 2 遍将测量数据写入新文件，同时把所选辅助数据合并到每个 ping 中，并生成后续程序使用的辅助文件：`.inf` 统计文件（通过运行 `mbinfo` 生成）、`.fnv` 导航文件、通常还有 `.fbt` 水深文件，以及姿态和传感器深度的时间序列文件。

输出文件名为输入文件名去掉扩展名后加上 `.mbXX`，其中 XX 为输出格式 ID。部分格式会转换为扩展的处理格式：例如 Kongsberg `.all` 数据（格式 58）变为格式 59（`0001_20250101_000000.mb59`），旧 Kongsberg 数据（56）变为格式 57，Imagenex 83P 数据（191）变为格式 192。其他格式保持原格式 ID（例如 `.kmall` 数据变为 `.mb261`）；如果输出文件名与输入文件名相同，则在扩展名前加字母 `r`。除非指定 `--output-directory`，输出文件写在输入文件旁边。

默认使用数据中已有的辅助数据值。每种数据都可以选择其他来源：外部时间序列文件（`--nav-file` 加 `--nav-file-format`，传感器深度、航向、高度、姿态和声速同理），或数据流中的另一种记录类型（如 `--nav-async=record_kind`）。合并导航、传感器深度、航向、姿态或声速，或给出任何 `...-apply-...` 选项时，也会开启水深重新计算。对于 Kongsberg `.kmall` 数据，程序会自动利用安装参数数据报选择当前使用的导航和姿态系统，并建立传感器几何关系。

平台文件（用 `mbmakeplatform` 生成）仅在数据中尚未应用传感器偏移时才需要，通常用于 AUV 或 ROV 测量；只有少数格式支持，例如 88、89、261 以及 3D at Depth 激光雷达格式。`--time-latency-...` 选项用于校正辅助数据或测量数据的时间偏差，`--filter` 用高斯滤波器平滑辅助数据；如果未给出 `...-apply-...` 选项，两者都会应用于全部辅助数据。`--kluge-...` 选项是针对特定数据集问题的修正，使用前请阅读手册页。

## 常用选项

- `--input=file` — 要预处理的条带文件或数据列表。务必指定。
- `--format=format_id` — 输入的 MBIO 格式。默认：根据文件扩展名推断。
- `--output-directory=directory` — 将输出文件写入该目录，而不是写在输入文件旁边。
- `--skip-existing` — 如果输出文件已存在且比输入文件更新、更大，则跳过该文件。
- `--verbose` — 显示进度（两遍处理的记录数）。
- `--nav-file=file` — 要合并的外部导航，替换数据中的导航。
- `--nav-file-format=format_id` — 导航文件格式：`1` 纪元秒 经度 纬度 [速度]；`2` 年 月 日 时 分 秒 经度 纬度；`3` 年 年积日 时 分 秒 经度 纬度；`4` 年 年积日 日内分钟 秒 经度 纬度；`5` L-DEO 导航；`6` 或 `7` NMEA 0183（ZDA、GLL、GGA 等）；`8` Simrad 90；`9` MB-System fnv 风格（日期时间、纪元秒、经度、纬度、航向、速度、传感器深度 [、横摇、纵摇、升沉]）；`10` R2R；`11` RVDAS；`12` Kongsberg Navlab 的 `navlab_smooth.bin`。
- `--nav-async=record_kind` — 数据中用作导航的记录类型，例如 `12`（导航）、`29`、`30`、`31`（导航系统 1-3）、`1`（测量记录）。
- `--sensordepth-file=file`、`--heading-file=file`、`--attitude-file=file`、`--altitude-file=file`、`--soundspeed-file=file` — 要合并这些值的外部时间序列文件；对应的 `...-file-format=` 选项可取 `1`（纪元秒和数值）、`2`（年 月 日 时 分 秒和数值）、`3`（年 年积日 时 分 秒 ...）、`4`（年 年积日 日内分钟 秒 ...），传感器深度、航向和姿态还可像导航一样使用 `9`、`11` 和 `12`。姿态文件包含横摇、纵摇和升沉。
- `--sensordepth-async=`、`--heading-async=`、`--attitude-async=`、`--altitude-async=`、`--soundspeed-async=record_kind` — 用于这些值的记录类型，例如 `59` 声呐深度、`16` 高度计记录、`17` 航向、`18` 姿态、`56`-`58` 姿态系统 1-3、`60` 高度、`19` SSV、`34` CTD。用 `mbinfo -N` 可查看文件中包含哪些记录类型。
- `--attitude-zero-heave` — 将所有升沉值置为零。
- `--platform-file=file` — 使用该平台文件进行杆臂和角度偏移校正。
- `--platform-target-sensor=sensor_id` — 获取测量数据的平台传感器（辅助传感器使用 `--nav-sensor`、`--sensordepth-sensor`、`--heading-sensor`、`--attitude-sensor`）。
- `--time-latency-constant=value` — 固定时间延迟（秒）；随时间变化的模型使用 `--time-latency-file=` 和 `--time-latency-file-format=`（1-4）。通过 `--time-latency-apply-...` 选项选择应用对象。
- `--filter=value` — 辅助数据高斯平滑滤波器的半宽（秒），通过 `--filter-apply-...` 选项选择应用对象。
- `--recalculate-bathymetry` — 在预处理过程中重新计算水深（取决于格式）。
- `--no-change-survey` — 不修改测量数据。
- `--multibeam-sidescan-source=recordid` — 多波束侧扫声呐图像所用的后向散射（格式 88、89、261）：`S` snippet、`C` 校准后的 snippet、`B` 宽波束后向散射、`W` 校准后的宽波束后向散射。默认：可用的最佳来源。
- `--ignore-water-column` — 不将水柱记录复制到输出（Reson/Teledyne 7k 格式）。
- `--help` — 显示帮助后退出。

## 示例

生成 Kongsberg 原始文件的数据列表并进行预处理，同时显示进度：

    mbmakedatalist -S .all
    mbpreprocess --input=datalist.mb-1 --verbose

对不断增加的数据集重新运行，跳过已预处理的文件，并将输出写入已存在的文件夹 `preprocessed`：

    mbpreprocess --input=datalist.mb-1 --skip-existing --output-directory=preprocessed

用后处理位置文件（每行为纪元秒、经度、纬度）替换导航，并校正导航 0.05 秒的固定延迟：

    mbpreprocess --input=datalist.mb-1 --nav-file=ppk_nav.txt --nav-file-format=1 --time-latency-constant=0.05 --time-latency-apply-nav

使用平台文件预处理 AUV 的 Reson 7k 数据，其中传感器 2 为多波束，传感器 0 为惯导，传感器 1 为压力传感器：

    mbpreprocess --input=datalist.mb-1 --platform-file=auv.plf --platform-target-sensor=2 --nav-sensor=0 --heading-sensor=0 --attitude-sensor=0 --sensordepth-sensor=1

## 注意

- `mbpreprocess` 只有长格式选项（`--name` 或 `--name=value`），没有单字母形式。
- `--output-directory` 和 `--platform-file` 指定的路径不能包含空格。其他含空格的文件名需加引号，例如 `--input="my survey\datalist.mb-1"`。
- 输出目录必须事先存在。
- 预处理之后，为新文件生成数据列表（例如 `mbmakedatalist -S .mb59 -P -O datalistp.mb-1`），然后继续执行 `mbclean`、`mbset` 和 `mbprocess`。
- `.inf` 文件是通过调用 `mbinfo` 生成的，因此请在 MB-System Command Prompt 或 MB-Studio 中运行，这些环境已将 MB-System 的 `bin` 文件夹加入 PATH。

## 另请参阅

`mbmakedatalist`、`mbdatalist`、`mbinfo`、`mbmakeplatform`、`mbclean`、`mbset`、`mbprocess`、`mbnavlist`。完整参考为 MB-System 手册页 mbpreprocess(1)（英文）。
