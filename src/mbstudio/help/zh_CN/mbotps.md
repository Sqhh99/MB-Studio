# mbotps — 用 OSU TPXO 潮汐模型预报潮位（OTPS 方法）

`mbotps` 利用俄勒冈州立大学（OSU）的全球潮汐模型 TPXO，并采用 OSU 潮汐预报软件（OTPS）的方法预报海洋潮汐。它可以生成单个位置在指定时段内的潮位时间序列，也可以沿每个条带文件的导航生成潮位文件，并可设置由 `mbprocess` 应用潮位改正。当测量没有验潮站数据或 GPS 潮位时使用。

## 语法

    mbotps
        [
        --input=datalist {-Idatalist}
        --format=format_id {-Fformat_id}
        --use-mbprocess {-M}
        --skip-existing {-S}
        --tide-output=file {-Ooutput}
        --tide-position=lon/lat {-Rlon/lat}
        --tide-format=format_id {-Atideformat}
        --interval=interval {-Dinterval}
        --start-time=year/month/day/hour/minute/second {-Byear/month/day/hour/minute/second}
        --end-time=year/month/day/hour/minute/second {-Eyear/month/day/hour/minute/second}
        --tide-station-file=file {-Ntidestationfile}
        --tide-station-position=lon/lat {-Ulon/lat}
        --tide-station-format=format_id {-Ctidestationformat}
        --otps-model=model_name {-Tmodel}
        --otps-path=path {-Ppath}
        --help {-H}
        --verbose {-V}
        ]

## 说明

`mbotps` 自行计算潮位（基于模型网格插值的调和预报，含交点改正和次要分潮推算），不需要 OTPS 的 Fortran 程序。但必须从 OSU 获取 TPXO 潮汐模型（学术及其他非商业用途免费，商业用途需付费许可），并将其数据文件安装在 `--otps-path` 所指 OTPS 位置下的 `DATA` 文件夹中，例如 `C:\otps\DATA\TPXO10_atlas_v2`。程序只读取 `h_*` 潮高文件。运行 `mbotps --help` 可查看当前使用的 OTPS 位置以及找到的模型。

单点模式（不使用 `--input`）：在 `--tide-position` 指定的位置，从 `--start-time` 到 `--end-time` 每隔 `--interval` 秒（默认 60）预报潮位，写入 `--tide-output`（默认 `tide_model.txt`）。

条带模式（使用 `--input`）：对文件或数据列表中的每个条带文件，按给定间隔沿导航（若存在 `.fnv` 文件则使用它）预报潮位，写入以条带文件名加 `.tde` 命名、位于条带文件旁边的文件。使用 `--use-mbprocess` 时，会设置每个文件的处理参数，使 `mbprocess` 应用该潮位文件。位于模型网格范围之外或陆地上的位置会被跳过并给出提示。

验潮站改正：使用 `--tide-station-file`、`--tide-station-position` 和 `--tide-station-format` 时，程序将附近验潮站的实测潮位与该站的模型值比较，并将差值作为随时间变化的改正量应用于两种模式下的预报潮位。

## 常用选项

- `--input=datalist` — 条带文件或数据列表；切换到条带模式（短格式 `-Idatalist`）。
- `--format=format_id` — 输入数据的 MB-System 格式；通常由文件名识别，数据列表为负值（`-Fformat_id`）。
- `--use-mbprocess` — 设置条带文件的处理参数，使 `mbprocess` 应用 `.tde` 文件（`-M`）。
- `--skip-existing` — 跳过 `.tde` 文件已比数据新的条带文件（`-S`）。
- `--tide-output=file` — 单点模式的输出文件（`-Ooutput`）。默认为 `tide_model.txt`。
- `--tide-position=lon/lat` — 单点模式的位置，十进制度（`-Rlon/lat`）。
- `--start-time=year/month/day/hour/minute/second` — 预报时间序列的开始时间（`-B...`）。
- `--end-time=year/month/day/hour/minute/second` — 预报时间序列的结束时间（`-E...`）。
- `--interval=interval` — 时间步长（秒）（`-Dinterval`）。默认为 60。
- `--tide-format=format_id` — 2 = `yr mon day hour min sec tide`（默认）；1、3 或 4 = 以历元秒表示的 `time_d tide`（`-Atideformat`）。
- `--tide-station-file=file` — 用于改正模型的验潮站实测数据（`-Ntidestationfile`）。
- `--tide-station-position=lon/lat` — 验潮站位置（`-Ulon/lat`）。
- `--tide-station-format=format_id` — 验潮站文件格式：1 = `time_d tide`，2 = `yr mon day hour min sec tide`（默认），3 = `yr jday hour min sec tide`，4 = `yr jday daymin sec tide`（`-Ctidestationformat`）。
- `--otps-model=model_name` — 使用的模型：`DATA` 下的模型文件夹名（例如 `TPXO10_atlas_v2`）或 `Model_` 控制文件的名称（`-Tmodel`）。默认为 `tpxo10_atlas`；若所请求的模型未安装，则使用其他已安装的模型。
- `--otps-path=path` — OTPS 位置，即包含 `DATA` 的文件夹（`-Ppath`）。默认值在编译时设定。
- `--verbose` — 详细输出，包括可用模型列表（`-V`）。
- `--help` — 显示用法、列出可用模型后退出（`-H`）。

## 示例

检查 `mbotps` 在何处查找潮汐模型以及找到了哪些模型：

    mbotps --otps-path=C:\otps --help

以 60 秒间隔预报西经 121.9 度、北纬 36.6 度一天的潮位：

    mbotps --otps-path=C:\otps --tide-position=-121.9/36.6 --start-time=2025/02/21/00/00/00 --end-time=2025/02/21/23/59/59 --tide-output=tide_20250221.txt

沿数据列表中所有文件的导航生成潮位文件，并用 `mbprocess` 应用：

    mbotps --otps-path=C:\otps --input=datalist.mb-1 --use-mbprocess --verbose
    mbprocess -I datalist.mb-1

用附近验潮站的观测值（格式 2 文件）改正模型：

    mbotps --otps-path=C:\otps --input=datalist.mb-1 --tide-station-file=gauge_2025.txt --tide-station-position=-121.89/36.61 --tide-station-format=2 --use-mbprocess

## 注意

- 选项 `--otps-path`、`--input`、`--tide-output` 和 `--tide-station-file` 的值只读取到第一个空格为止，因此请使用不含空格的路径（例如 `C:\otps`）。
- 找不到可用模型时，程序会输出 "Unable to find a valid OTPS tidal model" 并退出；请检查 `DATA` 文件夹结构和 `--otps-path` 的值。
- `.tde` 文件写在数据列表所列条带文件的旁边，而不是当前目录。
- 预报值只包含模型分潮的潮汐信号，不包括当地基准面和非潮汐水位变化（验潮站改正可以减小这类差异）。

## 另请参阅

`mbprocess`, `mbset`, `mbgpstide`, `mbdatalist`, `mbnavlist`。完整参考资料为 MB-System 手册页 mbotps(1)（英文）。
