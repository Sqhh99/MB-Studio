# mbmakeplatform — Create or modify an MB-System platform (sensor offset) file

`mbmakeplatform` creates or modifies an MB-System platform file (`.plf`), a text file that describes the sensors on a survey platform and the positional and angular offsets between them. Platform files are used by `mbpreprocess` (option `--platform-file`) when the raw data do not already carry a complete, correct sensor geometry, typically for custom systems on AUVs, ROVs or towed bodies.

## Synopsis

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

Here `<source>` is one of `bathymetry`, `backscatter`, `subbottom`, `camera`, `position`, `depth`, `heading`, `rollpitch` or `heave`, optionally followed by `1`, `2` or `3` (for example `bathymetry2`).

## Description

A platform file lists every sensor on the platform (its type, model, manufacturer, serial number and measurement capabilities), the positional and angular offsets of each sensor, optional time latencies, and which sensor is the source of each kind of data (bathymetry, backscatter, position, depth, heading, roll and pitch, heave, ...). Usually the position sensor is taken as the platform origin and given zero offsets.

A platform file is normally built with a single, long `mbmakeplatform` command. The options are processed in the order given: an `--add-sensor-...` option starts a new sensor, the following `--sensor-...` options describe that sensor, and `--end-sensor` closes it. In the same way `--modify-sensor=sensorid` (sensor ids count from 0) or `--modify-sensor-<source>` opens an existing sensor for changes until `--end-sensor`.

Offsets use x positive to starboard, y positive forward and z positive up; roll is positive starboard up, pitch positive forward up and azimuth positive clockwise from forward. Sensors with two elements (a multibeam with separate transmit and receive arrays, or a stereo camera rig) get two `--sensor-offsets` options: the first is the transmit array (or the left camera).

With `--input` an existing platform file is read and then changed by the following options. With `--swath` the program tries to extract a starting platform (sensor list and offsets) from swath data; this only works for recent Kongsberg and Teledyne formats (e.g. formats 58, 59, 88 and 89). If an output file is requested but no sensors were defined, a generic single sensor with zero offsets that is the source of all data is written.

No platform file is needed when the logged data already contain a full platform description; for example, Kongsberg multibeam data normally do not require one.

## Common options

- `--output=plffile` — Platform file to write. Nothing is written without this option.
- `--input=plffile` — Read an existing platform file and modify it with the following options.
- `--swath=datalist` or `--swath=swathfile` — Initialize the platform from the first platform definition found in the swath data (recent Kongsberg/Teledyne formats only).
- `--swath-format=value` — Format of the `--swath` input; needed only if it cannot be recognized from the file name. Give it before `--swath`.
- `--platform-type-surface-vessel`, `--platform-type-tow-body`, `--platform-type-rov`, `--platform-type-auv`, `--platform-type-aircraft`, `--platform-type-satellite` — Set the platform type.
- `--platform-name=string`, `--platform-organization=string` — Descriptive platform name and operating organization.
- `--add-sensor-<type>` — Start a new sensor. Types: `sonar-echosounder`, `sonar-multiechosounder`, `sonar-sidescan`, `sonar-interferometry`, `sonar-multibeam`, `sonar-multibeam-twohead`, `sonar-subbottom`, `camera-mono`, `camera-stereo`, `camera-video`, `lidar-scan`, `lidar-swath`, `position`, `compass`, `vru`, `imu`, `ins`, `ins-with-pressure`, `ctd`, `pressure`, `soundspeed`.
- `--sensor-model=string`, `--sensor-manufacturer=string`, `--sensor-serialnumber=string` — Describe the current sensor.
- `--sensor-capability-<capability>` — Add a measurement capability to the current sensor, e.g. `position`, `depth`, `heading`, `rollpitch`, `soundspeed`, `topography-multibeam`, `backscatter-multibeam`, `backscatter-sidescan`, `topography-lidar`, `stereophotography`. `--sensor-capability1=value` and `--sensor-capability2=value` set the two capability bitmasks directly.
- `--sensor-offsets=x/y/z/azimuth/roll/pitch` — Positional (m) and angular (degrees) offsets of the current sensor. Use `--sensor-offset-positions=x/y/z` for sensors without orientation (pressure, CTD, sound speed) and `--sensor-offset-angles=azimuth/roll/pitch` for orientation-only sensors (VRU).
- `--sensor-time-latency=value` — Static time latency of the current sensor in seconds. `--sensor-time-latency-model=file` reads a time series instead (two columns: epoch seconds and latency in seconds).
- `--sensor-flipsign-heading`, `--sensor-flipsign-roll`, `--sensor-flipsign-pitch` — Mark the sign of the sensor's heading, roll or pitch values as reversed.
- `--sensor-source-<source>` — Make the current sensor the source of that data type, e.g. `--sensor-source-bathymetry`, `--sensor-source-position`, `--sensor-source-heave`.
- `--end-sensor` — End the definition or modification of the current sensor.
- `--modify-sensor=sensorid` — Start modifying existing sensor `sensorid` (counting from 0); `--modify-sensor-bathymetry`, `--modify-sensor-position`, ... select the sensor that is currently the source for that data type.
- `--modify-offsets=ioff/x/y/z/azimuth/roll/pitch` — Change offset set `ioff` (0 or 1) of the sensor being modified; `--modify-offset-positions=ioff/x/y/z` and `--modify-offset-angles=ioff/azimuth/roll/pitch` change only the positions or angles. `--modify-time-latency=value` and `--modify-time-latency-model=file` change the latency.
- `--set-source-<source>=sensorid` — Make sensor `sensorid` the source of a data type without opening it.
- `--verbose` — Print the platform read and written. `--help` prints the usage.

