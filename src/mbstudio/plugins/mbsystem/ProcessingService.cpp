// Typed recipes for the MB-System processing programs. Every option is
// mapped explicitly from the help pages in src/mbstudio/help/en.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

#include "MbioServices.h"
#include "mbstudio/platform/Paths.h"

extern "C" {
#include "mb_define.h"
#include "mb_format.h"
}

namespace mbstudio::mbsystem {

namespace {

constexpr double kPi = 3.14159265358979323846;

// Numbers in program options always use '.' whatever the C locale is.
std::string num(double value) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(10) << value;
  return out.str();
}

std::string boundsText(const GeoBounds &b) {
  return num(b.west) + "/" + num(b.east) + "/" + num(b.south) + "/" + num(b.north);
}

JobStep step(std::string title, std::string tool, std::vector<std::string> arguments) {
  JobStep s;
  s.title = std::move(title);
  s.tool = std::move(tool);
  s.arguments = std::move(arguments);
  return s;
}

CommandPlan failed(const std::string &error) {
  CommandPlan plan;
  plan.error = error;
  return plan;
}

bool validUnits(const std::string &units) {
  return units == "meters" || units == "km" || units == "feet" || units == "degrees" || units == "arcmin" ||
         units == "arcsec";
}

double metersPerUnit(const std::string &units, double latitude) {
  constexpr double kDeg = 111319.49;
  if (units == "km")
    return 1000.0;
  if (units == "feet")
    return 0.3048;
  if (units == "degrees")
    return kDeg * std::max(0.05, std::cos(latitude * kPi / 180.0));
  if (units == "arcmin")
    return kDeg / 60.0 * std::max(0.05, std::cos(latitude * kPi / 180.0));
  if (units == "arcsec")
    return kDeg / 3600.0 * std::max(0.05, std::cos(latitude * kPi / 180.0));
  return 1.0;
}

}  // namespace

bool ProcessingService::datalistText(const std::vector<DatalistEntry> &entries, const std::string &datalistDirectory,
                                     bool processed, std::string &text, std::string &error) const {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << "# MB-System datalist written by MB-Studio\n";
  if (processed)
    out << "$PROCESSED\n";
  for (const auto &e : entries) {
    const std::string path = platform::relativePath(e.path, datalistDirectory);
    // MBIO reads each entry with "%s %d %lf": a path cannot contain spaces
    if (path.find_first_of(" \t") != std::string::npos) {
      error = "MB-System datalists cannot reference paths with spaces: " + path;
      return false;
    }
    if (e.format == 0) {
      error = "unknown data format for " + path;
      return false;
    }
    out << path << ' ' << e.format << ' ' << num(e.weight) << '\n';
  }
  text = out.str();
  return true;
}

std::string ProcessingService::processedDatalistText(const std::string &datalistName) const {
  return "# MB-System datalist written by MB-Studio: processed files where they exist\n$PROCESSED\n" +
         datalistName + " -1 1\n";
}

int ProcessingService::preprocessOutputFormat(int format) const {
  // as in mbpreprocess: the vendor formats are rewritten to the
  // corresponding extended processing formats
  switch (format) {
    case MBF_EMOLDRAW:
    case MBF_EM12IFRM:
    case MBF_EM12DARW:
    case MBF_EM300RAW:
    case MBF_EM300MBA: return MBF_EM300MBA;
    case MBF_EM710RAW:
    case MBF_EM710MBA: return MBF_EM710MBA;
    case MBF_IMAGE83P: return MBF_IMAGEMBA;
    case MBF_3DWISSLR: return MBF_3DWISSLP;
    case MBF_OICGEODA: return MBF_OICMBARI;
    case MBF_SB2100RW: return MBF_SB2100B2;
    default: return format;
  }
}

bool ProcessingService::recommendPreprocess(int format) const {
  switch (format) {
    case MBF_EMOLDRAW:
    case MBF_EM12IFRM:
    case MBF_EM12DARW:
    case MBF_EM300RAW:
    case MBF_EM710RAW:
    case MBF_IMAGE83P:
    case MBF_3DWISSLR:
    case MBF_OICGEODA:
    case MBF_SB2100RW:
    case MBF_RESON7KR:
    case MBF_KEMKMALL: return true;
    default: return false;
  }
}

