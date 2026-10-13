# mbmapscale — 显示每度经纬度对应的米数

`mbmapscale` 输出指定纬度处地理坐标（经度和纬度）与东向、北向米数之间的局部比例关系。在局部笛卡尔近似中进行度与米的换算时使用，例如确定与某个米制间距相当的以度为单位的网格单元大小，或解释测区内的偏移量。

## 语法

    mbmapscale
        --alvinxy {-A}
        --help {-H}
        --latitude=latitude {-Llatitude}
        --verbose {-V}

## 说明

比例在 `-L` 指定的纬度（十进制度，北为正，南为负）处计算。未指定纬度时，在赤道（纬度 0.0）处计算。

默认使用 WGS72 椭球，与其他 MB-System 程序的局部比例计算一致。指定 `-A` 时，改用 AlvinXY 局部坐标约定所采用的 1866 年 Clarke 椭球计算。

结果以 4 个数值写入标准输出：每度经度的米数、每度纬度的米数，以及它们的倒数（标注为“Meters to degree longitude”和“Meters to degree latitude”），即东向和北向 1 米所对应的度数。

## 常用选项

- `-Llatitude` — 计算比例的纬度（十进制度）（长格式 `--latitude=latitude`）。默认值：0.0（赤道）。
- `-A` — 使用 AlvinXY 约定的 1866 年 Clarke 椭球代替 WGS72 椭球（长格式 `--alvinxy`）。
- `-V` — 详细模式（长格式 `--verbose`）：同时显示程序名和 MB-System 版本。
- `-H` — 显示简要说明和用法后退出（长格式 `--help`）。

## 示例

使用默认的 WGS72 椭球，显示北纬 36.8 度处的比例：

    mbmapscale -L36.8

输出形式如下：

    Local scaling between degrees longitude and latitude and meters east and north:
        Using WGS72 ellipsoid
        Meters per degree longitude: 89244.230
        Meters per degree latitude:  110973.870
        Meters to degree longitude:  0.000011205
        Meters to degree latitude:   0.000009011

在同一纬度按 AlvinXY（1866 年 Clarke 椭球）约定显示比例：

    mbmapscale -L36.8 -A

显示南半球纬度（负值）处的比例：

    mbmapscale --latitude=-41.5

## 注意

- 结果只与纬度有关，不需要经度。
- 选项也可以用小写字母（`-l`、`-a`、`-v`、`-h`）。

## 另请参阅

`mbtime`、`mbgrid`、`mbmosaic`、`mbinfo`。完整参考请见 MB-System 手册页 mbmapscale(1)（英文）。
