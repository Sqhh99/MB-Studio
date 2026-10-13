# mbcontour — 由条带数据绘制等深线和航迹

`mbcontour` 是 MB-System GMT 补充包中的模块，直接从数据文件逐 ping 对条带水深绘制等深线，并绘制带有时间、日期、ping 编号和文件名注记的航迹。可用于在地图上添加等深线或带注记的航迹线（常叠加在 `mbswath` 图像上），或快速制作测量航迹图。

## 语法

    gmt mbcontour -I<inputfile> -J<parameters> -R<west>/<east>/<south>/<north>
        [-A<cont_int>/<col_int>/<tick_int>/<label_int>/<tick_len>/<label_hgt>/<label_spacing>]
        [-B<tickinfo>] [-b<year>/<month>/<day>/<hour>/<minute>/<second>]
        [-C<contourfile>] [-D<time_tick>/<time_annot>/<date_annot>/<time_tick_len>]
        [-e<year>/<month>/<day>/<hour>/<minute>/<second>]
        [-F<format>] [-G<name_hgt>[/<name_perp>]] [-L<lonflip>]
        [-M<pingnumber_tick>/<pingnumber_annot>/<pingnumber_tick_len>]
        [-N<nplot>] [-p<pings>] [-Q] [-S<speed>] [-T<timegap>] [-Z<algorithm>]
        [-K] [-O] [-P] [-U] [-V] [-X<x-shift>] [-Y<y-shift>]

## 说明

`mbcontour` 读取一个条带数据文件或数据列表中引用的所有文件，在读取过程中按 ping 组对水深绘制等深线。深度以米为单位，向下为正。标记为无效的波束不参与计算。

未使用 `-C` 时，等深线每隔 cont_int 米绘制一条（`-A`），使用四种颜色（黑、红、绿、蓝），每隔 col_int 米换一种颜色；tick_int 整数倍的等深线带有指向下坡的刻度，label_int 整数倍的等深线带有标注。使用 `-C` 时，由等深线文件明确给出各等深线值及其样式和颜色，此时忽略 `-A`。

有两种等深线算法可选（`-Z`）。默认的 ping 间算法速度快，但在一个 ping 的波束位于前一个 ping 的波束之后的地方（急转弯、纵摇导向）等深线效果较差。三角网算法由测深点构建三角网并在其上追踪等深线，效果更好，但运行时间随测深点数的平方增长，因此每次处理的 ping 数较少（`-N`）。

航迹通过 `-D`（时间刻度和注记）、`-M`（ping 编号刻度）和 `-G`（每个文件起点处的文件名）绘制。如果 `-A`、`-C`、`-D`、`-M` 和 `-Q` 都未给出，则同时绘制等深线（使用默认间隔）和航迹。只给出 `-A` 或 `-C` 时不绘制航迹。

数据在 `-R` 范围向四周各扩展 25% 的区域内读取。读取文件之前，`mbcontour` 会检查其 ".inf" 文件（`mbinfo` 的输出），如果范围内没有数据则跳过该文件。

## 常用选项

