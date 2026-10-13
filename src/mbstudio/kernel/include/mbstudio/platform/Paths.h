// MB-Studio platform layer - file system locations.
//
// All strings are UTF-8. On Windows they are converted to UTF-16 at the
// Win32 API boundary, so non-ASCII paths work regardless of the code page.
#pragma once

#include <string>
#include <vector>

#include "mbstudio/kernel/Export.h"

namespace mbstudio::platform {

MBSTUDIO_KERNEL_API std::string executablePath();
MBSTUDIO_KERNEL_API std::string executableDirectory();
// Parent of the directory holding the executable (the install prefix for
// <prefix>/bin/mbstudio[.exe]).
MBSTUDIO_KERNEL_API std::string installPrefix();
// MBSTUDIO_PLUGIN_PATH entries followed by <prefix>/<libdir>/mbstudio/plugins.
MBSTUDIO_KERNEL_API std::vector<std::string> pluginSearchPath(const std::string &relativePluginDir);
// MBSTUDIO_TOOL_PATH entries, the executable directory and <prefix>/bin.
MBSTUDIO_KERNEL_API std::vector<std::string> toolSearchPath();
// Per-user settings directory: %APPDATA%\MB-Studio on Windows,
// $XDG_CONFIG_HOME/mb-studio (default ~/.config/mb-studio) elsewhere.
MBSTUDIO_KERNEL_API std::string userConfigDirectory();
MBSTUDIO_KERNEL_API std::string homeDirectory();

MBSTUDIO_KERNEL_API char pathListSeparator();       // ';' on Windows, ':' elsewhere
MBSTUDIO_KERNEL_API const char *executableSuffix();  // ".exe" on Windows
MBSTUDIO_KERNEL_API std::vector<std::string> splitPathList(const std::string &list);
MBSTUDIO_KERNEL_API std::string joinPath(const std::string &a, const std::string &b);
MBSTUDIO_KERNEL_API std::string parentPath(const std::string &path);
MBSTUDIO_KERNEL_API std::string fileName(const std::string &path);
// Convert separators to '/' (accepted by every Windows API) or to the native one.
MBSTUDIO_KERNEL_API std::string toGenericSeparators(const std::string &path);
MBSTUDIO_KERNEL_API std::string toNativeSeparators(const std::string &path);
MBSTUDIO_KERNEL_API bool isAbsolutePath(const std::string &path);

MBSTUDIO_KERNEL_API bool fileExists(const std::string &path);
MBSTUDIO_KERNEL_API bool isDirectory(const std::string &path);
// Whole file (without a UTF-8 byte order mark); false if it cannot be read.
MBSTUDIO_KERNEL_API bool readTextFile(const std::string &path, std::string &contents);
MBSTUDIO_KERNEL_API bool makeDirectories(const std::string &path);
// Write (replace) or append to a file; UTF-8 file names on every platform.
MBSTUDIO_KERNEL_API bool writeTextFile(const std::string &path, const std::string &contents);
MBSTUDIO_KERNEL_API bool appendTextFile(const std::string &path, const std::string &contents);
MBSTUDIO_KERNEL_API bool removeFile(const std::string &path);
// Size in bytes (-1 if missing) and last modification (seconds since the epoch, 0 if missing).
MBSTUDIO_KERNEL_API long long fileSize(const std::string &path);
MBSTUDIO_KERNEL_API double fileModifiedTime(const std::string &path);
// path relative to base (possibly starting with "../"), or path unchanged
// when there is no relative form (another drive); generic '/' separators.
MBSTUDIO_KERNEL_API std::string relativePath(const std::string &path, const std::string &base);
MBSTUDIO_KERNEL_API std::string absolutePath(const std::string &path);
MBSTUDIO_KERNEL_API std::vector<std::string> listDirectory(const std::string &path);

MBSTUDIO_KERNEL_API std::string getEnv(const std::string &name);
MBSTUDIO_KERNEL_API bool setEnv(const std::string &name, const std::string &value);

#ifdef _WIN32
MBSTUDIO_KERNEL_API std::wstring widen(const std::string &utf8);
MBSTUDIO_KERNEL_API std::string narrow(const std::wstring &utf16);
#endif

}  // namespace mbstudio::platform
