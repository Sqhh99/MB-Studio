# mbmesh — 由条带水深数据重建三维三角网格（GLB）

`mbmesh` 通过 MB-System 数据列表读取条带水深数据，由被采用的测深点重建连续的三维三角形曲面，并输出为二进制 glTF 文件（`mesh.glb`）。在数据完成清理和处理之后，如果需要真正的三维海底模型（例如陡壁、悬垂地形或构筑物），而不是用 `mbgrid` 生成的 2.5 维网格，可使用本程序。

## 语法

    mbmesh -I datalist [-O outputdir] [-R west/east/south/north] [-L meters]
           [--decimate meters] [--metadata] [output options] [-V] [-H]

## 说明

`mbmesh` 读取数据列表所引用的全部有效（未标记）水深测深点，也可以用 `-R` 限定在一个地理范围内。程序估计典型点间距并自动推导重建参数，利用传感器位置估计有向曲面法线，用 Screened Poisson 方法重建曲面，用 Marching Cubes 方法提取曲面，最后裁剪掉没有数据支撑的网格区域。如果裁剪会删除整个网格，程序将报错退出，而不会写出空曲面。

细节层级 `-L` 是要分辨的最小地形尺寸（米）。未指定时，取估计平均点间距的约 2 倍。该值被限制在 0.01～5 米之间。若请求的细节层级比数据密度所能支持的更精细，会输出警告。若重建网格超出内部内存上限，`mbmesh` 将报错退出；此时请使用更粗的细节层级（更大的 `-L`）或更小的范围（`-R`）。

`mesh.glb` 的顶点坐标位于以第一个被采用 ping 的传感器位置为原点的局部笛卡尔坐标系（东-北-天，单位米；X 向东，Y 向北，Z 向上）。经纬度通过等距圆柱近似换算为局部米制坐标。该坐标系不同于 `mbgrd2gltf` 使用的 ECEF 坐标系（也不同于其 `--geoorigin` 坐标系），因此两者的输出必须先转换到同一坐标系才能对齐。

所有文件都写入输出目录（默认 `mbmesh_output`，必要时自动创建）。默认只写出 `mesh.glb`；点云、诊断用 GLB 文件和 HTML 查看页面仅在指定相应输出选项时写出。`--metadata` 输出输入计数、地理范围和估计点间距后退出，不生成网格，便于快速确定 `-R` 和 `-L`。

## 常用选项

- `-I datalist` — 输入的 MB-System 数据列表（长格式 `--input=datalist`）。必需；也可以把数据列表作为第一个非选项参数给出。
- `-O outputdir` — 输出目录（长格式 `--output=outputdir`）。默认为 `mbmesh_output`。
- `-R west/east/south/north` — 只使用该经纬度范围（度）内的测深点（长格式 `--bounds=west/east/south/north`）。
- `-L meters` — 要重建的最小地形尺寸（长格式 `--lod=meters`、`--level-of-detail=meters`）。默认由点间距自动推导。
- `--decimate meters` — 重建前按给定单元尺寸（米）的体素网格对点云进行抽稀。
- `--metadata` — 输出数据集信息（计数、范围、点间距）后退出（也可用 `--info`）。
- `--xyz` — 同时写出 `pointcloud-local.xyz`（与 `mesh.glb` 相同的局部坐标系）和 `pointcloud-ecef.xyz`（WGS84 ECEF）。只需其中之一时使用 `--local-xyz` 或 `--ecef-xyz`。
- `--oriented-ply` — 同时写出带法线的点云 `oriented-pointcloud-ecef.ply`。
- `--raw-mesh-glb` — 同时写出裁剪前的网格 `raw_mesh.glb`。
- `--diagnostics` — 同时写出 `pointcloud.glb`、`normals.glb` 和 `origins.glb`（单独指定用 `--pointcloud-glb`、`--normal-glb`、`--origin-glb`）；适用于较小的数据集。
- `--html` — 同时写出用于查看 `mesh.glb` 的 X3DOM 页面 `mesh.html`，并尝试启动本地 Web 服务器在浏览器中打开。
- `--all-outputs` — 写出全部可选输出（包括 `--html`）。
- `-V` — 输出进度和诊断信息（长格式 `--verbose`）。
- `-H` — 显示用法后退出（也可用 `-h`、`--help`）。

输出类长选项也可以只用一个短横线（例如 `-xyz`、`-diagnostics`）。

## 示例

由数据列表中全部已处理数据生成网格，写出 `mbmesh_output\mesh.glb`：

    mbmesh -I datalist.mb-1

在生成网格前检查范围、测深点数和点间距：

    mbmesh -I datalist.mb-1 --metadata

以 0.5 米细节层级对一个区域建网，并将结果和两种 XYZ 点云写入文件夹 `wall_mesh`：

    mbmesh -I datalist.mb-1 -O wall_mesh -R-122.5/-121.8/36.5/37.2 -L 0.5 --xyz -V

将高密度数据按 0.25 米体素抽稀，并保留裁剪前的网格用于比较：

    mbmesh -I datalist.mb-1 --decimate 0.25 --raw-mesh-glb -V

## 注意

- 未给出完整路径时，输出目录相对于当前目录。包含空格的路径请加引号，例如 `-O "D:\Survey 2025\mesh"`。
- `mesh.glb` 可以用任何 glTF 查看器或三维软件打开。其坐标是局部米制坐标，而不是经纬度。
- `--html` 自动打开查看器依赖 `python3`；无法启动时程序会给出警告，但 `mesh.html` 仍会写入输出目录。
- 请先清理数据（例如使用 `mbclean` 或 `mbareaclean` 以及 `mbprocess`）。已标记的测深点会被忽略，但残留的异常值会在网格中形成尖刺。
- 大范围或高密度数据的重建需要大量内存和时间；先用 `--metadata` 检查，再使用 `-R`、更大的 `-L` 或 `--decimate`。

## 另请参阅

`mbgrid`, `mbgrd2gltf`, `mbdatalist`, `mbprocess`, `mbinfo`。完整参考资料为 MB-System 手册页 mbmesh(1)（英文）。
