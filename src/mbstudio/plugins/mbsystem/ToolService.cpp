#include "MbioServices.h"

#include <algorithm>
#include <set>

#include "mbstudio/platform/Paths.h"

namespace mbstudio::mbsystem {

namespace {

ToolInfo entry(const char *name, const char *category, const char *summary, const char *example) {
  ToolInfo t;
  t.name = name;
  t.category = category;
  t.summary = summary;
  t.example = example;
  return t;
}

}  // namespace

const std::vector<ToolInfo> &ToolService::catalog() {
  static const std::vector<ToolInfo> tools = {
      // Inspection
      entry("mbinfo", "Inspection", "Basic statistics, time span and bounds of swath files", "-I datalist.mb-1"),
      entry("mblist", "Inspection", "List navigation, beams and other values as text",
            "-I datalist.mb-1 -O tXYz"),
      entry("mbnavlist", "Inspection", "List navigation records of swath files", "-I file.mb88 -O tXY"),
      entry("mbformat", "Inspection", "Describe MB-System data formats", "-L"),
      entry("mbdatalist", "Inspection", "Parse recursive datalists and list the data files", "-I datalist.mb-1"),
      entry("mbhistogram", "Inspection", "Histogram of bathymetry, amplitude or sidescan values",
            "-I datalist.mb-1"),
      entry("mbctdlist", "Inspection", "List CTD records contained in swath files", "-I file.mb88"),
      entry("mbsvplist", "Inspection", "List sound velocity profiles contained in swath files", "-I datalist.mb-1"),
      entry("mbauvloglist", "Inspection", "List table data from MBARI AUV mission logs", "-I navigation.log"),
      entry("mbdumpesf", "Inspection", "Dump an edit save file (.esf) as text", "-I file.mb88.esf"),
      // Processing
      entry("mbmakedatalist", "Processing", "Create a datalist from the swath files in a directory", "-S.mb88"),
      entry("mbpreprocess", "Processing", "Preprocess raw swath files into the processing format",
            "-I datalist.mb-1"),
      entry("mbclean", "Processing", "Automatically flag bathymetry artifacts", "-I datalist.mb-1 -M1 -C3.5"),
      entry("mbareaclean", "Processing", "Flag outliers by area statistics", "-I datalist.mb-1 -S50"),
      entry("mbvoxelclean", "Processing", "Flag outliers with a 3D voxel occupancy filter", "-I datalist.mb-1"),
      entry("mbprocess", "Processing", "Apply edits, navigation, sound speed and corrections", "-I datalist.mb-1"),
      entry("mbset", "Processing", "Set values in mbprocess parameter files", "-I datalist.mb-1 -PEDITSAVEMODE:1"),
      entry("mbbackangle", "Processing", "Amplitude and sidescan grazing angle correction tables",
            "-I datalist.mb-1"),
      entry("mbfilter", "Processing", "Filter amplitude and sidescan data", "-I file.mb88 -S2/5/5"),
      entry("mbgetesf", "Processing", "Write an edit save file from the beam flags of a file", "-I file.mb88"),
      entry("mbcopy", "Processing", "Copy and convert swath files between formats", "-I in.all -F58/88 -O out.mb88"),
      entry("mbmakeplatform", "Processing", "Create or modify a platform (sensor offsets) file",
            "--swath=file.mb88 --output=platform.plf"),
      entry("mbrolltimelag", "Processing", "Estimate the roll time lag from bathymetry slopes", "-I datalist.mb-1"),
      entry("mbdefaults", "Processing", "Show or set the MBIO default parameters", ""),
      // Gridding and mosaicing
      entry("mbgrid", "Gridding", "Grid bathymetry, amplitude or sidescan into a GMT grid",
            "-I datalist.mb-1 -O bathymetry -E50/0/meters!"),
      entry("mbmosaic", "Gridding", "Mosaic amplitude or sidescan data", "-I datalist.mb-1 -O mosaic -E2/0/meters!"),
      entry("mbsslayout", "Gridding", "Lay out raw sidescan on a topography model", "--input=datalist.mb-1"),
      entry("mbgrd2gltf", "Gridding", "Convert a GMT grid to a glTF 3D model", "grid.grd -o model.gltf"),
      entry("mbmesh", "Gridding", "Generate 3D meshes from swath bathymetry", "--input=datalist.mb-1"),
      // Navigation and tide
      entry("mbnavadjustmerge", "Navigation", "Manipulate and merge mbnavadjust projects",
            "--input=project.nvh --list"),
      entry("mbgpstide", "Navigation", "Generate tide files from GPS altitude", "-I datalist.mb-1"),
      entry("mbotps", "Navigation", "Predict tides with the OSU Tidal Prediction Software",
            "-R-122/36 -B2025/01/01/00/00/00 -E2025/01/02/00/00/00 -D60"),
      entry("mbroutetime", "Navigation", "Times at which a survey reached route waypoints",
            "-I datalist.mb-1 -R route.rte"),
      entry("mbminirovnav", "Navigation", "Merge MBARI MiniROV USBL and CTD navigation", "--help"),
      entry("mbusbl2fnv", "Navigation", "Convert USBL tracking CSV to fast navigation", "--input=usbl.csv"),
      entry("mbfnv2navlab", "Navigation", "Convert fast navigation to Kongsberg Navlab format",
            "--input=file.fnv"),
      entry("mbnavlab2fnv", "Navigation", "Convert Kongsberg Navlab navigation to fnv", "--input=navlab_smooth.bin"),
      entry("mbmapscale", "Navigation", "Metres per degree of longitude and latitude", "--latitude=36.8"),
      entry("mbtime", "Navigation", "Convert between epoch seconds and calendar time", "-T2025/01/01/00/00/00"),
      // Sound velocity and water properties
      entry("mblevitus", "Sound velocity", "Mean annual sound velocity profile from the Levitus database",
            "-R-122/36 -O velocity.svp"),
      entry("mbsvpselect", "Sound velocity", "Choose the best sound speed profile for each file",
            "-I datalist.mb-1 -S svplist.txt"),
      entry("mbabsorption", "Sound velocity", "Sound absorption in sea water", "-F200 -T10 -S35 -D100"),
      // Sub-bottom and SEG-Y
      entry("mbextractsegy", "SEG-Y", "Extract sub-bottom and seismic data to SEG-Y", "-I datalist.mb-1"),
      entry("mbsegyinfo", "SEG-Y", "Summarise a SEG-Y file", "-I file.segy"),
      entry("mbsegylist", "SEG-Y", "List trace headers of a SEG-Y file", "-I file.segy"),
      entry("mbsegygrid", "SEG-Y", "Grid SEG-Y trace data", "-I file.segy -O section"),
      entry("mbsegypsd", "SEG-Y", "Power spectral density of SEG-Y traces", "-I file.segy -O psd"),
      entry("mb7k2jstar", "SEG-Y", "Extract Edgetech sub-bottom and sidescan from Reson 7k data",
            "-I file.s7k"),
      // Plotting with GMT and the MB-System modules (mbswath, mbcontour, mbgrdtiff)
      entry("gmt", "Plotting", "GMT with the MB-System modules mbswath, mbcontour and mbgrdtiff",
            "mbswath -I datalist.mb-1 -JM15c -R-122.1/-121.9/36.7/36.9 -Z2 -png swath"),
      // Export
      entry("mbswath2las", "Export", "Export swath bathymetry to LAS point clouds", "-I datalist.mb-1"),
      entry("dump_gsf", "Export", "Dump the records of a GSF file", "file.gsf"),
      entry("mbconfig", "Export", "Installation locations and build flags", "--prefix"),
  };
  return tools;
}

ToolService::ToolService(std::vector<std::string> searchPath, const EventBus *events)
    : searchPath_(std::move(searchPath)), events_(events) {}

std::string ToolService::locate(const std::string &name) const {
  const std::string file = name + platform::executableSuffix();
  for (const auto &dir : searchPath_) {
    const std::string candidate = platform::joinPath(dir, file);
    if (platform::fileExists(candidate))
      return candidate;
    // Build tree layout: <build>/src/utilities, <build>/src/gsf, ...
    for (const char *sub : {"../gsf", "../mbgrd2gltf", "../mbmesh", "../otps", "../mbnavadjust"}) {
      const std::string nested = platform::joinPath(platform::joinPath(dir, sub), file);
      if (platform::fileExists(nested))
        return nested;
    }
  }
  return std::string();
}

std::vector<ToolInfo> ToolService::tools() const {
  std::vector<ToolInfo> out = catalog();
  for (auto &t : out) {
    t.path = locate(t.name);
    t.available = !t.path.empty();
  }
  return out;
}

bool ToolService::tool(const std::string &name, ToolInfo &info) const {
  for (const auto &t : catalog()) {
    if (t.name == name) {
      info = t;
      info.path = locate(name);
      info.available = !info.path.empty();
      return true;
    }
  }
  return false;
}

std::vector<std::string> ToolService::categories() const {
  std::vector<std::string> out;
  for (const auto &t : catalog())
    if (std::find(out.begin(), out.end(), t.category) == out.end())
      out.push_back(t.category);
  return out;
}

platform::ProcessResult ToolService::run(const std::string &name, const std::vector<std::string> &arguments,
                                         const std::string &workingDirectory,
                                         const platform::OutputHandler &output,
                                         const std::atomic<bool> *cancel) const {
  platform::ProcessResult result;
  ToolInfo info;
  if (!tool(name, info)) {
    result.error = "unknown MB-System program: " + name;
    return result;
  }
  if (!info.available) {
    result.error = "program " + name + " is not installed";
    return result;
  }
  const std::int64_t job = nextJob_++;
  if (events_ != nullptr)
    events_->publish("tool.started", {{"run", job},
                                     {"tool", name},
                                     {"command", platform::formatCommand(name, arguments)}});
  platform::ProcessSpec spec;
  spec.program = info.path;
  spec.arguments = arguments;
  spec.workingDirectory = workingDirectory;
  // MB-System programs start other programs by name (mbdatalist -O runs
  // mbinfo, mbcopy and mblist; mbgrid creates missing .inf files): put the
  // directory of the program first on their PATH.
  const std::string current = platform::getEnv("PATH");
  spec.environment.emplace_back("PATH", platform::parentPath(info.path) +
                                            (current.empty() ? std::string() : platform::pathListSeparator() + current));
  result = platform::runProcess(
      spec,
      [&](const std::string &chunk) {
        if (output)
          output(chunk);
        if (events_ != nullptr)
          events_->publish("tool.output", {{"run", job}, {"text", chunk}});
      },
      cancel);
  if (events_ != nullptr)
    events_->publish("tool.finished", {{"run", job},
                                      {"tool", name},
                                      {"exitCode", static_cast<std::int64_t>(result.exitCode)},
                                      {"cancelled", result.cancelled},
                                      {"error", result.error}});
  return result;
}

}  // namespace mbstudio::mbsystem
