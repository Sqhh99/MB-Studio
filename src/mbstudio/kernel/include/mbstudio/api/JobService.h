// MB-Studio service interface - background processing jobs.
//
// A job is a named sequence of steps, each either an MB-System program run
// with a structured argument list (never a shell command line) or an
// in-process action (for example writing a datalist between two programs).
// Jobs run on worker threads with a bounded concurrency, can be cancelled
// and retried, and keep a log file. The service publishes
//   job.queued, job.started, job.step, job.output, job.finished
// events with the job id ("job") so that any front end can follow them.
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "mbstudio/kernel/Value.h"

namespace mbstudio {

struct JobStep {
  std::string title;                    // shown while the step runs
  std::string tool;                     // MB-System program; empty for an action
  std::vector<std::string> arguments;   // passed as they are, one per argv entry
  std::string workingDirectory;         // default: the job working directory
  // In-process step: return false (and write a reason to log) to fail the job.
  std::function<bool(std::string &log)> action;
  bool allowFailure = false;            // a non-zero exit code does not stop the job
};

struct JobSpec {
  std::string title;             // e.g. "Grid bathymetry (10 m)"
  std::string kind;              // e.g. "grid", "import", "clean", "tool"
  std::string owner;             // who submitted it ("project", "ui", ...)
  std::string workingDirectory;  // created if missing
  std::string logPath;           // default: <workingDirectory>/job-<id>.log
  std::vector<JobStep> steps;
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;  // expected products
  ValueMap parameters;               // user level parameters (provenance)
};

enum class JobState { Queued, Running, Completed, Failed, Cancelled };

inline const char *jobStateName(JobState state) {
  switch (state) {
    case JobState::Queued: return "queued";
    case JobState::Running: return "running";
    case JobState::Completed: return "completed";
    case JobState::Failed: return "failed";
    case JobState::Cancelled: return "cancelled";
  }
  return "queued";
}

struct JobInfo {
  std::int64_t id = 0;
  std::string title;
  std::string kind;
  std::string owner;
  JobState state = JobState::Queued;
  int stepCount = 0;
  int currentStep = -1;     // index of the running (or failing) step
  std::string stepTitle;
  std::string lastLine;     // last line of output (stage message)
  double queuedAt = 0.0, startedAt = 0.0, finishedAt = 0.0;  // epoch seconds
  int exitCode = 0;
  std::string error;
  std::string workingDirectory;
  std::string logPath;
  std::vector<std::string> commandLines;  // display form of every program step
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  ValueMap parameters;
  std::int64_t retryOf = 0;  // id of the job this one repeats
};

class IJobService {
 public:
  static constexpr const char *kServiceId = "mbsystem.jobs";
  virtual ~IJobService() = default;
  // Queue a job; returns its id (0 if the specification is invalid, see error).
  virtual std::int64_t submit(const JobSpec &spec, std::string *error = nullptr) = 0;
  virtual bool cancel(std::int64_t id) = 0;
  // Queue the same steps again; returns the new id.
  virtual std::int64_t retry(std::int64_t id) = 0;
  virtual std::vector<JobInfo> jobs() const = 0;
  virtual bool job(std::int64_t id, JobInfo &info) const = 0;
  virtual std::string log(std::int64_t id) const = 0;
  // Wait until the job has finished; false on time-out.
  virtual bool wait(std::int64_t id, int timeoutMs) const = 0;
  // Remove finished jobs from the list (their log files stay).
  virtual void clearFinished() = 0;
  virtual int maxConcurrentJobs() const = 0;
  virtual void setMaxConcurrentJobs(int count) = 0;
};

}  // namespace mbstudio
