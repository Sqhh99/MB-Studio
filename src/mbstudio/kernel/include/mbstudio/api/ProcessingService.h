// MB-Studio service interface - typed MB-System processing recipes.
//
// Each recipe turns user level parameters (cell size in metres, gridding
// algorithm, cleaning thresholds, ...) into the job steps that run the
// MB-System programs with a structured argument list, together with the
// products it will write and warnings for the user. The mapping from a
// parameter to a program option is explicit, tool by tool (see the help
// pages in src/mbstudio/help), never derived from parameter names.
#pragma once

#include <string>
#include <vector>

#include "mbstudio/api/JobService.h"
#include "mbstudio/platform/Process.h"

namespace mbstudio {

struct DatalistEntry {
  std::string path;  // absolute
  int format = 0;
  double weight = 1.0;
};

struct GeoBounds {
  double west = 0.0, east = 0.0, south = 0.0, north = 0.0;
  bool valid() const { return east > west && north > south; }
};

struct CommandPlan {
  bool ok = false;
  std::string error;                 // why the plan cannot run
  std::string title;
  std::vector<JobStep> steps;
  std::vector<std::string> outputs;  // products, absolute paths
  std::vector<std::string> warnings;
  ValueMap parameters;               // provenance
  // display form of the programs ("mbgrid -I... -O...") in step order
  std::vector<std::string> commandLines() const {
    std::vector<std::string> out;
    for (const auto &step : steps)
      if (!step.tool.empty())
        out.push_back(platform::formatCommand(step.tool, step.arguments));
    return out;
  }
};

struct PreprocessParameters {
  std::string outputDirectory;   // must exist and must not contain spaces
  std::string navigationFile;    // optional external navigation
  int navigationFormat = 9;
  std::string platformFile;      // optional lever arms
  bool skipExisting = true;
  bool recalculateBathymetry = false;
  bool ignoreWaterColumn = true;
};

struct CleanParameters {
  bool useSlope = true;
  double slope = 1.0;            // tangent
  int slopeMode = 1;             // 1 flag the outlying beam, 2 flag both
  bool useMedianFraction = false;
  double fractionLow = 0.8, fractionHigh = 1.2;
  bool useDepthRange = false;
  double depthMin = 0.0, depthMax = 11000.0;
  int outerBeams = 0;            // flag this many beams on each side
  bool spikes = false;
  double spikeSlope = 1.0;
  int spikeMode = 3;             // 1 acrosstrack, 2 alongtrack, 3 both
  double pingRms = 0.0;          // 0: off
  bool areaClean = false;        // also run mbareaclean (statistical bins)
  double areaBinSize = 0.0;      // m, 0 automatic
  double areaStdDev = 2.0;
  int areaMinSoundings = 10;
};

struct CorrectionParameters {
  int svpMode = 0;               // 0 keep, 1 SVP file, 2 Levitus climatology
  std::string svpFile;
  double longitude = 0.0, latitude = 0.0;  // for the Levitus profile
  bool useRollBias = false;
  double rollBias = 0.0;         // degrees
  bool usePitchBias = false;
  double pitchBias = 0.0;
  bool useDraft = false;
  double draft = 0.0;            // m
  int tideMode = 0;              // 0 none, 1 tide file, 2 OTPS prediction
  std::string tideFile;
  int tideFormat = 2;
  std::string workingDirectory;  // where generated files go (SVP)
};

struct ProcessParameters {
  int threads = 2;
  bool force = false;
  std::string processedDatalist;  // to refresh .inf/.fbt/.fnv afterwards
};

struct GridParameters {
  int dataType = 1;              // 1 bathymetry, 2 topography, 3 amplitude, 4 sidescan
  double spacing = 25.0;
  std::string units = "meters";
  bool exactSpacing = true;
  GeoBounds bounds;              // invalid: automatic
  double boundsFactor = 0.0;     // > 1 enlarges automatic bounds
  int algorithm = 1;             // 1 mean, 2 median, 3 min, 4 max, 5 footprint slope, 6 footprint
  double threshold = 0.0;        // for 3 and 4: weighted min/max
  int clip = 0;                  // spline interpolation distance in cells, 0 none
  int clipMode = 1;
  double tension = 0.0;
  double gaussianScale = 1.0;
  bool extraGrids = true;        // root_num.grd and root_sd.grd
  std::string projection;        // empty: geographic, "U": automatic UTM, ...
  double timeDifference = 0.0;   // -U minutes, 0 off
  std::string outputRoot;        // absolute, without .grd
};

struct GridEstimate {
  long long nx = 0, ny = 0;
  long long cells = 0;
  double megabytes = 0.0;        // working memory, rough
};

struct MosaicParameters {
  int dataType = 4;              // 3 amplitude, 4 sidescan
  double spacing = 5.0;
  std::string units = "meters";
  GeoBounds bounds;
  int prioritySource = 0;        // -Y table 1..8, 0 none
  double priorityRange = 0.0;    // -F, 0 off
  int clip = 0;
  bool extraGrids = false;
  std::string projection;
  bool angleCorrection = false;  // mbbackangle + mbprocess first
  std::string processedDatalist; // input when angleCorrection is set
  std::string outputRoot;
};

struct ExportParameters {
  std::string kind;              // grid: geotiff, esri, xyz, gltf; soundings: las, xyz
  std::string output;            // absolute file path (or root for gltf)
  std::string projection;        // LAS only
  double exaggeration = 1.0;     // glTF
};

class IProcessingService {
 public:
  static constexpr const char *kServiceId = "mbsystem.processing";
  virtual ~IProcessingService() = default;

  // Datalist text; paths relative to datalistDirectory when possible.
  // Fails for paths with spaces, which MB-System datalists cannot hold.
  virtual bool datalistText(const std::vector<DatalistEntry> &entries, const std::string &datalistDirectory,
                            bool processed, std::string &text, std::string &error) const = 0;
  // A datalist that references another one with $PROCESSED set.
  virtual std::string processedDatalistText(const std::string &datalistName) const = 0;

  // Format written by mbpreprocess for an input format, and whether
  // preprocessing is recommended (raw vendor formats).
  virtual int preprocessOutputFormat(int format) const = 0;
  virtual bool recommendPreprocess(int format) const = 0;
  // File name (without directory) mbpreprocess writes for an input file.
  virtual std::string preprocessOutputName(const std::string &inputPath, int format) const = 0;
  // mbprocess output for a processing format file ("xp.mb59").
  virtual std::string processedName(const std::string &path) const = 0;

  virtual CommandPlan ancillary(const std::string &datalist) const = 0;
  virtual CommandPlan preprocess(const std::string &datalist, const PreprocessParameters &p) const = 0;
  virtual CommandPlan clean(const std::string &datalist, const CleanParameters &p) const = 0;
  virtual CommandPlan corrections(const std::string &datalist, const CorrectionParameters &p) const = 0;
  virtual CommandPlan process(const std::string &datalist, const ProcessParameters &p) const = 0;
  virtual CommandPlan grid(const std::string &datalist, const GridParameters &p) const = 0;
  virtual GridEstimate estimateGrid(const GridParameters &p, const GeoBounds &dataBounds) const = 0;
  virtual CommandPlan mosaic(const std::string &datalist, const MosaicParameters &p) const = 0;
  virtual CommandPlan exportGrid(const std::string &grid, const ExportParameters &p) const = 0;
  virtual CommandPlan exportSoundings(const std::string &datalist, const ExportParameters &p) const = 0;
};

}  // namespace mbstudio
