# gmt — 带有 MB-System 绘图模块的 GMT

`gmt` 是 Generic Mapping Tools（GMT 6）的驱动程序，与 MB-System 的 GMT 扩展模块一起随 MB-Studio 安装。除所有标准 GMT 模块（`grdimage`、`coast`、`basemap`、`grdcontour` 等）外，它还可以运行直接绘制条带数据和网格的 MB-System 模块 `mbswath`、`mbcontour` 和 `mbgrdtiff`。可用于绘制由 `mbgrid` 和 `mbmosaic` 生成的网格以及条带数据本身的地图。

## 语法

    gmt MODULE [module options]
    gmt mbswath -I datalist -J... -R... [options]
    gmt mbcontour -I datalist -J... -R... [options]
    gmt mbgrdtiff -I grid -O geotiff [options]
    gmt --version | --help

## 说明

每个 GMT 程序都是 `gmt` 驱动程序的一个模块：第一个参数指定模块名称，其后是该模块的选项。不带选项运行 `gmt MODULE` 会显示该模块的用法，例如 `gmt mbswath`。

GMT 6 有两种模式。在现代模式下，一幅图是一个会话（`gmt begin map png`、绘图命令、`gmt end show`）；单行形式则在一条命令后附加图形格式和文件名（`-png map`）。在经典模式下，每个模块将 PostScript 写到标准输出，并重定向到文件（`> map.ps`），使用 `-K` 和 `-O` 叠加多个图层。两种模式都可以在 Windows 命令提示符中使用。

MB-System 模块从安装目录的 `bin\gmt_plugins` 文件夹中加载，无需额外配置。GMT 从 `share\gmt` 读取其自身的数据（色标、字体，以及已安装的海岸线数据）。

## 常用选项

- `MODULE` — 要运行的 GMT 或 MB-System 模块名称，例如 `grdimage` 或 `mbswath`。
- `--version` — 显示 GMT 版本号后退出。
- `--help` — 显示 GMT 模块列表后退出。
- `--show-modules` — 列出可用的模块，包括 MB-System 模块。
- `--show-plugindir` — 显示搜索扩展模块的文件夹。
- `--show-sharedir` — 显示 GMT 数据文件所在的文件夹。

## 示例

将数据列表中条带水深的晕渲图输出为 swath.png（现代模式单行形式）：

    gmt mbswath -I datalist.mb-1 -JM15c -R-122.1/-121.9/36.7/36.9 -Z2 -png swath

绘制由 mbgrid 生成的网格的彩色图像：

    gmt grdimage bathymetry.grd -JM15c -Cgeo -B -png bathymetry

列出 GMT 识别的 MB-System 模块：

    gmt --show-modules

## 注意

- 包含空格的文件名需加引号，例如 `-I "D:\Survey 2025\datalist.mb-1"`。
- 经典模式的 PostScript 必须重定向到文件（`> map.ps`）；在 MB-Studio 的“程序”页面中无法使用重定向，请改用现代模式的单行形式（`-png name`）。
- GMT 的完整文档位于 docs.generic-mapping-tools.org。

## 另请参阅

`mbswath`、`mbcontour`、`mbgrdtiff`、`mbgrid`、`mbmosaic`。