std::string ProcessingService::preprocessOutputName(const std::string &inputPath, int format) const {
  const std::string name = platform::fileName(inputPath);
  // mb_get_format strips a recognised suffix: ".all", ".kmall", ".s7k", ".mbXX", ...
  std::string root = name;
  const auto dot = name.find_last_of('.');
  if (dot != std::string::npos && dot > 0)
    root = name.substr(0, dot);
  const std::string output = root + ".mb" + std::to_string(preprocessOutputFormat(format));
  return output == name ? root + "r.mb" + std::to_string(preprocessOutputFormat(format)) : output;
}

std::string ProcessingService::processedName(const std::string &path) const {
  // mbprocess: "root.mbXX" -> "rootp.mbXX"
  const auto dot = path.find_last_of('.');
  const auto slash = path.find_last_of("/\\");
  if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
    return path + "p";
  return path.substr(0, dot) + "p" + path.substr(dot);
}

CommandPlan ProcessingService::ancillary(const std::string &datalist) const {
  CommandPlan plan;
  plan.title = "Survey line statistics and caches";
  // -O: create .inf, .fbt and .fnv only where missing or out of date
  plan.steps.push_back(step("Statistics, fast bathymetry and navigation files", "mbdatalist",
                            {"-I" + datalist, "-O", "-V"}));
  plan.parameters["datalist"] = datalist;
  plan.ok = true;
  return plan;
}

CommandPlan ProcessingService::preprocess(const std::string &datalist, const PreprocessParameters &p) const {
  if (p.outputDirectory.empty())
    return failed("an output directory is required");
  if (p.outputDirectory.find(' ') != std::string::npos)
    return failed("mbpreprocess cannot write to a directory whose path contains spaces: " + p.outputDirectory);
  CommandPlan plan;
  plan.title = "Preprocess raw survey lines";
  std::vector<std::string> args = {"--input=" + datalist, "--output-directory=" + p.outputDirectory, "--verbose"};
  if (p.skipExisting)
    args.push_back("--skip-existing");
  if (p.recalculateBathymetry)
    args.push_back("--recalculate-bathymetry");
  if (p.ignoreWaterColumn)
    args.push_back("--ignore-water-column");
  if (!p.navigationFile.empty()) {
    args.push_back("--nav-file=" + p.navigationFile);
    args.push_back("--nav-file-format=" + std::to_string(p.navigationFormat));
  }
  if (!p.platformFile.empty()) {
    if (p.platformFile.find(' ') != std::string::npos)
      return failed("mbpreprocess cannot read a platform file whose path contains spaces");
    args.push_back("--platform-file=" + p.platformFile);
  }
  plan.steps.push_back(step("Preprocess (merge navigation and attitude, write processing format)", "mbpreprocess",
                            args));
  plan.parameters["outputDirectory"] = p.outputDirectory;
  plan.parameters["skipExisting"] = p.skipExisting;
  plan.parameters["recalculateBathymetry"] = p.recalculateBathymetry;
  plan.parameters["navigationFile"] = p.navigationFile;
  plan.parameters["platformFile"] = p.platformFile;
  plan.ok = true;
  return plan;
}

CommandPlan ProcessingService::clean(const std::string &datalist, const CleanParameters &p) const {
  CommandPlan plan;
  plan.title = "Clean bathymetry";
  std::vector<std::string> args = {"-I" + datalist};
  bool any = false;
  if (p.useSlope) {
    if (p.slope <= 0.0)
      return failed("the maximum slope must be positive");
    args.push_back("-M" + std::to_string(p.slopeMode == 2 ? 2 : 1));
    args.push_back("-C" + num(p.slope) + "/0");
    any = true;
  }
  if (p.useMedianFraction) {
    if (p.fractionLow <= 0.0 || p.fractionHigh <= p.fractionLow)
      return failed("the depth fraction range is invalid");
    args.push_back("-G" + num(p.fractionLow) + "/" + num(p.fractionHigh));
    any = true;
  }
  if (p.useDepthRange) {
    if (p.depthMax <= p.depthMin)
      return failed("the depth range is invalid");
    args.push_back("-B" + num(p.depthMin) + "/" + num(p.depthMax));
    any = true;
  }
  if (p.outerBeams > 0) {
    args.push_back("-X" + std::to_string(p.outerBeams));
    any = true;
  }
  if (p.spikes) {
    args.push_back("-S" + num(p.spikeSlope) + "/" + std::to_string(std::clamp(p.spikeMode, 1, 3)) + "/0");
    any = true;
  }
  if (p.pingRms > 0.0) {
    args.push_back("-N" + num(p.pingRms));
    any = true;
  }
  if (!any && !p.areaClean)
    return failed("choose at least one cleaning test");
  args.push_back("-V");
  if (any)
    plan.steps.push_back(step("Flag outliers ping by ping", "mbclean", args));
  if (p.areaClean) {
    std::vector<std::string> area = {"-I" + datalist};
    if (p.areaBinSize > 0.0)
      area.push_back("-S" + num(p.areaBinSize));
    area.push_back("-D" + num(p.areaStdDev) + "/" + std::to_string(std::max(1, p.areaMinSoundings)));
    area.push_back("-V");
    plan.steps.push_back(step("Flag outliers by area statistics", "mbareaclean", area));
  }
  plan.warnings.push_back("Cleaning flags soundings in edit save files; run Process to apply the flags.");
  plan.parameters["slope"] = p.useSlope ? p.slope : 0.0;
  plan.parameters["outerBeams"] = static_cast<std::int64_t>(p.outerBeams);
  plan.parameters["areaClean"] = p.areaClean;
  plan.ok = true;
  return plan;
}

