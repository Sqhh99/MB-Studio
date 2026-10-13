# mbgrid — 由条带文件生成水深、强度或侧扫网格

`mbgrid` 将数据列表中所列条带声呐文件的水深、地形、波束强度或侧扫声呐数据网格化，并写出 GMT 网格（或其他网格格式）。这是由处理后的 MB-System 数据生成数字地形模型的标准方法，通常在编辑和 `mbprocess` 之后运行；生成的网格可用于制图、`gmt mbgrdtiff`、`mbgrd2gltf` 或导出到 GIS。

## 语法

    mbgrid
        --algorithm=mode[/threshold] {-Fmode[/threshold]}
        --background-data=background {-Kbackground}
        --border=border {-Bborder}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --bounds=factor {-Rfactor}
        --clip=clip[/mode] {-Cclip[/mode]}
        --data-type=datatype {-Adatatype}
        --dimensions=xdim/ydim {-Dxdim/ydim}
        --extend=extend {-Xextend}
        --extra-grids {-M}
        --gaussian-scale=scale {-Wscale}
        --help {-H}
        --input=filelist {-Ifilelist}
        --longitude-domain=lonflip {-Llonflip}
        --output=root {-Oroot}
        --output-format=gridkind {-Ggridkind}
        --pings=pings {-Ppings}
        --projection=projection {-Jprojection}
        --shift=shiftx/shifty[/mode] {-Yshiftx/shifty[/mode]}
        --spacing=dx/dy/units[!] {-Edx/dy/units[!]}
        --speed-minimum=speed {-Sspeed}
        --tension=tension {-Ttension}
        --time-difference=time {-Utime}
        --use-feet {-Q}
        --use-nan {-N}
        --verbose {-V}

## 说明

输入是列出条带文件及其格式的数据列表（`-I`）。对于水深，如果条带文件存在紧凑的 `.fbt` 文件，则改为读取该文件。数据列表中格式为 0 的条目按经度、纬度、数值三列的 ASCII 文件读取，因此也可以加入常规测深数据。输出文件名由 `-O` 给出的根生成：网格为 `root.grd`（ASCII 格式为 `root.asc`，旧二进制格式为 `root.grd1`），实际参与网格化的文件列表写入数据列表 `root.mb-1`。

如果未给出范围（`-R`）或网格大小（`-D` 或 `-E`），`mbgrid` 使用覆盖全部数据的范围，网格间距取最大声呐高度的 0.02 倍。该范围由各条带文件的 `.inf` 文件得到；缺失或过期的 `.inf`、`.fbt` 和 `.fnv` 文件会先自动生成（与 `mbdatalist -N` 相同）。给出明确范围时，`.inf` 文件显示在网格内没有数据的文件会被跳过，因此可以高效地使用一个总数据列表。

`-F` 提供八种网格化算法：1 高斯加权平均（默认）、2 中值滤波、3 最小值滤波、4 最大值滤波、5 考虑坡度的波束脚印、6 波束脚印，以及最小或最大加权平均（`-F3/threshold`、`-F4/threshold`，只对与网格单元内最浅或最深测深相差 `threshold` 米以内的测深求平均）。编辑过的水深通常用加权平均；未编辑的水深、强度和侧扫数据用中值滤波。脚印算法（5、6）只适用于水深，可生成小于波束脚印的网格单元。海道测量中常用最小值滤波保留最浅测深。

未被数据填充的单元可通过薄板样条插值（`-C`、`-T`）填充，对于水深还可用背景网格（`-K`）填充。空单元写为 99999.9，使用 `-N` 时写为 NaN（用于 GMT 和 GIS 时推荐）。默认网格为地理坐标（WGS84 经纬度）；`-J` 生成投影坐标系（如 UTM，东坐标和北坐标，单位米）的网格。

## 常用选项

