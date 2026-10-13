# mbabsorption — Calculate sound absorption in sea water (dB/km)

`mbabsorption` calculates the absorption of sound in sea water in dB/km as a function of frequency, temperature, salinity, depth, pH and sound speed. Use it to obtain the absorption coefficient to enter in sonar acquisition settings, or to check the value used when correcting backscatter amplitudes for a given sonar frequency and water mass.

## Synopsis

    mbabsorption
        --depth=depth {-Ddepth}
        --frequency=frequency {-Ffrequency}
        --help {-H}
        --ph=ph {-Pph}
        --salinity=salinity {-Ssalinity}
        --sound-speed=soundspeed {-Csoundspeed}
        --temperature=temperature {-Ttemperature}
        --verbose {-V}

## Description

The calculation uses the Francois and Garrison (1982) equations, which model absorption as the sum of three contributions: boric acid, magnesium sulfate and pure water. The boric acid contribution is significant below about 10 kHz; at typical multibeam frequencies the magnesium sulfate and pure water terms dominate.

All inputs have defaults, so only the values that differ need to be given: frequency 200 kHz, temperature 10 degrees C, salinity 35 per mil, depth 0 m (sea surface) and pH 8. If the sound speed is not given, it is estimated from temperature, salinity and depth as approximately 1412 + 3.21 T + 1.19 S + 0.0167 D (m/s).

Normally the program prints only the absorption value in dB/km. With `-V` it prints the input parameters, the absorption and, in addition, the sea water density (kg/m3) computed from salinity, temperature and pressure.

## Common options

- `-Ffrequency` — Sound frequency in kHz (long form `--frequency=frequency`). Default: 200.
- `-Ttemperature` — Water temperature in degrees C (long form `--temperature=temperature`). Default: 10.0.
- `-Ssalinity` — Salinity in per mil (long form `--salinity=salinity`). Default: 35.
- `-Ddepth` — Depth in meters at which the absorption is calculated (long form `--depth=depth`). Default: 0.0.
- `-Pph` — pH of the water (long form `--ph=ph`). Default: 8.
- `-Csoundspeed` — Water sound speed in m/s (long form `--sound-speed=soundspeed`). If not given, it is calculated from temperature, salinity and depth.
- `-V` — Verbose mode (long form `--verbose`): prints the parameters used, the result with units and the water density.
- `-H` — Prints a short description and the usage, then exits (long form `--help`).

## Examples

Absorption for a 200 kHz multibeam operating at 1000 m depth (for example on an AUV) in water of 35 per mil salinity, pH 8 and 4 degrees C (prints a value of about `39.77`):

    mbabsorption -D1000 -F200 -P8 -S35 -T4

The same calculation with a readable listing of the parameters and result:

    mbabsorption -D1000 -F200 -P8 -S35 -T4 -V

Absorption at the surface for a 12 kHz deep-water multibeam in warm tropical water, with a measured sound speed of 1540 m/s:

    mbabsorption -F12 -T27 -S35 -C1540

## Notes

- Units: frequency in kHz (not Hz), depth in meters, temperature in degrees C, salinity in per mil (practical salinity), sound speed in m/s.
- Options may also be given in lower case (`-f`, `-t`, `-s`, `-d`, `-p`, `-c`, `-v`, `-h`).

## See also

`mblevitus`, `mbbackangle`, `mbsvplist`. The complete reference is the MB-System manual page mbabsorption(1) (English).
