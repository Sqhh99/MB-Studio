# MB-Studio architecture

MB-Studio is a **microkernel** application. The kernel knows nothing about
MB-System, Qt or any user interface; everything else, including the
graphical user interface, is a plugin.

```
                      ┌──────────────────────────────────────────┐
  mbstudio.exe  ───▶  │ kernel (mbstudio_kernel)                 │
  (host: starts the   │  PluginManager  ServiceRegistry          │
   kernel, hands      │  EventBus       CommandRegistry          │
   control to the     │  platform layer: paths, DLLs, processes  │
   front end or runs  └───────▲──────────────────▲───────────────┘
   --command ...)             │ services,        │ IFrontend,
                              │ commands, events │ uses services
              ┌───────────────┴───────┐   ┌──────┴────────────────────┐
              │ plugin: mbsystem      │   │ plugin: qmlui             │
              │  formats, swath,      │   │  view models (C++)        │
              │  grids, rendering,    │   │  QML presentation         │
              │  MB-System programs   │   │                           │
              │  (MBIO, netCDF, CLI)  │   │                           │
              └───────────────────────┘   └───────────────────────────┘
```

## Kernel (`kernel/`)

- **Plugins** are shared libraries named `mbstudio-plugin-<name>` that export
  `mbstudio_plugin_abi_version`, `mbstudio_create_plugin` and
  `mbstudio_destroy_plugin` (`MBSTUDIO_DECLARE_PLUGIN`). The plugin manager
  loads them from `MBSTUDIO_PLUGIN_PATH` and `<prefix>/lib/mbstudio/plugins`,
  rejects other ABI versions, starts them in dependency order and stops them
  in reverse order. Plugins can be disabled with `--disable` or
  `MBSTUDIO_DISABLE_PLUGINS`.
- **Services** are interfaces declared in `include/mbstudio/api`, looked up by
  a string id (`T::kServiceId`), which works reliably across DLL boundaries.
- **Commands** expose plugin functionality by name with simple key/value
  arguments and results, for scripts, tests and alternative front ends.
- **Contributions** (`kernel/Contributions.h`) declare what a plugin adds to
  the workbench: workspaces (document views with a QML component), menu
  actions bound to commands, and dock panels. The front end builds its menus
  and tabs from them, so a plugin adds a view without editing `Main.qml`.
- **Events** (`log.*`, `job.*`, `plugin.*`) are published synchronously; the
  user interface marshals them to its own thread.
- **Platform layer** (`include/mbstudio/platform`): UTF-8 paths (UTF-16 at
  the Win32 boundary), `LoadLibraryExW`/`dlopen`, and a process runner
  (`CreateProcessW`/`fork`) with correct Windows argument quoting, live
  output and cancellation.

## Plugins (`plugins/`)

- **mbsystem** adapts MB-System: data formats (`mb_format_description`),
  swath file inspection (`mb_read`), GMT grids (netCDF), colour shaded relief
  rendering, and the catalogue of MB-System programs, which it runs through the
  platform process runner. Commands: `formats.list`, `formats.identify`,
  `swath.inspect`, `grid.info`, `tools.list`, `tools.run`.
  It also provides the **job service** (`IJobService`: a queue of jobs made
  of program runs with structured argument lists and in-process steps, run on
  worker threads with cancel, retry, a log per job and `job.*` events) and the
  **processing service** (`IProcessingService`: typed recipes that map user
  parameters explicitly to the options of `mbpreprocess`, `mbclean`,
  `mbareaclean`, `mbset`, `mbprocess`, `mbgrid`, `mbmosaic` and the export
  programs). Commands also include `grid.stats`, `processing.grid`,
  `jobs.list`, `jobs.cancel`.
- **project** (Qt Core for JSON only) provides `IProjectService`: the project
  file, survey lines with their metadata, products, layers, the open views
  and the processing history with provenance. It writes the datalists,
  turns processing requests on a selection of lines into jobs and registers
  their products. Commands: `project.create`, `project.open`, `project.add`,
  `project.grid`, `project.info`.
- **qmlui** provides the `IFrontend` service. It is layered:
  - `viewmodels/` – C++ adapters between the kernel services and QML
    (`AppController`, `ProjectViewModel`, `ProcessingViewModel` with the
    declarative parameter schemas of the processing panels, `JobListModel`,
    `WorkspaceModel`, `MapView`, `SwathViewModel`, `GridViewModel` + image
    provider, `ToolsViewModel`, `FormatListModel`, `LogModel`, `TrackPlot`).
    Long running work runs on worker threads.
  - `qml/` – presentation only: the workbench shell (`Main.qml`), dock
    panels (`panels/`), workspaces (`workspaces/`), pages, components, theme
    and icons.

## Command line

```
mbstudio                              start the user interface
mbstudio --list-plugins               plugins and their state
mbstudio --list-services --list-commands
mbstudio --command swath.inspect path=line.mb88
mbstudio --command tools.run tool=mbinfo args="-I datalist.mb-1"
mbstudio --command project.create dir=D:\Surveys\EM122 name=EM122
mbstudio --command project.add project=D:\Surveys\EM122 path=D:\Data\EM122
mbstudio --command project.grid project=D:\Surveys\EM122 spacing=25
```

## Writing a plugin

```cpp
#include "mbstudio/kernel/Kernel.h"
#include "mbstudio/kernel/Plugin.h"

class HelloPlugin : public mbstudio::IPlugin {
 public:
  mbstudio::PluginInfo info() const override { return {"hello", "Hello", "1.0", "Example", {"mbsystem"}}; }
  bool initialize(mbstudio::Kernel &kernel, std::string &) override {
    kernel.commands().add({"hello.say", "Say hello", "", "hello"}, [](const mbstudio::ValueMap &args) {
      return mbstudio::CommandResult::success({{"text", "Hello " + mbstudio::getString(args, "name")}});
    });
    return true;
  }
};
MBSTUDIO_DECLARE_PLUGIN(HelloPlugin)
```

Build it with `mbstudio_add_plugin(<target> hello hello.cpp)` (see
`CMakeLists.txt`).

## Tests (`tests/`)

- `mbstudio_kernel_test` – kernel and platform layer (fixture plugin
  libraries, ABI and dependency handling, quoting, Unicode, processes).
- `mbstudio_mbsystem_test` – the MB-System services against real data and
  programs (including an `mbgrid` run).
- `mbstudio_project_test` – projects in folders with spaces and Chinese
  characters: import, metadata, gridding through the job service,
  persistence and the project commands.
- `mbstudio_host_test` – the `mbstudio` executable in its command line modes.
- `mbstudio_qmlui_test` – view models with fake and real services, and an
  offscreen smoke test of the complete QML interface that fails on any QML
  warning and saves screenshots of every workspace and panel.