CommandPlan ProcessingService::corrections(const std::string &datalist, const CorrectionParameters &p) const {
  CommandPlan plan;
  plan.title = "Set processing corrections";
  std::vector<std::string> set = {"-I" + datalist};
  if (p.svpMode == 1) {
    if (p.svpFile.empty())
      return failed("choose a sound velocity profile file");
    set.push_back("-PSVPFILE:" + p.svpFile);
  } else if (p.svpMode == 2) {
    const std::string svp = platform::joinPath(p.workingDirectory.empty() ? platform::parentPath(datalist)
                                                                         : p.workingDirectory,
                                               "levitus.svp");
    plan.steps.push_back(step("Sound velocity profile from the Levitus climatology", "mblevitus",
                              {"-R" + num(std::round(p.longitude)) + "/" + num(std::round(p.latitude)), "-O" + svp}));
    set.push_back("-PSVPFILE:" + svp);
    plan.outputs.push_back(svp);
    plan.warnings.push_back("The Levitus profile is a climatological mean; use a measured profile when available.");
  }
  if (p.useRollBias)
    set.push_back("-PROLLBIAS:" + num(p.rollBias));
  if (p.usePitchBias) {
    set.push_back("-PPITCHBIASMODE:1");
    set.push_back("-PPITCHBIAS:" + num(p.pitchBias));
  }
  if (p.useDraft)
    set.push_back("-PDRAFT:" + num(p.draft));
  if (p.tideMode == 1) {
    if (p.tideFile.empty())
      return failed("choose a tide file");
    set.push_back("-PTIDEFILE:" + p.tideFile);
    set.push_back("-PTIDEFORMAT:" + std::to_string(p.tideFormat));
  }
  if (set.size() > 1) {
    set.push_back("-V");
    plan.steps.push_back(step("Write the mbprocess parameter files", "mbset", set));
  }
  if (p.tideMode == 2) {
    plan.steps.push_back(step("Predict tides (OTPS) for every survey line", "mbotps",
                              {"--input=" + datalist, "--use-mbprocess", "--skip-existing", "--verbose"}));
    plan.warnings.push_back("Tide prediction needs the OTPS tide model data to be installed.");
  }
  if (plan.steps.empty())
    return failed("no correction selected");
  plan.warnings.push_back("Corrections take effect when the survey lines are processed.");
  plan.parameters["svpMode"] = static_cast<std::int64_t>(p.svpMode);
  plan.parameters["svpFile"] = p.svpFile;
  plan.parameters["rollBias"] = p.useRollBias ? p.rollBias : 0.0;
  plan.parameters["pitchBias"] = p.usePitchBias ? p.pitchBias : 0.0;
  plan.parameters["draft"] = p.useDraft ? p.draft : 0.0;
  plan.parameters["tideMode"] = static_cast<std::int64_t>(p.tideMode);
  plan.ok = true;
  return plan;
}

CommandPlan ProcessingService::process(const std::string &datalist, const ProcessParameters &p) const {
  CommandPlan plan;
  plan.title = "Process survey lines";
  std::vector<std::string> args = {"-I" + datalist, "-C" + std::to_string(std::max(1, p.threads))};
  if (p.force)
    args.push_back("-P");
  plan.steps.push_back(step("Apply edits and corrections (mbprocess)", "mbprocess", args));
  if (!p.processedDatalist.empty()) {
    JobStep refresh = step("Statistics of the processed files", "mbdatalist", {"-I" + p.processedDatalist, "-O"});
    refresh.allowFailure = true;
    plan.steps.push_back(refresh);
  }
  plan.parameters["threads"] = static_cast<std::int64_t>(p.threads);
  plan.parameters["force"] = p.force;
  plan.ok = true;
  return plan;
}

