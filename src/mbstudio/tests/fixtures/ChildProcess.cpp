// Helper program for the process tests: prints its arguments (one per line,
// wrapped in <>), the value of MBSTUDIO_TEST_VAR and its working directory,
// then exits with the code given by --exit=N (or sleeps with --sleep).
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
// shellapi.h needs windows.h first
#include <direct.h>
#include <shellapi.h>
#else
#include <unistd.h>
#endif

int main(int argc, char **argv) {
  int code = 0;
#ifdef _WIN32
  // Print the arguments exactly as Windows hands them over (UTF-16 -> UTF-8).
  int wargc = 0;
  LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
  for (int i = 1; i < wargc; i++) {
    char buffer[4096];
    WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, buffer, sizeof(buffer), nullptr, nullptr);
    std::printf("<%s>\n", buffer);
  }
  LocalFree(wargv);
#else
  for (int i = 1; i < argc; i++)
    std::printf("<%s>\n", argv[i]);
#endif
  for (int i = 1; i < argc; i++) {
    if (std::strncmp(argv[i], "--exit=", 7) == 0)
      code = std::atoi(argv[i] + 7);
    if (std::strcmp(argv[i], "--sleep") == 0) {
      std::fflush(stdout);
      std::this_thread::sleep_for(std::chrono::seconds(30));
    }
  }
  const char *var = std::getenv("MBSTUDIO_TEST_VAR");
  std::printf("var=%s\n", var != nullptr ? var : "");
  char cwd[4096] = "";
#ifdef _WIN32
  _getcwd(cwd, sizeof(cwd));
#else
  if (getcwd(cwd, sizeof(cwd)) == nullptr)
    cwd[0] = '\0';
#endif
  std::printf("cwd=%s\n", cwd);
  std::fprintf(stderr, "stderr line\n");
  return code;
}
