# mbabsorption — 计算海水中的声吸收（dB/km）

`mbabsorption` 以频率、温度、盐度、深度、pH 和声速为参数，计算海水中的声吸收，单位为 dB/km。可用于求取在声呐采集设置中输入的吸收系数，或针对特定声呐频率和水团核查用于反向散射强度校正的数值。

## 语法

    mbabsorption
        --depth=depth {-Ddepth}
        --frequency=frequency {-Ffrequency}
        --help {-H}
        --ph=ph {-Pph}
        --salinity=salinity {-Ssalinity}
        --sound-speed=soundspeed {-Csoundspeed}
        --temperature=temperature {-Ttemperature}
        --verbose {-V}

## 说明

计算采用 Francois 和 Garrison（1982）的公式，将吸收建模为硼酸、硫酸镁和纯水三部分贡献之和。硼酸的贡献在约 10 kHz 以下较为显著；在常见的多波束频率下，硫酸镁和纯水项占主导。

所有输入均有默认值，只需给出不同的数值：频率 200 kHz、温度 10 ℃、盐度 35‰、深度 0 m（海面）、pH 8。未指定声速时，由温度、盐度和深度按约 1412 + 3.21 T + 1.19 S + 0.0167 D（m/s）估算。

通常程序只输出吸收值（dB/km）。使用 `-V` 时，会输出输入参数和吸收值，另外还给出由盐度、温度和压力计算的海水密度（kg/m3）。

## 常用选项

- `-Ffrequency` — 声波频率（kHz）（长格式 `--frequency=frequency`）。默认值：200。
- `-Ttemperature` — 水温（℃）（长格式 `--temperature=temperature`）。默认值：10.0。
- `-Ssalinity` — 盐度（‰）（长格式 `--salinity=salinity`）。默认值：35。
- `-Ddepth` — 计算吸收的深度（m）（长格式 `--depth=depth`）。默认值：0.0。
- `-Pph` — 海水 pH 值（长格式 `--ph=ph`）。默认值：8。
- `-Csoundspeed` — 水中声速（m/s）（长格式 `--sound-speed=soundspeed`）。未指定时由温度、盐度和深度计算。
- `-V` — 详细模式（长格式 `--verbose`）：显示所用参数、带单位的结果以及海水密度。
- `-H` — 显示简要说明和用法后退出（长格式 `--help`）。

## 示例

计算在盐度 35‰、pH 8、水温 4 ℃ 的海水中、于 1000 m 深度工作（例如搭载在 AUV 上）的 200 kHz 多波束的吸收（输出约为 `39.77`）：

    mbabsorption -D1000 -F200 -P8 -S35 -T4

进行相同计算，并以易读的形式列出参数和结果：

    mbabsorption -D1000 -F200 -P8 -S35 -T4 -V

在温暖的热带海域海面，针对 12 kHz 深水多波束，使用实测声速 1540 m/s 计算吸收：

    mbabsorption -F12 -T27 -S35 -C1540

## 注意

- 单位：频率为 kHz（不是 Hz），深度为米，温度为 ℃，盐度为‰（实用盐度），声速为 m/s。
- 选项也可以用小写字母（`-f`、`-t`、`-s`、`-d`、`-p`、`-c`、`-v`、`-h`）。

## 另请参阅

`mblevitus`、`mbbackangle`、`mbsvplist`。完整参考请见 MB-System 手册页 mbabsorption(1)（英文）。
