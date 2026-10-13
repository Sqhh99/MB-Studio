# mbmosaic — 由条带文件生成强度或侧扫镶嵌图

`mbmosaic` 利用数据列表中所列条带文件的波束强度或侧扫后向散射数据生成网格化镶嵌图。可以按视掠射角、视向方位或平台艏向为每个样本设定优先级，例如抑制镜面反射强烈的正下方区域，或生成某一视向的镶嵌图。通常在用 `mbbackangle` 和 `mbprocess` 改正后向散射之后运行。

## 语法

    mbmosaic
        --altitude-default=bathdef {-Zbathdef}
        --border=border {-Bborder}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --bounds=factor {-Rfactor}
        --data-type=datatype {-Adatatype}
        --directional-priority=bearing/factor[/mode] {-Ubearing/factor[/mode]}
        --extend=extend {-Xextend}
        --extra-grids {-M}
        --grid-dimensions=xdim/ydim {-Dxdim/ydim}
        --grid-format=gridkind {-Ggridkind}
        --grid-spacing=dx/dy/units {-Edx/dy/units}
        --help {-H}
        --input=filelist {-Ifilelist}
        --longitude-domain=lonflip {-Llonflip}
        --output=root {-Oroot}
        --pings=pings {-Ppings}
        --priority-range=priority_range[/weight] {-Fpriority_range[/weight]}
        --priority-source=priority_source {-Ypriority_source}
        --projection=projection {-Jprojection}
        --speed-minimum=speed {-Sspeed}
        --spline-interpolation=clip/mode/tension {-Cclip/mode/tension}
        --topography-grid=topogridfile {-Ttopogridfile}
        --use-nan {-N}
        --verbose {-V}
        --weighting-scale=scale {-Wscale}

## 说明

输入数据列表（`-I`）和输出根（`-O`）的用法与 `mbgrid` 相同：镶嵌图写入 `root.grd`（ASCII 和旧二进制格式分别为 `root.asc` / `root.grd1`），参与镶嵌的文件列表写入数据列表 `root.mb-1`。未给出范围或网格大小时，使用覆盖全部数据的范围和最大声呐高度 0.02 倍的网格间距；范围取自 `.inf` 文件，缺失或过期时会自动生成。水深不能做镶嵌，请用 `mbgrid` 网格化。

每个样本的优先级最多是三个因子的乘积：随视掠射角变化的优先级（`-Y`，取自预定义表或文件）、视向方位优先级和平台艏向优先级（`-U`）。默认情况下，每个单元取优先级最高的单个样本的值。使用 `-F` 时，改为对单元内优先级与最高优先级相差不超过 `priority_range` 的所有样本求高斯加权平均；可选的 `weight` 还会再按优先级（1）或优先级的平方（2）对样本加权。

掠射角为由横向距离 x 和声呐高度 z 求得的角度 arctan(x/z)：正下方为 0，左舷为负，右舷为正。优先级文件有两列：角度（度）和优先级（0 至 1），从最左舷角度到最右舷角度依次排列；中间角度的优先级线性插值，表范围以外为零。

空单元可通过薄板样条插值（`-C`）填充。空单元写为 99999.9，使用 `-N` 时写为 NaN。与 `mbgrid` 一样，`-J` 可生成 UTM 等投影坐标系的镶嵌图。

## 常用选项

