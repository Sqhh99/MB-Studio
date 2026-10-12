// MB-Studio host executable.
//
// The host knows only the kernel. It configures the plugin and tool search
// paths, starts the kernel (which loads the plugins) and then either hands
// control to the front end plugin (the graphical user interface) or, in its
// command line modes, executes kernel commands directly:
//
//   mbstudio                       start the graphical user interface
//   mbstudio --list-plugins        show the loaded plugins and their state
//   mbstudio --list-commands       show the commands registered by plugins
//   mbstudio --command <id> [key=value ...]
//                                  run one command and print its JSON result
//   mbstudio --headless            start the kernel without a front end
//   mbstudio --disable <plugin>    do not start a plugin (repeatable)
//   mbstudio --plugin-dir <dir>    additional plugin directory (repeatable)

#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#include "mbstudio/api/Frontend.h"
#include "mbstudio/kernel/Kernel.h"
#include "mbstudio/platform/Paths.h"

namespace {

#ifdef _WIN32
// The host is a Windows GUI-subsystem program so that no console window
// appears when the user starts MB-Studio. For the command line modes attach
// to the console of the parent process, unless the standard handles were
// already redirected (pipes or files, e.g. by a script or the test suite).
void attachParentConsole() {
  HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
  if (out != nullptr && out != INVALID_HANDLE_VALUE && GetFileType(out) != FILE_TYPE_UNKNOWN)
    return;
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    FILE *ignored = nullptr;
    ignored = freopen("CONOUT$", "w", stdout);
    ignored = freopen("CONOUT$", "w", stderr);
    (void)ignored;
    SetConsoleOutputCP(CP_UTF8);
  }
}

// UTF-8 command line arguments independent of the active code page.
std::vector<std::string> utf8Arguments(int argc, char **argv) {
  std::vector<std::string> out;
  int wargc = 0;
  LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
  if (wargv == nullptr) {
    for (int i = 0; i < argc; i++)
      out.emplace_back(argv[i]);
    return out;
  }
  for (int i = 0; i < wargc; i++)
    out.push_back(mbstudio::platform::narrow(wargv[i]));
  LocalFree(wargv);
  return out;
}
#else
std::vector<std::string> utf8Arguments(int argc, char **argv) { return std::vector<std::string>(argv, argv + argc); }
#endif

void printUsage() {
  std::printf(
      "MB-Studio %s - desktop front end for MB-System\n\n"
      "usage: mbstudio [options] [file ...]\n\n"
      "  --headless               start the kernel and plugins without the user interface\n"
      "  --list-plugins           list plugins and their state\n"
      "  --list-commands          list the commands provided by plugins\n"
      "  --list-services          list the services provided by plugins\n"
      "  --command ID [k=v ...]   run a command and print its result as JSON\n"
      "  --disable PLUGIN         do not start PLUGIN (repeatable)\n"
      "  --plugin-dir DIR         also load plugins from DIR (repeatable)\n"
      "  --version                print the version\n"
      "  --help                   show this help\n",
      MBSTUDIO_VERSION);
}

mbstudio::Value parseValue(const std::string &text) {
  if (text == "true")
    return true;
  if (text == "false")
    return false;
  char *end = nullptr;
  const long long i = std::strtoll(text.c_str(), &end, 10);
  if (!text.empty() && end != nullptr && *end == '\0')
    return static_cast<std::int64_t>(i);
  return text;
}

}  // namespace

