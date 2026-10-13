// MB-Studio QML UI - the IFrontend service implementation.
#pragma once

#include "mbstudio/api/Frontend.h"

namespace mbstudio::ui {

class QmlFrontend : public IFrontend {
 public:
  std::string name() const override { return "Qt Quick"; }
  int run(Kernel &kernel, int argc, char **argv) override;
};

// Register the C++ types used by the QML layer (also used by the tests).
void registerQmlTypes();

}  // namespace mbstudio::ui
