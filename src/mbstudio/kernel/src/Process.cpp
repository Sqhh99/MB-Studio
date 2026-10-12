#include "mbstudio/platform/Process.h"

#include <cstring>
#include <map>

#include "mbstudio/platform/Paths.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace mbstudio::platform {

std::string quoteWindowsArgument(const std::string &argument) {
  if (!argument.empty() && argument.find_first_of(" \t\n\v\"") == std::string::npos)
    return argument;
  std::string out = "\"";
  for (size_t i = 0;; i++) {
    size_t backslashes = 0;
    while (i < argument.size() && argument[i] == '\\') {
      backslashes++;
      i++;
    }
    if (i == argument.size()) {
      // Double the trailing backslashes so the closing quote is not escaped.
      out.append(backslashes * 2, '\\');
      break;
    }
    if (argument[i] == '"') {
      out.append(backslashes * 2 + 1, '\\');
      out += '"';
    } else {
      out.append(backslashes, '\\');
      out += argument[i];
    }
  }
  out += '"';
  return out;
}

std::string buildWindowsCommandLine(const std::string &program, const std::vector<std::string> &arguments) {
  // argv[0] is parsed with simpler rules: quotes only, no escapes.
  std::string line = program.find_first_of(" \t") != std::string::npos ? "\"" + program + "\"" : program;
  for (const auto &arg : arguments)
    line += " " + quoteWindowsArgument(arg);
  return line;
}

std::vector<std::string> splitArguments(const std::string &line) {
  std::vector<std::string> out;
  std::string current;
  bool inWord = false;
  char quote = 0;
  for (size_t i = 0; i < line.size(); i++) {
    const char c = line[i];
    if (quote != 0) {
      if (c == '\\' && quote == '"' && i + 1 < line.size() && line[i + 1] == '"') {
        current += '"';
        i++;
      } else if (c == quote) {
        quote = 0;
      } else {
        current += c;
      }
      continue;
    }
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      if (inWord) {
        out.push_back(current);
        current.clear();
        inWord = false;
      }
      continue;
    }
    inWord = true;
    if (c == '"' || c == '\'') {
      quote = c;
    } else if (c == '\\' && i + 1 < line.size() && (line[i + 1] == '"' || line[i + 1] == '\'')) {
      current += line[i + 1];
      i++;
    } else {
      current += c;
    }
  }
  if (inWord)
    out.push_back(current);
  return out;
}

std::string formatCommand(const std::string &program, const std::vector<std::string> &arguments) {
  auto display = [](const std::string &s) {
    if (!s.empty() && s.find_first_of(" \t\"'") == std::string::npos)
      return s;
    std::string out = "\"";
    for (char c : s) {
      if (c == '"')
        out += "\\\"";
      else
        out += c;
    }
    return out + "\"";
  };
  std::string out = display(program);
  for (const auto &arg : arguments)
    out += " " + display(arg);
  return out;
}

#ifdef _WIN32

namespace {

std::wstring upper(const std::wstring &s) {
  std::wstring out = s;
  for (auto &c : out)
    c = static_cast<wchar_t>(towupper(c));
  return out;
}

// Build a CREATE_UNICODE_ENVIRONMENT block: the current environment with the
// requested variables replaced (names are case-insensitive on Windows).
std::wstring buildEnvironmentBlock(const std::vector<std::pair<std::string, std::string>> &overrides) {
  std::map<std::wstring, std::wstring> entries;
  wchar_t *env = GetEnvironmentStringsW();
  for (const wchar_t *p = env; p != nullptr && *p != L'\0'; p += wcslen(p) + 1) {
    const std::wstring entry(p);
    const size_t eq = entry.find(L'=', 1);  // entries like "=C:=C:\" start with '='
    const std::wstring name = eq == std::wstring::npos ? entry : entry.substr(0, eq);
    entries[upper(name)] = entry;
  }
  if (env != nullptr)
    FreeEnvironmentStringsW(env);
  for (const auto &kv : overrides) {
    const std::wstring name = widen(kv.first);
    entries[upper(name)] = name + L"=" + widen(kv.second);
  }
  std::wstring block;
  for (const auto &kv : entries) {
    block += kv.second;
    block += L'\0';
  }
  block += L'\0';
  return block;
}

}  // namespace

