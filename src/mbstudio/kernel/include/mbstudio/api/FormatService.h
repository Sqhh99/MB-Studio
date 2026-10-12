// MB-Studio service interface - MB-System data formats.
#pragma once

#include <string>
#include <vector>

namespace mbstudio {

struct FormatInfo {
  int id = 0;
  std::string name;         // e.g. "MBF_HSATLRAW"
  std::string description;  // informal description
  std::string attributes;   // sonar, data types, beams, ...
};

class IFormatService {
 public:
  static constexpr const char *kServiceId = "mbsystem.formats";
  virtual ~IFormatService() = default;
  virtual std::vector<FormatInfo> formats() const = 0;
  virtual bool format(int id, FormatInfo &info) const = 0;
  // MB-System format id implied by a file name (suffix or .inf/datalist),
  // 0 if unknown.
  virtual int identify(const std::string &path) const = 0;
};

}  // namespace mbstudio