CommandPlan ProcessingService::grid(const std::string &datalist, const GridParameters &p) const {
  if (p.outputRoot.empty())
    return failed("an output name is required");
  if (p.dataType < 1 || p.dataType > 4)
    return failed("unknown data type");
  if (p.spacing <= 0.0)
    return failed("the cell size must be positive");
  if (!validUnits(p.units))
    return failed("unknown cell size unit: " + p.units);
  if (p.algorithm < 1 || p.algorithm > 6)
    return failed("unknown gridding algorithm");
  if (p.algorithm >= 5 && p.dataType > 2)
    return failed("the footprint algorithms grid bathymetry or topography only");
  if (p.clip < 0 || p.clipMode < 1 || p.clipMode > 3)
    return failed("invalid interpolation settings");

  CommandPlan plan;
  const char *kinds[] = {"", "bathymetry", "topography", "amplitude", "sidescan"};
  plan.title = std::string("Grid ") + kinds[p.dataType] + " (" + num(p.spacing) + " " + p.units + ")";
  std::vector<std::string> args = {"-I" + datalist, "-O" + p.outputRoot, "-A" + std::to_string(p.dataType),
                                   "-E" + num(p.spacing) + "/" + num(p.spacing) + "/" + p.units +
                                       (p.exactSpacing ? "!" : "")};
  if (p.bounds.valid())
    args.push_back("-R" + boundsText(p.bounds));
  else if (p.boundsFactor > 1.0)
    args.push_back("-R" + num(p.boundsFactor));
  std::string algorithm = "-F" + std::to_string(p.algorithm);
  if ((p.algorithm == 3 || p.algorithm == 4) && p.threshold > 0.0)
    algorithm += "/" + num(p.threshold);
  args.push_back(algorithm);
  if (p.clip > 0) {
    args.push_back("-C" + std::to_string(p.clip) + "/" + std::to_string(p.clipMode));
    if (p.tension > 0.0)
      args.push_back("-T" + num(p.tension));
    plan.warnings.push_back("Interpolation fills gaps between data; filled cells are not measured data.");
    if (p.clipMode == 1 && p.clip > 10)
      plan.warnings.push_back("A large interpolation distance with gap filling (mode 1) is very slow.");
  }
  if (p.gaussianScale > 0.0 && std::fabs(p.gaussianScale - 1.0) > 1e-9)
    args.push_back("-W" + num(p.gaussianScale));
  if (p.timeDifference != 0.0)
    args.push_back("-U" + num(p.timeDifference));
  if (!p.projection.empty())
    args.push_back("-J" + p.projection);
  const bool minmax = p.algorithm == 3 || p.algorithm == 4;
  const bool extra = p.extraGrids && !minmax;
  if (p.extraGrids && minmax)
    plan.warnings.push_back("Quality grids are not written with the minimum or maximum filter.");
  if (extra)
    args.push_back("-M");
  args.push_back("-N");  // empty cells as NaN
  args.push_back("-V");
  if (p.dataType >= 3 && p.algorithm == 1)
    plan.warnings.push_back("Amplitude and sidescan are usually gridded with the median filter.");
  plan.steps.push_back(step(plan.title, "mbgrid", args));
  plan.outputs.push_back(p.outputRoot + ".grd");
  if (extra) {
    plan.outputs.push_back(p.outputRoot + "_num.grd");
    plan.outputs.push_back(p.outputRoot + "_sd.grd");
  }
  plan.parameters["dataType"] = static_cast<std::int64_t>(p.dataType);
  plan.parameters["spacing"] = p.spacing;
  plan.parameters["units"] = p.units;
  plan.parameters["algorithm"] = static_cast<std::int64_t>(p.algorithm);
  plan.parameters["clip"] = static_cast<std::int64_t>(p.clip);
  plan.parameters["clipMode"] = static_cast<std::int64_t>(p.clipMode);
  plan.parameters["projection"] = p.projection;
  plan.parameters["extraGrids"] = extra;
  if (p.bounds.valid())
    plan.parameters["bounds"] = boundsText(p.bounds);
  plan.ok = true;
  return plan;
}

