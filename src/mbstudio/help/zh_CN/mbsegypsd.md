# mbsegypsd — 计算 SEGY 道的功率谱密度并网格化

`mbsegypsd` 计算 SEGY 文件中每一道的功率谱密度（PSD），并以 GMT netCDF 网格
（声谱图）的形式写出，x 轴为道号，y 轴为频率。可用于查看浅地层剖面仪或水听器
数据的频率成分，例如识别噪声源或检查发射信号。

## 语法

    mbsegypsd
        --decimate=decimatex {-Ddecimatex}
        --fft-length=nfft {-Nnfft}
        --help {-H}
        --input=file {-Ifile}
        --log-scale {-L}
        --output=root {-Oroot}
        --shot-scale=shotscale[/frequencyscale] {-Ashotscale[/frequencyscale]}
        --time-sweep=sweep[/delay] {-Tsweep[/delay]}
        --trace-range=mode[/start/end[/schan/echan]] {-Smode[/start/end[/schan/echan]]}
        --verbose {-V}
        --window=mode/start/end {-Wmode/start/end}

## 说明

每道的 PSD 通过集合平均估计：将道分成长度为 `nfft` 个采样（`-N`，默认 1024）
的段，每段乘以汉宁（余弦）窗，计算快速傅里叶变换，再对所有段的频谱取平均。
估计值经过归一化，以补偿加窗造成的信号损失。频率轴从 0 Hz 到奈奎斯特频率
1/(2 * 采样间隔)，共 `nfft`/2 + 1 个频点。

程序写出两个文件：

- `<root>.grd` — PSD 网格，单位为 Intensity/Hz；使用 `-L` 时为 dB/Hz
  （PSD 的 20 * log10）；
- `<root>_psd.txt` — 所有道平均后的 PSD，每行为频率和 PSD 值的文本。

如果未给出道范围（`-S`）或时间扫描长度（`-T`），则使用所有道和采样；范围从
SEGY 文件的 `.sinf` 文件读取，该文件缺失或过时时会用 `mbsegyinfo -O` 自动
生成。

## 常用选项

- `-I<file>` — 输入 SEGY 文件。必需。
- `-O<root>` — `.grd` 和 `_psd.txt` 文件的输出名根。必需。
- `-N<nfft>` — FFT 长度（采样数）。默认：1024。
- `-L` — 网格以 dB/Hz 而非线性单位写出。
- `-S<mode>[/<start>/<end>[/<schan>/<echan>]]` — 要处理的道。模式 `0`：
  `start` 和 `end` 为炮号；模式 `1`：CMP（RP、CDP）号。`schan`/`echan` 用于
  选择多道数据的通道。默认：所有道。
- `-T<sweep>[/<delay>]` — 所用时间范围的长度和起始时间（秒）。
- `-W<mode>/<start>/<end>` — 只使用每道的一部分：模式 `1` 绝对时间，`2` 相对
  海底到达的时间，`3` 相对声呐深度对应时间的时间。
- `-D<decimatex>` — 输出网格的道抽稀。默认：1。
- `-A<shotscale>/<frequencyscale>` — 用炮间距（米）将 x 轴从炮号换算为距离。
  两个值都必须给出。
- `-V` — 详细输出，显示更多进度信息。

## 示例

用 1024 点 FFT 计算 dB/Hz 声谱图（生成 `testpsd.grd` 和 `testpsd_psd.txt`）：

    mbsegypsd -I hydrophone_ch025.segy -N1024 -L -O testpsd -V

使用更长的 FFT 以提高频率分辨率，并只处理炮 100 到 500：

    mbsegypsd -I sbp_0001_env.segy -N4096 -L -S0/100/500 -O sbp_0001_psd

## 注意

- 程序最后会调用绘图宏 `mbm_grdplot` 和 `mbm_xyplot`，它们在 Windows 上不可用；
  这些步骤会显示提示后跳过，输出文件仍会正常写出。请用 `gmt grdimage` 绘制
  网格，用 `gmt plot` 绘制平均谱。
- `-A` 的 `frequencyscale` 值会被记录，但不会改变频率轴。

## 另请参阅

`mbsegygrid`、`mbsegyinfo`、`mbsegylist`、`mbextractsegy`、`gmt`。完整参考请见
MB-System 手册页 mbsegypsd(1)（英文）。
