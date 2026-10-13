// MB-Studio service interface - help pages of the MB-System programs.
//
// The pages are Markdown documents, one per program and language
// (<help dir>/<language>/<program>.md). The first line has the form
// "# <program> — <summary>".
#pragma once

#include <string>
#include <vector>

namespace mbstudio {

struct HelpPage {
  bool found = false;
  std::string program;
  std::string language;  // language of the returned page (after fallback)
  std::string summary;   // from the title line
  std::string markdown;
  std::string path;
};

class IHelpService {
 public:
  static constexpr const char *kServiceId = "mbsystem.help";
  virtual ~IHelpService() = default;
  // Language codes with help pages, "en" first (e.g. en, ja, zh_CN).
  virtual std::vector<std::string> languages() const = 0;
  // Programs with an English help page, sorted by name.
  virtual std::vector<std::string> programs() const = 0;
  // The page of a program in the requested language; falls back to the base
  // language ("zh" for "zh_TW") and then to English.
  virtual HelpPage page(const std::string &program, const std::string &language) const = 0;
};

}  // namespace mbstudio
