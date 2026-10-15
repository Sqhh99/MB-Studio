// Background processing jobs: a queue served by worker threads, each job a
// sequence of MB-System program runs (structured argument lists) and
// in-process actions, with cancellation, retry and a log file per job.
#include <algorithm>
#include <chrono>
#include <ctime>

#include "MbioServices.h"
#include "mbstudio/platform/Paths.h"

namespace mbstudio::mbsystem {

namespace {

constexpr int kMaxWorkers = 8;
constexpr std::size_t kMaxLogInMemory = 2 * 1024 * 1024;

double now() {
  return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string timestamp() {
  static std::mutex mutex;  // std::localtime uses a shared buffer
  const std::time_t t = std::time(nullptr);
  char buffer[32] = "";
  std::lock_guard<std::mutex> lock(mutex);
  if (const std::tm *tm = std::localtime(&t))
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
  return buffer;
}

bool finished(JobState s) { return s == JobState::Completed || s == JobState::Failed || s == JobState::Cancelled; }

}  // namespace

struct JobService::Job {
  JobInfo info;
  JobSpec spec;
  std::shared_ptr<std::atomic<bool>> cancel = std::make_shared<std::atomic<bool>>(false);
  std::string log;      // in-memory copy (bounded)
  std::string partial;  // incomplete output line
};

JobService::JobService(std::shared_ptr<IToolService> tools, const EventBus *events, int maxConcurrent)
    : tools_(std::move(tools)), events_(events), maxConcurrent_(std::clamp(maxConcurrent, 1, kMaxWorkers)) {}

JobService::~JobService() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stopping_ = true;
    for (auto &kv : jobs_)
      kv.second->cancel->store(true);
  }
  changed_.notify_all();
  for (auto &t : workers_)
    if (t.joinable())
      t.join();
}

void JobService::publish(const std::string &topic, const std::shared_ptr<Job> &job) const {
  if (events_ == nullptr)
    return;
  JobInfo info;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    info = job->info;
  }
  events_->publish(topic, {{"job", info.id},
                           {"title", info.title},
                           {"kind", info.kind},
                           {"owner", info.owner},
                           {"state", std::string(jobStateName(info.state))},
                           {"step", static_cast<std::int64_t>(info.currentStep)},
                           {"steps", static_cast<std::int64_t>(info.stepCount)},
                           {"stepTitle", info.stepTitle},
                           {"exitCode", static_cast<std::int64_t>(info.exitCode)},
                           {"blockedBy", info.blockedBy},
                           {"error", info.error}});
}

std::int64_t JobService::blockerLocked(std::size_t position) const {
  const auto it = jobs_.find(queue_[position]);
  if (it == jobs_.end())
    return 0;
  const JobSpec &spec = it->second->spec;
  for (const auto &running : active_)
    if (jobsConflict(spec, running->spec))
      return running->info.id;
  // keep the submission order of conflicting jobs
  for (std::size_t k = 0; k < position; k++) {
    const auto earlier = jobs_.find(queue_[k]);
    if (earlier != jobs_.end() && jobsConflict(spec, earlier->second->spec))
      return earlier->second->info.id;
  }
  return 0;
}

int JobService::runnableLocked() const {
  if (static_cast<int>(active_.size()) >= maxConcurrent_)
    return -1;
  for (std::size_t i = 0; i < queue_.size(); i++)
    if (blockerLocked(i) == 0)
      return static_cast<int>(i);
  return -1;
}

std::vector<std::shared_ptr<JobService::Job>> JobService::updateBlockedLocked() {
  std::vector<std::shared_ptr<Job>> changed;
  for (std::size_t i = 0; i < queue_.size(); i++) {
    const auto it = jobs_.find(queue_[i]);
    if (it == jobs_.end())
      continue;
    const std::int64_t blocker = blockerLocked(i);
    if (blocker != it->second->info.blockedBy) {
      it->second->info.blockedBy = blocker;
      changed.push_back(it->second);
    }
  }
  return changed;
}

std::int64_t JobService::submit(const JobSpec &spec, std::string *error) { return enqueue(spec, 0, error); }

