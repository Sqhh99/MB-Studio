# mbswath2las — 将条带水深测深点导出为 LAS 1.4 点云

`mbswath2las` 读取 MB-System 支持的任意格式的条带水深数据，并将每个有效测深点
作为一个点写入 LAS 1.4 文件。可用它将多波束测深点导入无法读取 MB-System 格式
的点云查看器、GIS 软件和处理工具（例如 CloudCompare、QGIS 或 PDAL），通常在
用 `mbprocess` 完成数据编辑和处理之后使用。

## 语法

    mbswath2las [-A -Bbegintime -Eendtime -Fformat -Iinputfile -Jprojection -Llonflip -Ooutputfile -P -Rwest/east/south/north -Sspeedmin -Ttimegap -V -H]

长选项：`--use-amplitude`（`-A`）、`--begin-time`（`-B`）、`--end-time`
（`-E`）、`--format`（`-F`）、`--input`（`-I`）、`--projection`（`-J`）、
`--lonflip`（`-L`）、`--output`（`-O`）、`--write-prj`（`-P`）、`--bounds`
（`-R`）、`--speed-minimum`（`-S`）、`--time-gap`（`-T`）、`--verbose`
（`-V`）、`--help`（`-H`）。

## 说明

每个测深点的位置由导航、艏向以及波束的横向和纵向距离计算，方法与 `mblist`
相同。被波束标记为坏的波束会被跳过。由于 LAS 高程向上为正，LAS 高程（Z）取
水深的负值。默认情况下位置为地理经纬度（WGS84）；使用 `-J` 时为所选投影坐标系
中以米为单位的东坐标和北坐标。坐标系以 OGC WKT 记录存储在 LAS 文件中，`-P`
还会在每个 LAS 文件旁写出 Esri 格式的 `.prj` 文件。

输入为单个条带文件或数据列表。不使用 `-O` 时，每个输入文件生成一个单独的 LAS
文件，文件名为将输入的 MB-System 格式扩展名替换为 `.las`（例如
`0001_20250101_000000.mb59` 生成 `0001_20250101_000000.las`；没有可识别扩展名的
文件名则直接追加 `.las`）。使用 `-O` 时，所有测深点写入同一个 LAS 文件。

为节省时间，若输入文件存在仅含水深的 `.fbt` 文件，程序会读取该文件。`.fbt`
文件不含振幅，因此此时 LAS 强度（intensity）字段为 0。使用 `-A` 时会读取原始
条带文件，并将每个波束的振幅从整个输入的振幅范围线性映射到 LAS 范围 0-65535
（相对值，并非经过校准的反射强度）。使用 `-J` 或 `-A` 时，会先生成缺失或过时的
`.inf`、`.fbt` 和 `.fnv` 文件，测区范围和振幅范围取自 `.inf` 文件。

LAS 文件使用点数据记录格式 1。X/Y 分辨率为 1e-7 度（地理坐标）或 0.01 米
（投影坐标）；Z 分辨率为 0.001 米。GPS 时间字段保存的是每个测深点的 Unix
时间，而非真正的 GPS 时间。

## 常用选项

- `-I<file>` — 输入条带文件或数据列表。默认：`datalist.mb-1`。
- `-F<format>` — 输入格式编号；负值表示数据列表。默认：根据文件扩展名推断。
- `-O<file>` — 将所有测深点写入此单个 LAS 文件。
- `-J<projection>` — 输出投影坐标：`U` 或 `UTM` 表示测区中心所在的 UTM 带，
  `UTM31N` 形式的带名，`L` 或 `LTM` 表示以测区为中心的局部横轴墨卡托投影，
  `LTM<lon0>/<lat0>` 表示指定原点，或 EPSG 代码或 PROJ 定义。默认：地理坐标
  WGS84。
- `-P` — 同时写出 `.prj` 文件（文件名为将 LAS 文件名中的 `.las` 替换为
  `.prj`）。
- `-A` — 用波束振幅填充 LAS 强度字段（读取原始文件而不是 `.fbt` 文件）。
- `-R<west>/<east>/<south>/<north>` — 只导出此范围内的数据。
- `-B<yr/mo/da/hr/mn/sc>` / `-E<yr/mo/da/hr/mn/sc>` — 只导出此时间段内的数据。
- `-S<speed>` — 跳过船速低于此值（km/h）的脉冲。默认：0。
- `-T<timegap>` — 报告为数据间断的时间间隔（分钟）。默认：1。
- `-L<lonflip>` — 经度范围：`-1` = -360 到 0，`0` = -180 到 180，`1` = 0 到
  360。默认：由 `mbdefaults` 设置。
- `-V` — 详细输出：显示实际使用的坐标系、每个输出文件名以及写出的点数。

## 示例

将一个条带文件转换为 `0002_20180911_020351_EX1811_MB.las`：

    mbswath2las -I 0002_20180911_020351_EX1811_MB.mb88 -F88

将数据列表中的每个文件分别转换为 LAS 文件并报告进度：

    mbswath2las -I datalist.mb-1 -V

将数据列表的所有测深点合并为一个 UTM 坐标的 LAS 文件，并为 ArcGIS 生成
`.prj` 文件：

    mbswath2las -I datalist.mb-1 -JUTM -P -O survey.las

使用局部横轴墨卡托投影，并用振幅填充强度：

    mbswath2las -I datalist.mb-1 -JLTM -A -O survey.las

## 注意

- 不使用 `-O` 时，每个 LAS 文件写在其输入文件所在位置。路径含空格时请加引号。
- 请先运行 `mbprocess`，并确保数据列表选用处理后的文件，这样导出的测深点才会
  包含您的编辑和改正。

## 另请参阅

`mblist`、`mbdatalist`、`mbinfo`、`mbgrid`、`mbcopy`、`mbprocess`。完整参考
请见 MB-System 手册页 mbswath2las(1)（英文）。
