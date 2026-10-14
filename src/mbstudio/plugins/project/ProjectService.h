// MB-Studio project plugin - the survey project service (see
// mbstudio/api/ProjectService.h). Qt Core is used only for JSON.
#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "mbstudio/api/FormatService.h"
#include "mbstudio/api/GridService.h"
#include "mbstudio/api/JobService.h"
#include "mbstudio/api/ProcessingService.h"
#include "mbstudio/api/ProjectService.h"
#include "mbstudio/api/SoundingService.h"
#include "mbstudio/api/SwathService.h"
#include "mbstudio/kernel/EventBus.h"

namespace mbstudio::project {

class ProjectService : public IProjectService, public std::enable_shared_from_this<ProjectService> {
 public:
  static constexpr const char *kSuffix = ".mbsproj";

  ProjectService(std::shared_ptr<IFormatService> formats, std::shared_ptr<ISwathService> swath,
                 std::shared_ptr<IProcessingService> processing, std::shared_ptr<IJobService> jobs,
                 EventBus *events, std::string mbsystemVersion, std::string mbstudioVersion,
                 std::shared_ptr<ISoundingService> soundings = nullptr, std::shared_ptr<IGridService> grids = nullptr);
  ~ProjectService() override;

  // Follow the job service (call once the object is owned by a shared_ptr).
  void attach();
  // Stop following jobs and cancel the ones this service started.
  void detach();

  bool create(const std::string &directory, const std::string &name, std::string &error) override;
  bool open(const std::string &path, std::string &error) override;
  bool save(std::string &error) override;
  void close() override;
  bool isOpen() const override;
  std::string name() const override;
  std::string root() const override;
  std::string file() const override;

  std::vector<SurveyLine> lines() const override;
  std::vector<ProjectAsset> assets() const override;
  std::vector<ProjectLayer> layers() const override;
  std::vector<JobRecord> history() const override;
  GeoBounds bounds(const std::vector<std::string> &lineIds = {}) const override;

  ImportResult addFiles(const std::vector<std::string> &paths, bool recursive) override;
  int removeLines(const std::vector<std::string> &ids) override;
  bool removeAsset(const std::string &id, bool deleteFiles) override;
  std::string addGrid(const std::string &path) override;
  std::vector<LineVersion> lineVersions(const std::string &lineId) const override;

  bool setLayer(const ProjectLayer &layer) override;
  bool moveLayer(const std::string &asset, int position) override;
  std::string workspaceState() const override;
  void setWorkspaceState(const std::string &json) override;

  std::vector<TrackPoint> navigation(const std::string &lineId, std::size_t maxPoints) const override;

  CommandPlan plan(const ProcessingRequest &request) const override;
  std::int64_t run(const ProcessingRequest &request, std::string &error) override;
  std::int64_t retry(std::int64_t sequence, std::string &error) override;
  bool request(std::int64_t sequence, ProcessingRequest &request) const override;
  std::int64_t regenerate(const std::string &assetId, std::string &error) override;
  std::int64_t saveEdits(const std::map<std::string, std::vector<BeamEdit>> &edits, std::string &error) override;
  NextAction nextAction() const override;
  std::int64_t sequenceOfJob(std::int64_t jobId) const override;
  std::string datalist(const std::string &which) const override;

  // Bring the history entry of a job up to date (idempotent).
  void syncJob(std::int64_t jobId);
  // Wait for every job of this project (command line use); false on time-out.
  bool waitForJobs(int timeoutMs);

  // A file name root without characters that MB-System or the shell dislike.
  static std::string safeName(const std::string &name);
  // Resources a job of this kind reads and writes (see JobSpec), for the
  // survey lines, the grid it reads and the files it writes.
  static void resources(const std::string &kind, const std::vector<std::string> &lineIds, const std::string &readFile,
                        const std::vector<std::string> &outputs, bool angleCorrection, std::vector<std::string> &reads,
                        std::vector<std::string> &writes);

 private:
  struct Selection {
    std::vector<SurveyLine> lines;
    std::string work, workProcessed, raw;  // datalist paths
  };
  void changed(const std::string &what) const;
  bool saveLocked(std::string &error);
  bool loadLocked(const std::string &file, std::string &error);
  bool writeProjectDatalistsLocked(std::string &error);
  bool writeSelectionLocked(const std::vector<SurveyLine> &lines, const std::string &directory, Selection &selection,
                            std::string &error) const;
  std::vector<SurveyLine> selectLocked(const std::vector<std::string> &ids) const;
  CommandPlan planLocked(const ProcessingRequest &request, const std::string &jobDirectory,
                         const Selection &selection, std::int64_t sequence) const;
  // Plan, record and submit a request (mutex_ held); returns the job id.
  std::int64_t submitLocked(const ProcessingRequest &request, std::int64_t retryOf, int attempt, std::string &error);
  // The statistics/cache job of survey lines (import, mutex_ held).
  std::int64_t submitImportLocked(const std::vector<std::string> &ids, std::int64_t retryOf, int attempt,
                                  std::string &error);
  std::string uniqueRootLocked(const std::string &directory, const std::string &name, const std::string &ext) const;
  std::string newId(char prefix);
  void collect(const std::string &path, bool recursive, int depth, std::vector<std::string> &files,
               std::vector<std::string> &skipped, bool explicitFile) const;
  ProjectAsset *assetLocked(const std::string &id);
  const JobRecord *recordLocked(std::int64_t sequence) const;
  void deriveStatusLocked(ProjectAsset &asset) const;
  std::string fnvOf(const SurveyLine &line) const;
  void registerOutputs(const std::string &kind, const std::vector<std::string> &outputs, const std::string &label,
                       std::int64_t sequence, const ValueMap &parameters,
                       const std::map<std::string, std::string> &inputs);
  void release(std::int64_t sequence);

  std::shared_ptr<IFormatService> formats_;
  std::shared_ptr<ISwathService> swath_;
  std::shared_ptr<IProcessingService> processing_;
  std::shared_ptr<IJobService> jobs_;
  EventBus *events_;
  std::string mbsystemVersion_;
  std::string mbstudioVersion_;
  std::shared_ptr<ISoundingService> soundings_;
  std::shared_ptr<IGridService> grids_;
  EventBus::SubscriptionId subscription_ = 0;

  mutable std::mutex mutex_;
  bool open_ = false;
  std::string name_;
  std::string root_;
  std::string file_;
  double created_ = 0.0;
  std::int64_t nextLine_ = 1, nextAsset_ = 1, nextJob_ = 1;
  std::vector<SurveyLine> lines_;
  std::vector<ProjectAsset> assets_;
  std::vector<ProjectLayer> layers_;
  std::vector<JobRecord> history_;
  std::map<std::int64_t, std::int64_t> pending_;  // job service id -> history sequence
  // output files of queued and running jobs (by history sequence): a new
  // job never chooses a name that one of them is about to write
  std::map<std::int64_t, std::vector<std::string>> reserved_;
  std::string workspace_ = "{}";
};

}  // namespace mbstudio::project
