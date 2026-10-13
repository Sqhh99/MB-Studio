// MB-Studio kernel - named commands.
//
// Plugins expose their functionality as commands ("swath.inspect",
// "tools.run", ...) taking and returning ValueMaps. Front ends, scripts and
// tests invoke them by name without linking against the plugin.
#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "mbstudio/kernel/Export.h"
#include "mbstudio/kernel/Value.h"

namespace mbstudio {

struct CommandInfo {
  std::string id;
  std::string title;
  std::string description;
  std::string owner;  // id of the plugin that registered the command
  std::string category;  // grouping for menus and the command palette (optional)
};

struct MBSTUDIO_KERNEL_API CommandResult {
  bool ok = false;
  std::string message;
  ValueMap data;

  static CommandResult success(ValueMap data = {}, std::string message = std::string());
  static CommandResult failure(std::string message);
};

class MBSTUDIO_KERNEL_API CommandRegistry {
 public:
  using Handler = std::function<CommandResult(const ValueMap &args)>;

  bool add(const CommandInfo &info, Handler handler);
  bool remove(const std::string &id);
  std::size_t removeOwnedBy(const std::string &owner);
  bool has(const std::string &id) const;
  CommandResult execute(const std::string &id, const ValueMap &args = {}) const;
  std::vector<CommandInfo> list() const;

 private:
  struct Entry {
    CommandInfo info;
    Handler handler;
  };
  mutable std::mutex mutex_;
  std::map<std::string, Entry> commands_;
};

}  // namespace mbstudio
