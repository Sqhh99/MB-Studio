// MB-Studio platform layer - running the MB-System command line programs.
//
// Output (stdout and stderr merged) is streamed to a callback as it arrives.
// On Windows processes are started with CreateProcessW without a console
// window, with arguments quoted by buildWindowsCommandLine().
#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "mbstudio/kernel/Export.h"

namespace mbstudio::platform {

struct ProcessSpec {
  std::string program;
  std::vector<std::string> arguments;
  std::string workingDirectory;
  std::vector<std::pair<std::string, std::string>> environment;  // overrides
};

struct ProcessResult {
  bool started = false;
  bool cancelled = false;
  int exitCode = -1;
  std::string error;
};

using OutputHandler = std::function<void(const std::string &chunk)>;

MBSTUDIO_KERNEL_API ProcessResult runProcess(const ProcessSpec &spec, const OutputHandler &output,
                                             const std::atomic<bool> *cancel = nullptr);

// Quote one argument following the rules of CommandLineToArgvW / the MSVC
// and MinGW C runtimes (backslashes are literal unless they precede a quote).
MBSTUDIO_KERNEL_API std::string quoteWindowsArgument(const std::string &argument);
MBSTUDIO_KERNEL_API std::string buildWindowsCommandLine(const std::string &program,
                                                        const std::vector<std::string> &arguments);
// Split a user-typed argument string. Double and single quotes group words;
// backslashes are kept literally (Windows paths) except before a quote.
MBSTUDIO_KERNEL_API std::vector<std::string> splitArguments(const std::string &line);
// Render a command for display (quotes arguments containing spaces).
MBSTUDIO_KERNEL_API std::string formatCommand(const std::string &program,
                                              const std::vector<std::string> &arguments);

}  // namespace mbstudio::platform
