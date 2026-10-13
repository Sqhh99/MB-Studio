# mbset — create and modify mbprocess parameter files

`mbset` creates or modifies the `.par` parameter files that control `mbprocess`. Use it to tell `mbprocess` which SVP, navigation, tide or edit files to use and which corrections (roll bias, draft, heading offset, ...) to apply, either for a single swath file or for every file of a datalist at once.

## Synopsis

    mbset
        --explicit {-E}
        --format=format_id {-Fformat_id}
        --help {-H}
        --input=file {-Ifile}
        --look-for-files {-L}
        --parameter=PARAMETER:value {-PPARAMETER:value}
        --remove-nav-adjust {-N}
        --verbose {-V}

## Description

Each swath data file can have an `mbprocess` parameter file with the same name plus a `.par` suffix (for `survey.mb58` it is `survey.mb58.par`). The file contains one command per line, for example `SVPFILE`, `ROLLBIAS` or `EDITSAVEFILE`. `mbset` reads the existing parameter file, or starts from default parameters if there is none, changes the parameters given with `-P`, and writes the file back.

The `-P` option sets one parameter and can be repeated as often as needed. If the input is a datalist, the same changes are applied to the parameter files of all swath files referenced by it (recursively), which is the usual way to set uniform processing parameters for a whole survey.

Unless `-E` is given, setting a file or value also switches on the corresponding processing mode. For example `-PSVPFILE:best.svp` also sets `SVPMODE` to 1 so that the SVP is actually used, `-PROLLBIAS:0.5` sets `ROLLBIASMODE` to 1, `-PDRAFT:1.95` sets `DRAFTMODE` to 4 (constant draft), `-PTIDEFILE:...` turns tide correction on and `-PNAVADJFILE:...` turns on merging of adjusted navigation and depth offsets (`NAVADJMODE` 2).

Commonly used parameter commands (see the manual page for the complete list and the meaning of each mode):

- Navigation: `NAVMODE`, `NAVFILE`, `NAVFORMAT`, `NAVHEADING`, `NAVSPEED`, `NAVDRAFT`, `NAVATTITUDE`, `NAVINTERP`, `NAVTIMESHIFT`, `NAVSHIFT`, `NAVOFFSETX`, `NAVOFFSETY`, `NAVSHIFTLON`, `NAVSHIFTLAT`, `NAVADJMODE`, `NAVADJFILE`.
- Attitude and sonar depth: `ATTITUDEMODE`, `ATTITUDEFILE`, `ATTITUDEFORMAT`, `SONARDEPTHMODE`, `SONARDEPTHFILE`, `SONARDEPTHFORMAT`.
- Editing and cutting: `EDITSAVEMODE`, `EDITSAVEFILE`, `DATACUTCLEAR`, `DATACUT`, `BATHCUTNUMBER`, `BATHCUTDISTANCE`, `BATHCUTSPEED`, `AMPCUTNUMBER`, `SSCUTNUMBER`, ...
- Bathymetry recalculation: `SVPMODE`, `SVPFILE`, `SSVMODE`, `SSV`, `TTMULTIPLY`, `ANGLEMODE`, `SOUNDSPEEDREF`, `STATICMODE`, `STATICFILE`.
- Corrections: `DRAFTMODE`, `DRAFT`, `DRAFTOFFSET`, `DRAFTMULTIPLY`, `HEAVEMODE`, `HEAVEOFFSET`, `LEVERMODE`, `ROLLBIASMODE`, `ROLLBIAS`, `ROLLBIASPORT`, `ROLLBIASSTBD`, `PITCHBIASMODE`, `PITCHBIAS`, `HEADINGMODE`, `HEADINGOFFSET`, `TIDEMODE`, `TIDEFILE`, `TIDEFORMAT`.
- Amplitude and sidescan: `AMPCORRMODE`, `AMPCORRFILE`, `AMPCORRSYMMETRY`, `AMPCORRANGLE`, `AMPCORRSLOPE`, `SSCORRMODE`, `SSCORRFILE`, `SSCORRSYMMETRY`, `SSCORRANGLE`, `SSCORRSLOPE`, `AMPSSCORRTOPOFILE`, `SSRECALCMODE`.
- General and metadata: `FORMAT`, `OUTFILE`, `METAVESSEL`, `METAINSTITUTION`, `METAPLATFORM`, `METASONAR`, `METACRUISEID`, `METACRUISENAME`, `METAPI`, `METACLIENT`, ...

## Common options

- `-I<file>` / `--input=<file>` — swath file or datalist whose parameter files are created or updated. Default: `datalist.mb-1`.
- `-P<PARAMETER>:<value>` / `--parameter=<PARAMETER>:<value>` — set one parameter; repeat for several. The separator may be `:` or `=`; only the first one counts, so the value may itself contain `:` (for example a Windows drive letter).
- `-E` / `--explicit` — do not switch processing modes on implicitly; every mode must be set explicitly with `-P`.
- `-L` / `--look-for-files` — look for an edit save file and navigation files with the standard names next to each swath file and, if found, set the parameters to use them.
- `-N` / `--remove-nav-adjust` — remove the reference to an `mbnavadjust` adjusted navigation file: the referenced file is deleted and `NAVADJMODE`/`NAVADJFILE` are reset.
- `-F<format_id>` / `--format=<format_id>` — MBIO format id of the input, overriding detection from the file name.
- `-V` / `--verbose` — list the parameters written to each parameter file.
- `-H` / `--help` — print a short help message and exit.

## Examples

Use an SVP file, a roll bias of +0.5 degrees and a constant draft of 1.95 m for one file:

    mbset -I0051_20010829_223755.mb57 -PSVPFILE:0051_20010829_223755.mb57.svp -PROLLBIAS:0.5 -PDRAFT:1.95 -V

Apply the same tide file (format 2: year month day hour minute second tide) to every file of the survey:

    mbset -Idatalist.mb-1 -PTIDEFILE:survey.tide -PTIDEFORMAT:2

Apply an edit save file that was extracted with `mbgetesf`:

    mbset -Ihs_ew9103_143.mb24 -PEDITSAVEMODE:1 -PEDITSAVEFILE:hs_ew9103_143.mb24.esf

Then process the data:

    mbprocess -Idatalist.mb-1

## Notes

- `mbset` only writes parameter files; nothing changes in the processed data until `mbprocess` is run.
- A file name value is read up to the first space, so file names used in parameters must not contain spaces. Relative names are taken relative to the folder of the swath file, so ancillary files are usually kept next to the data.
- Parameter names must be written in capital letters.

## See also

`mbprocess`, `mbclean`, `mbsvplist`, `mbbackangle`, `mbgetesf`, `mbotps`, `mbdatalist`. The complete reference is the MB-System manual page mbset(1) (English).
