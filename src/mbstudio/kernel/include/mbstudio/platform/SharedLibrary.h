// MB-Studio platform layer - dynamic libraries (LoadLibraryExW / dlopen).
#pragma once

#include <string>

#include "mbstudio/kernel/Export.h"

namespace mbstudio::platform {

class MBSTUDIO_KERNEL_API SharedLibrary {
 public:
  SharedLibrary() = default;
  ~SharedLibrary();
  SharedLibrary(const SharedLibrary &) = delete;
  SharedLibrary &operator=(const SharedLibrary &) = delete;

  // On Windows the library's own directory is searched for its dependent
  // DLLs, in addition to the application directory and System32.
  bool open(const std::string &path);
  void close();
  bool isOpen() const { return handle_ != nullptr; }
  void *symbol(const std::string &name) const;
  const std::string &errorString() const { return error_; }
  const std::string &path() const { return path_; }

 private:
  void *handle_ = nullptr;
  std::string path_;
  mutable std::string error_;
};

}  // namespace mbstudio::platform
