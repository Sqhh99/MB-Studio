# mbgrd2gltf — 将水深网格转换为 glTF 三维模型

`mbgrd2gltf` 将 GMT 水深网格（netCDF `.grd`，例如由 `mbgrid` 生成）转换为 glTF 三维模型（文本 `.gltf` 或二进制 `.glb`），供三维查看器、游戏引擎、网页和演示工具使用。可通过选项设置垂直夸大、Draco 网格压缩、用于提高渲染精度的局部坐标原点，以及可选的 HTML 查看页面。

## 语法

    mbgrd2gltf --input FILE [--binary] [--output ROOT] [--exaggeration NUM]
               [--geoorigin[=LON,LAT,ELEV]] [--draco] [--quantize-position NUM]
               [--quantize-normal NUM] [--quantize-texcoord NUM] [--quantize-color NUM]
               [--html] [--verbose] [--help]

    mbgrd2gltf -IFILE [-B] [-OROOT] [-ENUM] [-G[LON,LAT,ELEV]] [-D]
               [-QpNUM] [-QnNUM] [-QtNUM] [-QcNUM] [-W] [-V] [-H]

## 说明

输入为 netCDF 格式的 GMT 网格（可读取 COARDS 经纬度网格和旧版 GMT netCDF 网格），坐标为经度和纬度，数值为高程或水深。网格被转换为三角网格，大网格采用分块处理。顶点位置使用地心地固（ECEF）直角坐标系，单位为米：原点位于地球质心，Z 轴指向北极，X 轴穿过赤道与本初子午线的交点。

由于 ECEF 坐标数值达数百万米，在显卡的单精度运算中可能损失精度。使用 `--geoorigin` 时，顶点改为相对于局部原点的偏移；原点可以用经度、纬度和高程指定，也可以由网格中心和平均高度自动计算。不使用 `--geoorigin` 时，直接使用完整的 ECEF 坐标。

输出文件为 `ROOT.gltf`，使用 `--binary` 时为 `ROOT.glb`。`ROOT` 默认是去掉扩展名的输入文件名，位于输入文件所在文件夹。Draco 压缩（`--draco`）可显著减小文件；量化选项设置位置、法线、纹理坐标和颜色所用的位数（位数越少，文件越小，精度越低）。使用 `--html` 时还会写出 HTML 查看页面 `ROOT.html`，用 X3DOM 显示模型及处理信息。

## 常用选项

- `--input FILE`、`-IFILE` — 输入 GMT 网格文件（必需）。
- `--output ROOT`、`-OROOT` — 输出文件根，自动添加 `.gltf` 或 `.glb`。默认值：输入文件夹中去掉扩展名的输入文件名。
- `--binary`、`-B` — 写出二进制 glTF（`.glb`），更紧凑、加载更快。默认值：文本 glTF。
- `--exaggeration NUM`、`-ENUM` — 垂直夸大系数，须大于 0。默认值：1.0。
- `--geoorigin[=LON,LAT,ELEV]`、`-G[LON,LAT,ELEV]` — 以局部原点为参考写出顶点；不给值时使用网格中心和平均高度。默认值：完整 ECEF 坐标。
- `--draco`、`-D` — 启用 Draco 网格压缩。
- `--quantize-position NUM`、`-QpNUM` — Draco 位置量化位数，2-30。默认值：16。
- `--quantize-normal NUM`、`-QnNUM` — Draco 法线量化位数，2-30。默认值：7。
- `--quantize-texcoord NUM`、`-QtNUM` — Draco 纹理坐标量化位数，2-30。默认值：10。
- `--quantize-color NUM`、`-QcNUM` — Draco 颜色量化位数，2-30。默认值：8。
- `--html`、`-W` — 另外写出 HTML 查看页面 `ROOT.html`。
- `--verbose`、`-V` — 显示读取网格和内存使用的详细信息。
- `--help`、`-H` — 显示帮助信息后退出。

## 示例

以默认设置将网格转换为文本 glTF 模型（生成 `survey_bathy.gltf`）：

    mbgrd2gltf --input survey_bathy.grd

写出紧凑的二进制模型，垂直夸大 5 倍，并以网格中心为局部原点：

    mbgrd2gltf --input survey_bathy.grd --binary --exaggeration 5 --geoorigin

将提高了位置精度的 Draco 压缩二进制模型写到另一个文件夹：

    mbgrd2gltf --input survey_bathy.grd --binary --draco --quantize-position 20 --output "D:\Models\survey area"

明确指定局部原点，并同时生成 HTML 查看页面：

    mbgrd2gltf --input monterey.grd --geoorigin=-122.0,36.8,-1000 --binary --html

## 注意

- 原点数值须用 `=` 给出（`--geoorigin=-122.0,36.8,-1000`，旧格式为 `-G-122.0,36.8,-1000`）；对该选项，写在单独参数中的值不会被读取。
- 使用旧格式语法时，`-D` 必须放在 `-Q` 选项之前。
- 网格应为地理坐标（`mbgrid` 中不要使用 `-J`）；建议将空单元设为 NaN（`mbgrid -N`）。
- 默认输出名取输入名第一个点号之前的部分，因此 `area.v2.grd` 生成 `area.gltf`；如需其他名称请使用 `--output`。
- 浏览器通常拒绝从直接打开的本地 `.html` 文件加载模型；请用本地 Web 服务器（例如 `python -m http.server 8000`）提供该文件夹，并通过 `http://localhost:8000/` 打开页面。

## 另请参阅

`mbgrid`、`mbinfo`、`gmt mbgrdtiff`。完整参考见 MB-System 手册页 mbgrd2gltf(1)（英文）。
