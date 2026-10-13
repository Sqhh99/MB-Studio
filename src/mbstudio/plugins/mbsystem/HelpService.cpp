#include "MbioServices.h"

#include <algorithm>

#include "mbstudio/platform/Paths.h"

#ifndef MBSTUDIO_HELP_SOURCE_DIR
#define MBSTUDIO_HELP_SOURCE_DIR ""
#endif

namespace mbstudio::mbsystem {

namespace {

const std::string kEnglish = "en";
const std::string kSuffix = ".md";

bool endsWith(const std::string &s, const std::string &suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// "# mbgrid — Grid bathymetry ..." -> "Grid bathymetry ..."
std::string summaryOf(const std::string &markdown) {
  const std::string line = markdown.substr(0, markdown.find('\n'));
  const std::string dash = "\xE2\x80\x94";  // U+2014 EM DASH
  const size_t pos = line.find(dash);
  if (pos == std::string::npos)
    return std::string();
  std::string summary = line.substr(pos + dash.size());
  const size_t first = summary.find_first_not_of(" \t");
  const size_t last = summary.find_last_not_of(" \t\r");
  return first == std::string::npos ? std::string() : summary.substr(first, last - first + 1);
}

}  // namespace

std::vector<std::string> HelpService::defaultDirectories() {
  std::vector<std::string> dirs = platform::splitPathList(platform::getEnv("MBSTUDIO_HELP_PATH"));
  dirs.push_back(platform::joinPath(platform::installPrefix(), "share/mbstudio/help"));
  if (MBSTUDIO_HELP_SOURCE_DIR[0] != '\0')
    dirs.emplace_back(MBSTUDIO_HELP_SOURCE_DIR);
  return dirs;
}

HelpService::HelpService(std::vector<std::string> directories) {
  for (const auto &dir : directories) {
    if (platform::isDirectory(platform::joinPath(dir, kEnglish))) {
      directory_ = dir;
      break;
    }
  }
}

std::vector<std::string> HelpService::languages() const {
  std::vector<std::string> out;
  if (directory_.empty())
    return out;
  out.push_back(kEnglish);
  std::vector<std::string> others;
  for (const auto &entry : platform::listDirectory(directory_))
    if (entry != kEnglish && platform::isDirectory(platform::joinPath(directory_, entry)))
      others.push_back(entry);
  std::sort(others.begin(), others.end());
  out.insert(out.end(), others.begin(), others.end());
  return out;
}

std::vector<std::string> HelpService::programs() const {
  std::vector<std::string> out;
  if (directory_.empty())
    return out;
  for (const auto &entry : platform::listDirectory(platform::joinPath(directory_, kEnglish)))
    if (endsWith(entry, kSuffix))
      out.push_back(entry.substr(0, entry.size() - kSuffix.size()));
  std::sort(out.begin(), out.end());
  return out;
}

HelpPage HelpService::page(const std::string &program, const std::string &language) const {
  HelpPage page;
  page.program = program;
  // program names are plain identifiers: never let them escape the folder
  if (directory_.empty() || program.empty() ||
      program.find_first_of("/\\:.") != std::string::npos)
    return page;

  std::vector<std::string> candidates;
  std::string lang = language;
  std::replace(lang.begin(), lang.end(), '-', '_');
  if (!lang.empty() && lang.find_first_of("/\\.") == std::string::npos) {
    candidates.push_back(lang);
    const std::string base = lang.substr(0, lang.find('_'));
    if (base != lang)
      candidates.push_back(base);
    if (base == "zh")  // any Chinese variant: Simplified Chinese
      candidates.emplace_back("zh_CN");
  }
  candidates.push_back(kEnglish);

  for (const auto &candidate : candidates) {
    const std::string path = platform::joinPath(platform::joinPath(directory_, candidate), program + kSuffix);
    if (platform::readTextFile(path, page.markdown)) {
      page.found = true;
      page.language = candidate;
      page.path = path;
      page.summary = summaryOf(page.markdown);
      return page;
    }
  }
  return page;
}

}  // namespace mbstudio::mbsystem
