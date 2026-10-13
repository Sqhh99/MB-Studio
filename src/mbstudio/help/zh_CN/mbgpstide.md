# mbgpstide — 由条带数据中的 GPS 椭球高生成潮位文件

`mbgpstide` 根据条带数据文件中记录的 GPS 高度（椭球高）生成潮位文件。高度可以加上固定偏移量或用大地水准面模型改正，从而使水深以椭球面或大地水准面为基准，而不是以水面为基准。当测量使用了 RTK 或 PPK GPS 高程时，可用它代替验潮站或潮汐模型，并由 `mbprocess` 应用结果。

## 语法

    mbgpstide [-Atideformat -Dinterval -Fformat -Idatalist -M -Ooutput -Roffset -S -Tgeoid -Usource -V -H]
    mbgpstide [--tideformat=tideformat --interval=interval --format=format --input=datalist
              --setparameters --output=output --offset=offset --skipexisting --geoid=geoid
              --use=source --verbose --help]

## 说明

程序读取单个条带文件或数据列表（默认 `datalist.mb-1`），并按固定时间间隔（默认 300 秒）对 GPS 高度取平均。写入潮位文件的每个值等于平均高度加上 `-R` 偏移量；使用 `-T` 时再减去大地水准面高。

高度来源取决于数据格式。对于 Kongsberg（Simrad）EM 格式 56、57、58 和 59，高度取自 Height 数据报。对于 GSF 文件（格式 121），高度取自 ping 头；使用 `-U1` 时会加上该 ping 的分离量（`sep`），得到相对于 GSF 文件垂直基准面的高度。其他格式不含可用的高度，生成的潮位文件将为空。

不使用 `-O` 时，为每个条带文件生成一个潮位文件，文件名为条带文件名加 `.gps.tde`，并放在条带文件旁边。使用 `-O` 时，所有数值写入同一个文件。`-M` 会更新每个条带文件的参数文件，使 `mbprocess` 应用新的潮位文件；之后运行 `mbprocess` 才能使改正生效。

## 常用选项

- `-I datalist` — 输入条带文件或数据列表（长格式 `--input=datalist`）。默认为 `datalist.mb-1`。
- `-F format` — 输入数据的 MB-System 格式编号；负值表示输入为数据列表（长格式 `--format=format`）。通常可根据文件名自动识别。
- `-D interval` — 潮位值之间的时间间隔（秒）（长格式 `--interval=interval`）。默认为 300。`-D0` 时写出每个高度值，此时请使用 `-A1` 以避免时间戳重复。
- `-A tideformat` — 输出格式（长格式 `--tideformat=tideformat`）：2 = `year month day hour minute second tide`（默认），1 = `time_d tide`（历元秒），5 = CARIS 格式 `yyyy/mm/dd hh:mm:ss.sss tide`。格式 5 不能被 `mbprocess` 读取，请勿与 `-M` 同时使用。
- `-O output` — 将所有潮位值写入这一个文件（长格式 `--output=output`）；`-O-` 写到标准输出（不能与 `-M` 同时使用）。
- `-M` — 设置条带文件的处理参数，使 `mbprocess` 应用潮位文件（长格式 `--setparameters`）。
- `-R offset` — 给每个潮位值加上该常数（米）（长格式 `--offset=offset`）。
- `-S` — 跳过 `.gps.tde` 文件已比数据新的条带文件（长格式 `--skipexisting`）；仅在不使用 `-O` 时有效。
- `-T geoid` — 大地水准面相对椭球面高度的 GMT 网格（例如由 EGM2008 转换而来）；沿航迹的值会被减去（长格式 `--geoid=geoid`）。
- `-U source` — 高度来源（长格式 `--use=source`）：0 = Kongsberg Height 数据报或 GSF 椭球高（默认），1 = GSF 相对垂直基准面的高度。
- `-V` — 详细输出（长格式 `--verbose`）。
- `-H` — 显示用法后退出（长格式 `--help`）。

## 示例

为数据列表中的每个文件生成潮位文件，并设置由 `mbprocess` 应用：

    mbgpstide -I datalist.mb-1 -M
    mbprocess -I datalist.mb-1

使用 60 秒平均值并加上 -35.20 米的固定偏移，跳过已有最新潮位文件的文件：

    mbgpstide -I datalist.mb-1 -D60 -R-35.20 -S -M

以历元秒格式为整个测区写出一个潮位文件：

    mbgpstide -I datalist.mb-1 -A1 -O survey_gps_tide.txt

对一个文件使用 GSF 相对垂直基准面的高度：

    mbgpstide -I line_0001.gsf -F121 -U1 -M

## 注意

- 潮位文件写在数据列表所列条带文件的旁边，而不是当前目录（使用 `-O` 时除外）。
- `-T` 通过系统 shell 运行由 `awk` 或 `mblist` 通过管道传给 GMT `grdtrack` 的命令来提取大地水准面高。Windows 命令提示符中通常没有以这些名称提供的命令，因此 `-T` 在那里可能失败；如果测区内大地水准面高近似为常数，可改用 `-R` 作为常数改正。
- 包含空格的路径请加引号。

## 另请参阅

`mbprocess`, `mbset`, `mbotps`, `mbdatalist`, `mblist`。完整参考资料为 MB-System 手册页 mbgpstide(1)（英文）。