int main(int argc, char **argv) {
  const std::vector<std::string> args = utf8Arguments(argc, argv);

  bool headless = false;
  bool listPlugins = false;
  bool listCommands = false;
  bool listServices = false;
  std::string command;
  mbstudio::ValueMap commandArgs;
  mbstudio::KernelConfig config;
  config.version = MBSTUDIO_VERSION;

  for (const auto &id : mbstudio::platform::splitPathList(mbstudio::platform::getEnv("MBSTUDIO_DISABLE_PLUGINS")))
    config.disabledPlugins.insert(id);

  std::vector<std::string> extraPluginDirs;
  for (size_t i = 1; i < args.size(); i++) {
    const std::string &a = args[i];
    if (a == "--help" || a == "-h") {
#ifdef _WIN32
      attachParentConsole();
#endif
      printUsage();
      return 0;
    } else if (a == "--version") {
#ifdef _WIN32
      attachParentConsole();
#endif
      std::printf("MB-Studio %s\n", MBSTUDIO_VERSION);
      return 0;
    } else if (a == "--headless") {
      headless = true;
    } else if (a == "--list-plugins") {
      listPlugins = true;
    } else if (a == "--list-commands") {
      listCommands = true;
    } else if (a == "--list-services") {
      listServices = true;
    } else if (a == "--command" && i + 1 < args.size()) {
      command = args[++i];
      while (i + 1 < args.size() && args[i + 1].find('=') != std::string::npos && args[i + 1].rfind("--", 0) != 0) {
        const std::string kv = args[++i];
        const size_t eq = kv.find('=');
        commandArgs[kv.substr(0, eq)] = parseValue(kv.substr(eq + 1));
      }
    } else if (a == "--disable" && i + 1 < args.size()) {
      config.disabledPlugins.insert(args[++i]);
    } else if (a == "--plugin-dir" && i + 1 < args.size()) {
      extraPluginDirs.push_back(args[++i]);
    }
  }

  const bool commandLineMode = headless || listPlugins || listCommands || listServices || !command.empty();
#ifdef _WIN32
  if (commandLineMode)
    attachParentConsole();
#endif

  config.pluginDirectories = extraPluginDirs;
  for (const auto &dir : mbstudio::platform::pluginSearchPath(MBSTUDIO_PLUGIN_RELDIR))
    config.pluginDirectories.push_back(dir);
  config.toolDirectories = mbstudio::platform::toolSearchPath();
#ifdef MBSTUDIO_BUILD_TOOL_DIR
  // Developer convenience: find the programs when running from the build tree.
  if (mbstudio::platform::isDirectory(MBSTUDIO_BUILD_TOOL_DIR))
    config.toolDirectories.push_back(MBSTUDIO_BUILD_TOOL_DIR);
#endif
  if (headless || !command.empty())
    config.disabledPlugins.insert("qmlui");  // no user interface needed

  mbstudio::Kernel kernel(config);
  kernel.start();

  int status = 0;
  if (listPlugins) {
    for (const auto &p : kernel.plugins().plugins()) {
      std::printf("%-12s %-10s %-9s %s%s%s\n", p.info.id.c_str(), mbstudio::pluginStateName(p.state),
                  p.info.version.c_str(), p.info.name.c_str(), p.error.empty() ? "" : " - ", p.error.c_str());
    }
  }
  if (listServices) {
    for (const auto &s : kernel.services().list())
      std::printf("%-24s %s\n", s.id.c_str(), s.provider.c_str());
  }
  if (listCommands) {
    for (const auto &c : kernel.commands().list())
      std::printf("%-20s %-10s %s\n", c.id.c_str(), c.owner.c_str(), c.description.c_str());
  }
  if (!command.empty()) {
    const mbstudio::CommandResult result = kernel.commands().execute(command, commandArgs);
    mbstudio::ValueMap out = result.data;
    out["ok"] = result.ok;
    if (!result.message.empty())
      out["message"] = result.message;
    std::printf("%s\n", mbstudio::valueMapToJson(out).c_str());
    status = result.ok ? 0 : 1;
  }

  if (!commandLineMode) {
    auto frontend = kernel.services().get<mbstudio::IFrontend>();
    if (frontend) {
      status = frontend->run(kernel, argc, argv);
    } else {
#ifdef _WIN32
      attachParentConsole();
      MessageBoxW(nullptr,
                  L"The MB-Studio user interface plugin could not be loaded.\n"
                  L"Run \"mbstudio --list-plugins\" from a console for details.",
                  L"MB-Studio", MB_ICONERROR | MB_OK);
#endif
      std::fprintf(stderr, "No user interface plugin is available. Plugins:\n");
      for (const auto &p : kernel.plugins().plugins())
        std::fprintf(stderr, "  %s: %s %s\n", p.info.id.c_str(), mbstudio::pluginStateName(p.state), p.error.c_str());
      status = 2;
    }
  }

  std::fflush(stdout);
  kernel.stop();
  return status;
}
