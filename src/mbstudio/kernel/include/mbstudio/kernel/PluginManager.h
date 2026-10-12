// MB-Studio kernel - plugin discovery, dependency ordering and life cycle.
#pragma once

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "mbstudio/kernel/Export.h"
#include "mbstudio/kernel/Plugin.h"

namespace mbstudio {

class Kernel;

namespace platform {
class SharedLibrary;
}

enum class PluginState { Discovered, Active, Failed, Disabled, Stopped };

MBSTUDIO_KERNEL_API const char *pluginStateName(PluginState state);

struct PluginRecord {
  PluginInfo info;
  std::string path;  // empty for built-in (statically registered) plugins
  PluginState state = PluginState::Discovered;
  std::string error;
};

class MBSTUDIO_KERNEL_API PluginManager {
 public:
  using Factory = std::function<std::unique_ptr<IPlugin>()>;

  PluginManager();
  ~PluginManager();
  PluginManager(const PluginManager &) = delete;
  PluginManager &operator=(const PluginManager &) = delete;

  static const char *filePrefix();  // "mbstudio-plugin-"
  static const char *fileSuffix();  // ".dll", ".so" or ".dylib"

  // Register a plugin compiled into the host (or a test).
  void addStatic(Factory factory);
  // Load every plugin library found in the given directories. Returns the
  // number of libraries loaded; failures are recorded as Failed plugins.
  std::size_t discover(const std::vector<std::string> &directories);
  // Load one plugin library.
  bool load(const std::string &path, std::string *error = nullptr);

  void setDisabled(const std::set<std::string> &ids);

  // Initialize plugins in dependency order. Returns the number now active.
  std::size_t startAll(Kernel &kernel);
  void stopAll(Kernel &kernel);

  std::vector<PluginRecord> plugins() const;
  bool isActive(const std::string &id) const;

 private:
  struct Loaded;
  std::vector<std::unique_ptr<Loaded>> plugins_;
  std::vector<Loaded *> startOrder_;
  std::set<std::string> disabled_;
};

}  // namespace mbstudio
