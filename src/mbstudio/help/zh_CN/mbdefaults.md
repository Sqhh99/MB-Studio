# mbdefaults — 设置或列出 MBIO 默认控制参数

`mbdefaults` 用于列出或修改用户的 MB-System（MBIO）默认控制参数，例如经度范围约定、判定数据间断的时间间隔、I/O 缓冲区大小以及“快速水深”（`.fbt`）文件的版本。不带选项运行时显示当前默认值；给出选项时保存新值，供以后所有 MB-System 程序使用。

## 语法

    mbdefaults
        --fbt-version=fbtversion {-Ffbtversion}
        --file-io-buffer=fileiobuffer {-Bfileiobuffer}
        --help {-H}
        --image-display=imagedisplay {-Iimagedisplay}
        --lonflip=lonflip {-Llonflip}
        --mbview-settings=mbviewsettings {-Mmbviewsettings}
        --project=mbproject {-Wmbproject}
        --ps-display=psdisplay {-Dpsdisplay}
        --time-gap=timegap {-Ttimegap}
        --use-lock-files=yes|no {-Uyes|no}
        --verbose {-V}

## 说明

默认值保存在用户主目录下的文本文件 `.mbio_defaults` 中。在 Windows 上，如果设置了环境变量 `HOME` 则使用该文件夹，否则使用 `%USERPROFILE%`（例如 `C:\Users\name\.mbio_defaults`）。该文件不存在时使用 MBIO 内置的默认值。

不给出任何设置选项时，`mbdefaults` 只显示当前值。只要给出任一设置选项，就会写出新的 `.mbio_defaults` 文件，其中包含新值以及所有未修改的值，并显示新的参数列表。

这些默认值影响通过 MBIO 读取条带数据的程序，例如 `mblist` 或 `mbgrid` 使用的经度表示方式，以及导航数据中间断的检测。早期版本的 MB-System 还在此保存格式编号、ping 平均、区域和时间窗口，现在这些已不属于默认值。

## 常用选项

- `-Llonflip` — 返回经度的范围（`--lonflip`）：`-1` 为 -360 至 0 度，`0` 为 -180 至 180 度，`1` 为 0 至 360 度。默认值：0。
- `-Ttimegap` — 相邻 ping 之间的最大时间间隔（分钟），超过即视为数据间断（`--time-gap`）。默认值：1。
- `-Bfileiobuffer` — 对支持的格式（目前为格式 58 和 88）设置文件 I/O 缓冲区大小（千字节），例如 `-B10000`（`--file-io-buffer`）。主要在通过网络读取数据时有用。默认值：0（系统默认缓冲）。
- `-Ffbtversion` — `mbdatalist` 和 `mbprocess` 写出的 `.fbt` 文件版本（`--fbt-version`）：`3`、`new` 或 `NEW` 为当前版本，`2`、`old` 或 `OLD` 为 5.3.1897 之前的版本。默认值：3。
- `-Uyes|no` — 是否使用锁文件，使同一条带文件同一时间只被一个程序处理或编辑（`--use-lock-files`）；取 `yes`/`1` 或 `no`/`0`。
- `-Mmbviewsettings` — 三维查看器的默认显示设置（`--mbview-settings`）：`-MPcolortable/mode/shade_mode`、`-MGcolortable/mode`（坡度）、`-MOcolortable/mode`（叠加）、`-MImagnitude/elevation/azimuth`（光照）或 `-MSmagnitude`（坡度晕渲）。色表：0 Haxby、1 Bright、2 Muted、3 Grayscale、4 Flat gray、5 Sealevel 1、6 Sealevel 2。
- `-Dpsdisplay` — 绘图脚本用来显示 PostScript 文件的程序（`--ps-display`）。默认值：`Default`（系统默认查看器）。
- `-Iimagedisplay` — 用来显示图像文件的程序（`--image-display`）。默认值：`Default`。
- `-Wmbproject` — 默认项目名称（`--project`）。默认值：`none`。
- `-V` — 同时显示程序版本（`--verbose`）。
- `-H` — 显示简要说明和用法后退出（`--help`）。

## 示例

列出当前的默认参数：

    mbdefaults

以 0 至 360 度范围返回经度（适用于跨越 180 度经线的测区）：

    mbdefaults -L1

将间断时间间隔设为 2 分钟，文件 I/O 缓冲区设为 10 MB：

    mbdefaults -T2 -B10000

## 注意

- 设置因用户而异：每个 Windows 用户账户都有自己的 `.mbio_defaults` 文件。
- 每次修改都会用当前值重写整个文件，因此只有给出的选项对应的值会改变。
- Windows 版本不使用锁文件机制，因此 `-U` 设置在 Windows 上实际上不起作用。
- PostScript/图像查看器以及 mbview 设置仅供 Windows 版本中不包含的工具（绘图宏、三维查看器）使用。
- `-MS` 中的 `S` 必须为大写。

## 另请参阅

`mbinfo`、`mblist`、`mbgrid`、`mbdatalist`、`mbprocess`、`gmt mbswath`、`gmt mbcontour`、`gmt mbgrdtiff`。完整参考见 MB-System 手册页 mbdefaults(1)（英文）。
