# mbmakeplatform — 创建或修改平台（传感器偏移）文件

`mbmakeplatform` 用于创建或修改 MB-System 平台文件（`.plf`）。平台文件是一个文本文件，描述测量平台上的各个传感器以及它们之间的位置偏移和角度偏移。当原始数据中没有完整、正确的传感器几何关系时（常见于 AUV、ROV 或拖体上的定制系统），`mbpreprocess` 通过选项 `--platform-file` 使用平台文件。

## 语法

    mbmakeplatform
        --output=plffile
        [
        --verbose
        --help
        --input=plffile
        --swath=datalist
        --swath=swathfile
        --swath-format=value
        --platform-type-surface-vessel | --platform-type-tow-body | --platform-type-rov
        --platform-type-auv | --platform-type-aircraft | --platform-type-satellite
        --platform-name=string
        --platform-organization=string
        --platform-documentation-url=string
        --platform-start-time=yyyy/mm/dd/hh/mm/ss.ssssss
        --platform-end-time=yyyy/mm/dd/hh/mm/ss.ssssss
        --add-sensor-<type>
        --modify-sensor=sensorid
        --modify-sensor-<source>
        --sensor-model=string
        --sensor-manufacturer=string
        --sensor-serialnumber=string
        --sensor-capability-<capability>
        --sensor-capability1=value
        --sensor-capability2=value
        --sensor-offsets=x/y/z/azimuth/roll/pitch
        --sensor-offset-positions=x/y/z
        --sensor-offset-angles=azimuth/roll/pitch
        --sensor-time-latency=value
        --sensor-time-latency-model=file
        --sensor-flipsign-heading
        --sensor-flipsign-roll
        --sensor-flipsign-pitch
        --sensor-source-<source>
        --modify-offsets=ioff/x/y/z/azimuth/roll/pitch
        --modify-offset-positions=ioff/x/y/z
        --modify-offset-angles=ioff/azimuth/roll/pitch
        --modify-time-latency=value
        --modify-time-latency-model=file
        --end-sensor
        --set-source-<source>=sensorid
        ]

其中 `<source>` 为 `bathymetry`、`backscatter`、`subbottom`、`camera`、`position`、`depth`、`heading`、`rollpitch` 或 `heave` 之一，后面还可以加 `1`、`2` 或 `3`（例如 `bathymetry2`）。

## 说明

平台文件列出平台上的所有传感器（类型、型号、制造商、序列号和测量能力）、每个传感器的位置偏移和角度偏移、可选的时间延迟，以及每类数据（水深、后向散射、位置、深度、艏向、横摇纵摇、升沉等）由哪个传感器提供。通常以定位传感器作为平台原点，其偏移为零。

平台文件通常由一条很长的 `mbmakeplatform` 命令一次生成。选项按给出的顺序处理：`--add-sensor-...` 开始定义一个新传感器，随后的 `--sensor-...` 选项描述该传感器，`--end-sensor` 结束定义。同样，`--modify-sensor=sensorid`（传感器编号从 0 开始）或 `--modify-sensor-<source>` 打开一个已有传感器进行修改，直到 `--end-sensor`。

偏移的符号约定：x 向右舷为正，y 向前为正，z 向上为正；横摇以右舷抬起为正，纵摇以船首抬起为正，方位角（azimuth）从正前方顺时针为正。对于有两个单元的传感器（如发射阵和接收阵分离的多波束，或双相机立体相机），需给出两次 `--sensor-offsets`，第一组为发射阵（或左相机）。

使用 `--input` 时先读入已有的平台文件，再由后续选项修改。使用 `--swath` 时程序尝试从条带数据中提取初始平台（传感器列表和偏移），这只适用于较新的 Kongsberg 和 Teledyne 格式（例如格式 58、59、88 和 89）。如果指定了输出文件但没有定义任何传感器，则写出一个偏移为零、作为所有数据来源的通用传感器。

如果记录的数据中已经包含完整的平台描述，则不需要平台文件；例如 Kongsberg 多波束数据通常不需要。

## 常用选项

