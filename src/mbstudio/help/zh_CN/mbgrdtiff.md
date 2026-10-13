# mbgrdtiff — 由 GMT 网格生成带地理参考的 GeoTIFF 图像

`mbgrdtiff` 是 MB-System GMT 补充包中的模块，可将 GMT 网格（例如由 `mbgrid` 生成的水深网格或由 `mbmosaic` 生成的侧扫镶嵌图）转换为 GeoTIFF 图像。网格值用 GMT 色标（CPT）文件着色，并可用光照强度网格进行晕渲。可用于将测量成果以图像形式交付，使 GIS 和海图制作软件能够将其加载到正确位置。

## 语法

    gmt mbgrdtiff -I<grdfile> -C<cptfile> -J<parameters> -O<tiff_file>
        [-I<intensgrid>|<value>] [-M] [-N<nudge_x>/<nudge_y>]
        [-R<west>/<east>/<south>/<north>] [-V]
    gmt mbgrdtiff <grd_r> <grd_g> <grd_b> -J<parameters> -O<tiff_file> [options]

## 说明

第一个 `-I` 选项指定要转换的网格。每个网格单元成为图像中的一个像素，其颜色取自 `-C` 给出的 CPT 文件。无数据的单元（NaN）使用 CPT 中的 NaN 颜色。如果 CPT 中所有颜色都是灰色（红 = 绿 = 蓝），TIFF 为 8 位灰度图像，否则为 24 位彩色图像。也可以不用一个网格加 CPT，而直接以文件名给出分别存放红、绿、蓝分量（0-255）的三个网格。

第二个 `-I` 选项用于添加晕渲，可以是光照强度网格（例如用 `gmt grdgradient` 生成、与数据网格尺寸相同的网格），也可以是一个常数强度值。

地理参考信息以 GeoTIFF 标签写入 TIFF。`mbgrdtiff` 能识别 `mbgrid` 和 `mbmosaic` 记录在网格中的坐标系：UTM 投影带和 EPSG 投影坐标系写为相应的投影坐标系；其他所有网格都按地理坐标（经纬度，WGS84）处理。此外总会在 TIFF 旁边写出一个同名、后缀为 ".tfw" 的世界文件，供不读取 GeoTIFF 标签的软件使用。

`-R` 将图像限制为网格的一部分。`-N` 将图像的地理参考位置平移给定的米数，例如用于改正已知的定位偏移。

## 常用选项

- `-I<grdfile>` — 要转换的网格（第一个 `-I`）。除非给出 r/g/b 三个网格，否则必需。
- `-I<intensgrid>|<value>` — 第二个 `-I`：用于晕渲的光照强度网格，或常数强度值。
- `-C<cptfile>` — 将网格值转换为颜色的 GMT 色标文件。除非给出 r/g/b 三个网格，否则必需。
- `-O<tiff_file>` — 输出 GeoTIFF 文件名（必需）。请使用以 ".tif" 结尾的文件名：世界文件名由将最后四个字符替换为 ".tfw" 得到。
- `-J<parameters>` — GMT 要求给出投影；请使用线性投影，例如 `-JX15c`。尺寸无关紧要：图像始终为每个网格单元一个像素。
- `-M` — 将颜色转换为灰度级，生成单色（8 位灰度）图像。
- `-N<nudge_x>/<nudge_y>` — 将图像位置向东平移 nudge_x 米，向北平移 nudge_y 米。
- `-R<west>/<east>/<south>/<north>` — 只使用网格的这一部分。默认值：整个网格。
- `-V` — 显示详细进度信息。

## 示例

根据 `mbgrid` 生成的水深网格的数值范围生成色标（深处为蓝色），并将网格转换为 GeoTIFF（同时写出 survey.tfw）：

    gmt grd2cpt survey.grd -Chaxby -Z -I > survey.cpt
    gmt mbgrdtiff -Isurvey.grd -Csurvey.cpt -JX15c -Osurvey.tif

使用西北方向光照计算的强度网格生成晕渲地形图像：

    gmt grdgradient survey.grd -A315 -Ne0.6 -Gsurvey_int.grd
    gmt mbgrdtiff -Isurvey.grd -Isurvey_int.grd -Csurvey.cpt -JX15c -Osurvey_shaded.tif

将 `mbmosaic` 生成的侧扫镶嵌图转换为 8 位灰度 GeoTIFF：

    gmt makecpt -Cgray -T0/60/1 -Z > ss.cpt
    gmt mbgrdtiff -Imosaic.grd -Css.cpt -JX15c -Omosaic.tif

将图像向东平移 2.5 m、向南平移 1 m：

    gmt mbgrdtiff -Isurvey.grd -Csurvey.cpt -JX15c -N2.5/-1 -Osurvey_shifted.tif

## 注意

- `mbgrdtiff` 不是独立程序，而是 MB-System GMT 补充包中的模块，始终通过 GMT 驱动程序以 `gmt mbgrdtiff` 运行。GMT 会自动在安装目录的 `bin\gmt_plugins` 文件夹中找到补充包，无需设置 GMT_CUSTOM_LIBS。
- 与 GMT 绘图模块不同，`mbgrdtiff` 不输出 PostScript；这里的 `-O` 是输出文件名，而不是 GMT 的叠加选项。
- 不要对 `mbgrdtiff` 使用 `-JM` 等非线性投影：网格会为制图而重采样，而 GeoTIFF 中记录的仍是原始坐标。
- `mbgrid` 网格默认包含水深（向下为正），使用 `-A2` 时包含地形（向上为正）。`gmt grd2cpt` 会使色标适应网格的数值范围；对于地形网格，请去掉第一个示例中的 `-I`。
- 选项参数紧跟在选项字母之后，不加空格。含空格的文件名需加引号。

## 另请参阅

`mbgrid`、`mbmosaic`、`mbswath`、`mbcontour`。完整参考见 MB-System 手册页 mbgrdtiff(1)（英文）。
