# mbbackangle — 生成振幅/侧扫声呐的掠射角改正表

`mbbackangle` 计算波束振幅和侧扫声呐数值随海底掠射角的变化，并将这些函数写成改正表。`mbprocess` 随后利用这些表消除反向散射强烈的角度依赖性，使 `mbmosaic` 生成的镶嵌图反映海底反射特性而不是条带几何形状。

## 语法

    mbbackangle
        --altitude=altitude {-Zaltitude}
        --angle-bins=nangles/angle_max {-Nnangles/angle_max}
        --beam-pattern-mode=mode[/beamwidth/depression] {-Bmode[/beamwidth/depression]}
        --data-kind=kind {-Akind}
        --dump-output {-D}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --output-grid=gridmode/angle/min/max/n_columns/n_rows {-Ggridmode/angle/min/max/n_columns/n_rows}
        --pings-average=pings {-Ppings}
        --reference-angle=refangle {-Rrefangle}
        --skip-existing {-S}
        --slope-correction {-Q}
        --symmetric-correction {-C}
        --topography-grid=topogridfile {-Ttopogridfile}
        --verbose {-V}

## 说明

程序读取包含波束振幅和/或侧扫声呐数据的条带文件（单个文件或数据列表中的所有文件），按掠射角分箱并求平均。从 `-angle_max` 到 `+angle_max` 的条带被分为 `nangles` 个角度箱；在天底区域以外（偏离垂直方向超过 15 度），每个值取相邻 3 个箱的平均。每张表是若干个 ping（默认 50 个）的平均值，并以平均 ping 时间作为时间标签，因此一个文件通常会有许多张表。

默认情况下，掠射角根据每个波束或像素的横向距离和水深计算，并假设海底平坦。使用 `-Q` 时考虑由条带水深求得的横向坡度；使用 `-T` 时改为从地形网格求坡度。没有水深的地方，假设海底平坦且声呐高度为 `-Z` 指定的值；不指定 `-Z` 时这些数据不被使用。

改正表写在每个条带文件旁，名称为条带文件名加 `.aga`（振幅）或 `.sga`（侧扫声呐）。程序会创建或更新该条带文件的 `mbprocess` 参数文件，启用使用这些表的振幅和/或侧扫改正（`AMPCORRFILE`、`SSCORRFILE` 及相关参数）。运行 `mbprocess` 时，改正表会按时间和掠射角插值。对于 dB 单位的数据，先减去改正值，再加回参考角（`-R`）处的表值；对于线性数据，先除以改正值，再乘以参考值。

使用 `-G` 时，程序还会为每个条带文件写出一个 GMT 网格，内容为数值相对掠射角的归一化直方图（名称为条带文件名加 `_aga.grd` 或 `_sga.grd`），便于检查改正表。

## 常用选项

- `-I<file>` / `--input=<file>` — 条带文件或数据列表。默认值：`datalist.mb-1`。
- `-A<kind>` / `--data-kind=<kind>` — 处理波束振幅（`1`）或侧扫声呐（`2`）。要同时处理两者，请指定两次。默认值：两者都处理。
- `-N<nangles>/<angle_max>` / `--angle-bins=...` — 角度箱数和最大角度（度）；使用奇数，使中间的箱以 0 度为中心。默认值：`81/80.0`（2 度间隔）。
- `-P<pings>` / `--pings-average=<pings>` — 每张表平均的 ping 数。默认值：50。
- `-Q` / `--slope-correction` — 计算掠射角时使用由条带水深求得的横向坡度。
- `-T<grid>` / `--topography-grid=<grid>` — 使用该地形网格（例如由 `mbgrid` 生成）的坡度代替条带水深。
- `-Z<altitude>` / `--altitude=<altitude>` — 对没有水深的数据，假设海底平坦时使用的默认声呐高度（米）。
- `-R<refangle>` / `--reference-angle=<refangle>` — `mbprocess` 应用改正时使用的参考角。默认值：30 度。
- `-C` / `--symmetric-correction` — 强制改正表关于垂直方向对称。通常最好保持改正表不对称，之后再用 `mbset`（`AMPCORRSYMMETRY`、`SSCORRSYMMETRY`）设置对称性。
- `-B<mode>[/<beamwidth>/<depression>]` / `--beam-pattern-mode=...` — `0`：由观测平均值构成的经验表（默认）；`1`：高斯波束方向图结合球面扩展的参数模型，需指定有效波束宽度和俯角（度，默认值 50 和 20）。
- `-G<kind>/<angle>/<min>/<max>/<n_columns>/<n_rows>` / `--output-grid=...` — 写出振幅（`kind` 1）或侧扫声呐（`kind` 2）的直方图网格，角度范围 `-angle` 到 `+angle`，数值范围 `min` 到 `max`；两者都需要时指定两次。
- `-S` / `--skip-existing` — 跳过改正表已存在且为最新的条带文件。
- `-D` / `--dump-output` — 将改正表输出到屏幕（标准输出），而不是 `.aga`/`.sga` 文件。
- `-F<format>` / `--format=<format>` — MBIO 格式编号；通常根据文件名推断。
- `-V` / `--verbose` — 显示进度和写出的改正表数。

## 示例

对数据列表中的所有文件，以 2 度间隔的角度箱、每 50 个 ping 平均，生成侧扫声呐和振幅改正表：

    mbbackangle -Idatalist.mb-1 -N81/80.0 -P50 -V

只生成侧扫声呐改正表，海底坡度取自水深网格，并跳过已处理的文件：

    mbbackangle -Idatalist.mb-1 -A2 -Tsurvey_bathy.grd -S -V

对一个文件同时写出用于检查改正表的直方图网格：

    mbbackangle -I0003_20020425_022926.mb57 -G1/80/60/81/121 -G2/80/60/81/121 -V

应用改正：

    mbprocess -Idatalist.mb-1

## 注意

- 不含振幅或侧扫声呐数据的格式会被跳过并显示提示。
- 使用 `-G` 时，程序还会尝试运行 `mbm_grdplot` 脚本绘图；Windows 上没有 MB-System 脚本，因此这一步会失败（命令提示符报告无法识别该命令），但网格本身已写出，可以用 GMT 绘制。
- `-G` 中可以省略 `min`（`kind/angle/max/n_columns/n_rows`），此时 `min` 为 0。
- 请对原始条带文件（不含 `$PROCESSED` 指令的数据列表）运行：改正表会写入实际读取文件的参数文件，而 `mbprocess` 使用的是原始文件的参数文件。

## 另请参阅

`mbprocess`, `mbset`, `mbfilter`, `mbmosaic`, `mbgrid`, `mbpreprocess`。完整参考资料为 MB-System 手册页 mbbackangle(1)（英文）。
