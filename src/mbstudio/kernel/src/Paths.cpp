#include "mbstudio/platform/Paths.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#endif

namespace fs = std::filesystem;

namespace mbstudio::platform {

#ifdef _WIN32
std::wstring widen(const std::string &utf8) {
  if (utf8.empty())
    return std::wstring();
  const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
  std::wstring out(static_cast<size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), out.data(), n);
  return out;
}

std::string narrow(const std::wstring &utf16) {
  if (utf16.empty())
    return std::string();
  const int n = WideCharToMultiByte(CP_UTF8, 0, utf16.data(), static_cast<int>(utf16.size()), nullptr, 0,
                                    nullptr, nullptr);
  std::string out(static_cast<size_t>(n), '\0');
  WideCharToMultiByte(CP_UTF8, 0, utf16.data(), static_cast<int>(utf16.size()), out.data(), n, nullptr, nullptr);
  return out;
}
#endif

namespace {

fs::path toPath(const std::string &utf8) {
#ifdef _WIN32
  return fs::path(widen(utf8));
#else
  return fs::path(utf8);
#endif
}

std::string fromPath(const fs::path &path) {
#ifdef _WIN32
  return narrow(path.wstring());
#else
  return path.string();
#endif
}

}  // namespace

std::string getEnv(const std::string &name) {
#ifdef _WIN32
  const std::wstring wname = widen(name);
  const DWORD n = GetEnvironmentVariableW(wname.c_str(), nullptr, 0);
  if (n == 0)
    return std::string();
  std::wstring value(n, L'\0');
  const DWORD m = GetEnvironmentVariableW(wname.c_str(), value.data(), n);
  value.resize(m);
  return narrow(value);
#else
  const char *value = std::getenv(name.c_str());
  return value != nullptr ? std::string(value) : std::string();
#endif
}

bool setEnv(const std::string &name, const std::string &value) {
#ifdef _WIN32
  // Keep the C runtime environment (getenv) and the Win32 one in sync.
  const bool ok = SetEnvironmentVariableW(widen(name).c_str(), widen(value).c_str()) != 0;
  _putenv_s(name.c_str(), value.c_str());
  return ok;
#else
  return ::setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}

std::string executablePath() {
#ifdef _WIN32
  std::wstring buffer(32768, L'\0');
  const DWORD n = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  buffer.resize(n);
  return narrow(buffer);
#elif defined(__APPLE__)
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  std::string buffer(size, '\0');
  if (_NSGetExecutablePath(buffer.data(), &size) != 0)
    return std::string();
  buffer.resize(strlen(buffer.c_str()));
  std::error_code ec;
  const fs::path canonical = fs::canonical(buffer, ec);
  return ec ? buffer : canonical.string();
#else
  std::error_code ec;
  const fs::path p = fs::read_symlink("/proc/self/exe", ec);
  return ec ? std::string() : p.string();
#endif
}

std::string executableDirectory() { return parentPath(executablePath()); }

std::string installPrefix() { return parentPath(executableDirectory()); }

char pathListSeparator() {
#ifdef _WIN32
  return ';';
#else
  return ':';
#endif
}

const char *executableSuffix() {
#ifdef _WIN32
  return ".exe";
#else
  return "";
#endif
}

std::vector<std::string> splitPathList(const std::string &list) {
  std::vector<std::string> out;
  std::string current;
  for (char c : list) {
    if (c == pathListSeparator()) {
      if (!current.empty())
        out.push_back(current);
      current.clear();
    } else {
      current += c;
    }
  }
  if (!current.empty())
    out.push_back(current);
  return out;
}

std::string joinPath(const std::string &a, const std::string &b) {
  if (a.empty())
    return b;
  if (b.empty())
    return a;
  if (isAbsolutePath(b))
    return b;
  const char last = a.back();
  if (last == '/' || last == '\\')
    return a + b;
#ifdef _WIN32
  return a + "\\" + b;
#else
  return a + "/" + b;
#endif
}

std::string parentPath(const std::string &path) {
  const size_t pos = path.find_last_of(
#ifdef _WIN32
      "\\/"
#else
      "/"
#endif
  );
  if (pos == std::string::npos)
    return std::string();
  if (pos == 0)
    return path.substr(0, 1);
#ifdef _WIN32
  if (pos == 2 && path[1] == ':')
    return path.substr(0, 3);  // "C:\"
#endif
  return path.substr(0, pos);
}

std::string fileName(const std::string &path) {
  const size_t pos = path.find_last_of(
#ifdef _WIN32
      "\\/"
#else
      "/"
#endif
  );
  return pos == std::string::npos ? path : path.substr(pos + 1);
}

std::string toGenericSeparators(const std::string &path) {
  std::string out = path;
#ifdef _WIN32
  for (char &c : out)
    if (c == '\\')
      c = '/';
#endif
  return out;
}

std::string toNativeSeparators(const std::string &path) {
  std::string out = path;
#ifdef _WIN32
  for (char &c : out)
    if (c == '/')
      c = '\\';
#endif
  return out;
}