std::int64_t JobService::enqueue(const JobSpec &spec, std::int64_t retryOf, std::string *error) {
  if (spec.steps.empty()) {
    if (error != nullptr)
      *error = "the job has no steps";
    return 0;
  }
  for (const auto &s : spec.steps) {
    if (s.tool.empty() && !s.action) {
      if (error != nullptr)
        *error = "a job step has neither a program nor an action";
      return 0;
    }
    ToolInfo tool;
    if (!s.tool.empty() && tools_ && !tools_->tool(s.tool, tool)) {
      if (error != nullptr)
        *error = "unknown MB-System program: " + s.tool;
      return 0;
    }
  }
  auto job = std::make_shared<Job>();
  job->spec = spec;
  JobInfo &info = job->info;
  info.title = spec.title.empty() ? spec.steps.front().title : spec.title;
  info.kind = spec.kind.empty() ? std::string("tool") : spec.kind;
  info.owner = spec.owner;
  info.stepCount = static_cast<int>(spec.steps.size());
  info.workingDirectory = spec.workingDirectory;
  info.inputs = spec.inputs;
  info.outputs = spec.outputs;
  info.parameters = spec.parameters;
  info.retryOf = retryOf;
  info.retryable = spec.retryable;
  info.reads = spec.reads;
  info.writes = spec.writes;
  info.queuedAt = now();
  for (const auto &s : spec.steps)
    if (!s.tool.empty())
      info.commandLines.push_back(platform::formatCommand(s.tool, s.arguments));
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stopping_) {
      if (error != nullptr)
        *error = "the job service is shutting down";
      return 0;
    }
    info.id = nextId_++;
    if (info.workingDirectory.empty())
      info.workingDirectory = platform::homeDirectory();
    info.logPath = !spec.logPath.empty()
                       ? spec.logPath
                       : platform::joinPath(info.workingDirectory, "job-" + std::to_string(info.id) + ".log");
    // what the job will wait for (every queued job was submitted earlier)
    for (const auto &running : active_)
      if (info.blockedBy == 0 && jobsConflict(spec, running->spec))
        info.blockedBy = running->info.id;
    for (const auto queued : queue_) {
      const auto it = jobs_.find(queued);
      if (info.blockedBy == 0 && it != jobs_.end() && jobsConflict(spec, it->second->spec))
        info.blockedBy = queued;
    }
    jobs_[info.id] = job;
  }
  // announce the job before a worker can pick it up
  const std::int64_t id = info.id;
  publish("job.queued", job);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stopping_)
      return id;
    queue_.push_back(id);
    // start workers lazily, up to the concurrency limit
    if (static_cast<int>(workers_.size()) < maxConcurrent_)
      workers_.emplace_back([this] { worker(); });
  }
  changed_.notify_all();
  return id;
}

void JobService::worker() {
  for (;;) {
    std::shared_ptr<Job> job;
    std::vector<std::shared_ptr<Job>> waiting;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      changed_.wait(lock, [this] { return stopping_ || runnableLocked() >= 0; });
      if (stopping_)
        return;
      const int position = runnableLocked();
      const std::int64_t id = queue_[static_cast<std::size_t>(position)];
      queue_.erase(queue_.begin() + position);
      const auto it = jobs_.find(id);
      if (it == jobs_.end())
        continue;
      job = it->second;
      if (job->cancel->load()) {
        job->info.state = JobState::Cancelled;
        job->info.finishedAt = now();
        waiting = updateBlockedLocked();
        lock.unlock();
        publish("job.finished", job);
        for (const auto &w : waiting)
          publish("job.waiting", w);
        changed_.notify_all();
        continue;
      }
      active_.push_back(job);
      job->info.state = JobState::Running;
      job->info.startedAt = now();
      job->info.blockedBy = 0;
      waiting = updateBlockedLocked();
    }
    for (const auto &w : waiting)
      publish("job.waiting", w);
    run(job);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      active_.erase(std::remove(active_.begin(), active_.end(), job), active_.end());
      waiting = updateBlockedLocked();
    }
    publish("job.finished", job);
    for (const auto &w : waiting)
      publish("job.waiting", w);
    changed_.notify_all();
  }
}

