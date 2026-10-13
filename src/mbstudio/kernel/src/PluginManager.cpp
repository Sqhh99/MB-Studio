#include "mbstudio/kernel/PluginManager.h"

#include <algorithm>
#include <map>

#include "mbstudio/kernel/Kernel.h"
#include "mbstudio/platform/Paths.h"
#include "mbstudio/platform/SharedLibrary.h"

namespace mbstudio {

const char *pluginStateName(PluginState state) {
  switch (state) {
    case PluginState::Discovered: return "discovered";
    case PluginState::Active: return "active";
    case PluginState::Failed: return "failed";
    case PluginState::Disabled: return "disabled";
    case PluginState::Stopped: return "stopped";
  }
  return "unknown";
}

using AbiVersionFn = int (*)();
using CreateFn = IPlugin *(*)();
using DestroyFn = void (*)(IPlugin *);

struct PluginManager::Loaded {
  PluginRecord record;
  std::unique_ptr<platform::SharedLibrary> library;  // null for static plugins
  IPlugin *plugin = nullptr;
  DestroyFn destroy = nullptr;           // for library plugins
  std::unique_ptr<IPlugin> owned;        // for static plugins

  ~Loaded() {
    // The plugin object's code lives in the library: destroy it first.
    if (plugin != nullptr && destroy != nullptr)
      destroy(plugin);
    plugin = nullptr;
    owned.reset();
    if (library)
      library->close();
  }
};

PluginManager::PluginManager() = default;
PluginManager::~PluginManager() = default;

const char *PluginManager::filePrefix() { return "mbstudio-plugin-"; }

const char *PluginManager::fileSuffix() {
#if defined(_WIN32)
  return ".dll";
#elif defined(__APPLE__)
  return ".dylib";
#else
  return ".so";
#endif
}

void PluginManager::addStatic(Factory factory) {
  auto loaded = std::make_unique<Loaded>();
  loaded->owned = factory ? factory() : nullptr;
  if (!loaded->owned) {
    loaded->record.state = PluginState::Failed;
    loaded->record.error = "plugin factory returned no plugin";
  } else {
    loaded->plugin = loaded->owned.get();
    loaded->record.info = loaded->plugin->info();
  }
  plugins_.push_back(std::move(loaded));
}

bool PluginManager::load(const std::string &path, std::string *error) {
  auto loaded = std::make_unique<Loaded>();
  loaded->record.path = path;
  loaded->record.info.id = platform::fileName(path);
  loaded->library = std::make_unique<platform::SharedLibrary>();

  auto fail = [&](const std::string &message) {
    loaded->record.state = PluginState::Failed;
    loaded->record.error = message;
    if (error != nullptr)
      *error = message;
    plugins_.push_back(std::move(loaded));
    return false;
  };

  if (!loaded->library->open(path))
    return fail("cannot load " + path + ": " + loaded->library->errorString());

  auto abi = reinterpret_cast<AbiVersionFn>(loaded->library->symbol("mbstudio_plugin_abi_version"));
  auto create = reinterpret_cast<CreateFn>(loaded->library->symbol("mbstudio_create_plugin"));
  auto destroy = reinterpret_cast<DestroyFn>(loaded->library->symbol("mbstudio_destroy_plugin"));
  if (abi == nullptr || create == nullptr || destroy == nullptr)
    return fail(path + " is not an MB-Studio plugin (missing entry points)");
  if (abi() != kPluginAbiVersion)
    return fail(path + " was built for plugin ABI " + std::to_string(abi()) + ", the kernel provides " +
                std::to_string(kPluginAbiVersion));

  loaded->plugin = create();
  loaded->destroy = destroy;
  if (loaded->plugin == nullptr)
    return fail(path + ": plugin creation failed");
  loaded->record.info = loaded->plugin->info();
  plugins_.push_back(std::move(loaded));
  return true;
}

std::size_t PluginManager::discover(const std::vector<std::string> &directories) {
  const std::string prefix = filePrefix();
  const std::string suffix = fileSuffix();
  std::size_t count = 0;
  for (const auto &dir : directories) {
    std::vector<std::string> names = platform::listDirectory(dir);
    std::sort(names.begin(), names.end());
    for (const auto &name : names) {
      if (name.size() <= prefix.size() + suffix.size() || name.compare(0, prefix.size(), prefix) != 0 ||
          name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0)
        continue;
      if (load(platform::joinPath(dir, name)))
        count++;
    }
  }
  return count;
}

void PluginManager::setDisabled(const std::set<std::string> &ids) { disabled_ = ids; }

std::size_t PluginManager::startAll(Kernel &kernel) {
  // Index candidate plugins by id; reject duplicates and disabled plugins.
  std::map<std::string, Loaded *> byId;
  for (auto &p : plugins_) {
    if (p->record.state != PluginState::Discovered)
      continue;
    const std::string &id = p->record.info.id;
    if (disabled_.count(id) > 0) {
      p->record.state = PluginState::Disabled;
      continue;
    }
    if (byId.count(id) > 0) {
      p->record.state = PluginState::Failed;
      p->record.error = "duplicate plugin id '" + id + "'";
      continue;
    }
    byId[id] = p.get();
  }

  // Start in dependency order (repeat until no further progress).
  std::size_t active = 0;
  bool progress = true;
  while (progress) {
    progress = false;
    for (auto &kv : byId) {
      Loaded *p = kv.second;
      if (p->record.state != PluginState::Discovered)
        continue;
      bool ready = true;
      for (const auto &dep : p->record.info.dependencies) {
        const auto it = byId.find(dep);
        const bool depActive = isActive(dep);
        if (depActive)
          continue;
        if (it == byId.end() || it->second->record.state == PluginState::Failed ||
            it->second->record.state == PluginState::Disabled) {
          p->record.state = PluginState::Failed;
          p->record.error = "required plugin '" + dep + "' is not available";
          kernel.log(LogLevel::Warning, "kernel", "plugin " + p->record.info.id + ": " + p->record.error);
          progress = true;
        }
        ready = false;
        break;
      }
      if (!ready)
        continue;

      std::string error;
      bool ok = false;
      try {
        ok = p->plugin->initialize(kernel, error);
      } catch (const std::exception &e) {
        error = e.what();
      } catch (...) {
        error = "unknown exception";
      }
      progress = true;
      if (ok) {
        p->record.state = PluginState::Active;
        startOrder_.push_back(p);
        active++;
        kernel.log(LogLevel::Info, "kernel",
                   "started plugin " + p->record.info.id + " " + p->record.info.version);
        kernel.events().publish("plugin.started", {{"id", p->record.info.id}});
      } else {
        p->record.state = PluginState::Failed;
        p->record.error = error.empty() ? "initialization failed" : error;
        kernel.commands().removeOwnedBy(p->record.info.id);
        kernel.services().withdrawProvidedBy(p->record.info.id);
        kernel.contributions().removeOwnedBy(p->record.info.id);
        kernel.log(LogLevel::Error, "kernel", "plugin " + p->record.info.id + " failed: " + p->record.error);
      }
    }
  }

  // Anything still waiting is part of a dependency cycle.
  for (auto &kv : byId) {
    if (kv.second->record.state == PluginState::Discovered) {
      kv.second->record.state = PluginState::Failed;
      kv.second->record.error = "circular plugin dependency";
    }
  }
  return active;
}

void PluginManager::stopAll(Kernel &kernel) {
  for (auto it = startOrder_.rbegin(); it != startOrder_.rend(); ++it) {
    Loaded *p = *it;
    try {
      p->plugin->shutdown(kernel);
    } catch (...) {
    }
    kernel.commands().removeOwnedBy(p->record.info.id);
    kernel.services().withdrawProvidedBy(p->record.info.id);
    kernel.contributions().removeOwnedBy(p->record.info.id);
    p->record.state = PluginState::Stopped;
    kernel.events().publish("plugin.stopped", {{"id", p->record.info.id}});
  }
  startOrder_.clear();
}

std::vector<PluginRecord> PluginManager::plugins() const {
  std::vector<PluginRecord> out;
  for (const auto &p : plugins_)
    out.push_back(p->record);
  return out;
}

bool PluginManager::isActive(const std::string &id) const {
  for (const auto &p : plugins_)
    if (p->record.info.id == id && p->record.state == PluginState::Active)
      return true;
  return false;
}

}  // namespace mbstudio