- `-I<inputfile>` — 要绘制的条带数据文件或数据列表（必需）。以 ".mb-1" 结尾的文件按数据列表读取。
- `-J<parameters>` — GMT 地图投影和尺寸，例如 `-JM15c`。必需。
- `-R<west>/<east>/<south>/<north>` — 地图范围（度）。必需。
- `-A<cont_int>/<col_int>/<tick_int>/<label_int>/<tick_len>/<label_hgt>/<label_spacing>` — 等深线间距、换色间距、刻度间距和标注间距（米）；刻度长度和标注高度（英寸）；标注最小间距（英寸，0 = 由标注高度推算）。末尾省略的值保持默认。默认值：25/100/100/100/0.05/0.1/0。
- `-C<contourfile>` — 等深线值的文本文件，每行一条：`level label tick red green blue`，其中 label 为 `a`（标注）或 `n`，tick 为 `t`（加刻度）或 `n`，red、green、blue 取 0-255。没有有效 label 和 tick 的行以黑色绘制，不带标注和刻度。如果给出 GMT CPT 文件，其颜色分界值将绘制为普通黑色等深线。
- `-D<time_tick>/<time_annot>/<date_annot>/<time_tick_len>` — 绘制航迹，每 time_tick 小时加时间标记，每 time_annot 小时加时间注记（hh:mm），每 date_annot 小时加日期注记（hh:mm/年积日）；time_tick_len 为标记大小（英寸）。默认值：0.25/1/4/0.1。
- `-G<name_hgt>[/<name_perp>]` — 在每个文件的起点注记文件名，字高 name_hgt 英寸；name_perp = 1 时文件名垂直于航迹书写。同时绘制航迹。默认字高：0.1。
- `-M<pingnumber_tick>/<pingnumber_annot>/<pingnumber_tick_len>` — 每 pingnumber_tick 个 ping 在航迹上加刻度，每 pingnumber_annot 个 ping 注记 ping 编号；刻度长度单位为英寸。默认值：50/100/0.1。
- `-Z<algorithm>` — 等深线算法：0 = ping 间（默认），1 = 三角网。
- `-N<nplot>` — 每次绘制等深线前读取的 ping 数。默认值：50，使用 `-Z1` 时为 5。
- `-Q` — 同时绘制三角网算法（`-Z1`）的三角形，以显示数据分布。
- `-p<pings>` — ping 平均：1 = 不平均，n = 对 n 个 ping 求平均，0 = 自动。默认值：取自 `mbdefaults`。
- `-F<format>` — 输入的 MBIO 格式编号（`-f` 等效）。默认值：由文件名后缀确定；-1 表示数据列表。
- `-b<year>/<month>/<day>/<hour>/<minute>/<second>`、`-e<year>/<month>/<day>/<hour>/<minute>/<second>` — 只使用这两个时间之间的 ping。默认值：取自 `mbdefaults`。
- `-S<speed>` — 跳过船速低于该值（km/h）的 ping。默认值：取自 `mbdefaults`。
- `-T<timegap>` — ping 之间的时间间隔（分钟）超过该值时视为数据中断。默认值：取自 `mbdefaults`。
- `-L<lonflip>` — 经度范围：-1 = -360 至 0，0 = -180 至 180，1 = 0 至 360。默认值：由 `-R` 确定。
- `-B<tickinfo>` — GMT 图框和注记，例如 `-Ba`。
- `-K`、`-O`、`-P`、`-X`、`-Y`、`-U`、`-V` — GMT 经典模式选项：后续还有 PostScript、叠加到已有图上、纵向版面、平移绘图原点、时间戳，以及详细进度信息。

## 示例

使用默认设置绘制等深线和航迹：

    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -Ba -P > contours.ps

使用三角网算法绘制 10 m 间距的等深线，每 50 m 换色并加刻度和标注：

    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -A10/50/50/50/0.03/0.08 -Z1 -Ba -P > contours.ps

只绘制航迹，每 15 分钟一个时间标记，每小时一个时间注记，每 4 小时一个日期注记，文件名垂直于航迹：

    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -D0.25/1/4/0.1 -G0.08/1 -Ba -P > track.ps

按 levels.txt 中给定的深度值绘制等深线，然后将图转换为 PNG 图像：

    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -Clevels.txt -Ba -P > contours.ps
    gmt psconvert contours.ps -Tg -A

其中 levels.txt 的内容例如：

    100 a t 0 0 0
    150 n t 0 0 255
    200 a t 0 0 0

## 注意

- `mbcontour` 不是独立程序，而是 MB-System GMT 补充包中的模块，始终通过 GMT 驱动程序以 `gmt mbcontour` 运行。GMT 会自动在安装目录的 `bin\gmt_plugins` 文件夹中找到补充包，无需设置 GMT_CUSTOM_LIBS。
- 请像示例那样使用 GMT 经典模式（`-K`/`-O`，并用 `>` 和 `>>` 重定向输出）。要在 `mbswath` 图像上叠加等深线，先运行 `gmt mbswath ... -K > map.ps`，再用相同的 `-J` 和 `-R` 运行 `gmt mbcontour ... -O -K >> map.ps`。本模块不识别现代模式的单行形式（例如 `-png map`），而且这里的 `-p` 表示 ping 平均。
- 标注、刻度和文件名的尺寸以英寸给出，与 GMT 的长度单位设置无关。
- 手册页中的 `-W` 选项（深度以英尺为单位）在本版本中不可用；`-W` 会被接受但不起作用。
- 选项参数紧跟在选项字母之后，不加空格。含空格的文件名需加引号。

## 另请参阅

`mbswath`、`mbgrdtiff`、`mbgrid`、`mbdatalist`、`mbinfo`、`mbnavlist`。完整参考见 MB-System 手册页 mbcontour(1)（英文）。
