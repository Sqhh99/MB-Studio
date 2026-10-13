// MB-Studio QML UI - the IFrontend service implementation.
#pragma once

#include <string>

#include "mbstudio/api/Frontend.h"

namespace mbstudio::ui {

class QmlFrontend : public IFrontend {
 public:
  std::string name() const override { return "Qt Quick"; }
  int run(Kernel &kernel, int argc, char **argv) override;
};

// Register the C++ types used by the QML layer (also used by the tests).
void registerQmlTypes();

// Register the workspaces and menu actions of the user interface in the
// kernel's contribution registry (also used by the tests).
bool registerContributions(Kernel &kernel, std::string &error);

}  // namespace mbstudio::ui
