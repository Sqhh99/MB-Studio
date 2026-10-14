// MB-Studio project plugin: survey projects on top of the MB-System
// services. Provides IProjectService, the project.* commands (also for the
// command line: mbstudio --command project.add project=... path=...) and the
// workbench contributions of the processing chain.
#include <memory>
#include <sstream>

#include "ProjectPlugin.h"

#include "ProjectService.h"
#include "mbstudio/kernel/Kernel.h"
#include "mbstudio/platform/Paths.h"

namespace mbstudio::project {

namespace {

constexpr int kCommandTimeoutMs = 6 * 3600 * 1000;

std::vector<std::string> splitList(const std::string &text) {
  // '|' separates paths (';' and ':' occur in Windows paths)
  std::vector<std::string> out;
  std::string item;
  std::istringstream in(text);
  while (std::getline(in, item, '|'))
    if (!item.empty())
      out.push_back(item);
  return out;
}

// Open the project named by "project=" unless it is already open.
bool ensureOpen(ProjectService &service, const ValueMap &args, std::string &error) {
  const std::string path = getString(args, "project");
  if (path.empty()) {
    if (!service.isOpen()) {
      error = "no project: give project=<file or folder>";
      return false;
    }
    return true;
  }
  if (service.isOpen() && (service.file() == platform::absolutePath(path) || service.root() == platform::absolutePath(path)))
    return true;
  return service.open(path, error);
}

ValueMap summary(const ProjectService &service) {
  ValueMap data;
  data["name"] = service.name();
  data["file"] = service.file();
  const auto lines = service.lines();
  const auto assets = service.assets();
  const auto history = service.history();
  data["lines"] = static_cast<std::int64_t>(lines.size());
  data["assets"] = static_cast<std::int64_t>(assets.size());
  data["jobs"] = static_cast<std::int64_t>(history.size());
  std::int64_t inspected = 0;
  for (const auto &l : lines)
    inspected += l.inspected ? 1 : 0;
  data["inspected"] = inspected;
  for (std::size_t i = 0; i < lines.size(); i++)
    data["line." + std::to_string(i)] = lines[i].id + " " + lines[i].processingStatus() + " " + lines[i].path;
  for (std::size_t i = 0; i < assets.size(); i++)
    data["asset." + std::to_string(i)] =
        assets[i].type + " " + (assets[i].status.empty() ? std::string("-") : assets[i].status) + " " + assets[i].path;
  for (std::size_t i = 0; i < history.size(); i++)
    data["job." + std::to_string(i)] = history[i].state + " " + history[i].title;
  const NextAction next = service.nextAction();
  data["next"] = next.kind;
  data["nextReason"] = next.reason;
  const GeoBounds b = service.bounds();
  if (b.valid()) {
    data["west"] = b.west;
    data["east"] = b.east;
    data["south"] = b.south;
    data["north"] = b.north;
  }
  return data;
}

}  // namespace

PluginInfo ProjectPlugin::info() const {
  PluginInfo info;
  info.id = "project";
  info.name = "Survey projects";
  info.version = MBSTUDIO_VERSION;
  info.description = "Projects, survey lines, products, processing history and the MB-System processing chain";
  info.dependencies = {"mbsystem"};
  return info;
}

bool ProjectPlugin::initialize(Kernel &kernel, std::string &error) {
  const std::string owner = "project";
  auto formats = kernel.services().get<IFormatService>();
  auto swath = kernel.services().get<ISwathService>();
  auto processing = kernel.services().get<IProcessingService>();
  auto jobs = kernel.services().get<IJobService>();
  if (!formats || !swath || !processing || !jobs) {
    error = "the MB-System services are not available";
    return false;
  }
  std::string mbsystemVersion;
  for (const auto &p : kernel.plugins().plugins())
    if (p.info.id == "mbsystem")
      mbsystemVersion = p.info.version;
  service_ = std::make_shared<ProjectService>(formats, swath, processing, jobs, &kernel.events(), mbsystemVersion,
                                              MBSTUDIO_VERSION, kernel.services().get<ISoundingService>(),
                                              kernel.services().get<IGridService>());
  service_->attach();
  kernel.services().provide<IProjectService>(service_, owner);
  std::weak_ptr<ProjectService> weak = service_;

  CommandRegistry &commands = kernel.commands();
  commands.add({"project.create", "New project", "Create a project (dir=..., name=...)", owner, "project"},
               [weak](const ValueMap &args) {
                 auto service = weak.lock();
                 std::string error;
                 if (!service || !service->create(getString(args, "dir"), getString(args, "name"), error))
                   return CommandResult::failure(error);
                 return CommandResult::success(summary(*service));
               });
  commands.add({"project.open", "Open project", "Open a project file or folder (path=...)", owner, "project"},
               [weak](const ValueMap &args) {
                 auto service = weak.lock();
                 std::string error;
                 if (!service || !service->open(getString(args, "path"), error))
                   return CommandResult::failure(error);
                 return CommandResult::success(summary(*service));
               });
  commands.add({"project.info", "Project summary", "Lines, products and jobs of a project (project=...)", owner,
                "project"},
               [weak](const ValueMap &args) {
                 auto service = weak.lock();
                 std::string error;
                 if (!service || !ensureOpen(*service, args, error))
                   return CommandResult::failure(error);
                 return CommandResult::success(summary(*service));
               });
  commands.add({"project.add", "Add survey lines",
                "Add swath files or folders (project=..., path=..., paths=a|b, recursive=true, wait=true)", owner,
                "project"},
               [weak](const ValueMap &args) {
                 auto service = weak.lock();
                 std::string error;
                 if (!service || !ensureOpen(*service, args, error))
                   return CommandResult::failure(error);
                 std::vector<std::string> paths = splitList(getString(args, "paths"));
                 if (!getString(args, "path").empty())
                   paths.push_back(getString(args, "path"));
                 const ImportResult r = service->addFiles(paths, getBool(args, "recursive", true));
                 if (getBool(args, "wait", true) && !service->waitForJobs(kCommandTimeoutMs))
                   return CommandResult::failure("timed out waiting for the import job");
                 ValueMap data = summary(*service);
                 data["added"] = static_cast<std::int64_t>(r.added);
                 data["skipped"] = static_cast<std::int64_t>(r.skipped.size());
                 for (std::size_t i = 0; i < r.skipped.size(); i++)
                   data["skipped." + std::to_string(i)] = r.skipped[i];
                 if (r.added == 0)
                   return CommandResult::failure("no survey lines added");
                 return CommandResult::success(data);
               });
  commands.add({"project.grid", "Grid survey lines",
                "Grid all lines of a project (project=..., name=..., spacing=25, algorithm=1, clip=0, type=1)", owner,
                "project"},
               [weak](const ValueMap &args) {
                 auto service = weak.lock();
                 std::string error;
                 if (!service || !ensureOpen(*service, args, error))
                   return CommandResult::failure(error);
                 ProcessingRequest request;
                 request.kind = "grid";
                 request.name = getString(args, "name", "bathymetry");
                 request.grid.spacing = getDouble(args, "spacing", request.grid.spacing);
                 request.grid.algorithm = static_cast<int>(getInt(args, "algorithm", request.grid.algorithm));
                 request.grid.clip = static_cast<int>(getInt(args, "clip", request.grid.clip));
                 request.grid.dataType = static_cast<int>(getInt(args, "type", request.grid.dataType));
                 const std::int64_t id = service->run(request, error);
                 if (id == 0)
                   return CommandResult::failure(error);
                 if (getBool(args, "wait", true) && !service->waitForJobs(kCommandTimeoutMs))
                   return CommandResult::failure("timed out waiting for the gridding job");
                 ValueMap data = summary(*service);
                 for (const auto &r : service->history())
                   if (r.jobId == id) {
                     data["state"] = r.state;
                     data["error"] = r.error;
                     if (r.state != "completed" && getBool(args, "wait", true))
                       return CommandResult::failure("gridding " + r.state + ": " + r.error);
                   }
                 return CommandResult::success(data);
               });

  commands.add({"project.retry", "Run a job again",
                "Repeat a job of the project history as a new attempt (project=..., sequence=..., wait=true)", owner,
                "project"},
               [weak](const ValueMap &args) {
                 auto service = weak.lock();
                 std::string error;
                 if (!service || !ensureOpen(*service, args, error))
                   return CommandResult::failure(error);
                 const std::int64_t id = service->retry(getInt(args, "sequence"), error);
                 if (id == 0)
                   return CommandResult::failure(error);
                 if (getBool(args, "wait", true) && !service->waitForJobs(kCommandTimeoutMs))
                   return CommandResult::failure("timed out waiting for the job");
                 ValueMap data = summary(*service);
                 data["sequence"] = service->sequenceOfJob(id);
                 return CommandResult::success(data);
               });
  commands.add({"project.figure", "Map figure",
                "Make a GMT figure of a project grid (project=..., asset=<grid id or empty>, template=bathymetry|"
                "overview|quality|perspective, name=..., formats=png|pdf|ps, width=16, contours=0, title=...)",
                owner, "project"},
               [weak](const ValueMap &args) {
                 auto service = weak.lock();
                 std::string error;
                 if (!service || !ensureOpen(*service, args, error))
                   return CommandResult::failure(error);
                 ProcessingRequest request;
                 request.kind = "figure";
                 request.name = getString(args, "name");
                 request.asset = getString(args, "asset");
                 request.figure.kind = getString(args, "template", "bathymetry");
                 if (request.asset.empty() && request.figure.kind != "overview")
                   for (const auto &a : service->assets())  // the newest grid
                     if (a.type == "grid" && a.parent.empty())
                       request.asset = a.id;
                 const std::vector<std::string> formats = splitList(getString(args, "formats", "png|pdf"));
                 if (!formats.empty())
                   request.figure.formats = formats;
                 request.figure.widthCm = getDouble(args, "width", request.figure.widthCm);
                 request.figure.contourInterval = getDouble(args, "contours", 0.0);
                 request.figure.title = getString(args, "title");
                 const std::int64_t id = service->run(request, error);
                 if (id == 0)
                   return CommandResult::failure(error);
                 if (getBool(args, "wait", true) && !service->waitForJobs(kCommandTimeoutMs))
                   return CommandResult::failure("timed out waiting for the figure job");
                 ValueMap data = summary(*service);
                 for (const auto &r : service->history())
                   if (r.jobId == id) {
                     data["state"] = r.state;
                     data["error"] = r.error;
                     for (std::size_t i = 0; i < r.outputs.size(); i++)
                       data["output." + std::to_string(i)] = r.outputs[i];
                     if (r.state != "completed" && getBool(args, "wait", true))
                       return CommandResult::failure("figure " + r.state + ": " + r.error);
                   }
                 return CommandResult::success(data);
               });

  // Workbench contributions: the processing chain as menu actions. The
  // front end maps the "ui:" commands to its own panels.
  std::vector<ActionContribution> actions = {
      {"project.new", "New project…", "file", "ui:newProject", "Ctrl+N", "folderOpen", owner, 10},
      {"project.openFile", "Open project…", "file", "ui:openProject", "Ctrl+Shift+O", "folderOpen", owner, 11},
      {"project.addData", "Add survey data…", "data", "ui:addData", "Ctrl+I", "add", owner, 10},
      {"data.addFolder", "Add a folder of survey data…", "data", "ui:addFolder", "", "folder", owner, 11},
      {"data.inspect", "Inspect the selected line", "data", "ui:inspectSelected", "", "waves", owner, 20},
      {"data.selectAll", "Select all survey lines", "data", "ui:selectAllLines", "", "check", owner, 30},
      {"data.remove", "Remove the selected lines", "data", "ui:removeSelectedLines", "", "close", owner, 40},
      {"process.preprocess", "Preprocess…", "process", "ui:panel:preprocess", "", "tune", owner, 10},
      {"process.clean", "Clean bathymetry…", "process", "ui:panel:clean", "", "sweep", owner, 20},
      {"process.corrections", "Corrections…", "process", "ui:panel:corrections", "", "tune", owner, 30},
      {"process.process", "Process lines…", "process", "ui:panel:process", "", "play", owner, 40},
      {"process.grid", "Grid bathymetry…", "process", "ui:panel:grid", "Ctrl+G", "terrain", owner, 50},
      {"process.mosaic", "Backscatter mosaic…", "process", "ui:panel:mosaic", "", "layers", owner, 60},
      {"process.export", "Export…", "process", "ui:panel:export", "Ctrl+E", "openInNew", owner, 70},
      {"analysis.figure", "Map figure (GMT)…", "analysis", "ui:panel:figure", "Ctrl+Shift+F", "map", owner, 40},
  };
  if (!kernel.contributions().addAll({}, actions, {}, &error))
    return false;
  return true;
}

void ProjectPlugin::shutdown(Kernel &kernel) {
  (void)kernel;
  if (service_) {
    service_->detach();
    service_->close();
    service_.reset();
  }
}

}  // namespace mbstudio::project