GridEstimate ProcessingService::estimateGrid(const GridParameters &p, const GeoBounds &dataBounds) const {
  GridEstimate e;
  const GeoBounds b = p.bounds.valid() ? p.bounds : dataBounds;
  if (!b.valid() || p.spacing <= 0.0)
    return e;
  const double factor = !p.bounds.valid() && p.boundsFactor > 1.0 ? p.boundsFactor : 1.0;
  const double lat = 0.5 * (b.south + b.north);
  const double cell = p.spacing * metersPerUnit(p.units, lat);
  const double width = (b.east - b.west) * 111319.49 * std::max(0.05, std::cos(lat * kPi / 180.0)) * factor;
  const double height = (b.north - b.south) * 111319.49 * factor;
  e.nx = static_cast<long long>(std::ceil(width / cell)) + 1;
  e.ny = static_cast<long long>(std::ceil(height / cell)) + 1;
  e.cells = e.nx * e.ny;
  // mbgrid keeps several double arrays per cell (sums, weights, counts, ...)
  const int arrays = p.extraGrids ? 7 : 5;
  e.megabytes = static_cast<double>(e.cells) * 8.0 * arrays / (1024.0 * 1024.0);
  return e;
}

CommandPlan ProcessingService::mosaic(const std::string &datalist, const MosaicParameters &p) const {
  if (p.outputRoot.empty())
    return failed("an output name is required");
  if (p.dataType != 3 && p.dataType != 4)
    return failed("mosaics are made of amplitude (3) or sidescan (4)");
  if (p.spacing <= 0.0 || !validUnits(p.units))
    return failed("invalid cell size");
  CommandPlan plan;
  plan.title = std::string("Mosaic ") + (p.dataType == 3 ? "amplitude" : "sidescan") + " (" + num(p.spacing) + " " +
               p.units + ")";
  std::string input = datalist;
  if (p.angleCorrection) {
    plan.steps.push_back(step("Grazing angle correction tables", "mbbackangle",
                              {"-I" + datalist, "-A" + std::string(p.dataType == 3 ? "1" : "2"), "-Q", "-V"}));
    plan.steps.push_back(step("Apply the correction (mbprocess)", "mbprocess", {"-I" + datalist}));
    if (!p.processedDatalist.empty())
      input = p.processedDatalist;
  }
  std::vector<std::string> args = {"-I" + input, "-O" + p.outputRoot, "-A" + std::to_string(p.dataType),
                                   "-E" + num(p.spacing) + "/" + num(p.spacing) + "/" + p.units + "!"};
  if (p.bounds.valid())
    args.push_back("-R" + boundsText(p.bounds));
  if (p.prioritySource >= 1 && p.prioritySource <= 8)
    args.push_back("-Y" + std::to_string(p.prioritySource));
  const bool weighted = p.priorityRange > 0.0;
  if (weighted)
    args.push_back("-F" + num(p.priorityRange));
  if (p.clip > 0) {
    args.push_back("-C" + std::to_string(p.clip) + "/1/0");
    plan.warnings.push_back("Interpolation fills gaps between data; filled cells are not measured data.");
  }
  if (!p.projection.empty())
    args.push_back("-J" + p.projection);
  const bool extra = p.extraGrids && weighted;
  if (p.extraGrids && !weighted)
    plan.warnings.push_back("Quality grids of a mosaic need a priority range.");
  if (extra)
    args.push_back("-M");
  args.push_back("-N");
  args.push_back("-V");
  plan.steps.push_back(step(plan.title, "mbmosaic", args));
  plan.outputs.push_back(p.outputRoot + ".grd");
  if (extra) {
    plan.outputs.push_back(p.outputRoot + "_num.grd");
    plan.outputs.push_back(p.outputRoot + "_sd.grd");
  }
  plan.parameters["dataType"] = static_cast<std::int64_t>(p.dataType);
  plan.parameters["spacing"] = p.spacing;
  plan.parameters["units"] = p.units;
  plan.parameters["angleCorrection"] = p.angleCorrection;
  plan.ok = true;
  return plan;
}

