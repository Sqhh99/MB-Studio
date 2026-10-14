// MB-Studio service interface - survey projects.
//
// A project is a folder with a "<name>.mbsproj" file (JSON) that references
// the survey lines (raw swath files stay where they are), the products made
// from them (grids, mosaics, figures, exports), the layers shown in the
// workbench and the processing history with full provenance. The service
// writes the MB-System datalists itself, so the user never has to, and turns
// processing requests on a selection of lines into background jobs whose
// products are registered in the project when they finish.
//
// Correctness rules:
//  - every job declares the survey lines and files it reads and writes; the
//    job service never runs conflicting jobs at the same time;
//  - output names are reserved while a job is queued or running, so two
//    jobs never write the same product;
//  - a job is repeated (retry) as a new attempt with its own history entry,
//    job folder and outputs;
//  - each survey line counts its edit, correction and source revisions and
//    remembers which ones its processed file contains; each product
//    remembers what it was made from, so the project can tell which lines
//    need processing and which products are outdated.
//
// Events: "project.changed" with "what" = opened | closed | lines | assets |
// layers | history | workspace.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "mbstudio/api/ProcessingService.h"
#include "mbstudio/api/SoundingService.h"
#include "mbstudio/api/SwathService.h"
#include "mbstudio/kernel/Value.h"

namespace mbstudio {

struct SurveyLine {
  std::string id;
  std::string path;            // original (raw) file, absolute
  int format = 0;
  std::string formatName;
  long long size = 0;
  bool missing = false;        // the file no longer exists
  std::string processingPath;  // preprocessed file in the processing format (empty: the raw file)
  int processingFormat = 0;
  std::string state;           // raw | preprocessed | processed (files that exist)
  // revisions: edits (cleaning, sounding editor) and corrections (mbset,
  // tides) set for the working file, the working file itself (a new one
  // after preprocessing), and what the processed file was made from (-1:
  // never processed)
  int editRevision = 0;
  int correctionRevision = 0;
  int sourceRevision = 0;
  int processedEdit = -1, processedCorrection = -1, processedSource = -1;
  // metadata (after the import job)
  bool inspected = false;
  long records = 0;
  int maxBeams = 0;
  long beamsTotal = 0, beamsGood = 0;
  std::string timeStart, timeEnd;  // ISO 8601
  GeoBounds bounds;
  double depthMin = 0.0, depthMax = 0.0;
  double trackLengthKm = 0.0;

  // the file the processing programs work on
  std::string workPath() const { return processingPath.empty() ? path : processingPath; }
  int workFormat() const { return processingPath.empty() ? format : processingFormat; }
  bool everProcessed() const { return processedEdit >= 0 && processedSource == sourceRevision; }
  // unprocessed | processing-required | edits-pending | corrections-pending | processed
  std::string processingStatus() const {
    if (processedEdit < 0 || processedSource != sourceRevision)
      return editRevision > 0 || correctionRevision > 0 ? "processing-required" : "unprocessed";
    if (processedEdit != editRevision)
      return "edits-pending";
    if (processedCorrection != correctionRevision)
      return "corrections-pending";
    return "processed";
  }
  // What a product made now reads from the line: gridding uses the
  // processed file where it exists ($PROCESSED), otherwise the working file.
  std::string dataStamp() const {
    if (everProcessed())
      return "p" + std::to_string(sourceRevision) + "." + std::to_string(processedEdit) + "." +
             std::to_string(processedCorrection);
    return "w" + std::to_string(sourceRevision);
  }
};

// A file version of a survey line that can be inspected or edited.
struct LineVersion {
  std::string version;  // original | preprocessed | processed
  std::string path;
  int format = 0;
  bool exists = false;
};

struct ProjectAsset {
  std::string id;
  std::string path;     // absolute
  // grid | grid_num | grid_sd | mosaic | mosaic_num | mosaic_sd | figure | export | svp | datalist
  std::string type;
  std::string label;
  std::string parent;   // asset id of the main product (quality grids, figure formats), or empty
  std::int64_t job = 0; // history entry that made it
  bool regenerable = true;
  ValueMap parameters;
  double created = 0.0;
  // survey line id -> SurveyLine::dataStamp() when the product was made
  std::map<std::string, std::string> inputs;
  // derived when the assets are read: current | outdated | pending | unknown
  // (pending: made from current data, but edits or corrections of some of
  // its lines are not processed yet)
  std::string status;
  int staleInputs = 0;  // lines that changed (outdated) or wait for processing (pending)
};

struct ProjectLayer {
  std::string asset;    // asset id
  bool visible = true;
  double opacity = 1.0;
  int colorMap = 0;
  bool hillshade = true;
};

struct JobRecord {
  std::int64_t sequence = 0;  // project wide number (folder jobs/<sequence>)
  std::int64_t jobId = 0;     // id in the job service of this session
  std::string kind;
  std::string title;
  std::string state;          // queued, running, completed, failed, cancelled
  double started = 0.0, finished = 0.0;
  int exitCode = 0;
  std::string error;
  std::vector<std::string> commands;
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  ValueMap parameters;
  std::string logPath;
  std::string workingDirectory;
  std::string mbsystemVersion;
  int attempt = 1;            // 2, 3, ... for repeated jobs
  std::int64_t retryOf = 0;   // sequence of the attempt this one repeats
  std::vector<std::string> lines;  // survey lines the job worked on
  std::string request;        // the processing request (JSON), to run it again
};

struct ProcessingRequest {
  // preprocess | clean | corrections | process | grid | mosaic | export-grid |
  // export-soundings | figure
  std::string kind;
  std::vector<std::string> lines;  // line ids (empty: all lines)
  std::string asset;               // grid asset for export-grid and figure
  std::string name;                // product name (file root)
  PreprocessParameters preprocess;
  CleanParameters clean;
  CorrectionParameters corrections;
  ProcessParameters process;
  GridParameters grid;
  MosaicParameters mosaic;
  ExportParameters exporting;
  FigureParameters figure;
};

struct ImportResult {
  int added = 0;
  std::vector<std::string> skipped;  // "<file>: <reason>"
  std::int64_t job = 0;              // the statistics job (0 if none)
};

// The step the project needs next.
struct NextAction {
  std::string kind;     // import | preprocess | process | regenerate | grid | "" (nothing to do)
  std::string reason;   // for the user
  int count = 0;        // lines or products concerned
};

class IProjectService {
 public:
  static constexpr const char *kServiceId = "mbstudio.project";
  virtual ~IProjectService() = default;

