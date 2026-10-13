// MB-Studio service interface - the MB-System command line programs.
#pragma once

#include <atomic>
#include <string>
#include <vector>

#include "mbstudio/platform/Process.h"

namespace mbstudio {

struct ToolInfo {
  std::string name;       // program name without suffix, e.g. "mbinfo"
  std::string category;   // e.g. "Inspection"
  std::string summary;    // one line description
  std::string example;    // example argument string
  std::string path;       // resolved executable, empty if not installed
  bool available = false;
};

class IToolService {
 public:
  static constexpr const char *kServiceId = "mbsystem.tools";
  virtual ~IToolService() = default;
  virtual std::vector<ToolInfo> tools() const = 0;
  virtual bool tool(const std::string &name, ToolInfo &info) const = 0;
  virtual std::vector<std::string> categories() const = 0;
  virtual platform::ProcessResult run(const std::string &name, const std::vector<std::string> &arguments,
                                      const std::string &workingDirectory,
                                      const platform::OutputHandler &output,
                                      const std::atomic<bool> *cancel = nullptr) const = 0;
};

}  // namespace mbstudio