## Examples

Create a platform file for a surface vessel with an INS as the platform origin and a multibeam with separate transmit and receive arrays:

    mbmakeplatform --output=vessel.plf --platform-type-surface-vessel --platform-name="Survey Vessel" --add-sensor-ins --sensor-capability-position --sensor-capability-heading --sensor-capability-rollpitch --sensor-offsets=0/0/0/0/0/0 --sensor-source-position --sensor-source-heading --sensor-source-rollpitch --sensor-source-heave --end-sensor --add-sensor-sonar-multibeam --sensor-model="T50" --sensor-manufacturer="Reson" --sensor-capability-topography-multibeam --sensor-capability-backscatter-multibeam --sensor-offsets=0.0/1.20/-2.50/0.0/0.0/0.0 --sensor-offsets=0.0/1.40/-2.50/0.0/0.0/0.0 --sensor-source-bathymetry --sensor-source-backscatter --end-sensor

Correct the receive array offsets (offset set 1) of sensor 1 in an existing platform file and write a new file:

    mbmakeplatform --input=vessel.plf --output=vessel_v2.plf --modify-sensor=1 --modify-offsets=1/0.0/1.45/-2.48/0.0/0.10/0.0 --end-sensor

Start a platform file from the sensor geometry stored in Teledyne 7k data:

    mbmakeplatform --swath=0001_20250101_000000.mb88 --output=rov.plf --verbose

Use the platform file when preprocessing the survey:

    mbpreprocess --input=datalist.mb-1 --platform-file=vessel.plf

## Notes

- The whole command must be on one line in the command prompt. For long platform definitions it is convenient to put the command in a batch file (`.bat`), where a line can be continued with `^` at its end.
- Options are applied in order: give `--input` or `--swath` first, and `--swath-format` before `--swath`.
- Quote values that contain spaces, e.g. `--platform-name="Survey Vessel"`.
- In the current source, `--platform-start-time` and `--platform-end-time` read the time as `yyyy/mm/dd hh:mm:ss.ssssss` (date and time separated by a space, so quote the value).

## See also

`mbpreprocess`, `mbprocess`, `mbinfo`, `mbsslayout`. The complete reference is the MB-System manual page mbmakeplatform(1) (English).
