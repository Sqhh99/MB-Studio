# mblevitus — 由 Levitus 图集生成平均声速剖面

`mblevitus` 利用 Levitus（1982）世界海洋气候图集中的温度和盐度数据，为指定位置生成年平均声速剖面。当没有实测声速剖面（CTD、XBT 或 SVP 投放）而又需要通过射线追踪重新计算水深时，或者需要一条与实测剖面进行比较的参考剖面时，使用本程序。

## 语法

    mblevitus
        --help {-H}
        --location=lon/lat {-Rlon/lat}
        --output-file=file {-Ooutfile}
        --verbose {-V}

## 说明

Levitus 数据库的分辨率为 1 度 × 1 度。`mblevitus` 选取包含指定位置的网格单元，并同时报告请求的位置和实际使用的单元中心（例如请求 -95/2 时，使用以经度 265.5、纬度 2.5 为中心的单元）。剖面代表该单元的年平均水柱结构。

声速由温度和盐度通过 DelGrosso 公式计算，压力由深度和纬度求得。声速在 0～12000 m 的标准深度上给出。若数据库只覆盖到比 12000 m 浅的深度（反映了区域水深），则使用可用的最深温度和盐度值将剖面延伸到 12000 m。若指定位置位于陆地，则不存在剖面，也不会生成输出文件。

输出文件为纯文本：开头是一组说明剖面来源的 `#` 注释行，其后每行一对 `深度 声速`（米，米/秒）。这正是 `mbset`/`mbprocess` 进行射线追踪时读取的声速剖面格式（参数 `SVPFILE`）。

程序会输出请求位置和使用位置、直接取自数据库的数值个数和外推的数值个数，以及输出文件名。使用 `-V` 时，还会输出完整的深度、温度、盐度和声速表。

## 常用选项

- `-Rlon/lat` — 剖面位置的经度和纬度（度）（长格式 `--location=lon/lat`）。西经和南纬为负值。由于分辨率为 1 度，使用整数度即可。默认值：0/0。
- `-Ooutfile` — 声速剖面输出文件（长格式 `--output-file=file`）。默认值：当前目录下的 `velocity`。
- `-V` — 详细模式（长格式 `--verbose`）：同时显示程序版本和完整的水柱数据表。
- `-H` — 显示简要说明和用法后退出（长格式 `--help`）。

## 示例

为西经 95 度、北纬 2 度生成年平均剖面，写入 `velocity_profile.svp` 并显示完整数据表：

    mblevitus -R-95/2 -Ovelocity_profile.svp -V

使用长格式选项，为北海（东经 3 度、北纬 56 度）的测量生成剖面：

    mblevitus --location=3/56 --output-file=northsea_levitus.svp

用该剖面对数据列表中所有文件启用射线追踪水深重算，然后重新处理：

    mbset -I datalist.mb-1 -PSVPFILE:northsea_levitus.svp
    mbprocess -I datalist.mb-1

## 注意

- 数据库文件 `LevitusAnnual82.dat` 安装在 MB-Studio 安装目录的 `share\mbsystem` 文件夹中，程序会根据 `mblevitus.exe` 的位置自动找到它。若设置了环境变量 `MBSYSTEM_SHAREDIR`，则改从该文件夹读取数据库。
- 气候平均剖面只是近似；如有测量期间获取的实测剖面，应优先使用。
- 请将数值紧接在选项后书写（`-R-95/2`），以免负经度被误认为另一个选项。

## 另请参阅

`mbset`、`mbprocess`、`mbsvplist`、`mbsvpselect`、`mbabsorption`。完整参考请见 MB-System 手册页 mblevitus(1)（英文）。