ProcessResult runProcess(const ProcessSpec &spec, const OutputHandler &output, const std::atomic<bool> *cancel) {
  ProcessResult result;
  std::string program = spec.program;
  if (!fileExists(program) && fileExists(program + ".exe"))
    program += ".exe";

  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = nullptr;
  sa.bInheritHandle = TRUE;

  HANDLE readPipe = nullptr;
  HANDLE writePipe = nullptr;
  if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) {
    result.error = "cannot create pipe";
    return result;
  }
  SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
  HANDLE nul = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);

  STARTUPINFOW si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = nul;
  si.hStdOutput = writePipe;
  si.hStdError = writePipe;

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  std::wstring commandLine = widen(buildWindowsCommandLine(toNativeSeparators(program), spec.arguments));
  std::wstring environment;
  if (!spec.environment.empty())
    environment = buildEnvironmentBlock(spec.environment);
  const std::wstring workingDirectory = widen(toNativeSeparators(spec.workingDirectory));

  // With a bare program name let CreateProcessW search the PATH.
  const std::wstring application = widen(toNativeSeparators(program));
  const bool hasDirectory = program.find_first_of("\\/") != std::string::npos;
  const BOOL created = CreateProcessW(
      hasDirectory ? application.c_str() : nullptr, commandLine.data(), nullptr, nullptr, TRUE,
      CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, environment.empty() ? nullptr : environment.data(),
      workingDirectory.empty() ? nullptr : workingDirectory.c_str(), &si, &pi);
  CloseHandle(writePipe);
  if (nul != INVALID_HANDLE_VALUE)
    CloseHandle(nul);
  if (!created) {
    result.error = "cannot start " + program + " (error " + std::to_string(GetLastError()) + ")";
    CloseHandle(readPipe);
    return result;
  }
  result.started = true;

  char buffer[4096];
  for (;;) {
    DWORD available = 0;
    if (!PeekNamedPipe(readPipe, nullptr, 0, nullptr, &available, nullptr)) {
      break;  // broken pipe: the process closed its output
    }
    if (available > 0) {
      DWORD read = 0;
      if (ReadFile(readPipe, buffer, sizeof(buffer), &read, nullptr) && read > 0 && output)
        output(std::string(buffer, read));
      continue;
    }
    if (cancel != nullptr && cancel->load()) {
      TerminateProcess(pi.hProcess, 1);
      result.cancelled = true;
      break;
    }
    if (WaitForSingleObject(pi.hProcess, 20) == WAIT_OBJECT_0) {
      // drain what is left in the pipe
      DWORD read = 0;
      while (PeekNamedPipe(readPipe, nullptr, 0, nullptr, &available, nullptr) && available > 0 &&
             ReadFile(readPipe, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
        if (output)
          output(std::string(buffer, read));
      }
      break;
    }
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exitCode = 0;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  result.exitCode = static_cast<int>(exitCode);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  CloseHandle(readPipe);
  return result;
}

#else  // POSIX

ProcessResult runProcess(const ProcessSpec &spec, const OutputHandler &output, const std::atomic<bool> *cancel) {
  ProcessResult result;
  if (spec.program.find('/') != std::string::npos && !fileExists(spec.program)) {
    result.error = "program not found: " + spec.program;
    return result;
  }
  int fds[2];
  if (pipe(fds) != 0) {
    result.error = "cannot create pipe";
    return result;
  }

  std::vector<std::string> argvStrings;
  argvStrings.push_back(spec.program);
  argvStrings.insert(argvStrings.end(), spec.arguments.begin(), spec.arguments.end());
  std::vector<char *> argv;
  for (auto &s : argvStrings)
    argv.push_back(s.data());
  argv.push_back(nullptr);

  const pid_t pid = fork();
  if (pid < 0) {
    close(fds[0]);
    close(fds[1]);
    result.error = "fork failed";
    return result;
  }
  if (pid == 0) {
    const int devnull = open("/dev/null", O_RDONLY);
    if (devnull >= 0)
      dup2(devnull, 0);
    dup2(fds[1], 1);
    dup2(fds[1], 2);
    close(fds[0]);
    close(fds[1]);
    if (!spec.workingDirectory.empty() && chdir(spec.workingDirectory.c_str()) != 0)
      _exit(126);
    for (const auto &kv : spec.environment)
      setenv(kv.first.c_str(), kv.second.c_str(), 1);
    execvp(spec.program.c_str(), argv.data());
    _exit(127);
  }

  close(fds[1]);
  result.started = true;
  char buffer[4096];
  for (;;) {
    struct pollfd pfd;
    pfd.fd = fds[0];
    pfd.events = POLLIN;
    pfd.revents = 0;
    const int ready = poll(&pfd, 1, 50);
    if (ready > 0) {
      const ssize_t n = read(fds[0], buffer, sizeof(buffer));
      if (n > 0) {
        if (output)
          output(std::string(buffer, static_cast<size_t>(n)));
        continue;
      }
      if (n == 0)
        break;  // EOF
    }
    if (cancel != nullptr && cancel->load()) {
      kill(pid, SIGTERM);
      result.cancelled = true;
      break;
    }
  }
  close(fds[0]);
  int status = 0;
  waitpid(pid, &status, 0);
  if (WIFEXITED(status))
    result.exitCode = WEXITSTATUS(status);
  else if (WIFSIGNALED(status))
    result.exitCode = 128 + WTERMSIG(status);
  if (result.exitCode == 127 && !result.cancelled)
    result.error = "cannot execute " + spec.program;
  return result;
}

#endif

}  // namespace mbstudio::platform
