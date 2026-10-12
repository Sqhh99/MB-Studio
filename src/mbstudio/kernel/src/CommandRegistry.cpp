#include "mbstudio/kernel/CommandRegistry.h"

#include <exception>

namespace mbstudio {

CommandResult CommandResult::success(ValueMap data, std::string message) {
  CommandResult result;
  result.ok = true;
  result.data = std::move(data);
  result.message = std::move(message);
  return result;
}

CommandResult CommandResult::failure(std::string message) {
  CommandResult result;
  result.ok = false;
  result.message = std::move(message);
  return result;
}

bool CommandRegistry::add(const CommandInfo &info, Handler handler) {
  if (info.id.empty() || !handler)
    return false;
  std::lock_guard<std::mutex> lock(mutex_);
  return commands_.emplace(info.id, Entry{info, std::move(handler)}).second;
}

bool CommandRegistry::remove(const std::string &id) {
  std::lock_guard<std::mutex> lock(mutex_);
  return commands_.erase(id) > 0;
}

std::size_t CommandRegistry::removeOwnedBy(const std::string &owner) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::size_t removed = 0;
  for (auto it = commands_.begin(); it != commands_.end();) {
    if (it->second.info.owner == owner) {
      it = commands_.erase(it);
      removed++;
    } else {
      ++it;
    }
  }
  return removed;
}

bool CommandRegistry::has(const std::string &id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return commands_.count(id) > 0;
}

CommandResult CommandRegistry::execute(const std::string &id, const ValueMap &args) const {
  Handler handler;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = commands_.find(id);
    if (it == commands_.end())
      return CommandResult::failure("unknown command: " + id);
    handler = it->second.handler;
  }
  try {
    return handler(args);
  } catch (const std::exception &e) {
    return CommandResult::failure(std::string("command ") + id + " failed: " + e.what());
  } catch (...) {
    return CommandResult::failure("command " + id + " failed");
  }
}

std::vector<CommandInfo> CommandRegistry::list() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<CommandInfo> out;
  out.reserve(commands_.size());
  for (const auto &kv : commands_)
    out.push_back(kv.second.info);
  return out;
}

}  // namespace mbstudio
