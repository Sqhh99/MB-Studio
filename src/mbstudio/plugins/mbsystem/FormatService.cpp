#include <cstring>
#include <sstream>

#include "MbioServices.h"

extern "C" {
#include "mb_define.h"
#include "mb_status.h"
}

namespace mbstudio::mbsystem {

std::mutex &mbioMutex() {
  static std::mutex mutex;
  return mutex;
}

namespace {
std::string trim(const std::string &s) {
  const size_t b = s.find_first_not_of(" \t\r\n");
  if (b == std::string::npos)
    return std::string();
  const size_t e = s.find_last_not_of(" \t\r\n");
  return s.substr(b, e - b + 1);
}
}  // namespace

FormatInfo FormatService::parseDescription(int id, const std::string &text) {
  FormatInfo info;
  info.id = id;
  std::istringstream in(text);
  std::string line;
  std::string *current = nullptr;
  while (std::getline(in, line)) {
    const size_t colon = line.find(':');
    const std::string key = colon == std::string::npos ? std::string() : trim(line.substr(0, colon));
    if (key == "Format name") {
      info.name = trim(line.substr(colon + 1));
      current = nullptr;
    } else if (key == "Informal Description") {
      info.description = trim(line.substr(colon + 1));
      current = &info.description;
    } else if (key == "Attributes") {
      info.attributes = trim(line.substr(colon + 1));
      current = &info.attributes;
    } else if (current != nullptr && !trim(line).empty()) {
      *current += " " + trim(line);  // continuation line
    }
  }
  return info;
}

void FormatService::ensureLoaded() const {
  std::call_once(loaded_, [this]() {
    std::lock_guard<std::mutex> lock(mbioMutex());
    std::vector<char> description(MB_DESCRIPTION_LENGTH + 1, '\0');
    for (int id = 1; id <= 1000; id++) {
      int format = id;
      int error = MB_ERROR_NO_ERROR;
      description[0] = '\0';
      if (mb_format_description(0, &format, description.data(), &error) == MB_SUCCESS && format == id)
        formats_.push_back(parseDescription(id, description.data()));
    }
  });
}

std::vector<FormatInfo> FormatService::formats() const {
  ensureLoaded();
  return formats_;
}

bool FormatService::format(int id, FormatInfo &info) const {
  ensureLoaded();
  for (const auto &f : formats_) {
    if (f.id == id) {
      info = f;
      return true;
    }
  }
  return false;
}

int FormatService::identify(const std::string &path) const {
  if (path.empty() || path.size() >= MB_PATH_MAXLINE)
    return 0;
  std::lock_guard<std::mutex> lock(mbioMutex());
  char file[MB_PATH_MAXLINE];
  char root[MB_PATH_MAXLINE];
  std::strncpy(file, path.c_str(), sizeof(file) - 1);
  file[sizeof(file) - 1] = '\0';
  int format = 0;
  int error = MB_ERROR_NO_ERROR;
  mb_get_format(0, file, root, &format, &error);
  return format;
}

}  // namespace mbstudio::mbsystem