  // Create a project in directory (created if needed) and open it.
  virtual bool create(const std::string &directory, const std::string &name, std::string &error) = 0;
  // Open a ".mbsproj" file (or the project in a folder).
  virtual bool open(const std::string &path, std::string &error) = 0;
  virtual bool save(std::string &error) = 0;
  virtual void close() = 0;
  virtual bool isOpen() const = 0;
  virtual std::string name() const = 0;
  virtual std::string root() const = 0;
  virtual std::string file() const = 0;

  virtual std::vector<SurveyLine> lines() const = 0;
  virtual std::vector<ProjectAsset> assets() const = 0;
  virtual std::vector<ProjectLayer> layers() const = 0;
  virtual std::vector<JobRecord> history() const = 0;
  virtual GeoBounds bounds(const std::vector<std::string> &lineIds = {}) const = 0;

  // Add swath files, or every swath file found in folders (recursive), skip
  // what cannot be used, write the datalists and start a background job for
  // the statistics, caches and metadata of the new lines.
  virtual ImportResult addFiles(const std::vector<std::string> &paths, bool recursive) = 0;
  virtual int removeLines(const std::vector<std::string> &ids) = 0;
  virtual bool removeAsset(const std::string &id, bool deleteFiles) = 0;
  // Register an existing grid (e.g. opened from disk) as a product.
  virtual std::string addGrid(const std::string &path) = 0;

  // The file versions of a survey line: original, preprocessed (if the
  // line was preprocessed) and processed (if mbprocess wrote it).
  virtual std::vector<LineVersion> lineVersions(const std::string &lineId) const = 0;

  virtual bool setLayer(const ProjectLayer &layer) = 0;
  virtual bool moveLayer(const std::string &asset, int position) = 0;
  // Opaque front end state (open tabs, view settings) kept in the project.
  virtual std::string workspaceState() const = 0;
  virtual void setWorkspaceState(const std::string &json) = 0;

  // Navigation of a line from its ".fnv" file (processed one preferred).
  virtual std::vector<TrackPoint> navigation(const std::string &lineId, std::size_t maxPoints) const = 0;

  // The job a request would run (for previews), and running it.
  virtual CommandPlan plan(const ProcessingRequest &request) const = 0;
  virtual std::int64_t run(const ProcessingRequest &request, std::string &error) = 0;
  // Run a recorded job again as a new attempt (new history entry, job
  // folder and outputs); returns the job id.
  virtual std::int64_t retry(std::int64_t sequence, std::string &error) = 0;
  // The request of a history entry (false if it was not recorded).
  virtual bool request(std::int64_t sequence, ProcessingRequest &request) const = 0;
  // Make a product again from its recorded request and the current data.
  virtual std::int64_t regenerate(const std::string &assetId, std::string &error) = 0;
  // Save sounding edits (line id -> edits) as a job that appends them to
  // the edit save files of the lines' working files.
  virtual std::int64_t saveEdits(const std::map<std::string, std::vector<BeamEdit>> &edits, std::string &error) = 0;
  virtual NextAction nextAction() const = 0;
  // History sequence of a job of this session (0 if it is not a project job).
  virtual std::int64_t sequenceOfJob(std::int64_t jobId) const = 0;
  // Paths of the project datalists (raw, work, work with $PROCESSED).
  virtual std::string datalist(const std::string &which) const = 0;
};

}  // namespace mbstudio
