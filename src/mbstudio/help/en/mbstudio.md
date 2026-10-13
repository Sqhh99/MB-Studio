# mbstudio — MB-Studio desktop application and command line front end

`mbstudio` starts the MB-Studio desktop application: inspect swath sonar files, view GMT grids as shaded relief, run every MB-System program with live output and read these help pages, in English, Japanese or Simplified Chinese. The same executable also works from the command line, where it can list its plugins, print help pages and run MB-Studio commands that return JSON.

## Synopsis

    mbstudio [file ...]
    mbstudio --help-program NAME [--lang CODE]
    mbstudio --command ID [key=value ...]
    mbstudio --list-plugins | --list-commands | --list-services
    mbstudio --headless | --version | --help
    mbstudio --disable PLUGIN | --plugin-dir DIR

## Description

Without options `mbstudio` opens the main window. Swath data files, datalists and GMT grids given on the command line, or dropped on the window, are opened in the Inspect or Grids page. The Programs page lists the MB-System programs installed in the `bin` folder; select one to read its help page, fill in the arguments and the working directory, and run it. The output appears in the Output tab and the exit code is shown when the program ends.

MB-Studio is built as a small kernel with plugins: the `mbsystem` plugin provides the MB-System services (formats, swath files, grids, programs and help pages) and the `qmlui` plugin provides the user interface. Every function of the services is also available as a command, so it can be used from scripts with `--command`, without the user interface.

The language of the user interface follows the Windows display language; choose another one with the language button in the side bar (English, 日本語, 简体中文). The choice is remembered. The help pages of the programs are shown in the same language.

## Common options

- `--help-program NAME` — Print the help page of the MB-System program NAME (for example `mbgrid`) as text and exit.
- `--lang CODE` — Language of the help page: `en`, `ja` or `zh_CN`. Default: the `MBSTUDIO_LANG` environment variable, otherwise the Windows display language.
- `--command ID [key=value ...]` — Run an MB-Studio command and print its result as JSON, for example `swath.inspect`, `grid.info`, `formats.list`, `tools.list`, `help.show`. The exit status is 0 when the command succeeds.
- `--list-plugins` — List the plugins with their state and version.
- `--list-commands` — List the commands that can be used with `--command`.
- `--list-services` — List the services provided by the plugins.
- `--headless` — Start the kernel and the plugins without the user interface (for checks).
- `--disable PLUGIN` — Do not start the plugin PLUGIN (repeatable).
- `--plugin-dir DIR` — Also load plugins from the folder DIR (repeatable).
- `--version` — Print the version and exit.
- `--help` — Print a short usage message and exit.

## Examples

Open a swath file in the desktop application:

    mbstudio 0001_20250101_000000.mb58

Read the Japanese help page of mbgrid in the console:

    mbstudio --help-program mbgrid --lang ja

Get the statistics of a swath file as JSON, for use in a script:

    mbstudio --command swath.inspect path=line1.mb88

Check that both plugins are loaded:

    mbstudio --list-plugins

## Notes

- The settings (language, theme, recent files, working directory of the Programs page) are stored in `%APPDATA%\MB-Studio\mbstudio.ini`.
- Set `MBSTUDIO_LANG` (`en`, `ja` or `zh_CN`) to force a language for both the user interface and `--help-program`.
- The MB-System Command Prompt in the Start menu opens a console where `mbstudio`, the MB-System programs and `gmt` can be run directly.

## See also

`mbinfo`, `mbgrid`, `mbprocess`, `gmt`. The help pages of all programs are also installed as HTML (Start menu: MB-System program help).