- `--output=plffile` — 要写出的平台文件。没有此选项时不写出任何文件。
- `--input=plffile` — 读入已有的平台文件，并用后续选项修改。
- `--swath=datalist` 或 `--swath=swathfile` — 用条带数据中找到的第一个平台定义进行初始化（仅限较新的 Kongsberg/Teledyne 格式）。
- `--swath-format=value` — `--swath` 输入的格式，仅在无法从文件名识别时需要，须放在 `--swath` 之前。
- `--platform-type-surface-vessel`、`--platform-type-tow-body`、`--platform-type-rov`、`--platform-type-auv`、`--platform-type-aircraft`、`--platform-type-satellite` — 设置平台类型。
- `--platform-name=string`、`--platform-organization=string` — 平台名称和运营单位。
- `--add-sensor-<type>` — 开始定义新传感器。类型：`sonar-echosounder`、`sonar-multiechosounder`、`sonar-sidescan`、`sonar-interferometry`、`sonar-multibeam`、`sonar-multibeam-twohead`、`sonar-subbottom`、`camera-mono`、`camera-stereo`、`camera-video`、`lidar-scan`、`lidar-swath`、`position`、`compass`、`vru`、`imu`、`ins`、`ins-with-pressure`、`ctd`、`pressure`、`soundspeed`。
- `--sensor-model=string`、`--sensor-manufacturer=string`、`--sensor-serialnumber=string` — 当前传感器的型号、制造商和序列号。
- `--sensor-capability-<capability>` — 为当前传感器添加测量能力，例如 `position`、`depth`、`heading`、`rollpitch`、`soundspeed`、`topography-multibeam`、`backscatter-multibeam`、`backscatter-sidescan`、`topography-lidar`、`stereophotography`。也可以用 `--sensor-capability1=value` 和 `--sensor-capability2=value` 直接设置两个能力位掩码。
- `--sensor-offsets=x/y/z/azimuth/roll/pitch` — 当前传感器的位置偏移（米）和角度偏移（度）。与姿态无关的传感器（压力计、CTD、声速计）使用 `--sensor-offset-positions=x/y/z`，只测姿态的传感器（VRU）使用 `--sensor-offset-angles=azimuth/roll/pitch`。
- `--sensor-time-latency=value` — 当前传感器的固定时间延迟（秒）。`--sensor-time-latency-model=file` 则读入时间序列（两列：历元秒和延迟秒）。
- `--sensor-flipsign-heading`、`--sensor-flipsign-roll`、`--sensor-flipsign-pitch` — 指明该传感器的艏向、横摇或纵摇数值符号相反。
- `--sensor-source-<source>` — 将当前传感器设为该类数据的来源，例如 `--sensor-source-bathymetry`、`--sensor-source-position`、`--sensor-source-heave`。
- `--end-sensor` — 结束当前传感器的定义或修改。
- `--modify-sensor=sensorid` — 开始修改已有传感器 `sensorid`（从 0 开始计数）；`--modify-sensor-bathymetry`、`--modify-sensor-position` 等选择当前作为该类数据来源的传感器。
- `--modify-offsets=ioff/x/y/z/azimuth/roll/pitch` — 修改正在编辑的传感器的第 `ioff` 组偏移（0 或 1）；`--modify-offset-positions=ioff/x/y/z` 和 `--modify-offset-angles=ioff/azimuth/roll/pitch` 只修改位置或角度。`--modify-time-latency=value` 和 `--modify-time-latency-model=file` 修改时间延迟。
- `--set-source-<source>=sensorid` — 不打开传感器，直接将传感器 `sensorid` 设为该类数据的来源。
- `--verbose` — 显示读入和写出的平台信息。`--help` 显示用法。

## 示例

为一艘测量船创建平台文件：以 INS 作为平台原点，并配有发射阵和接收阵分离的多波束：

    mbmakeplatform --output=vessel.plf --platform-type-surface-vessel --platform-name="Survey Vessel" --add-sensor-ins --sensor-capability-position --sensor-capability-heading --sensor-capability-rollpitch --sensor-offsets=0/0/0/0/0/0 --sensor-source-position --sensor-source-heading --sensor-source-rollpitch --sensor-source-heave --end-sensor --add-sensor-sonar-multibeam --sensor-model="T50" --sensor-manufacturer="Reson" --sensor-capability-topography-multibeam --sensor-capability-backscatter-multibeam --sensor-offsets=0.0/1.20/-2.50/0.0/0.0/0.0 --sensor-offsets=0.0/1.40/-2.50/0.0/0.0/0.0 --sensor-source-bathymetry --sensor-source-backscatter --end-sensor

修正已有平台文件中传感器 1 的接收阵偏移（偏移组 1），并写出新文件：

    mbmakeplatform --input=vessel.plf --output=vessel_v2.plf --modify-sensor=1 --modify-offsets=1/0.0/1.45/-2.48/0.0/0.10/0.0 --end-sensor

根据 Teledyne 7k 数据中记录的传感器几何关系生成平台文件：

    mbmakeplatform --swath=0001_20250101_000000.mb88 --output=rov.plf --verbose

在预处理时使用平台文件：

    mbpreprocess --input=datalist.mb-1 --platform-file=vessel.plf

## 注意

- 在命令提示符中整条命令必须写在一行。较长的平台定义可以写入批处理文件（`.bat`），在批处理文件中行尾加 `^` 即可续行。
- 选项按顺序生效：`--input` 或 `--swath` 应放在最前面，`--swath-format` 应放在 `--swath` 之前。
- 含空格的值需加引号，例如 `--platform-name="Survey Vessel"`。
- 在当前源代码中，`--platform-start-time` 和 `--platform-end-time` 按 `yyyy/mm/dd hh:mm:ss.ssssss` 读取时间（日期与时间之间为空格，因此需给值加引号）。

## 另请参阅

`mbpreprocess`、`mbprocess`、`mbinfo`、`mbsslayout`。完整参考见 MB-System 手册页 mbmakeplatform(1)（英文）。
