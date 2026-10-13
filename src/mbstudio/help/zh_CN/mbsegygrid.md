# mbsegygrid — 将 SEGY 道数据网格化为地震剖面

`mbsegygrid` 将 SEGY 文件中的道转换为表示地震或浅地层剖面的 GMT netCDF 网格：
x 轴为道号（或距离），y 轴为双程旅行时（或深度）。可在 `mbextractsegy` 之后
使用，生成浅地层测线的剖面图像，再用 `gmt` 绘图。

## 语法

    mbsegygrid -Ifile -Oroot
        --agc=agcmaxvalue/agcwindow {-Bagcmaxvalue/agcwindow}
        --decimation=decimatex/decimatey {-Ddecimatex/decimatey}
        --distance-plot=distancebin/startlon/endlon/startlat/endlat {-Rdistancebin/startlon/endlon/startlat/endlat}
        --filter=filtermode/filterwindow {-Ffiltermode/filterwindow}
        --gain=gainmode/gain[/gainwindow[/gaindelay]] {-Ggainmode/gain[/gainwindow[/gaindelay]]}
        --geometry=geometrymode {-Cgeometrymode}
        --help {-H}
        --input=file {-Ifile}
        --output=fileroot {-Ofileroot}
        --scale-to-distance=shotscale/timescale {-Ashotscale/timescale}
        --time-sweep=timesweep[/timedelay] {-Ttimesweep[/timedelay]}
        --trace-mode=tracemode[/tracestart/traceend[/chanstart/chanend]] {-Stracemode[/tracestart/traceend[/chanstart/chanend]]}
        --verbose {-V}
        --window=windowmode/windowstart/windowend {-Wwindowmode/windowstart/windowend}

## 说明

输出网格写入 `<root>.grd`。网格每列对应一道（经 `-D` 抽稀后），每行对应一个
时间采样。旅行时以负值存储，因此较晚的到达位于剖面下方。无数据的网格单元设为
NaN。

如果未给出道范围（`-S`）或时间扫描长度（`-T`），程序会从 SEGY 文件的 `.sinf`
文件中读取，从而将所有道和所有采样网格化。`.sinf` 文件缺失或过时时，会自动
运行 `mbsegyinfo -O` 生成。

网格化之前可以对道进行处理：时变增益（`-G`）、余弦低通滤波（`-F`）、自动增益
控制（`-B`），以及只保留感兴趣采样的时间窗（`-W`）。使用 `-R` 时，道被投影到
两点之间的直线上，按沿线距离而不是道号分箱。使用 `-A` 时，坐标轴换算为以米
为单位的距离和深度。

## 常用选项

- `-I<file>` — 输入 SEGY 文件。必需。
- `-O<root>` — 输出名根；网格写入 `<root>.grd`。必需。
- `-S<mode>[/<start>/<end>[/<schan>/<echan>]]` — 要网格化的道。模式 `0`：
  `start` 和 `end` 为炮号；模式 `1`：CMP（RP、CDP）号；模式 `2`：炮号，合并
  所有通道。`schan`/`echan` 用于选择多道数据的通道。默认：所有道。
- `-T<sweep>[/<delay>]` — 要网格化的时间范围长度（秒）及其起始时间。默认：
  整个道长。
- `-D<decimatex>/<decimatey>` — 在每个网格单元内取平均，以减少道数和采样数。
  默认：`1/1`。
- `-G<mode>/<gain>[/<window>[/<delay>]]` — 增益：模式 `0` 关闭，`1` 从道起始
  开始，`2` 从海底到达开始。每个采样乘以 (1 + gain * t)；`window` 限制施加
  增益的区间（之后的采样置零），`delay` 推迟增益起点（秒）。模式 `3` 对每道
  进行缩放，使海底附近 `window` 内的最大值等于 `gain`。
- `-B<maxvalue>[/<window>]` — 自动增益控制：缩放每个采样，使 `window` 秒滑动
  窗口内的最大值等于 `maxvalue`。不给 `window` 时对整道归一化。
- `-F<mode>/<window>` — 低通滤波；模式 `1` 为长度 `window` 秒的余弦锥形滤波。
- `-W<mode>/<start>/<end>` — 只网格化每道的一部分：模式 `1` 绝对时间，`2` 相对
  海底到达的时间（start 常为负值），`3` 相对声呐深度对应时间的时间。
- `-R<distancebin>/<startlon>/<endlon>/<startlat>/<endlat>` — 沿两点之间的直线
  按距离网格化，分箱大小为 `distancebin` 米。
- `-A<shotscale>/<timescale>` — 将 x 从炮换算为米（炮间距，米），将 y 从秒
  换算为米（水中通常为 750 m/s）。
- `-C<mode>` — 几何：`0` 垂直道（默认），`1` 根据记录的纵摇校正采样位置。
- `-V` — 详细输出，显示更多进度信息。

## 示例

将 SEGY 文件的所有道和采样网格化为 `sbp_0001.grd`：

    mbsegygrid -I sbp_0001_env.segy -O sbp_0001

网格化炮 1500 到 2000、从激发后 0.25 秒开始的 0.1 秒数据：

    mbsegygrid -I sbp_0001_env.segy -O sbp_0001_part -S0/1500/2000 -T0.1/0.25

从海底开始施加增益，只保留海底以上 0.02 秒到海底以下 0.15 秒的数据：

    mbsegygrid -I sbp_0001_env.segy -O sbp_0001_gain -G2/5.0 -W2/-0.02/0.15

以 0.5 米炮间距和 750 m/s 将剖面换算为米：

    mbsegygrid -I sbp_0001_env.segy -O sbp_0001_m -A0.5/750

## 注意

- 程序最后会调用绘图宏 `mbm_grdplot`，该宏在 Windows 上不可用；此步骤会显示
  提示后跳过，网格仍会正常写出。请用 `gmt grdimage` 绘制网格。
- 除非 `-O` 包含路径，否则输出文件写入当前目录。路径含空格时请加引号。

## 另请参阅

`mbextractsegy`、`mbsegyinfo`、`mbsegylist`、`mbsegypsd`、`gmt`。完整参考请见
MB-System 手册页 mbsegygrid(1)（英文）。
