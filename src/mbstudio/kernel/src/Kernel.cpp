#include "mbstudio/kernel/Kernel.h"

#include <chrono>

namespace mbstudio {

namespace {
constexpr std::size_t kMaxLogEntries = 2000;
}

const char *logLevelName(LogLevel level) {
  switch (level) {
    case LogLevel::Debug: return "debug";
    case LogLevel::Info: return "info";
    case LogLevel::Warning: return "warning";
    case LogLevel::Error: return "error";
  }
  return "info";
}

Kernel::Kernel(KernelConfig config) : config_(std::move(config)) {
  plugins_.setDisabled(config_.disabledPlugins);
}

Kernel::~Kernel() { stop(); }

std::string Kernel::kernelVersion() {
#ifdef MBSTUDIO_VERSION
  return MBSTUDIO_VERSION;
#else
  return "0.0.0";
#endif
}

std::size_t Kernel::start() {
  if (started_)
    return 0;
  started_ = true;
  log(LogLevel::Info, "kernel", config_.applicationName + " kernel " + kernelVersion() + " starting");
  plugins_.discover(config_.pluginDirectories);
  const std::size_t active = plugins_.startAll(*this);
  events_.publish("kernel.started", {{"plugins", static_cast<std::int64_t>(active)}});
  return active;
}

void Kernel::stop() {
  if (!started_)
    return;
  events_.publish("kernel.stopping");
  plugins_.stopAll(*this);
  started_ = false;
}

void Kernel::log(LogLevel level, const std::string &source, const std::string &message) {
  LogEntry entry;
  entry.level = level;
  entry.source = source;
  entry.message = message;
  entry.timestamp = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
  {
    std::lock_guard<std::mutex> lock(logMutex_);
    log_.push_back(entry);
    while (log_.size() > kMaxLogEntries)
      log_.pop_front();
  }
  events_.publish(std::string("log.") + logLevelName(level),
                  {{"source", source}, {"message", message}, {"timestamp", entry.timestamp}});
}

std::vector<LogEntry> Kernel::logHistory() const {
  std::lock_guard<std::mutex> lock(logMutex_);
  return std::vector<LogEntry>(log_.begin(), log_.end());
}

}  // namespace mbstudio