void JobService::run(const std::shared_ptr<Job> &job) {
  const JobSpec &spec = job->spec;
  std::string logPath, workingDirectory;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    logPath = job->info.logPath;
    workingDirectory = job->info.workingDirectory;
  }
  platform::makeDirectories(workingDirectory);
  platform::makeDirectories(platform::parentPath(logPath));

  auto appendLog = [this, &job, &logPath](const std::string &text) {
    platform::appendTextFile(logPath, text);
    std::lock_guard<std::mutex> lock(mutex_);
    if (job->log.size() < kMaxLogInMemory)
      job->log += text;
  };
  appendLog("# " + job->info.title + "\n# started " + timestamp() + "\n# working directory " + workingDirectory + "\n");
  publish("job.started", job);

  JobState state = JobState::Completed;
  std::string error;
  int exitCode = 0;
  for (std::size_t i = 0; i < spec.steps.size(); i++) {
    const JobStep &s = spec.steps[i];
    if (job->cancel->load()) {
      state = JobState::Cancelled;
      break;
    }
    {
      std::lock_guard<std::mutex> lock(mutex_);
      job->info.currentStep = static_cast<int>(i);
      job->info.stepTitle = s.title;
    }
    publish("job.step", job);
    appendLog("\n## Step " + std::to_string(i + 1) + "/" + std::to_string(spec.steps.size()) + ": " + s.title + "\n");

    if (s.tool.empty()) {
      std::string message;
      bool ok = false;
      try {
        ok = s.action(message);
      } catch (const std::exception &e) {
        message = e.what();
      } catch (...) {
        message = "unexpected error";
      }
      if (!message.empty())
        appendLog(message + (message.back() == '\n' ? "" : "\n"));
      if (!ok) {
        state = JobState::Failed;
        error = message.empty() ? s.title + " failed" : message;
        break;
      }
      continue;
    }

    appendLog("$ " + platform::formatCommand(s.tool, s.arguments) + "\n");
    const std::string cwd = s.workingDirectory.empty() ? workingDirectory : s.workingDirectory;
    const auto result = tools_->runWithEnvironment(
        s.tool, s.arguments, cwd, s.environment,
        [&](const std::string &chunk) {
          appendLog(chunk);
          std::string last;
          {
            std::lock_guard<std::mutex> lock(mutex_);
            job->partial += chunk;
            std::size_t start = 0;
            for (std::size_t k = 0; k < job->partial.size(); k++) {
              if (job->partial[k] == '\n' || job->partial[k] == '\r') {
                const std::string line = job->partial.substr(start, k - start);
                if (line.find_first_not_of(" \t") != std::string::npos)
                  last = line;
                start = k + 1;
              }
            }
            job->partial.erase(0, start);
            if (!last.empty())
              job->info.lastLine = last;
          }
          if (!last.empty() && events_ != nullptr)
            events_->publish("job.output", {{"job", job->info.id}, {"text", chunk}, {"line", last}});
        },
        job->cancel.get());
    if (!result.started) {
      state = JobState::Failed;
      error = result.error.empty() ? "could not start " + s.tool : result.error;
      appendLog(error + "\n");
      break;
    }
    if (result.cancelled || job->cancel->load()) {
      state = JobState::Cancelled;
      appendLog("[stopped by user]\n");
      break;
    }
    exitCode = result.exitCode;
    appendLog("[exit code " + std::to_string(result.exitCode) + "]\n");
    if (result.exitCode != 0 && !s.allowFailure) {
      state = JobState::Failed;
      std::string last;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        last = job->info.lastLine;
      }
      error = s.tool + " failed with exit code " + std::to_string(result.exitCode) + (last.empty() ? "" : ": " + last);
      break;
    }
  }
  appendLog("\n# " + std::string(jobStateName(state)) + " " + timestamp() + (error.empty() ? "" : " - " + error) + "\n");
  std::lock_guard<std::mutex> lock(mutex_);
  job->info.state = state;
  job->info.error = error;
  job->info.exitCode = exitCode;
  job->info.finishedAt = now();
}

bool JobService::cancel(std::int64_t id) {
  std::shared_ptr<Job> job;
  std::vector<std::shared_ptr<Job>> waiting;
  bool wasQueued = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = jobs_.find(id);
    if (it == jobs_.end() || finished(it->second->info.state))
      return false;
    job = it->second;
    job->cancel->store(true);
    const auto q = std::find(queue_.begin(), queue_.end(), id);
    if (q != queue_.end()) {
      queue_.erase(q);
      job->info.state = JobState::Cancelled;
      job->info.finishedAt = now();
      job->info.blockedBy = 0;
      wasQueued = true;
      waiting = updateBlockedLocked();
    }
  }
  if (wasQueued)
    publish("job.finished", job);
  for (const auto &w : waiting)
    publish("job.waiting", w);
  changed_.notify_all();
  return true;
}

std::int64_t JobService::retry(std::int64_t id) {
  JobSpec spec;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = jobs_.find(id);
    if (it == jobs_.end() || !finished(it->second->info.state) || !it->second->spec.retryable)
      return 0;
    spec = it->second->spec;
    spec.logPath.clear();  // a new log for the new attempt
  }
  return enqueue(spec, id, nullptr);
}

std::vector<JobInfo> JobService::jobs() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<JobInfo> out;
  out.reserve(jobs_.size());
  for (const auto &kv : jobs_)
    out.push_back(kv.second->info);
  return out;
}

bool JobService::job(std::int64_t id, JobInfo &info) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto it = jobs_.find(id);
  if (it == jobs_.end())
    return false;
  info = it->second->info;
  return true;
}

std::string JobService::log(std::int64_t id) const {
  std::string path;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = jobs_.find(id);
    if (it == jobs_.end())
      return std::string();
    if (it->second->log.size() < kMaxLogInMemory)
      return it->second->log;
    path = it->second->info.logPath;
  }
  std::string text;
  platform::readTextFile(path, text);
  return text;
}

bool JobService::wait(std::int64_t id, int timeoutMs) const {
  std::unique_lock<std::mutex> lock(mutex_);
  return changed_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this, id] {
    const auto it = jobs_.find(id);
    return it == jobs_.end() || finished(it->second->info.state);
  });
}

void JobService::clearFinished() {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto it = jobs_.begin(); it != jobs_.end();) {
    if (finished(it->second->info.state))
      it = jobs_.erase(it);
    else
      ++it;
  }
}

int JobService::maxConcurrentJobs() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return maxConcurrent_;
}

void JobService::setMaxConcurrentJobs(int count) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    maxConcurrent_ = std::clamp(count, 1, kMaxWorkers);
    while (static_cast<int>(workers_.size()) < maxConcurrent_ && !queue_.empty() && !stopping_)
      workers_.emplace_back([this] { worker(); });
  }
  changed_.notify_all();
}

}  // namespace mbstudio::mbsystem
