# mbfilter — 侧扫声呐/振幅数据的平滑、高通和对比度滤波

`mbfilter` 对侧扫声呐或波束振幅数据应用简单的图像处理滤波：用于降低斑点噪声的低通平滑、用于突出细小结构的高通滤波，以及对比度增强。通常在用 `mbmosaic` 制作反向散射镶嵌图之前，对处理后（并经过掠射角改正）的文件运行。

## 语法

    mbfilter
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --buffer-size=buffersize {-Nbuffersize}
        --contrast-filter=mode/xdim/ldim/iteration {-Cmode/xdim/ldim/iteration}
        --data-kind=kind {-Akind}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format {-Fformat}
        --help {-H}
        --hipass-filter=mode/xdim/ldim/iteration/offset {-Dmode/xdim/ldim/iteration/offset}
        --input=infile {-Iinfile}
        --smooth-filter=mode/xdim/ldim/iteration/threshold_lo/threshold_hi {-Smode/xdim/ldim/iteration/threshold_lo/threshold_hi}
        --threshold=threshold_lo/threshold_hi {-Tthreshold_lo/threshold_hi}
        --verbose {-V}

## 说明

条带数据被视为一幅图像，x 轴为像素（或波束）号，y 轴为 ping 号。每个滤波器是一个 `xdim` × `ldim`（横向 × 沿航向）的窗口，与该图像做卷积；缺失和已标记的值会被忽略。可用的滤波器有：

- `-S1` 矩形均值、`-S2` 高斯均值、`-S3` 矩形中值、`-S4` 逆梯度（低通，平滑）；
- `-D1` 矩形均值、`-D2` 高斯均值、`-D3` 矩形中值相减（高通）；
- `-C1` 边缘检测、`-C2` 梯度幅值相减（对比度增强）。

最多可指定十个滤波器，按命令行中的顺序依次应用，每个都有自己的迭代次数。降低斑点噪声通常反复使用 3x3 或 5x5 窗口；高通滤波通常使用 3x10 之类的狭长窗口。逆梯度滤波器对平坦区域的平滑强于边缘，因此能更好地保持地物清晰。中值滤波器可通过低/高比值阈值限定为只去除尖峰：只有当原值与中值之比低于 `threshold_lo` 或高于 `threshold_hi` 时才替换，这有助于抑制坏 ping 造成的条纹。

高通滤波器从数据中减去其低通版本，并加上一个偏移量使结果保持为正。对比度滤波器会放大噪声，一般只有在平滑之后才有效；边缘检测至少需要 5x5 的窗口。

条带文件本身不会被修改。滤波后的数值写入每个输入文件旁的辅助文件：侧扫声呐为文件名加 `.ffs`，振幅为文件名加 `.ffa`。要使用滤波后的数据，在绘图或镶嵌程序的数据类型后加 `F`，例如 `mbmosaic -A4F`（滤波后的侧扫声呐）或 `-A3F`（滤波后的振幅），或 `gmt mbswath -Z5F` / `-Z4F`。

## 常用选项

- `-I<file>` / `--input=<file>` — 条带文件或数据列表。默认值：`datalist.mb-1`。
- `-A<kind>` / `--data-kind=<kind>` — 滤波波束振幅（`1`）或侧扫声呐（`2`）。默认值：2。
- `-S<mode>/<xdim>/<ldim>[/<iteration>[/<threshold_lo>/<threshold_hi>]]` / `--smooth-filter=...` — 低通滤波：mode 1 矩形均值、2 高斯均值、3 矩形中值、4 逆梯度。阈值只对中值滤波器有效。默认值：迭代 1 次。
- `-D<mode>/<xdim>/<ldim>[/<iteration>[/<offset>]]` / `--hipass-filter=...` — 高通滤波：mode 1 矩形均值、2 高斯均值、3 矩形中值相减。`offset` 加到结果上以保持为正，应根据数据的取值范围选择。默认值：迭代 1 次，偏移量 1000。
- `-C<mode>/<xdim>/<ldim>[/<iteration>]` / `--contrast-filter=...` — 对比度增强：mode 1 边缘检测、2 梯度幅值相减。默认值：迭代 1 次。
- `-T<threshold_lo>/<threshold_hi>` / `--threshold=...` — 对此选项之后指定的中值平滑滤波器使用的比值阈值。
- `-B<yr/mo/da/hr/mn/sc>`, `-E<yr/mo/da/hr/mn/sc>` / `--begin-time`, `--end-time` — 要滤波数据的时间范围。
- `-R<west/east/south/north>` / `--bounds=...` — 要滤波数据的地理范围。
- `-N<buffersize>` / `--buffer-size=<buffersize>` — 缓冲区保存的记录数（10 到 5000）。默认值：5000。
- `-F<format>` / `--format=<format>` — MBIO 格式编号；通常根据文件名推断。
- `-V` / `--verbose` — 显示滤波设置和进度。

## 示例

对处理后文件的侧扫声呐应用一次 3x3 矩形均值滤波：

    mbfilter -Idatalistp.mb-1 -S1/3/3/1

应用五次 3x3 逆梯度滤波：

    mbfilter -Idatalistp.mb-1 -S4/3/3/5

先用高通滤波突出细小结构，再平滑以降低斑点噪声：

    mbfilter -Idatalistp.mb-1 -D1/3/10/1/10000 -S4/3/3/5

先用 7x7 中值滤波平滑，再应用 5x5 边缘检测：

    mbfilter -Idatalistp.mb-1 -S3/7/7/1 -C1/5/5/1

## 注意

- 每次运行都会覆盖 `.ffs` 或 `.ffa` 文件；之前运行的滤波结果不会累积，因此请在一条命令中给出完整的滤波器组合。
- 滤波器至少需要 `mode/xdim/ldim`；数值不足时该滤波器被忽略。
- `-T` 只对命令行中位于其后的 `-S` 滤波器有效；也可以将阈值作为 `-S3` 的第 5 和第 6 个数值给出。
- 处理后文件的数据列表（`datalistp.mb-1`）可用 `mbdatalist -Idatalist.mb-1 -Z` 生成。

## 另请参阅

`mbbackangle`, `mbprocess`, `mbmosaic`, `mbdatalist`, `gmt mbswath`。完整参考资料为 MB-System 手册页 mbfilter(1)（英文）。
