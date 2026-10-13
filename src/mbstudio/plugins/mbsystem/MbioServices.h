// MB-Studio mbsystem plugin - service implementations backed by MB-System.
#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "mbstudio/api/FormatService.h"
#include "mbstudio/api/GridService.h"
#include "mbstudio/api/SwathService.h"
#include "mbstudio/api/HelpService.h"
#include "mbstudio/api/ToolService.h"
#include "mbstudio/kernel/EventBus.h"

namespace mbstudio::mbsystem {

// The MBIO library keeps some global state; all calls into it are serialised.
std::mutex &mbioMutex();

class FormatService : public IFormatService {
 public:
  std::vector<FormatInfo> formats() const override;
  bool format(int id, FormatInfo &info) const override;
  int identify(const std::string &path) const override;

  // Parse the text returned by mb_format_description().
  static FormatInfo parseDescription(int id, const std::string &text);

 private:
  void ensureLoaded() const;
  mutable std::once_flag loaded_;
  mutable std::vector<FormatInfo> formats_;
};

class SwathService : public ISwathService {
 public:
  explicit SwathService(std::shared_ptr<IFormatService> formats) : formats_(std::move(formats)) {}
  SwathSummary inspect(const std::string &path, int format, std::size_t maxTrackPoints,
                       const std::atomic<bool> *cancel = nullptr) const override;

 private:
  std::shared_ptr<IFormatService> formats_;
};

class GridService : public IGridService {
 public:
  GridLoadResult load(const std::string &path) const override;
};

class GridRenderService : public IGridRenderService {
 public:
  std::vector<std::string> colorMapNames() const override;
  Image render(const Grid &grid, const RenderOptions &options) const override;
  std::uint32_t color(ColorMap map, double t) const override;
};

class ToolService : public IToolService {
 public:
  ToolService(std::vector<std::string> searchPath, const EventBus *events);
  std::vector<ToolInfo> tools() const override;
  bool tool(const std::string &name, ToolInfo &info) const override;
  std::vector<std::string> categories() const override;
  platform::ProcessResult run(const std::string &name, const std::vector<std::string> &arguments,
                              const std::string &workingDirectory, const platform::OutputHandler &output,
                              const std::atomic<bool> *cancel = nullptr) const override;

  // The built-in catalogue of MB-System programs (independent of installation).
  static const std::vector<ToolInfo> &catalog();

 private:
  std::string locate(const std::string &name) const;
  std::vector<std::string> searchPath_;
  const EventBus *events_;
  mutable std::atomic<long> nextJob_{1};
};

// Help pages from the first of the directories that has an "en" folder:
// MBSTUDIO_HELP_PATH entries, <prefix>/share/mbstudio/help and, for the build
// tree, the source directory.
class HelpService : public IHelpService {
 public:
  explicit HelpService(std::vector<std::string> directories = defaultDirectories());
  std::vector<std::string> languages() const override;
  std::vector<std::string> programs() const override;
  HelpPage page(const std::string &program, const std::string &language) const override;

  static std::vector<std::string> defaultDirectories();
  const std::string &directory() const { return directory_; }

 private:
  std::string directory_;
};

}  // namespace mbstudio::mbsystem
