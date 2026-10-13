// MB-Studio mbsystem plugin.
//
// Adapts MB-System to the MB-Studio kernel: it provides the format, swath,
// grid, rendering and tool services and exposes them as commands, so every
// function is available to any front end - the QML user interface, the
// mbstudio command line (--command) and the test suite alike.
#include <sstream>

#include "MbSystemPlugin.h"

#include "MbioServices.h"
#include "mbstudio/kernel/Kernel.h"
#include "mbstudio/platform/Process.h"

extern "C" {
#include "mb_define.h"
}

namespace mbstudio::mbsystem {

PluginInfo MbSystemPlugin::info() const {
  PluginInfo info;
  info.id = "mbsystem";
  info.name = "MB-System";
  info.version = std::string(MBSTUDIO_VERSION) + " (MB-System " + MB_VERSION + ")";
  info.description = "Swath sonar data access, grids and the MB-System command line programs";
  return info;
}

bool MbSystemPlugin::initialize(Kernel &kernel, std::string &error) {
  (void)error;
  const std::string owner = "mbsystem";
  auto formats = std::make_shared<FormatService>();
  auto swath = std::make_shared<SwathService>(formats);
  auto grids = std::make_shared<GridService>();
  auto render = std::make_shared<GridRenderService>();
  auto tools = std::make_shared<ToolService>(kernel.config().toolDirectories, &kernel.events());
  auto help = std::make_shared<HelpService>();
  auto processing = std::make_shared<ProcessingService>();
  auto jobs = std::make_shared<JobService>(tools, &kernel.events());

  ServiceRegistry &services = kernel.services();
  services.provide<IFormatService>(formats, owner);
  services.provide<ISwathService>(swath, owner);
  services.provide<IGridService>(grids, owner);
  services.provide<IGridRenderService>(render, owner);
  services.provide<IToolService>(tools, owner);
  services.provide<IHelpService>(help, owner);
  services.provide<IProcessingService>(processing, owner);
  services.provide<IJobService>(jobs, owner);

  CommandRegistry &commands = kernel.commands();
  commands.add({"formats.list", "List formats", "List the swath data formats supported by MB-System", owner},
               [formats](const ValueMap &) {
                 ValueMap data;
                 const auto list = formats->formats();
                 data["count"] = static_cast<std::int64_t>(list.size());
                 for (const auto &f : list)
                   data["format." + std::to_string(f.id)] = f.name;
                 return CommandResult::success(data);
               });
  commands.add({"formats.identify", "Identify format", "MB-System format of a file (path=...)", owner},
               [formats](const ValueMap &args) {
                 const std::string path = getString(args, "path");
                 const int id = formats->identify(path);
                 if (id == 0)
                   return CommandResult::failure("format of " + path + " not recognised");
                 FormatInfo info;
                 formats->format(id, info);
                 return CommandResult::success({{"format", static_cast<std::int64_t>(id)}, {"name", info.name}});
               });
  commands.add({"swath.inspect", "Inspect swath file", "Statistics of a swath file (path=..., format=0)", owner},
               [swath](const ValueMap &args) {
                 const SwathSummary s = swath->inspect(getString(args, "path"),
                                                       static_cast<int>(getInt(args, "format", 0)), 2, nullptr);
                 if (!s.ok)
                   return CommandResult::failure(s.error);
                 return CommandResult::success({{"format", static_cast<std::int64_t>(s.format)},
                                                {"formatName", s.formatName},
                                                {"records", static_cast<std::int64_t>(s.records)},
                                                {"maxBeams", static_cast<std::int64_t>(s.maxBeams)},
                                                {"beamsTotal", static_cast<std::int64_t>(s.beamsTotal)},
                                                {"beamsGood", static_cast<std::int64_t>(s.beamsGood)},
                                                {"timeStart", s.timeStartIso},
                                                {"timeEnd", s.timeEndIso},
                                                {"lonMin", s.lonMin},
                                                {"lonMax", s.lonMax},
                                                {"latMin", s.latMin},
                                                {"latMax", s.latMax},
                                                {"depthMin", s.depthMin},
                                                {"depthMax", s.depthMax},
                                                {"trackLengthKm", s.trackLengthKm}});
               });
  commands.add({"grid.info", "Grid information", "Dimensions and range of a GMT grid (path=...)", owner},
               [grids](const ValueMap &args) {
                 const GridLoadResult r = grids->load(getString(args, "path"));
                 if (!r.ok)
                   return CommandResult::failure(r.error);
                 const Grid &g = r.grid;
                 return CommandResult::success({{"nx", static_cast<std::int64_t>(g.nx)},
                                                {"ny", static_cast<std::int64_t>(g.ny)},
                                                {"xmin", g.xmin},
                                                {"xmax", g.xmax},
                                                {"ymin", g.ymin},
                                                {"ymax", g.ymax},
                                                {"zmin", g.zmin},
                                                {"zmax", g.zmax},
                                                {"geographic", g.geographic},
                                                {"title", g.title}});
               });
  commands.add({"tools.list", "List programs", "MB-System programs and whether they are installed", owner},
               [tools](const ValueMap &) {
                 ValueMap data;
                 std::int64_t available = 0;
                 for (const auto &t : tools->tools()) {
                   data["tool." + t.name] = t.available ? t.path : std::string("(not installed)");
                   available += t.available ? 1 : 0;
                 }
                 data["available"] = available;
                 return CommandResult::success(data);
               });
  commands.add({"tools.run", "Run program", "Run an MB-System program (tool=..., args=\"...\", cwd=...)", owner},
               [tools](const ValueMap &args) {
                 std::string output;
                 const auto result = tools->run(getString(args, "tool"),
                                                platform::splitArguments(getString(args, "args")),
                                                getString(args, "cwd"),
                                                [&output](const std::string &chunk) { output += chunk; });
                 if (!result.started)
                   return CommandResult::failure(result.error);
                 CommandResult r = CommandResult::success({{"exitCode", static_cast<std::int64_t>(result.exitCode)},
                                                           {"output", output}});
                 r.ok = result.exitCode == 0;
                 return r;
               });

  commands.add({"grid.stats", "Grid statistics", "Valid and empty (NaN) nodes of a GMT grid (path=...)", owner},
               [grids](const ValueMap &args) {
                 const GridLoadResult r = grids->load(getString(args, "path"));
                 if (!r.ok)
                   return CommandResult::failure(r.error);
                 const GridStatistics st = grids->statistics(r.grid);
                 return CommandResult::success({{"nodes", static_cast<std::int64_t>(st.nodes)},
                                                {"valid", static_cast<std::int64_t>(st.valid)},
                                                {"empty", static_cast<std::int64_t>(st.empty)},
                                                {"emptyRatio", st.emptyRatio},
                                                {"mean", st.mean},
                                                {"stddev", st.stddev}});
               });
  commands.add({"processing.grid", "Gridding command",
                "mbgrid arguments for typed parameters (datalist=..., output=..., spacing=25, algorithm=1, clip=0)",
                owner},
               [processing](const ValueMap &args) {
                 GridParameters p;
                 p.outputRoot = getString(args, "output");
                 p.spacing = getDouble(args, "spacing", p.spacing);
                 p.dataType = static_cast<int>(getInt(args, "type", p.dataType));
                 p.algorithm = static_cast<int>(getInt(args, "algorithm", p.algorithm));
                 p.clip = static_cast<int>(getInt(args, "clip", p.clip));
                 p.projection = getString(args, "projection");
                 const CommandPlan plan = processing->grid(getString(args, "datalist"), p);
                 if (!plan.ok)
                   return CommandResult::failure(plan.error);
                 ValueMap data;
                 const auto lines = plan.commandLines();
                 data["command"] = lines.empty() ? std::string() : lines.front();
                 data["outputs"] = static_cast<std::int64_t>(plan.outputs.size());
                 for (std::size_t i = 0; i < plan.outputs.size(); i++)
                   data["output." + std::to_string(i)] = plan.outputs[i];
                 return CommandResult::success(data);
               });
  commands.add({"jobs.list", "Jobs", "Background jobs and their state", owner}, [jobs](const ValueMap &) {
    ValueMap data;
    const auto list = jobs->jobs();
    data["count"] = static_cast<std::int64_t>(list.size());
    for (const auto &j : list)
      data["job." + std::to_string(j.id)] = std::string(jobStateName(j.state)) + " " + j.title;
    return CommandResult::success(data);
  });
  commands.add({"jobs.cancel", "Cancel job", "Cancel a queued or running job (id=...)", owner},
               [jobs](const ValueMap &args) {
                 return jobs->cancel(getInt(args, "id")) ? CommandResult::success()
                                                         : CommandResult::failure("no such running job");
               });

  commands.add({"help.programs", "Help pages", "Programs with help pages and the help languages", owner},
               [help](const ValueMap &) {
                 ValueMap data;
                 std::string languages;
                 for (const auto &l : help->languages())
                   languages += (languages.empty() ? "" : ",") + l;
                 data["languages"] = languages;
                 const auto programs = help->programs();
                 data["count"] = static_cast<std::int64_t>(programs.size());
                 for (const auto &p : programs)
                   data["program." + p] = help->page(p, "en").summary;
                 return CommandResult::success(data);
               });
  commands.add({"help.show", "Program help", "Help page of a program (name=..., lang=en|ja|zh_CN)", owner},
               [help](const ValueMap &args) {
                 const HelpPage page = help->page(getString(args, "name"), getString(args, "lang", "en"));
                 if (!page.found)
                   return CommandResult::failure("no help page for " + getString(args, "name"));
                 return CommandResult::success({{"program", page.program},
                                                {"language", page.language},
                                                {"summary", page.summary},
                                                {"markdown", page.markdown}});
               });

  if (help->directory().empty())
    kernel.log(LogLevel::Warning, owner, "help pages of the MB-System programs not found");
  kernel.log(LogLevel::Info, owner, std::string("MB-System ") + MB_VERSION + " services ready");
  return true;
}

void MbSystemPlugin::shutdown(Kernel &kernel) { kernel.log(LogLevel::Info, "mbsystem", "MB-System services stopped"); }

}  // namespace mbstudio::mbsystem
