// MB-Studio kernel.
//
// The kernel is deliberately small: it owns the service registry, the event
// bus, the command registry and the plugin manager, and knows nothing about
// MB-System, Qt or any user interface. All functionality is contributed by
// plugins (see Plugin.h), including the graphical front end.
#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "mbstudio/kernel/CommandRegistry.h"
#include "mbstudio/kernel/EventBus.h"
#include "mbstudio/kernel/Export.h"
#include "mbstudio/kernel/PluginManager.h"
#include "mbstudio/kernel/ServiceRegistry.h"

namespace mbstudio {

struct KernelConfig {
  std::string applicationName = "MB-Studio";
  std::string version;
  std::vector<std::string> pluginDirectories;
  std::set<std::string> disabledPlugins;
  std::vector<std::string> toolDirectories;  // where the MB-System programs live
};

enum class LogLevel { Debug, Info, Warning, Error };

MBSTUDIO_KERNEL_API const char *logLevelName(LogLevel level);

struct LogEntry {
  LogLevel level = LogLevel::Info;
  std::string source;
  std::string message;
  double timestamp = 0.0;  // seconds since the epoch
};

class MBSTUDIO_KERNEL_API Kernel {
 public:
  explicit Kernel(KernelConfig config = {});
  ~Kernel();
  Kernel(const Kernel &) = delete;
  Kernel &operator=(const Kernel &) = delete;

  const KernelConfig &config() const { return config_; }
  ServiceRegistry &services() { return services_; }
  EventBus &events() { return events_; }
  CommandRegistry &commands() { return commands_; }
  PluginManager &plugins() { return plugins_; }

  // Discover plugins in the configured directories and start them.
  std::size_t start();
  void stop();
  bool started() const { return started_; }

  // Log a message: kept in a bounded history and published as "log.<level>".
  void log(LogLevel level, const std::string &source, const std::string &message);
  std::vector<LogEntry> logHistory() const;

  static std::string kernelVersion();

 private:
  KernelConfig config_;
  // Declared first so it is destroyed last: plugin libraries are unloaded
  // only after every service, command and event handler (whose code lives in
  // those libraries) has been released.
  PluginManager plugins_;
  ServiceRegistry services_;
  EventBus events_;
  CommandRegistry commands_;
  bool started_ = false;
  mutable std::mutex logMutex_;
  std::deque<LogEntry> log_;
};

}  // namespace mbstudio
