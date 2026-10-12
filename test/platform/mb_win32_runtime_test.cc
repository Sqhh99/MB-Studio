/*--------------------------------------------------------------------
 *    MB-Studio:  test/platform/mb_win32_runtime_test.cc
 *
 *    Windows-only tests of the start-up shim (src/win32) that is linked into
 *    every MB-System executable built for Windows, and of the properties the
 *    build gives those executables.
 *
 *    See README.md file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/

#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include "mb_win32_runtime.h"

#include <gtest/gtest.h>

namespace {

std::string ParentDir(std::string path) {
  const size_t pos = path.find_last_of("\\/");
  return pos == std::string::npos ? std::string() : path.substr(0, pos);
}

std::string ExecutablePath() {
  char buffer[MB_WIN32_PATH_MAX];
  const DWORD n = GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
  return std::string(buffer, n);
}

}  // namespace

TEST(Win32Runtime, StandardStreamsAreBinary) {
  // _setmode returns the previous mode
  const int out_mode = _setmode(_fileno(stdout), _O_BINARY);
  const int err_mode = _setmode(_fileno(stderr), _O_BINARY);
  EXPECT_EQ(_O_BINARY, out_mode);
  EXPECT_EQ(_O_BINARY, err_mode);
}

TEST(Win32Runtime, InstallPrefixIsParentOfExecutableDirectory) {
  char prefix[MB_WIN32_PATH_MAX];
  ASSERT_EQ(0, mb_win32_install_prefix(prefix, sizeof(prefix)));
  EXPECT_EQ(ParentDir(ParentDir(ExecutablePath())), std::string(prefix));
}

TEST(Win32Runtime, InstallPrefixRejectsSmallBuffers) {
  char prefix[2];
  EXPECT_EQ(-1, mb_win32_install_prefix(prefix, sizeof(prefix)));
  EXPECT_EQ(-1, mb_win32_install_prefix(nullptr, 10));
}

TEST(Win32Runtime, DefaultEnvRespectsUserSetting) {
  _putenv_s("MB_TEST_RUNTIME_VAR", "user value");
  EXPECT_EQ(0, mb_win32_default_env("MB_TEST_RUNTIME_VAR", "share"));
  EXPECT_STREQ("user value", getenv("MB_TEST_RUNTIME_VAR"));
  _putenv_s("MB_TEST_RUNTIME_VAR", "");
}

TEST(Win32Runtime, DefaultEnvIgnoresMissingDirectories) {
  _putenv_s("MB_TEST_RUNTIME_MISSING", "");
  EXPECT_EQ(-1, mb_win32_default_env("MB_TEST_RUNTIME_MISSING", "share\\does-not-exist-mbstudio"));
  const char *value = getenv("MB_TEST_RUNTIME_MISSING");
  EXPECT_TRUE(value == nullptr || value[0] == '\0');
}

TEST(Win32Runtime, DefaultEnvPointsAtInstalledShareDirectory) {
  char prefix[MB_WIN32_PATH_MAX];
  ASSERT_EQ(0, mb_win32_install_prefix(prefix, sizeof(prefix)));
  const std::string share = std::string(prefix) + "\\share";
  const std::string data = share + "\\mbstudio-runtime-test";
  _mkdir(share.c_str());
  ASSERT_EQ(0, _mkdir(data.c_str()));

  _putenv_s("MB_TEST_RUNTIME_DATA", "");
  EXPECT_EQ(1, mb_win32_default_env("MB_TEST_RUNTIME_DATA", "share\\mbstudio-runtime-test"));
  ASSERT_NE(nullptr, getenv("MB_TEST_RUNTIME_DATA"));
  EXPECT_EQ(data, std::string(getenv("MB_TEST_RUNTIME_DATA")));

  _putenv_s("MB_TEST_RUNTIME_DATA", "");
  _rmdir(data.c_str());
  _rmdir(share.c_str());  // only removed if we created it (empty)
}

TEST(Win32Executable, Utf8ActiveCodePage) {
  if (GetACP() != CP_UTF8 && getenv("MB_TEST_ALLOW_LEGACY_CODEPAGE") != nullptr)
    GTEST_SKIP() << "UTF-8 active code page not supported by this Windows host";
  EXPECT_EQ(static_cast<UINT>(CP_UTF8), GetACP());
}

TEST(Win32Executable, LargeStackReserve) {
  const auto *base = reinterpret_cast<const unsigned char *>(GetModuleHandleA(nullptr));
  ASSERT_NE(nullptr, base);
  const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
  ASSERT_EQ(IMAGE_DOS_SIGNATURE, dos->e_magic);
  const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
  ASSERT_EQ(static_cast<DWORD>(IMAGE_NT_SIGNATURE), nt->Signature);
  EXPECT_GE(nt->OptionalHeader.SizeOfStackReserve, static_cast<ULONGLONG>(64u * 1024u * 1024u));
}

TEST(Win32Executable, BigStackFramesDoNotOverflow) {
  // Programs such as mbauvloglist place several megabytes on the stack.
  volatile char frame[8 * 1024 * 1024];
  frame[0] = 1;
  frame[sizeof(frame) - 1] = 2;
  EXPECT_EQ(3, frame[0] + frame[sizeof(frame) - 1]);
}
