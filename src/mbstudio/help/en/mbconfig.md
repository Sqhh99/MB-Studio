# mbconfig — Report the MB-System version and installation settings

`mbconfig` prints information about the installed MB-System: its version (in several forms), the installation prefix, the compiler and linker flags for building programs against the MB-System libraries, and the configured locations of the Levitus sound speed database and the OTPS tide prediction software. Use it to check which MB-System version is installed, for example in a processing log or a batch file.

## Synopsis

    mbconfig --verbose --help --prefix --cflags --libs
             --version --version-id --version-major --version-minor --version-archive
             --levitus --otps

## Description

Each requested value is written to standard output on its own line, with no other text, so the output can be captured easily. If no option is given, `mbconfig` prints the version string, as if `--version` had been given.

Several options can be combined. The values are always printed in a fixed order (prefix, compile flags, link flags, version, version id, major, minor and archive version, Levitus location, OTPS location), whatever the order of the options on the command line.

With `--verbose`, each value is preceded by a descriptive line starting with `#` (for example `# MB-System version:`), and the program name and version are also written to standard error.

The version id is an integer that always increases with newer releases: version_id = 10000000 x major + 100000 x minor + archive, so version 5.5.2304 has the id 50502304. It is convenient for comparing versions in scripts.

The prefix, flags and data locations are values fixed when MB-System was built. They describe the build configuration and need not match the folder where MB-Studio is installed (see Notes).

## Common options

- `--version` — Print the MB-System version string (the default when no option is given).
- `--version-id` — Print the version as a single integer (see Description).
- `--version-major` — Print the major version number (the first of the three numbers).
- `--version-minor` — Print the minor version number (the second number).
- `--version-archive` — Print the archive (revision) number (the third number).
- `--prefix` — Print the installation prefix configured at build time.
- `--cflags` — Print the compiler flag that locates the MB-System header files.
- `--libs` — Print the linker flags for the MB-System libraries.
- `--levitus` — Print the configured path of the Levitus annual sound speed database used by `mblevitus`.
- `--otps` — Print the configured location of the OTPS tide prediction software used by `mbotps`.
- `--verbose` — Precede each value with a descriptive `#` line. Give it twice or more for debugging output.
- `--help` — Print a short description and the usage on standard error.

## Examples

Print the installed MB-System version:

    mbconfig

Print the version with descriptive labels, together with the version id:

    mbconfig --version --version-id --verbose

Save the version to a text file, for example as part of a processing record:

    mbconfig --version > mbsystem_version.txt

## Notes

- Only long options (starting with `--`) are accepted; an unknown option prints the usage and ends with an error.
- `--prefix`, `--levitus` and `--otps` report paths fixed at build time. In the Windows build `mblevitus` finds its database relative to its own program folder (`share\mbsystem` in the installation folder) or in the folder given by the environment variable `MBSYSTEM_SHAREDIR`, so these paths may differ from the files actually used.
- `--cflags` and `--libs` are only of interest to developers compiling their own programs against the MB-System libraries.

## See also

`mbdefaults`, `mblevitus`, `mbotps`, `mbformat`. The complete reference is the MB-System manual page mbconfig(1) (English).
