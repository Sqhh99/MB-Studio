# mbtime — 日历时间与 Unix 纪元时间互换

`mbtime` 在 Unix 纪元时间（自 1970/01/01 00:00:00 起的秒数）与日历时间之间转换时间值。MB-System 的文档、数据列表和许多辅助文件以纪元秒（“time_d”）表示时间，因此本程序便于检查时间戳、为其他程序准备时间范围，或阅读 `mblist` 和导航输出。

## 语法

    mbtime [-Mtime_d -Tyear/month/day/hour/minute/second -V -H]
        --calendar-time=year/month/day/hour/minute/second {-Tyear/month/day/hour/minute/second}
        --epoch-time=time_d {-Mtime_d}
        --help {-H}
        --verbose {-V}

## 说明

输入时间用 `-M` 以纪元秒给出，或用 `-T` 以日历时间给出。程序将同一时刻以另一种形式写入标准输出：`-M` 输出 `yyyy/mm/dd/hh/mm/ss.ssssss` 形式的日历时间，`-T` 输出保留 6 位小数的纪元秒。

日历时间由年、月、日、时、分、秒组成，以斜杠分隔；秒可包含精确到微秒的小数部分。所有时间均按 UTC 处理，与 MB-System 其他部分一致。

每次运行只执行一次转换。若同时给出 `-M` 和 `-T`，以命令行中后出现者为准。两者都未给出时，程序转换纪元时间 0 并输出 `1970/01/01/00/00/00.000000`。

## 常用选项

- `-Mtime_d` — 以 Unix 纪元秒表示的输入时间（长格式 `--epoch-time=time_d`）；输出日历时间。
- `-Tyear/month/day/hour/minute/second` — 以日历时间表示的输入时间（长格式 `--calendar-time=...`）；输出纪元秒。
- `-V` — 详细模式（长格式 `--verbose`）：同时显示程序名和 MB-System 版本。
- `-H` — 显示简要说明和用法后退出（长格式 `--help`）。

## 示例

将 2008 年 6 月 6 日 18:37:14 转换为纪元秒（输出 `1212777434.000000`）：

    mbtime -T2008/06/06/18/37/14.0

将纪元时间 1212777434.0 转换为日历时间（输出 `2008/06/06/18/37/14.000000`）：

    mbtime -M1212777434.0

使用长格式选项转换带小数秒的日历时间：

    mbtime --calendar-time=2025/01/01/12/30/05.250

## 注意

- 月和日是日历上的月份和日期，而不是年积日。
- 选项也可以用小写字母（`-m`、`-t`、`-v`、`-h`）。

## 另请参阅

`mbinfo`、`mblist`、`mbnavlist`、`mbmapscale`。完整参考请见 MB-System 手册页 mbtime(1)（英文）。
