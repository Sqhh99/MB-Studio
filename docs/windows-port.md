# Notes on the native Windows port

MB-System was written for POSIX systems. MB-Studio builds it with the
MinGW-w64 toolchain, which provides most POSIX interfaces (`getopt`,
`unistd.h`, `strings.h`, ...) on top of the Microsoft C run time, and adapts
the remaining differences. Everything below is covered by tests that run on
Windows in CI (`test/platform`, `src/mbstudio/tests`).

## Process start-up (`src/win32`)

Every executable built for Windows gets a small start-up object linked in
automatically (`cmake/MBWindows.cmake`):

- **Binary standard streams.** `stdout` and `stderr` are switched to binary
  mode so that the programs emit exactly the same bytes as on Linux and macOS
  (`\n` line endings, no `^Z` handling). Output piped between programs and
  compared by the test suite is identical on all platforms.
- **Data locations.** `PROJ_DATA`, `GMT_SHAREDIR` and `GDAL_DATA` default to
  the `share` directory of the installation when they are not set.
- **Application manifest.** UTF-8 active code page (file names with Chinese or
  other non-ASCII characters work in every program), long path awareness,
  Windows 10/11 compatibility, `asInvoker`.
- **Stack size.** Some programs keep several megabytes of arrays on the stack;
  Windows reserves only 1 MB by default, so executables reserve 64 MB like on
  Linux.

## Library and program changes

| Area | Change |
|---|---|
| Home directory | `mb_get_home_dir()`: `HOME`, else `USERPROFILE`, else `HOMEDRIVE`+`HOMEPATH` (for `~/.mbio_defaults`) |
| User and host name | `USERNAME` fallback; computer name from `GetComputerNameA()` (Winsock is not initialised) |
| Paths in datalists | Drive letters and backslashes are recognised as absolute paths; relative entries are resolved against the directory of the list on any drive |
| Paths with spaces | Option arguments are copied whole (`mb_copy_arg()`) instead of with `sscanf("%s")`, so `C:\Users\First Last\...` works |
| XDR | The Windows XDR replacement pads opaque data to four bytes as required by RFC 4506 (the SURF and BSIO formats), and is usable from C++ |
| `system()` calls | `mb_system()` puts the program directory on `PATH` for chained programs (`mbinfo`, `gmt`, ...) and skips the `mbm_*` plot macros, which are Perl/shell scripts |
| Data files | `mblevitus` locates `share\mbsystem` relative to the executable; `MBSYSTEM_SHAREDIR` overrides it |
| Large files | 64-bit `off_t`, `fseeko` and `ftello` (`_FILE_OFFSET_BITS=64`) |
| `printf` | C99 conforming formatting (`__USE_MINGW_ANSI_STDIO`), so numbers print exactly as on Linux |
| Installation | DLLs go to `bin`; the MB-System GMT supplement goes to `bin\gmt_plugins`, where the bundled `gmt.exe` finds it (`gmt mbswath`, `gmt mbcontour`, ...) |

## Not available on Windows

- The X11/Motif graphical tools (`mbedit`, `mbnavedit`, `mbgrdviz`, ...): MB-Studio
  is the graphical front end on Windows.
- The `mbm_*` plotting macros (Perl and shell scripts).
- Photomosaicing (OpenCV) and terrain relative navigation (TRN) programs.

## Tests

`ctest` runs on Windows:

- the MB-System test suite (`test/mbio`, `test/utilities`, `test/gsf`, ...),
  driven by Python with the Windows run-time environment (`cmake/MBTesting.cmake`);
- the platform adaptation tests in `test/platform`: binary I/O, 64-bit file
  offsets, UTF-8 file names, home directory fallbacks, XDR byte order and
  padding, DLL data exports, Windows path syntax, the start-up shim, the
  manifest, the stack reserve, and end-to-end checks of the command line
  programs with spaces and Chinese characters in paths;
- the MB-Studio kernel, platform layer, MB-System plugin, user interface and
  host tests in `src/mbstudio/tests`.
