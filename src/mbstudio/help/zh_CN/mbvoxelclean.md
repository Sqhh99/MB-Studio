# mbvoxelclean — 基于三维体素密度滤波的波束标记

`mbvoxelclean` 使用三维密度滤波器清理条带水深数据：落在测深点稀疏的体素中的测深被视为噪声并加上标记，而测深点密集的体素则被视为海底等真实目标。该程序主要面向水下激光雷达等极高密度的点云，用于 `mbprocess` 之前的编辑阶段。

## 语法

    mbvoxelclean [--verbose --help --input=datalist --format=value
        --voxel-size=xysize[/zsize] --occupy-threshold=value
        --count-flagged --flag-empty --ignore-empty
        --unflag-occupied --ignore-occupied --neighborhood=value
        --range-minimum=value --range-maximum=value
        --acrosstrack-minimum=value --acrosstrack-maximum=value
        --amplitude-minimum=value --amplitude-maximum=value]

## 说明

输入为一个条带文件，或引用多个条带文件的数据列表；每个文件分别读取和滤波。程序将一个文件的全部测深点读入内存，并分配到指定大小的立方体（或长方体）体素中。测深点数不少于阈值的体素被判定为“占用”（有效目标），少于阈值的体素被判定为“空”。

随后可执行两种操作：对空体素中原本未标记的测深加上标记（默认，`--flag-empty`）；以及取消占用体素中原本已标记测深的标记（`--unflag-occupied`，默认关闭）。`--neighborhood` 选项可将占用区域扩展若干个体素，从而在密集区域附近放宽标记条件。

读取数据时，还可选择应用基于传感器距离、横向距离和波束振幅的简单滤波。这些滤波在密度滤波之前执行，因此使用 `--unflag-occupied` 时，它们设置的标记可能会被取消。

程序不会修改条带文件本身。所有波束标记的变化都写入该条带文件的编辑保存文件（`<file>.esf`，新建或追加），并更新 `mbprocess` 参数文件（`<file>.par`），使 `mbprocess` 应用这些编辑。这与 `mbclean` 和交互式编辑器使用的编辑保存文件相同。如果存在 `.fbt`（快速水深）文件，则读取该文件代替完整文件。启用锁文件时，被其他程序锁定的文件将被跳过。

## 常用选项

- `--input=datalist` — 要处理的条带文件或数据列表。默认值：`datalist.mb-1`。
- `--format=value` — 输入的 MBIO 格式编号；负值表示输入为数据列表。通常根据文件名自动识别。
- `--voxel-size=xysize[/zsize]` — 体素的水平尺寸及可选的垂直尺寸（米）；省略 `zsize` 时与 `xysize` 相同。默认值：0.05 m。
- `--occupy-threshold=value` — 体素被判定为占用所需的最少测深点数。默认值：5。
- `--count-flagged` — 判断体素是否占用时也计入已标记的测深（通常只计入未标记的测深）。
- `--flag-empty` — 对空体素中未标记的测深加上标记（默认）。
- `--ignore-empty` — 不改变空体素中的测深。
- `--unflag-occupied` — 取消占用体素中已标记测深的标记。
- `--ignore-occupied` — 占用体素中已标记的测深保持标记（默认）。
- `--neighborhood=value` — 距占用体素 `value` 个体素以内（x、y、z 方向）的体素也视为占用。默认值：0。
- `--range-minimum=value`, `--range-maximum=value` — 对距传感器比该距离（m）更近或更远的未标记测深加上标记。
- `--acrosstrack-minimum=value`, `--acrosstrack-maximum=value` — 对横向距离（m）比最小值更偏左舷或比最大值更偏右舷的未标记测深加上标记。
- `--amplitude-minimum=value`, `--amplitude-maximum=value` — 对波束振幅低于最小值或高于最大值的未标记测深加上标记。
- `--verbose` — 显示每个文件及总计的标记和取消标记波束数。
- `--help` — 显示简短帮助信息后退出。

## 示例

对数据列表中的所有文件，使用 4 cm 体素，对测深点少于 5 个的体素中的测深加上标记，并显示进度：

    mbvoxelclean --input=datalist.mb-1 --voxel-size=0.04/0.04 --occupy-threshold=5 --flag-empty --verbose

使用 10 cm 体素，同时恢复位于密集区域中原已标记的测深，并标记距传感器超过 30 m 的所有测深：

    mbvoxelclean --input=datalist.mb-1 --voxel-size=0.1 --occupy-threshold=8 --unflag-occupied --range-maximum=30

之后将编辑应用到数据：

    mbprocess -Idatalist.mb-1

## 注意

- 只接受长选项（`--name=value`），没有单字母形式。
- 一个文件的全部测深点都保存在内存中，因此用很小的体素处理很大的文件需要大量内存。
- 编辑保存文件写在条带文件所在的文件夹，而不是当前目录。数据列表中的文件名不能包含空格。
- 对于不支持波束标记的数据格式，`mbprocess` 会将被编辑的测深置空（置零）而不是标记；此时程序会显示警告。

## 另请参阅

`mbclean`, `mbareaclean`, `mbprocess`, `mbset`, `mbinfo`, `mbdatalist`。完整参考资料为 MB-System 手册页 mbvoxelclean(1)（英文）。