CommandPlan ProcessingService::exportGrid(const std::string &grid, const ExportParameters &p) const {
  if (p.output.empty())
    return failed("an output file is required");
  CommandPlan plan;
  if (p.kind == "geotiff") {
    plan.title = "Export grid as GeoTIFF";
    plan.steps.push_back(step(plan.title, "gmt", {"grdconvert", grid, p.output + "=gd:GTiff"}));
  } else if (p.kind == "esri") {
    plan.title = "Export grid as ESRI ASCII grid";
    plan.steps.push_back(step(plan.title, "gmt", {"grdconvert", grid, p.output + "=ef"}));
  } else if (p.kind == "xyz") {
    plan.title = "Export grid nodes as XYZ text";
    // "->file" is GMT's own output redirection (no shell involved)
    plan.steps.push_back(step(plan.title, "gmt", {"grd2xyz", grid, "-s", "->" + p.output}));
  } else if (p.kind == "gltf") {
    plan.title = "Export grid as a 3D model (glTF)";
    std::string root = p.output;
    if (root.size() > 4 && (root.compare(root.size() - 4, 4, ".glb") == 0))
      root = root.substr(0, root.size() - 4);
    plan.steps.push_back(step(plan.title, "mbgrd2gltf",
                              {"-I" + grid, "-O" + root, "-B", "-E" + num(p.exaggeration > 0 ? p.exaggeration : 1.0)}));
    plan.outputs.push_back(root + ".glb");
  } else {
    return failed("unknown grid export format: " + p.kind);
  }
  if (plan.outputs.empty())
    plan.outputs.push_back(p.output);
  plan.parameters["kind"] = p.kind;
  plan.parameters["source"] = grid;
  plan.ok = true;
  return plan;
}

CommandPlan ProcessingService::exportSoundings(const std::string &datalist, const ExportParameters &p) const {
  if (p.output.empty())
    return failed("an output file is required");
  CommandPlan plan;
  if (p.kind == "las") {
    plan.title = "Export soundings as LAS point cloud";
    std::vector<std::string> args = {"-I" + datalist, "-O" + p.output, "-P"};
    if (!p.projection.empty())
      args.push_back("-J" + p.projection);
    args.push_back("-V");
    plan.steps.push_back(step(plan.title, "mbswath2las", args));
  } else if (p.kind == "xyz") {
    plan.title = "Export soundings as XYZ text";
    plan.steps.push_back(step(plan.title, "mblist", {"-I" + datalist, "-D2", "-X" + p.output}));
  } else {
    return failed("unknown sounding export format: " + p.kind);
  }
  plan.outputs.push_back(p.output);
  plan.parameters["kind"] = p.kind;
  plan.ok = true;
  return plan;
}

}  // namespace mbstudio::mbsystem

