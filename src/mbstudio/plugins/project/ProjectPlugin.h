// MB-Studio project plugin class (also linked statically by the tests).
#pragma once

#include <memory>

#include "mbstudio/kernel/Plugin.h"

namespace mbstudio::project {

class ProjectService;

class ProjectPlugin : public IPlugin {
 public:
  PluginInfo info() const override;
  bool initialize(Kernel &kernel, std::string &error) override;
  void shutdown(Kernel &kernel) override;

 private:
  std::shared_ptr<ProjectService> service_;
};

}  // namespace mbstudio::project
