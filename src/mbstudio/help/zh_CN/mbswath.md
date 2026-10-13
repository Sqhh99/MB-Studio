# mbswath — 以彩色图绘制条带水深、强度或侧扫数据

`mbswath` 是 MB-System GMT 补充包中的模块，无需网格化，直接从数据文件将条带声呐数据绘制到 GMT 地图上。每个波束或侧扫像素按其脚印填色，可生成水深彩色填充图或晕渲地形图、波束强度或侧扫彩色图像，或以强度作晕渲的水深图。可用于快速查看原始或处理后测线的成图效果，并在用 `mbgrid` 网格化之前检查覆盖范围和数据质量。

## 语法

    gmt mbswath -I<inputfile> -J<parameters> -R<west>/<east>/<south>/<north> -C<cptfile>
        [-A<factor>/<mode>/<depth>] [-B<tickinfo>]
        [-b<year>/<month>/<day>/<hour>/<minute>/<second>]
        [-e<year>/<month>/<day>/<hour>/<minute>/<second>]
        [-Ei|<dpi>] [-F<format>] [-G<magnitude>/<azimuth | median>]
        [-L<lonflip>] [-N<cptfile>] [-p<pings>] [-S<speed>] [-T<timegap>]
        [-W] [-Z<mode>[F]] [-K] [-O] [-P] [-U] [-V] [-X<x-shift>] [-Y<y-shift>]

## 说明

`mbswath` 读取一个条带数据文件或数据列表中引用的所有文件，将每个 ping 的每个有效波束（或侧扫像素）作为一个小多边形（脚印）绘制，并按 `-C` 给出的色标着色。标记为无效的波束不绘制。结果写为 24 位彩色 PostScript 图像（色标为灰度时为 8 位），若给出 `-B` 则随后绘制图框。读取原始文件还是处理后文件由数据列表控制（例如 `$PROCESSED` 行）。

绘图方式由 `-Z` 选择：水深彩色填充（1）、水深彩色晕渲地形（2）、以强度作晕渲的水深（3）、波束强度彩色填充（4）或侧扫彩色填充（5）。水深值为以米为单位、向下为正的深度（使用 `-W` 时为英尺），因此色标必须以正数覆盖深度范围。对于晕渲地形，`-G` 设置光照的强度和方向。

数据在 `-R` 范围向四周各扩展 25% 的区域内读取，因此跨越地图边缘的条带会完整绘制到边缘。读取文件之前，`mbswath` 会查找其 ".inf" 文件（`mbinfo` 的输出）；如果 ".inf" 文件表明该文件在范围内没有数据，则跳过该文件。仅绘制水深时（`-Z1`、`-Z2`），若存在 ".fbt" 快速水深文件，则读取它而不是原始文件。请用 `mbdatalist -O` 生成这些辅助文件，以加快大型测量的绘图速度。

条带图像覆盖整个地图区域且不透明（无数据处为白色）。因此它必须是地图的第一层，并且一张地图上只能放置一个 `mbswath` 图像；等深线、航迹、海岸线和色标请用 `-K`/`-O` 绘制在其上方。

## 常用选项

