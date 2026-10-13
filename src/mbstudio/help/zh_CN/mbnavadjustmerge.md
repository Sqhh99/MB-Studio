# mbnavadjustmerge — 创建、编辑和合并 MBnavadjust 导航调整项目

`mbnavadjustmerge` 是操作 MBnavadjust 项目的命令行工具。MBnavadjust 通过匹配测线重叠或交叉处的海底地形特征来调整条带数据的导航。本程序可以创建项目、导入条带数据、查找交叉、自动拾取导航连接（tie）、反演校正后的导航、将导航应用到条带文件，以及编辑或合并项目。它使导航调整流程可以脚本化；在 Windows 上，由于交互式程序 `mbnavadjust` 不可用，它是处理 MBnavadjust 项目的途径。

## 语法

    mbnavadjustmerge --input=project_path [--input=project_path] [--output=project_path]
    mbnavadjustmerge --create-project=project_path [settings] [commands]
    mbnavadjustmerge --input=project_path [settings] [commands] [--verbose] [--help]

    settings:  --section-length=km --section-soundings=count --contour-interval=meters
               --color-interval=meters --tick-interval=meters --label-interval=meters
               --decimation=value --smoothing=value --zoffsetwidth=meters
    commands:  --import=path[:format] --import-as-survey=path[:format] --find-crossings
               --autopick --autopick-horizontal
               --autopick-crossing-type=all|mediocre|good|better|true
               --autopick-scope=all|survey|withsurvey|block|file|withfile|withsection
               --autopick-survey=survey --autopick-survey2=survey --autopick-file=file
               --autopick-section=section --autopick-overlap-threshold=percent
               --invert-navigation --update-grids --apply-navigation
               --set-global-tie=file:section[:snav]/xoffset/yoffset/zoffset[/xsigma/ysigma/zsigma]
               --set-tie=file1:section1/file2:section2/xoffset/yoffset/zoffset[/xsigma/ysigma/zsigma]
               --set-ties-xyz-all --set-ties-xyonly-all --set-ties-zonly-all
               --unset-tie=file1:section1/file2:section2 --skip-unset-crossings
               --update-file=file --update-all-files
               --import-tie-list=file --export-tie-list=file
               --triangulate --triangulate-all ...

## 说明

项目由其 `.nvh` 文件或 `.dir` 目录的路径指定。`--create-project` 创建一个新的空项目（不能与 `--input` 同时使用）；`--input` 打开已有项目。设置类选项（分段长度和分段测深点数、等深线显示、平滑、垂直偏移搜索宽度）可以在创建项目时或之后指定，并且总是先于其他命令执行。

`--import` 添加条带数据：可以是数据列表（不指定格式），也可以是单个文件加格式编号（`file:format`）。每个文件按当前分段设置切分为若干分段（section），并自动查找与已导入数据之间的交叉。时间上连续的文件属于同一个测量（survey）；出现时间间隔时开始新的测量，除非使用 `--import-as-survey`。

`--autopick` 在每个未分析的交叉处计算三维（东、北、垂直）水深失配网格，在最小值明确时于该偏移处设置连接；`--autopick-horizontal` 只使用水平偏移。自动拾取过滤选项用于限定参与的交叉（默认：重叠率不低于 10% 的全部交叉）。`--invert-navigation` 对连接和全局连接组成的网络进行求解，在项目内得到校正后的导航模型；`--apply-navigation` 随后把该解写入条带文件的处理参数，使下一次运行 `mbprocess` 时应用它。`--update-grids` 根据当前导航重新生成项目的参考水深网格。

给出两个 `--input` 项目时，第二个项目被添加到第一个项目中；给出 `--output` 时，两者都被复制到新的合并项目中，已有的连接会被保留。给出多个命令时，按其在命令行中出现的顺序执行。文件、分段和测量用其在项目中的编号表示（例如 `3:0` 表示文件 3 的分段 0）。

## 常用选项