- `-Ifilelist` — 输入文件的数据列表（`--input`）。默认值：`datalist.mb-1`。
- `-Oroot` — 输出文件名的根（`--output`）。默认值：`grid`。
- `-Adatatype` — 要镶嵌的数据（`--data-type`）：3 波束强度、4 侧扫（默认）、5 平坦海底掠射角、6 横向掠射角、7 横向坡度。加 `F`（例如 `-A4F`）则使用经 `mbfilter` 滤波的数据（`.ffa`/`.ffs` 文件）。
- `-Edx/dy/units` — 网格间距（`--grid-spacing`）；单位为 `meters`（默认）、`km`、`feet`、`degrees`、`arcmin` 或 `arcsec`。末尾加 `!` 时通过调整范围保持精确间距。
- `-Dxdim/ydim` — 用网格节点数代替网格间距（`--grid-dimensions`）。默认值：101/101。
- `-Rwest/east/south/north` — 镶嵌范围（`--bounds`）；`-Rfactor`（大于 1）扩大自动确定的范围。
- `-Ypriority_source` — 掠射角优先级（`--priority-source`）：用 1 至 8 的数字选择预定义表（1-4 优先外侧条带，至 ±60、67、75 或 85 度；5-8 优先正下方），或给出优先级文件名。
- `-Ubearing/factor[/mode]` — 方向优先级（`--directional-priority`）：mode 0（默认）优先视向方位 `bearing`，mode 1 优先平台艏向 `bearing`。优先级为 cos(factor × 差值)，差值超过 90 度 / factor 时为零。
- `-Fpriority_range[/weight]` — 对优先级与最高优先级相差不超过 `priority_range` 的样本求高斯加权平均（`--priority-range`）。默认值：只取最高优先级样本。
- `-Wscale` — 高斯权函数宽度相对于网格间距的倍数（`--weighting-scale`）。默认值：1.0。
- `-Cclip/mode/tension` — 样条插值（`--spline-interpolation`）：填充距数据 `clip` 个单元以内的空单元；mode 1 填充空隙，mode 2 填充数据附近的所有单元，mode 3 填满整个网格；`tension` 为 0 表示最小曲率。默认值：不插值。
- `-N` — 空单元写为 NaN 而不是 99999.9（`--use-nan`）。
- `-Jprojection` — 投影输出（`--projection`），例如 `-JU`、`-JUTM32N`、`-Jepsg32632`。
- `-Ggridkind` — 输出网格格式（`--grid-format`），与 `mbgrid` 相同。默认值：GMT netCDF 浮点。
- `-Ttopogridfile` — 用地形网格代替平坦海底假设来计算每个样本的脚印（`--topography-grid`）。
- `-Zbathdef` — 无水深可用时计算掠射角所用的深度（`--altitude-default`）。默认值：1000。
- `-M` — 另外写出每个单元的样本数和标准差（`root_num.grd`、`root_sd.grd`）；需要 `-F`（`--extra-grids`）。
- `-Xextend` — 将内部工作网格每边扩大网格尺寸的 `extend` 倍（`--extend`）。
- `-Bborder`、`-Llonflip`、`-Ppings`、`-Sspeed` — 边缘值、经度范围、ping 平均和最低航速（km/h），与 `mbgrid` 相同。
- `-V` — 详细输出（`--verbose`）。`-H` 显示用法（`--help`）。

## 示例

生成 1 m 间距的侧扫镶嵌图，优先外侧条带（预定义表 2），并对优先级与最高值相差 0.1 以内的所有样本求平均：

    mbmosaic -Idatalistp.mb-1 -Osidescan_mosaic -A4 -E1/1/meters -Y2 -F0.1 -N -V

使用优先级文件生成 UTM 波束强度镶嵌图，并用插值填充小空隙：

    mbmosaic -Idatalistp.mb-1 -Oamplitude_utm -A3 -JU -E2/2/meters -Yangle_priority.txt -F0.1 -C5/1/0 -N

生成朝东视向的侧扫镶嵌图（优先视向方位 90 度）：

    mbmosaic -Idatalistp.mb-1 -Ossmosaic_east -A4 -E2/2/meters -U90/2 -F0.05 -N

使正下方区域优先级低、45 度优先级最高的优先级文件 `angle_priority.txt` 示例：

    -60.0 0.2
    -45.0 1.0
    -15.0 0.8
    -14.9 0.1
    14.9 0.1
    15.0 0.8
    45.0 1.0
    60.0 0.2

## 注意

- 在 Windows 上，通常由 `mbm_grdplot` 宏生成的绘图脚本（`root.grd.cmd`）不会生成；请在 MB-Studio 中或用 `gmt mbgrdtiff` 查看镶嵌图。
- 应先针对掠射角影响改正后向散射（`mbbackangle`，然后 `mbprocess`），再对处理后的文件做镶嵌。
- 含空格的路径需加引号。

## 另请参阅

`mbgrid`、`mbbackangle`、`mbprocess`、`mbfilter`、`mbdatalist`、`gmt mbgrdtiff`。完整参考见 MB-System 手册页 mbmosaic(1)（英文）。
