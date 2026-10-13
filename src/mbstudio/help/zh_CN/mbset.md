# mbset — 创建和修改 mbprocess 参数文件

`mbset` 用于创建或修改控制 `mbprocess` 的 `.par` 参数文件。可以为单个条带文件或数据列表中的所有文件一次性指定 `mbprocess` 使用的 SVP、导航、潮汐或编辑文件，以及要应用的改正（横摇偏差、吃水、艏向偏移等）。

## 语法

    mbset
        --explicit {-E}
        --format=format_id {-Fformat_id}
        --help {-H}
        --input=file {-Ifile}
        --look-for-files {-L}
        --parameter=PARAMETER:value {-PPARAMETER:value}
        --remove-nav-adjust {-N}
        --verbose {-V}

## 说明

每个条带数据文件都可以有一个 `mbprocess` 参数文件，名称为数据文件名加 `.par` 后缀（`survey.mb58` 对应 `survey.mb58.par`）。该文件每行一条命令，例如 `SVPFILE`、`ROLLBIAS` 或 `EDITSAVEFILE`。`mbset` 读取已有的参数文件（若不存在则从默认参数开始），修改 `-P` 指定的参数，然后写回文件。

`-P` 选项设置一个参数，可根据需要重复多次。如果输入是数据列表，则对其（递归）引用的所有条带文件的参数文件应用相同的修改，这是为整个测量设置统一处理参数的常用方法。

除非指定 `-E`，设置某个文件或数值时也会自动打开相应的处理模式。例如 `-PSVPFILE:best.svp` 同时将 `SVPMODE` 设为 1，使 SVP 真正被使用；`-PROLLBIAS:0.5` 将 `ROLLBIASMODE` 设为 1；`-PDRAFT:1.95` 将 `DRAFTMODE` 设为 4（固定吃水）；`-PTIDEFILE:...` 打开潮汐改正；`-PNAVADJFILE:...` 打开调整后导航和深度偏移的合并（`NAVADJMODE` 2）。

常用的参数命令（完整列表及各模式含义见手册页）：

- 导航：`NAVMODE`, `NAVFILE`, `NAVFORMAT`, `NAVHEADING`, `NAVSPEED`, `NAVDRAFT`, `NAVATTITUDE`, `NAVINTERP`, `NAVTIMESHIFT`, `NAVSHIFT`, `NAVOFFSETX`, `NAVOFFSETY`, `NAVSHIFTLON`, `NAVSHIFTLAT`, `NAVADJMODE`, `NAVADJFILE`。
- 姿态和声呐深度：`ATTITUDEMODE`, `ATTITUDEFILE`, `ATTITUDEFORMAT`, `SONARDEPTHMODE`, `SONARDEPTHFILE`, `SONARDEPTHFORMAT`。
- 编辑和裁剪：`EDITSAVEMODE`, `EDITSAVEFILE`, `DATACUTCLEAR`, `DATACUT`, `BATHCUTNUMBER`, `BATHCUTDISTANCE`, `BATHCUTSPEED`, `AMPCUTNUMBER`, `SSCUTNUMBER` 等。
- 水深重算：`SVPMODE`, `SVPFILE`, `SSVMODE`, `SSV`, `TTMULTIPLY`, `ANGLEMODE`, `SOUNDSPEEDREF`, `STATICMODE`, `STATICFILE`。
- 改正：`DRAFTMODE`, `DRAFT`, `DRAFTOFFSET`, `DRAFTMULTIPLY`, `HEAVEMODE`, `HEAVEOFFSET`, `LEVERMODE`, `ROLLBIASMODE`, `ROLLBIAS`, `ROLLBIASPORT`, `ROLLBIASSTBD`, `PITCHBIASMODE`, `PITCHBIAS`, `HEADINGMODE`, `HEADINGOFFSET`, `TIDEMODE`, `TIDEFILE`, `TIDEFORMAT`。
- 振幅和侧扫声呐：`AMPCORRMODE`, `AMPCORRFILE`, `AMPCORRSYMMETRY`, `AMPCORRANGLE`, `AMPCORRSLOPE`, `SSCORRMODE`, `SSCORRFILE`, `SSCORRSYMMETRY`, `SSCORRANGLE`, `SSCORRSLOPE`, `AMPSSCORRTOPOFILE`, `SSRECALCMODE`。
- 通用和元数据：`FORMAT`, `OUTFILE`, `METAVESSEL`, `METAINSTITUTION`, `METAPLATFORM`, `METASONAR`, `METACRUISEID`, `METACRUISENAME`, `METAPI`, `METACLIENT` 等。

## 常用选项

- `-I<file>` / `--input=<file>` — 要创建或更新参数文件的条带文件或数据列表。默认值：`datalist.mb-1`。
- `-P<PARAMETER>:<value>` / `--parameter=<PARAMETER>:<value>` — 设置一个参数；设置多个时重复使用。分隔符可以是 `:` 或 `=`，只有第一个起分隔作用，因此数值本身可以包含 `:`（例如 Windows 盘符）。
- `-E` / `--explicit` — 不自动打开处理模式；所有模式都必须用 `-P` 显式设置。
- `-L` / `--look-for-files` — 在每个条带文件旁查找标准名称的编辑保存文件和导航文件，找到后设置参数以使用它们。
- `-N` / `--remove-nav-adjust` — 删除对 `mbnavadjust` 调整后导航文件的引用：被引用的文件会被删除，`NAVADJMODE`/`NAVADJFILE` 被重置。
- `-F<format_id>` / `--format=<format_id>` — 输入的 MBIO 格式编号，优先于根据文件名的自动识别。
- `-V` / `--verbose` — 列出写入每个参数文件的参数。
- `-H` / `--help` — 显示简短帮助信息后退出。

## 示例

为一个文件设置 SVP 文件、+0.5 度横摇偏差和 1.95 m 固定吃水：

    mbset -I0051_20010829_223755.mb57 -PSVPFILE:0051_20010829_223755.mb57.svp -PROLLBIAS:0.5 -PDRAFT:1.95 -V

对测量的所有文件应用同一个潮汐文件（格式 2：年 月 日 时 分 秒 潮位）：

    mbset -Idatalist.mb-1 -PTIDEFILE:survey.tide -PTIDEFORMAT:2

应用用 `mbgetesf` 提取的编辑保存文件：

    mbset -Ihs_ew9103_143.mb24 -PEDITSAVEMODE:1 -PEDITSAVEFILE:hs_ew9103_143.mb24.esf

然后处理数据：

    mbprocess -Idatalist.mb-1

## 注意

- `mbset` 只写参数文件；在运行 `mbprocess` 之前，处理后的数据不会有任何变化。
- 文件名数值只读取到第一个空格为止，因此参数中使用的文件名不能包含空格。相对路径相对于条带文件所在的文件夹，因此辅助文件通常与数据放在一起。
- 参数名必须使用大写字母。

## 另请参阅

`mbprocess`, `mbclean`, `mbsvplist`, `mbbackangle`, `mbgetesf`, `mbotps`, `mbdatalist`。完整参考资料为 MB-System 手册页 mbset(1)（英文）。
