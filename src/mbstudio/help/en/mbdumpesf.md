# mbdumpesf — Dump or filter the beam edits in an edit save (.esf) file

`mbdumpesf` reads an MB-System edit save file (`.esf`), which holds the beam flags created by `mbclean`, `mbareaclean` or interactive editing, and lists every edit event as text. It can also copy the edits to a new binary `.esf` file while leaving out selected kinds of edits. Use it to inspect what has been flagged in a file, or to strip particular edit types before running `mbprocess`.

## Synopsis

    mbdumpesf --input=esffile
        [--output=esffile --ignore-unflag --ignore-flag
        --ignore-filter --ignore-zero
        --verbose --help]

## Description

In MB-System, beam flags are not written into the raw swath file. They are stored as edit events in a binary "edit save file" named after the swath file with `.esf` appended (for example `0001_20250101_000000.all.esf`) and applied later by `mbprocess`. Each event holds a ping time stamp, a beam number and an action:

    action=1: flag manually
    action=2: unflag
    action=3: zero - make beam null
    action=4: flag by autofilter

Without `--output`, each event is written to standard output as one line, for example `EDITS READ: i:0 time: 947562291.799000 2008/04/22 22:26:36.799000 beam:21 action:1`, where the time is given both in epoch seconds (seconds since 1970-01-01) and as date and time.

With `--output=esffile` the events are written in binary form to a new edit save file instead. The `--ignore-...` options leave the corresponding kind of event out of the output; in text mode the ignored events are still listed, marked `** EDITS READ BUT IGNORED **`. With `--verbose`, totals for each kind of edit are printed at the end.

## Common options

- `--input=esffile` (or `-Iesffile`) — Edit save file to read. Required.
- `--output=esffile` — Write the (remaining) edits to this binary edit save file instead of listing them as text.
- `--ignore-flag` — Leave out manual flag events (action 1).
- `--ignore-unflag` — Leave out unflag events (action 2).
- `--ignore-zero` — Leave out zero (null beam) events (action 3).
- `--ignore-filter` — Leave out autofilter flag events (action 4).
- `--verbose` (or `-V`) — Print totals of the edits read and ignored.
- `--help` (or `-H`) — Print help and exit.

## Examples

List all edit events of one file on the screen:

    mbdumpesf -I survey\0001_20250101_000000.all.esf

Save the listing to a text file:

    mbdumpesf -I survey\0001_20250101_000000.all.esf > edits.txt

Write a copy of the edits without the automatic filter flags and show the totals:

    mbdumpesf --input=0001_20250101_000000.all.esf --output=manual_only.esf --ignore-filter --verbose

## Notes

- Only `-I`, `-V` and `-H` have short forms; the output and ignore options must be given in the long `--` form.
- To use a filtered copy, rename it to the swath file name with `.esf` appended (keep a backup of the original) and run `mbprocess` again.
- Quote file names that contain spaces.

## See also

`mbclean`, `mbareaclean`, `mbprocess`, `mbgetesf`, `mbvoxelclean`, `mbset`. The complete reference is the MB-System manual page mbdumpesf(1) (English).