- `--input=project_path` — 已有项目（`.nvh` 文件或 `.dir` 目录）。指定两次即可合并两个项目。
- `--output=project_path` — 接收合并后输入项目的新项目。省略时，第二个输入项目被添加到第一个中。
- `--create-project=project_path` — 创建新的空项目；同一命令行中的其他命令作用于该项目。
- `--section-length=km` — 之后导入文件的最大分段长度（公里）。
- `--section-soundings=count` — 之后导入文件每个分段的最大测深点数。
- `--smoothing=value` — 导航反演的平滑权重（默认 3.0，有效范围约 0.1～10）。
- `--zoffsetwidth=meters` — 失配网格和自动拾取所用垂直偏移搜索范围的宽度。
- `--import=path[:format]` — 导入数据列表，或附加格式编号的单个条带文件。可重复指定。
- `--import-as-survey=path[:format]` — 与 `--import` 相同，但无论有无时间间隔，所有导入文件都归入一个新的测量。
- `--find-crossings` — 查找新的交叉并重新计算重叠率（在合并项目或测量、修改不连续点之后有用）。
- `--autopick` — 在未分析的交叉处自动拾取连接（三维偏移）；`--autopick-horizontal` 只拾取水平偏移。
- `--autopick-crossing-type=all|mediocre|good|better|true` — 只考虑重叠率不低于 10%、25% 或 50% 的交叉，或只考虑航迹真正相交的交叉。
- `--autopick-scope=all|survey|withsurvey|block|file|withfile|withsection` — 将自动拾取限定在某个测量、一对测量（`block`）、某个文件或分段，具体对象用 `--autopick-survey`、`--autopick-survey2`、`--autopick-file` 和 `--autopick-section` 指定。
- `--autopick-overlap-threshold=percent` — 参与自动拾取的交叉的最小重叠率（默认 10）。
- `--invert-navigation` — 对连接和全局连接进行反演，得到校正后的导航模型。
- `--apply-navigation` — 将导航解写入条带文件的处理参数（在 `--invert-navigation` 之后运行，然后运行 `mbprocess`）。
- `--update-grids` — 根据当前导航重新生成项目的参考水深网格。
- `--set-global-tie=file:section[:snav]/xoffset/yoffset/zoffset[/xsigma/ysigma/zsigma]` — 将某个导航点与绝对参考系（如 GPS）相连；偏移单位为米。
- `--set-tie=file1:section1/file2:section2/xoffset/yoffset/zoffset[/xsigma/ysigma/zsigma]` — 添加或替换两个分段之间的连接（必要时会创建交叉）。
- `--set-ties-xyz-all`、`--set-ties-xyonly-all`、`--set-ties-zonly-all` — 设置所有连接的模式（也有按文件、测量或块的版本，例如 `--set-ties-zonly-by-block=survey1/survey2`）。
- `--unset-tie=file1:section1/file2:section2` — 删除一个连接（另有 `--unset-ties-with-file=file`、`--unset-ties-by-block=survey1/survey2`）。
- `--skip-unset-crossings` — 将所有未分析的交叉标记为跳过。
- `--update-file=file`、`--update-all-files` — 重新编辑后，用当前已处理的条带文件更新项目中保存的波束标记。
- `--export-tie-list=file`、`--import-tie-list=file` — 将连接写入文本文件，或导入从其他项目导出的连接。
- `--triangulate` — 预先生成用于绘制等深线的分段三角网（`--triangulate-all` 全部重新生成）。
- `--verbose` — 输出更多信息；`--help` 显示完整的选项列表。

## 示例

一步完成创建项目、设置 0.1 公里分段和导入测量数据列表：

    mbnavadjustmerge --create-project=Survey2025.nvh --section-length=0.1 --import=datalist.mb-1

在重叠率不低于 25% 的交叉处自动拾取连接，并导出连接以供检查：

    mbnavadjustmerge --input=Survey2025.nvh --autopick --autopick-crossing-type=good --export-tie-list=ties_autopick.txt

反演校正后的导航，将其应用到条带文件，然后重新处理：

    mbnavadjustmerge --input=Survey2025.nvh --invert-navigation --apply-navigation
    mbprocess -I datalist.mb-1

将两个已有项目合并为一个新项目：

    mbnavadjustmerge --input=Navadjust20140515m1.nvh --input=Navadjust20140515m2.nvh --output=Navadjust20140515All.nvh

## 注意

- 无人监督的自动拾取经常出错。在依赖反演结果之前，请检查拾取结果（例如用 `--export-tie-list`），删除或修正错误的连接。
- 用 `--import` 导入单个条带文件时必须给出格式编号（例如 `--import=0001_20250101_000000.all:58`）；程序不会根据文件后缀推断格式。
- `--update-grids` 和项目合并会通过系统 shell 运行辅助命令（`mbdatalist`、`mbprocess`、`mbgrid` 以及文件工具）；在 Windows 上这些步骤可能无法完成。如果参考网格没有更新，请用 `mbgrid` 生成。
- 在此版本中，`--unset-ties-all` 及其别名 `--unset-all-ties` 需要一个值；请写成 `--unset-ties-all=1`（该值会被忽略）。
- 包含空格的项目和数据路径请加引号。

## 另请参阅

`mbprocess`, `mbset`, `mbdatalist`, `mbgrid`, `mbnavlist`, `mbnavadjust`（交互式，Windows 上不可用）。完整参考资料为 MB-System 手册页 mbnavadjustmerge(1)（英文）。
