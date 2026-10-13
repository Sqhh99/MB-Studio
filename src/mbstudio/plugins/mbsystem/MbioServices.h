// MB-Studio mbsystem plugin - service implementations backed by MB-System.
#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "mbstudio/api/FormatService.h"
#include "mbstudio/api/GridService.h"
#include "mbstudio/api/SwathService.h"
#include "mbstudio/api/HelpService.h"
#include "mbstudio/api/JobService.h"
#include "mbstudio/api/ProcessingService.h"
#include "mbstudio/api/ToolService.h"
#include "mbstudio/kernel/EventBus.h"

namespace mbstudio::mbsystem {

// The MBIO library keeps some global state; all calls into it are serialised.
std::mutex &mbioMutex();

class FormatService : public IFormatService {
 public:
  std::vector<FormatInfo> formats() const override;
  bool format(int id, FormatInfo &info) const override;
  int identify(const std::string &path) const override;

  // Parse the text returned by mb_format_description().
  static FormatInfo parseDescription(int id, const std::string &text);

 private:
  void ensureLoaded() const;
  mutable std::once_flag loaded_;
  mutable std::vector<FormatInfo> formats_;
};

class SwathService : public ISwathService {
 public:
  explicit SwathService(std::shared_ptr<IFormatService> formats) : formats_(std::move(formats)) {}
  SwathSummary inspect(const std::string &path, int format, std::size_t maxTrackPoints,
                       const std::atomic<bool> *cancel = nullptr) const override;
  std::vector<TrackPoint> readNavigation(const std::string &fnvPath, std::size_t maxPoints) const override;

  // Parse the text of a ".fnv" fast navigation file.
  static std::vector<TrackPoint> parseFnv(const std::string &text, std::size_t maxPoints);

 private:
  std::shared_ptr<IFormatService> formats_;
};

class GridService : public IGridService {
 public:
  GridLoadResult load(const std::string &path) const override;
  GridStatistics statistics(const Grid &grid) const override;
};

class GridRenderService : public IGridRenderService {
 public:
  std::vector<std::string> colorMapNames() const override;
  Image render(const Grid &grid, const RenderOptions &options) const override;
  std::uint32_t color(ColorMap map, double t) const override;
};

class ToolService : public IToolService {
 public:
  ToolService(std::vector<std::string> searchPath, const EventBus *events);
  std::vector<ToolInfo> tools() const override;
  bool tool(const std::string &name, ToolInfo &info) const override;
  std::vector<std::string> categories() const override;
  platform::ProcessResult run(const std::string &name, const std::vector<std::string> &arguments,
                              const std::string &workingDirectory, const platform::OutputHandler &output,
                              const std::atomic<bool> *cancel = nullptr) const override;

  // The built-in catalogue of MB-System programs (independent of installation).
  static const std::vector<ToolInfo> &catalog();

 private:
  std::string locate(const std::string &name) const;
  std::vector<std::string> searchPath_;
  const EventBus *events_;
  mutable std::atomic<long> nextJob_{1};
};

class ProcessingService : public IProcessingService {
 public:
  bool datalistText(const std::vector<DatalistEntry> &entries, const std::string &datalistDirectory, bool processed,
                    std::string &text, std::string &error) const override;
  std::string processedDatalistText(const std::string &datalistName) const override;
  int preprocessOutputFormat(int format) const override;
  bool recommendPreprocess(int format) const override;
  std::string preprocessOutputName(const std::string &inputPath, int format) const override;
  std::string processedName(const std::string &path) const override;
  CommandPlan ancillary(const std::string &datalist) const override;
  CommandPlan preprocess(const std::string &datalist, const PreprocessParameters &p) const override;
  CommandPlan clean(const std::string &datalist, const CleanParameters &p) const override;
  CommandPlan corrections(const std::string &datalist, const CorrectionParameters &p) const override;
  CommandPlan process(const std::string &datalist, const ProcessParameters &p) const override;
  CommandPlan grid(const std::string &datalist, const GridParameters &p) const override;
  GridEstimate estimateGrid(const GridParameters &p, const GeoBounds &dataBounds) const override;
  CommandPlan mosaic(const std::string &datalist, const MosaicParameters &p) const override;
  CommandPlan exportGrid(const std::string &grid, const ExportParameters &p) const override;
  CommandPlan exportSoundings(const std::string &datalist, const ExportParameters &p) const override;
};

// Background jobs on a small pool of worker threads (see JobService.h).
class JobService : public IJobService {
 public:
  JobService(std::shared_ptr<IToolService> tools, const EventBus *events, int maxConcurrent = 2);
  ~JobService() override;
  JobService(const JobService &) = delete;
  JobService &operator=(const JobService &) = delete;

  std::int64_t submit(const JobSpec &spec, std::string *error = nullptr) override;
  bool cancel(std::int64_t id) override;
  std::int64_t retry(std::int64_t id) override;
  std::vector<JobInfo> jobs() const override;
  bool job(std::int64_t id, JobInfo &info) const override;
  std::string log(std::int64_t id) const override;
  bool wait(std::int64_t id, int timeoutMs) const override;
  void clearFinished() override;
  int maxConcurrentJobs() const override;
  void setMaxConcurrentJobs(int count) override;

 private:
  struct Job;
  std::int64_t enqueue(const JobSpec &spec, std::int64_t retryOf, std::string *error);
  void worker();
  void run(const std::shared_ptr<Job> &job);
  void publish(const std::string &topic, const std::shared_ptr<Job> &job) const;
  std::shared_ptr<IToolService> tools_;
  const EventBus *events_;
  mutable std::mutex mutex_;
  mutable std::condition_variable changed_;
  std::map<std::int64_t, std::shared_ptr<Job>> jobs_;
  std::deque<std::int64_t> queue_;
  std::vector<std::thread> workers_;
  int maxConcurrent_;
  int running_ = 0;
  bool stopping_ = false;
  std::int64_t nextId_ = 1;
};

// Help pages from the first of the directories that has an "en" folder:
// MBSTUDIO_HELP_PATH entries, <prefix>/share/mbstudio/help and, for the build
// tree, the source directory.
class HelpService : public IHelpService {
 public:
  explicit HelpService(std::vector<std::string> directories = defaultDirectories());
  std::vector<std::string> languages() const override;
  std::vector<std::string> programs() const override;
  HelpPage page(const std::string &program, const std::string &language) const override;

  static std::vector<std::string> defaultDirectories();
  const std::string &directory() const { return directory_; }

 private:
  std::string directory_;
};

}  // namespace mbstudio::mbsystem
