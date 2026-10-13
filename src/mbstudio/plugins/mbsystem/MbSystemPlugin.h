// MB-Studio mbsystem plugin class (also linked statically by the tests).
#pragma once

#include "mbstudio/kernel/Plugin.h"

namespace mbstudio::mbsystem {

class MbSystemPlugin : public IPlugin {
 public:
  PluginInfo info() const override;
  bool initialize(Kernel &kernel, std::string &error) override;
  void shutdown(Kernel &kernel) override;
};

}  // namespace mbstudio::mbsystem