- `-I<inputfile>` — 要绘制的条带数据文件或数据列表（必需）。以 ".mb-1" 结尾的文件按数据列表读取。
- `-J<parameters>` — GMT 地图投影和尺寸，例如 `-JM15c`（墨卡托投影，宽 15 cm）。必需。
- `-R<west>/<east>/<south>/<north>` — 地图范围（度）。必需。
- `-C<cptfile>` — 用于为数据着色的 GMT 色标（CPT）文件，例如用 `gmt makecpt` 生成。实际使用中必需。
- `-Z<mode>[F]` — 绘图方式：1 = 水深彩色填充，2 = 水深彩色晕渲地形，3 = 以强度作晕渲的水深，4 = 强度彩色填充，5 = 侧扫彩色填充。末尾加 `F`（例如 `-Z5F`）时绘制经 `mbfilter` 滤波的强度或侧扫数据（".ffa" / ".ffs" 文件）；这些文件不存在时绘图失败。默认值：1。
- `-G<magnitude>/<azimuth>` — 用于 `-Z2` 时：晕渲强度（等效垂直夸大，通常为 1-5）和光照方位角（度）。用于 `-Z3` 时：晕渲强度和产生中性晕渲的强度值（`-G<magnitude>/<median>`）。默认值：1/270。
- `-N<cptfile>` — 用于 `-Z3` 时：将强度转换为晕渲所用的灰度 CPT 文件，代替线性映射。
- `-A<factor>/<mode>/<depth>` — 沿航迹方向的脚印大小。mode 1：由声呐的纵向波束宽度乘以 factor 得到（脚印向条带外侧增大）；mode 2：ping 间距乘以 factor；mode 3：将每个测深点绘制为单个点。数据中没有水深时（纯侧扫）用 depth 计算脚印。factor 小于 1 适用于密集采样的数据，大于 1 可填补空隙。默认值：1/1。
- `-E<dpi>` — 条带图像的分辨率（每英寸点数）。默认值：100。`-Ei` 让 PostScript 设备以其自身分辨率对图像插值。
- `-p<pings>` — ping 平均：1 = 不平均，n = 对 n 个 ping 求平均，0 = 自动（使沿航迹间距与波束间距相匹配）。默认值：1。
- `-F<format>` — 输入的 MBIO 格式编号（`-f` 等效）。默认值：由文件名后缀确定；-1 表示数据列表。
- `-b<year>/<month>/<day>/<hour>/<minute>/<second>`、`-e<year>/<month>/<day>/<hour>/<minute>/<second>` — 只绘制这两个时间之间的 ping。默认值：取自 `mbdefaults`。
- `-S<speed>` — 跳过船速低于该值（km/h）的 ping。默认值：0。
- `-T<timegap>` — ping 之间的时间间隔（分钟）超过该值时条带断开。默认值：1。
- `-L<lonflip>` — 经度范围：-1 = -360 至 0，0 = -180 至 180，1 = 0 至 360。默认值：由 `-R` 确定。
- `-W` — 深度以英尺而不是米为单位（CPT 也必须以英尺为单位）。
- `-B<tickinfo>` — GMT 图框和注记，例如 `-Ba`。
- `-K`、`-O`、`-P`、`-X`、`-Y`、`-U`、`-V` — GMT 经典模式选项：后续还有 PostScript、叠加到已有图上、纵向版面、平移绘图原点、时间戳，以及详细进度信息。

## 示例

生成 0 至 1500 m 深度的色标（浅处红色，深处蓝色），以晕渲地形方式绘制条带，并将 PostScript 转换为 PNG 图像（swath.png）：

    gmt makecpt -Chaxby -T0/1500/50 -Z -I > depth.cpt
    gmt mbswath -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -Cdepth.cpt -Z2 -G2/315 -Ba -P > swath.ps
    gmt psconvert swath.ps -Tg -A

以灰度图像绘制侧扫数据（请根据 `mbinfo` 报告的侧扫值调整 -T 范围）：

    gmt makecpt -Cgray -T0/60/1 -Z > ss.cpt
    gmt mbswath -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -Css.cpt -Z5 -Ba -P > sidescan.ps

分层制作地图：水深彩色填充，上方叠加 `mbcontour` 的等深线，地图下方放置色标：

    gmt mbswath -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -Cdepth.cpt -Z1 -Ba -P -K > map.ps
    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -A50/250/250/250/0.04/0.1 -P -O -K >> map.ps
    gmt psscale -Cdepth.cpt -Dx7.5c/-1.5c+w10c/0.4c+h+jTC -Baf+l"Depth (m)" -O >> map.ps

## 注意

- `mbswath` 不是独立程序，而是 MB-System GMT 补充包中的模块，始终通过 GMT 驱动程序以 `gmt mbswath` 运行。GMT 会自动在安装目录的 `bin\gmt_plugins` 文件夹中找到补充包，无需设置 GMT_CUSTOM_LIBS。不带参数运行 `gmt mbswath` 会显示用法。
- 请像示例那样使用 GMT 经典模式：除最后一条外的每条命令都加 `-K`，除第一条外的每条命令都加 `-O`，第一条命令用 `>` 写出 PostScript，之后用 `>>` 追加。本模块不识别现代模式的单行形式（例如 `-png map`），而且这里的 `-p` 表示 ping 平均，而不是 GMT 的透视选项。
- 选项参数紧跟在选项字母之后，不加空格（`-Idatalist.mb-1`）。含空格的文件名需加引号。
- `gmt psconvert` 需要 Ghostscript 才能将 PostScript 转换为 PNG、JPEG 或 PDF。
- 请用 `mbdatalist -O` 生成 ".inf" 和 ".fbt" 文件，以便快速跳过地图范围以外的文件。

## 另请参阅

`mbcontour`、`mbgrdtiff`、`mbgrid`、`mbdatalist`、`mbinfo`、`mbfilter`、`mbprocess`。完整参考见 MB-System 手册页 mbswath(1)（英文）。
