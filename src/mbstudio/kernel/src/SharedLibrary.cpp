#include "mbstudio/platform/SharedLibrary.h"

#ifdef _WIN32
#include <windows.h>

#include "mbstudio/platform/Paths.h"
#else
#include <dlfcn.h>
#endif

namespace mbstudio::platform {

SharedLibrary::~SharedLibrary() { close(); }

#ifdef _WIN32
namespace {
std::string lastErrorMessage() {
  const DWORD code = GetLastError();
  wchar_t *buffer = nullptr;
  FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 nullptr, code, 0, reinterpret_cast<wchar_t *>(&buffer), 0, nullptr);
  std::string message = buffer != nullptr ? narrow(buffer) : std::string();
  if (buffer != nullptr)
    LocalFree(buffer);
  while (!message.empty() && (message.back() == '\n' || message.back() == '\r' || message.back() == ' '))
    message.pop_back();
  return "error " + std::to_string(code) + (message.empty() ? "" : ": " + message);
}
}  // namespace
#endif

bool SharedLibrary::open(const std::string &path) {
  close();
  path_ = path;
#ifdef _WIN32
  const std::wstring wpath = widen(toNativeSeparators(path));
  // Search the plugin's own directory for its dependencies, then the
  // application directory and System32.
  HMODULE module = LoadLibraryExW(wpath.c_str(), nullptr,
                                  LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
  // Fall back to the classic search order (the library's directory, then
  // PATH) when a dependency lives outside the application directory, as in
  // a build tree or a development environment.
  if (module == nullptr && (GetLastError() == ERROR_MOD_NOT_FOUND || GetLastError() == ERROR_INVALID_PARAMETER))
    module = LoadLibraryExW(wpath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
  if (module == nullptr) {
    error_ = lastErrorMessage();
    return false;
  }
  handle_ = reinterpret_cast<void *>(module);
#else
  handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (handle_ == nullptr) {
    const char *message = dlerror();
    error_ = message != nullptr ? message : "dlopen failed";
    return false;
  }
#endif
  error_.clear();
  return true;
}

void SharedLibrary::close() {
  if (handle_ == nullptr)
    return;
#ifdef _WIN32
  FreeLibrary(reinterpret_cast<HMODULE>(handle_));
#else
  dlclose(handle_);
#endif
  handle_ = nullptr;
}

void *SharedLibrary::symbol(const std::string &name) const {
  if (handle_ == nullptr) {
    error_ = "library not loaded";
    return nullptr;
  }
#ifdef _WIN32
  FARPROC proc = GetProcAddress(reinterpret_cast<HMODULE>(handle_), name.c_str());
  if (proc == nullptr)
    error_ = lastErrorMessage();
  return reinterpret_cast<void *>(proc);
#else
  dlerror();
  void *sym = dlsym(handle_, name.c_str());
  if (sym == nullptr) {
    const char *message = dlerror();
    error_ = message != nullptr ? message : "symbol not found";
  }
  return sym;
#endif
}

}  // namespace mbstudio::platform