bool isAbsolutePath(const std::string &path) {
  if (path.empty())
    return false;
  if (path[0] == '/')
    return true;
#ifdef _WIN32
  if (path[0] == '\\')
    return true;
  if (path.size() >= 2 && path[1] == ':' &&
      ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')))
    return true;
#endif
  return false;
}

bool fileExists(const std::string &path) {
  std::error_code ec;
  return !path.empty() && fs::exists(toPath(path), ec);
}

bool isDirectory(const std::string &path) {
  std::error_code ec;
  return !path.empty() && fs::is_directory(toPath(path), ec);
}

bool readTextFile(const std::string &path, std::string &contents) {
  std::ifstream in(toPath(path), std::ios::binary);
  if (!in)
    return false;
  std::ostringstream buffer;
  buffer << in.rdbuf();
  contents = buffer.str();
  if (contents.compare(0, 3, "\xEF\xBB\xBF") == 0)  // UTF-8 byte order mark
    contents.erase(0, 3);
  return true;
}

bool makeDirectories(const std::string &path) {
  std::error_code ec;
  fs::create_directories(toPath(path), ec);
  return isDirectory(path);
}

bool writeTextFile(const std::string &path, const std::string &contents) {
  std::ofstream out(toPath(path), std::ios::binary | std::ios::trunc);
  if (!out)
    return false;
  out << contents;
  return static_cast<bool>(out);
}

bool appendTextFile(const std::string &path, const std::string &contents) {
  std::ofstream out(toPath(path), std::ios::binary | std::ios::app);
  if (!out)
    return false;
  out << contents;
  return static_cast<bool>(out);
}

bool removeFile(const std::string &path) {
  std::error_code ec;
  return fs::remove(toPath(path), ec);
}

long long fileSize(const std::string &path) {
  std::error_code ec;
  const auto size = fs::file_size(toPath(path), ec);
  return ec ? -1 : static_cast<long long>(size);
}

double fileModifiedTime(const std::string &path) {
  std::error_code ec;
  const auto t = fs::last_write_time(toPath(path), ec);
  if (ec)
    return 0.0;
  // file_clock has no portable epoch in C++17: shift by the difference to now
  const auto sinceNow = t - fs::file_time_type::clock::now();
  const auto now = std::chrono::system_clock::now();
  const auto system = now + std::chrono::duration_cast<std::chrono::system_clock::duration>(sinceNow);
  return std::chrono::duration<double>(system.time_since_epoch()).count();
}

std::string absolutePath(const std::string &path) {
  std::error_code ec;
  const fs::path p = fs::absolute(toPath(path), ec);
  return ec ? path : fromPath(p.lexically_normal());
}

std::string relativePath(const std::string &path, const std::string &base) {
  const fs::path p = toPath(absolutePath(path));
  const fs::path b = toPath(absolutePath(base));
  const fs::path rel = p.lexically_relative(b);
  if (rel.empty())  // another drive
    return toGenericSeparators(path);
  return toGenericSeparators(fromPath(rel));
}

std::vector<std::string> listDirectory(const std::string &path) {
  std::vector<std::string> out;
  std::error_code ec;
  if (!isDirectory(path))
    return out;
  for (fs::directory_iterator it(toPath(path), ec), end; !ec && it != end; it.increment(ec))
    out.push_back(fromPath(it->path().filename()));
  return out;
}

std::string homeDirectory() {
  std::string home = getEnv("HOME");
#ifdef _WIN32
  if (home.empty())
    home = getEnv("USERPROFILE");
  if (home.empty()) {
    const std::string drive = getEnv("HOMEDRIVE");
    const std::string path = getEnv("HOMEPATH");
    if (!drive.empty() && !path.empty())
      home = drive + path;
  }
#endif
  return home;
}

std::string userConfigDirectory() {
#ifdef _WIN32
  std::string base = getEnv("APPDATA");
  if (base.empty())
    base = joinPath(homeDirectory(), "AppData\\Roaming");
  return joinPath(base, "MB-Studio");
#else
  std::string base = getEnv("XDG_CONFIG_HOME");
  if (base.empty())
    base = joinPath(homeDirectory(), ".config");
  return joinPath(base, "mb-studio");
#endif
}

std::vector<std::string> pluginSearchPath(const std::string &relativePluginDir) {
  std::vector<std::string> out = splitPathList(getEnv("MBSTUDIO_PLUGIN_PATH"));
  const std::string installed = joinPath(installPrefix(), toNativeSeparators(relativePluginDir));
  if (std::find(out.begin(), out.end(), installed) == out.end())
    out.push_back(installed);
  return out;
}

std::vector<std::string> toolSearchPath() {
  std::vector<std::string> out = splitPathList(getEnv("MBSTUDIO_TOOL_PATH"));
  out.push_back(executableDirectory());
  out.push_back(joinPath(installPrefix(), "bin"));
  return out;
}

}  // namespace mbstudio::platform
