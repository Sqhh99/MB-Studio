// MB-Studio service interface - user interface front end.
//
// A front end plugin (for example the Qt6/QML user interface) provides this
// service. The host executable starts the kernel and then hands control to
// the front end; with no front end (or --headless) the host runs commands
// from its command line instead.
#pragma once

#include <string>

namespace mbstudio {

class Kernel;

class IFrontend {
 public:
  static constexpr const char *kServiceId = "mbstudio.frontend";
  virtual ~IFrontend() = default;
  virtual std::string name() const = 0;
  // Run the user interface until the user quits; returns the process exit code.
  virtual int run(Kernel &kernel, int argc, char **argv) = 0;
};

}  // namespace mbstudio