namespace mbstudio::mbsystem {

namespace {

bool isAscii(const std::string &text) {
  for (const unsigned char c : text)
    if (c >= 0x80)
      return false;
  return true;
}

}  // namespace

std::string ProcessingService::findGhostscript(const std::vector<std::string> &extraDirectories) {
#ifdef _WIN32
  const std::vector<std::string> names = {"gswin64c.exe", "gswin32c.exe", "gs.exe"};
#else
  const std::vector<std::string> names = {"gs"};
#endif
  std::vector<std::string> dirs = extraDirectories;
  for (const auto &d : platform::splitPathList(platform::getEnv("PATH")))
    dirs.push_back(d);
#ifdef _WIN32
  // the Ghostscript installer does not change PATH: C:\Program Files\gs\gs10.03.1\bin
  for (const std::string &base : {platform::getEnv("ProgramFiles"), platform::getEnv("ProgramW6432"),
                                  platform::getEnv("ProgramFiles(x86)")}) {
    if (base.empty())
      continue;
    const std::string gs = platform::joinPath(base, "gs");
    std::vector<std::string> versions = platform::listDirectory(gs);
    std::sort(versions.rbegin(), versions.rend());  // newest first
    for (const auto &v : versions)
      dirs.push_back(platform::joinPath(platform::joinPath(gs, v), "bin"));
  }
#endif
  for (const auto &dir : dirs) {
    if (dir.empty())
      continue;
    for (const auto &name : names) {
      const std::string candidate = platform::joinPath(dir, name);
      if (platform::fileExists(candidate) && !platform::isDirectory(candidate))
        return candidate;
    }
  }
  return std::string();
}

double ProcessingService::scaleBarKm(double widthKm) {
  if (!(widthKm > 0.0))
    return 1.0;
  const double target = widthKm / 5.0;
  const double power = std::pow(10.0, std::floor(std::log10(target)));
  for (const double m : {5.0, 2.0, 1.0})
    if (m * power <= target)
      return m * power;
  return power;
}

FigureDependencies ProcessingService::figureDependencies() const {
  FigureDependencies d;
  std::vector<std::string> extra;
  ToolInfo gmt;
  if (tools_ && tools_->tool("gmt", gmt) && gmt.available) {
    d.gmt = true;
    extra.push_back(platform::parentPath(gmt.path));
  }
  d.ghostscriptPath = findGhostscript(extra);
  d.ghostscript = !d.ghostscriptPath.empty();
  if (!d.gmt)
    d.message = "GMT (gmt) is not installed with MB-Studio";
  else if (!d.ghostscript)
    d.message = "Ghostscript was not found: GMT needs it to write PNG and PDF figures (PostScript works without it). "
                "Install Ghostscript from https://ghostscript.com and restart MB-Studio.";
  return d;
}

CommandPlan ProcessingService::figure(const FigureParameters &p) const {
  const std::string &kind = p.kind;
  if (kind != "bathymetry" && kind != "overview" && kind != "quality" && kind != "perspective")
    return failed("unknown figure template: " + kind);
  if (p.outputRoot.empty())
    return failed("an output name is required");
  if (p.grid.empty() && kind != "overview")
    return failed("choose a grid for the figure");
  if (p.grid.empty() && p.tracks.empty())
    return failed("the figure has neither a grid nor survey lines");
  if (p.grid.empty() && !p.region.valid())
    return failed("the extent of the survey lines is not known yet");
  if (p.formats.empty())
    return failed("choose at least one figure format");
  if (!(p.widthCm >= 2.0 && p.widthCm <= 200.0))
    return failed("the figure width must be between 2 and 200 cm");
  if (p.dpi < 30 || p.dpi > 2400)
    return failed("the resolution must be between 30 and 2400 dpi");
  if (p.contourInterval < 0.0 || p.annotationInterval < 0.0)
    return failed("contour intervals cannot be negative");
  bool raster = false;
  std::string formats;
  for (const auto &f : p.formats) {
    if (f != "png" && f != "pdf" && f != "ps")
      return failed("unknown figure format: " + f);
    raster = raster || f != "ps";
    formats += (formats.empty() ? "" : ",") + f;
  }

  CommandPlan plan;
  const char *titles[] = {"Bathymetric map", "Survey overview", "Grid quality map", "Perspective view"};
  const int t = kind == "bathymetry" ? 0 : kind == "overview" ? 1 : kind == "quality" ? 2 : 3;
  plan.title = std::string(titles[t]) + " (GMT)";
  if (raster) {
    const FigureDependencies d = figureDependencies();
    if (tools_ && !d.gmt)
      return failed(d.message);
    if (!d.ghostscript && p.ghostscript.empty())
      return failed("PNG and PDF figures need Ghostscript, which was not found. Choose PostScript, or install "
                    "Ghostscript (https://ghostscript.com) and restart MB-Studio.");
  }
  if (!isAscii(p.title) || !isAscii(p.zLabel))
    plan.warnings.push_back("GMT's standard fonts cannot show non-Latin characters; use a title in Latin letters.");

  // every gmt call of the job joins this private session
  std::vector<std::pair<std::string, std::string>> environment;
  if (!p.session.empty())
    environment.emplace_back("GMT_SESSION_NAME", p.session);
  if (!p.temporaryDirectory.empty())
    environment.emplace_back("GMT_TMPDIR", p.temporaryDirectory);
  auto gmt = [&plan, &environment](const std::string &title, std::vector<std::string> args) {
    JobStep s = step(title, "gmt", std::move(args));
    s.environment = environment;
    plan.steps.push_back(s);
  };

  // psconvert options (raster and PDF output only; PostScript needs no
  // conversion): crop with a margin, the raster resolution and the
  // Ghostscript found here (GMT's own search does not know every location)
  std::vector<std::string> begin = {"begin", p.outputRoot, formats};
  if (raster) {
    std::string options = "A+m0.4c,E" + std::to_string(p.dpi);
    if (!p.ghostscript.empty() && p.ghostscript.find(',') == std::string::npos)
      options += ",G" + p.ghostscript;
    begin.push_back(options);
  }
  gmt("Start the figure", begin);

  const std::string width = num(p.widthCm) + "c";
  const std::string projection = p.geographic ? "-JM" + width : "-JX" + width + "/0";
  const std::string region = p.region.valid() ? "-R" + boundsText(p.region) : std::string();
  const std::string palette = p.palette.empty() ? std::string("haxby") : p.palette;
  std::string illumination;
  if (p.illumination)
    illumination = "-I+a" + num(p.lightAzimuth) + "+nt1";

  if (!p.grid.empty()) {
    std::vector<std::string> cpt = {"grd2cpt", p.grid, "-C" + palette, "-Z"};
    if (p.reversePalette)
      cpt.push_back("-I");
    gmt("Colour table from the grid", cpt);
  }

  if (kind == "perspective") {
    std::vector<std::string> args = {"grdview", p.grid, projection, "-JZ" + num(p.reliefHeightCm) + "c",
                                     "-p" + num(p.viewAzimuth) + "/" + num(p.viewElevation), "-Qi" + std::to_string(std::min(p.dpi, 300))};
    if (!region.empty())
      args.push_back(region);
    if (!illumination.empty())
      args.push_back(illumination);
    args.push_back("-Baf");
    args.push_back("-Bzaf+l" + p.zLabel);
    args.push_back(p.title.empty() ? "-BWSneZ" : "-BWSneZ+t" + p.title);
    gmt("Perspective surface (grdview)", args);
  } else if (!p.grid.empty()) {
    std::vector<std::string> args = {"grdimage", p.grid, projection};
    if (!region.empty())
      args.push_back(region);
    if (!illumination.empty() && kind != "quality")
      args.push_back(illumination);
    args.push_back("-Baf");
    args.push_back(p.title.empty() ? "-BWSne" : "-BWSne+t" + p.title);
    gmt("Grid image (grdimage)", args);
  } else {
    gmt("Map frame (basemap)", {"basemap", projection, region, "-Baf", p.title.empty() ? "-BWSne" : "-BWSne+t" + p.title});
  }

  if (p.contourInterval > 0.0 && !p.grid.empty() && kind != "perspective" && kind != "quality") {
    std::vector<std::string> args = {"grdcontour", p.grid, "-C" + num(p.contourInterval), "-W0.25p,gray25"};
    if (p.annotationInterval > 0.0)
      args.push_back("-A" + num(p.annotationInterval) + "+f6p,Helvetica,gray15");
    gmt("Contours (grdcontour)", args);
  }
  if (!p.tracks.empty() && kind != "perspective") {
    // ".fnv": date and time, epoch seconds, longitude (column 7), latitude (8)
    std::vector<std::string> args = {"plot"};
    args.insert(args.end(), p.tracks.begin(), p.tracks.end());
    args.push_back("-i7,8");
    args.push_back(kind == "overview" && p.grid.empty() ? "-W0.8p,darkblue" : "-W0.5p,black");
    gmt("Survey lines", args);
  }
  if (p.colorbar && !p.grid.empty()) {
    std::vector<std::string> args = {"colorbar", "-DJBC+o0/1.1c+w" + num(p.widthCm * 0.6) + "c/0.3c+h",
                                     "-Baf+l" + p.zLabel};
    if (kind == "perspective")
      args.push_back("-p");
    gmt("Colour scale", args);
  }
  if (p.scaleBar && p.geographic && kind != "perspective") {
    // about a fifth of the map width
    double widthKm = 0.0;
    const GeoBounds &extent = p.region.valid() ? p.region : p.gridBounds;
    if (extent.valid())
      widthKm = (extent.east - extent.west) * 111.32 *
                std::max(0.05, std::cos(0.5 * (extent.south + extent.north) * kPi / 180.0));
    const double length = scaleBarKm(widthKm > 0.0 ? widthKm : 10.0);
    std::vector<std::string> args = {"basemap", "-LjBL+w" + num(length) + "k+o0.4c/0.4c+f+u", "-Fl+gwhite@25+p0.25p"};
    gmt("Scale bar", args);
    if (widthKm <= 0.0)
      plan.warnings.push_back("The scale bar length is a default; set the extent for a matching scale.");
  }
  gmt("Write the figure", {"end"});

  for (const auto &f : p.formats)
    plan.outputs.push_back(p.outputRoot + "." + f);
  if (!p.geographic && p.scaleBar)
    plan.warnings.push_back("Projected maps show coordinates in metres; no scale bar is drawn.");
  plan.parameters["template"] = kind;
  plan.parameters["grid"] = p.grid;
  plan.parameters["palette"] = palette;
  plan.parameters["width_cm"] = p.widthCm;
  plan.parameters["dpi"] = static_cast<std::int64_t>(p.dpi);
  plan.parameters["contours"] = p.contourInterval;
  plan.parameters["formats"] = formats;
  plan.parameters["session"] = p.session;
  plan.ok = true;
  return plan;
}

}  // namespace mbstudio::mbsystem