- `-Ifilelist` — 输入文件的数据列表（`--input`）。默认值：`datalist.mb-1`。
- `-Oroot` — 输出文件名的根（`--output`）。默认值：`grid`。
- `-Adatatype` — 要网格化的数据（`--data-type`）：1 水深（向下为正，默认）、2 地形（向上为正）、3 波束强度、4 侧扫。
- `-Edx/dy/units[!]` — 网格间距（`--spacing`）；单位为 `meters`（默认）、`km`、`feet`、`degrees`、`arcmin` 或 `arcsec`。`dy` 为 0 时网格单元为正方形。末尾加 `!` 时通过调整范围来保持精确的网格间距。
- `-Dxdim/ydim` — 用网格节点数代替网格间距（`--dimensions`）。默认值：101/101。
- `-Rwest/east/south/north` — 网格范围（度）；若 `-J` 完整指定了投影，也可用东坐标/北坐标（`--bounds`）。`-Rfactor`（大于 1）按比例扩大自动确定的范围，例如 `-R1.1`。
- `-Fmode[/threshold]` — 网格化算法（`--algorithm`），见上文。默认值：1。
- `-Cclip[/mode]` — 对距数据 `clip` 个单元以内的空单元进行样条插值（`--clip`）。mode 1（默认）只填充两侧都有数据的空隙，mode 2 填充距数据 `clip` 个单元以内的所有单元，mode 3 填满整个网格。默认值：不插值。
- `-Ttension` — 样条插值的张力（`--tension`），0 为最小曲率解。默认值：0。
- `-Wscale` — 高斯权函数宽度相对于网格间距的倍数（`--gaussian-scale`）。默认值：1.0。
- `-N` — 空单元写为 NaN 而不是 99999.9（`--use-nan`）。
- `-Jprojection` — 输出投影网格（`--projection`）：`-JU` 自动选择 UTM 带，`-JUTM32N` / `-JUTM56S` 指定投影带，`-Jepsg32632` 使用 EPSG 代码。
- `-Ggridkind` — 输出网格格式（`--output-format`）。默认值：GMT netCDF 浮点（`nf`）。也可使用其他 GMT 格式编号，如 `bf`、`nd`、`ef`（ESRI ASCII），或完整形式 `=id/scale/offset/nan`；旧的数字编号 1（ASCII）、2（旧 GMT 二进制）、3（netCDF）和 4（Arc/Info ASCII 网格）仍可使用。ESRI ASCII 网格要求 x 和 y 方向间距相等。
- `-M` — 另外写出 `root_num.grd`（每个单元的数据个数）和 `root_sd.grd`（每个单元的标准差）（`--extra-grids`）；使用最小值或最大值滤波时忽略。
- `-Utime` — 不对重叠条带求平均（`--time-difference`）：每个单元中与第一个数据相差超过 `time` 分钟的数据被忽略；取负值时改为保留最后的数据。主要用于强度和侧扫数据。
- `-Xextend` — 将内部工作网格每边扩大网格尺寸的 `extend` 倍，使网格外的数据参与边缘处的插值（`--extend`）。默认值：0。
- `-Kbackground` — 用背景 GMT 网格文件填充剩余的空单元（`--background-data`）。
- `-Bborder` — 将插值网格中无数据的边缘单元设为 `border` 值（`--border`）。
- `-Q` — 水深或地形以英尺而不是米为单位网格化（`--use-feet`）。
- `-Llonflip` — 经度范围：-1、0 或 1（`--longitude-domain`）。
- `-Ppings`、`-Sspeed` — 输入数据的 ping 平均和最低航速（km/h）（`--pings`、`--speed-minimum`）。
- `-Yshiftx/shifty[/mode]` — 将网格范围（mode 1，默认）或全部输入数据（mode 2）向东、向北平移给定的米数（`--shift`）。
- `-V` — 详细输出（`--verbose`）。`-H` 显示用法（`--help`）。

## 示例

将数据列表中的水深按 5 m 间距在全部数据范围内网格化，空单元记为 NaN：

    mbgrid -Idatalist.mb-1 -Osurvey_bathy -E5/5/meters -N -V

生成 2 m 间距的 UTM 地形网格，用样条插值填充小空隙，并将范围扩大 10%：

    mbgrid -Idatalist.mb-1 -Osurvey_utm -A2 -JU -E2/2/meters -R1.1 -C3/1 -N

用最小值滤波对指定区域网格化，保留每个 10 m 单元中的最浅测深：

    mbgrid -Idatalistp.mb-1 -Ominimum_depth -R139.65/139.90/35.20/35.45 -E10/10/meters -F3 -N

用中值滤波对侧扫数据网格化，对重叠测线在每个单元中只使用最先的数据：

    mbgrid -Idatalistp.mb-1 -Osidescan -A4 -F2 -E1/1/meters -U10 -N

## 注意

- 在 Windows 上，通常由 `mbm_grdplot` 宏生成的绘图脚本（`root.grd.cmd`）不会生成；请在 MB-Studio 中查看网格，或用 GMT（例如 `gmt mbgrdtiff`）绘图。
- 除非 `-O` 中包含路径，输出文件写入当前目录。含空格的路径需加引号，例如 `-O"D:\Survey 2025\grids\area1"`。
- 应对处理后的数据进行网格化：使用列出处理后文件的数据列表（含 `$PROCESSED`，例如由 `mbdatalist -Z` 生成），这样编辑和改正才会生效。
- 较大的 clip 值与 clip mode 1 组合时速度非常慢。

## 另请参阅

`mbdatalist`、`mbinfo`、`mbprocess`、`mbmosaic`、`gmt mbgrdtiff`、`gmt mbswath`、`mbgrd2gltf`、`mbmakedatalist`。完整参考见 MB-System 手册页 mbgrid(1)（英文）。
