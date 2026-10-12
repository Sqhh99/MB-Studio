/*--------------------------------------------------------------------
 *    MB-Studio:  test/platform/mb_platform_test.cc
 *
 *    Platform adaptation tests for the MBIO library. Every test runs on all
 *    platforms; together they pin down the behaviour that the native Windows
 *    port has to reproduce exactly.
 *
 *    See README.md file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/

#include <sys/stat.h>
#include <sys/types.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "mb_define.h"
#include "mb_format.h"
#include "mb_status.h"

#define MB_NEED_SENSOR_TYPE
#include "mb_io.h"

#include <gtest/gtest.h>

namespace {

std::string TempDir() {
#ifdef _WIN32
  char buffer[MAX_PATH + 1];
  const DWORD n = GetTempPathA(sizeof(buffer), buffer);
  std::string dir = (n > 0 && n < sizeof(buffer)) ? std::string(buffer, n) : std::string(".\\");
  if (!dir.empty() && dir.back() != '\\' && dir.back() != '/')
    dir += '\\';
  return dir;
#else
  const char *tmp = getenv("TMPDIR");
  std::string dir = (tmp != nullptr && tmp[0] != '\0') ? tmp : "/tmp";
  if (dir.back() != '/')
    dir += '/';
  return dir;
#endif
}

std::string UniqueName(const char *stem) {
  static int counter = 0;
#ifdef _WIN32
  const unsigned long pid = GetCurrentProcessId();
#else
  const unsigned long pid = static_cast<unsigned long>(getpid());
#endif
  return TempDir() + stem + "_" + std::to_string(pid) + "_" + std::to_string(counter++);
}

void SetEnv(const char *name, const char *value) {
#ifdef _WIN32
  _putenv_s(name, value == nullptr ? "" : value);
#else
  if (value == nullptr)
    unsetenv(name);
  else
    setenv(name, value, 1);
#endif
}

// Restores an environment variable when it goes out of scope.
class ScopedEnv {
 public:
  ScopedEnv(const char *name, const char *value) : name_(name) {
    const char *old = getenv(name);
    had_value_ = old != nullptr;
    if (had_value_)
      old_ = old;
    SetEnv(name, value);
  }
  ~ScopedEnv() { SetEnv(name_.c_str(), had_value_ ? old_.c_str() : nullptr); }

 private:
  std::string name_;
  std::string old_;
  bool had_value_ = false;
};

}  // namespace

// Data files must round-trip byte for byte: on Windows a stream opened in
// text mode would turn 0x0A into 0x0D 0x0A and stop reading at 0x1A.
TEST(PlatformBinaryIo, AllByteValuesRoundTrip) {
  const std::string path = UniqueName("mb_binary_io");
  std::vector<unsigned char> bytes(256 * 4);
  for (size_t i = 0; i < bytes.size(); i++)
    bytes[i] = static_cast<unsigned char>(i % 256);

  FILE *fp = fopen(path.c_str(), "wb");
  ASSERT_NE(nullptr, fp);
  ASSERT_EQ(bytes.size(), fwrite(bytes.data(), 1, bytes.size(), fp));
  fclose(fp);

  struct stat st;
  ASSERT_EQ(0, stat(path.c_str(), &st));
  EXPECT_EQ(static_cast<off_t>(bytes.size()), st.st_size);

  std::vector<unsigned char> back(bytes.size() + 16, 0);
  fp = fopen(path.c_str(), "rb");
  ASSERT_NE(nullptr, fp);
  EXPECT_EQ(bytes.size(), fread(back.data(), 1, back.size(), fp));
  fclose(fp);
  back.resize(bytes.size());
  EXPECT_EQ(bytes, back);
  remove(path.c_str());
}

// MB-System reads multi-gigabyte sonar files; file offsets must be 64 bit.
TEST(PlatformBinaryIo, FileOffsetsAre64Bit) {
  EXPECT_EQ(8u, sizeof(off_t));
  const std::string path = UniqueName("mb_offsets");
  FILE *fp = fopen(path.c_str(), "wb+");
  ASSERT_NE(nullptr, fp);
  const char text[] = "0123456789";
  ASSERT_EQ(sizeof(text), fwrite(text, 1, sizeof(text), fp));
  ASSERT_EQ(0, fseeko(fp, 3, SEEK_SET));
  EXPECT_EQ(static_cast<off_t>(3), ftello(fp));
  EXPECT_EQ('3', fgetc(fp));
  fclose(fp);
  remove(path.c_str());
}

// Non-ASCII (e.g. Chinese) file names are passed around as UTF-8 strings.
// On Windows this relies on the UTF-8 active code page manifest embedded in
// every executable by src/win32.
TEST(PlatformUtf8, NonAsciiFileNamesWork) {
#ifdef _WIN32
  if (GetACP() != CP_UTF8 && getenv("MB_TEST_ALLOW_LEGACY_CODEPAGE") != nullptr)
    GTEST_SKIP() << "UTF-8 active code page not supported by this Windows host";
  ASSERT_EQ(static_cast<UINT>(CP_UTF8), GetACP())
      << "the UTF-8 application manifest is not active";
#endif
  // "测线_ü" : Chinese "survey line" plus a Latin-1 character
  const std::string path = UniqueName("mb_utf8_\xE6\xB5\x8B\xE7\xBA\xBF_\xC3\xBC") + ".mb21";
  FILE *fp = fopen(path.c_str(), "wb");
  ASSERT_NE(nullptr, fp) << strerror(errno);
  fputs("data", fp);
  fclose(fp);

  struct stat st;
  EXPECT_EQ(0, stat(path.c_str(), &st));
  EXPECT_EQ(4, st.st_size);

  // the format is still recognised from the file suffix
  char fileroot[MB_PATH_MAXLINE];
  int format = 0;
  int error = MB_ERROR_NO_ERROR;
  std::vector<char> name(path.begin(), path.end());
  name.push_back('\0');
  mb_get_format(0, name.data(), fileroot, &format, &error);
  EXPECT_EQ(21, format);
  EXPECT_EQ(0, remove(path.c_str()));
}

// Home directory used for ~/.mbio_defaults and friends.
TEST(PlatformHome, HomeVariableTakesPrecedence) {
  ScopedEnv home("HOME", "/some/home");
  ASSERT_NE(nullptr, mb_get_home_dir());
  EXPECT_STREQ("/some/home", mb_get_home_dir());
}

#ifdef _WIN32
TEST(PlatformHome, WindowsFallsBackToUserProfile) {
  ScopedEnv home("HOME", nullptr);
  ScopedEnv profile("USERPROFILE", "C:\\Users\\surveyor");
  ASSERT_NE(nullptr, mb_get_home_dir());
  EXPECT_STREQ("C:\\Users\\surveyor", mb_get_home_dir());
}

TEST(PlatformHome, WindowsFallsBackToHomeDriveAndPath) {
  ScopedEnv home("HOME", nullptr);
  ScopedEnv profile("USERPROFILE", nullptr);
  ScopedEnv drive("HOMEDRIVE", "D:");
  ScopedEnv path("HOMEPATH", "\\Users\\surveyor");
  ASSERT_NE(nullptr, mb_get_home_dir());
  EXPECT_STREQ("D:\\Users\\surveyor", mb_get_home_dir());
}
#endif

TEST(PlatformHome, DefaultsAreReadFromHomeDirectory) {
  const std::string dir = UniqueName("mb_home");
#ifdef _WIN32
  ASSERT_EQ(0, _mkdir(dir.c_str()));
#else
  ASSERT_EQ(0, mkdir(dir.c_str(), 0755));
#endif
  const std::string file = dir + "/.mbio_defaults";
  FILE *fp = fopen(file.c_str(), "w");
  ASSERT_NE(nullptr, fp);
  fputs("MBIO Default Control Parameters\nlonflip:    1\ntimegap:    1.0\n", fp);
  fclose(fp);

  ScopedEnv home("HOME", dir.c_str());
  int lonflip = -99;
  EXPECT_EQ(MB_SUCCESS, mb_lonflip(0, &lonflip));
  EXPECT_EQ(1, lonflip);

  remove(file.c_str());
#ifdef _WIN32
  _rmdir(dir.c_str());
#else
  rmdir(dir.c_str());
#endif
}

TEST(PlatformIdentity, UserHostAndDateAreKnown) {
  char user[256];
  char host[256];
  char date[32];
  int error = MB_ERROR_NO_ERROR;
  EXPECT_EQ(MB_SUCCESS, mb_user_host_date(0, user, host, date, &error));
  EXPECT_GT(strlen(date), 0u);
  EXPECT_EQ(nullptr, strchr(date, '\n'));
  EXPECT_GT(strlen(host), 0u) << "host name could not be determined";
  EXPECT_GT(strlen(user), 0u);
}

// The XDR encoding (used by the SURF and HMR1 formats; replaced by
// mb_xdr_win32 on Windows) must be big-endian and round-trip exactly.
TEST(PlatformXdr, EncodingIsBigEndianAndRoundTrips) {
  const std::string path = UniqueName("mb_xdr");
  FILE *fp = fopen(path.c_str(), "wb");
  ASSERT_NE(nullptr, fp);
  XDR xdrs;
  xdrstdio_create(&xdrs, fp, XDR_ENCODE);
  int ivalue = 0x01020304;
  int negative = -2;
  float fvalue = 1.5f;
  double dvalue = -1234.5678;
  char opaque[5] = {'M', 'B', 'S', 'Y', 'S'};
  EXPECT_TRUE(xdr_int(&xdrs, &ivalue));
  EXPECT_TRUE(xdr_int(&xdrs, &negative));
  EXPECT_TRUE(xdr_float(&xdrs, &fvalue));
  EXPECT_TRUE(xdr_double(&xdrs, &dvalue));
  EXPECT_TRUE(xdr_opaque(&xdrs, opaque, 5));
  xdr_destroy(&xdrs);
  fclose(fp);

  // 4 + 4 + 4 + 8 + 8 (opaque data padded to a multiple of 4)
  fp = fopen(path.c_str(), "rb");
  ASSERT_NE(nullptr, fp);
  unsigned char raw[64];
  const size_t n = fread(raw, 1, sizeof(raw), fp);
  fclose(fp);
  ASSERT_EQ(28u, n);
  EXPECT_EQ(0x01, raw[0]);
  EXPECT_EQ(0x02, raw[1]);
  EXPECT_EQ(0x03, raw[2]);
  EXPECT_EQ(0x04, raw[3]);
  EXPECT_EQ(0xFF, raw[4]);
  EXPECT_EQ(0xFE, raw[7]);
  EXPECT_EQ(0x3F, raw[8]);  // 1.5f == 0x3FC00000
  EXPECT_EQ(0xC0, raw[9]);
  EXPECT_EQ(0xC0, raw[12]);  // negative double, sign bit first
  EXPECT_EQ('M', raw[20]);

  fp = fopen(path.c_str(), "rb");
  ASSERT_NE(nullptr, fp);
  xdrstdio_create(&xdrs, fp, XDR_DECODE);
  int iback = 0;
  int nback = 0;
  float fback = 0.0f;
  double dback = 0.0;
  char oback[5] = {0};
  EXPECT_TRUE(xdr_int(&xdrs, &iback));
  EXPECT_TRUE(xdr_int(&xdrs, &nback));
  EXPECT_TRUE(xdr_float(&xdrs, &fback));
  EXPECT_TRUE(xdr_double(&xdrs, &dback));
  EXPECT_TRUE(xdr_opaque(&xdrs, oback, 5));
  xdr_destroy(&xdrs);
  fclose(fp);
  EXPECT_EQ(ivalue, iback);
  EXPECT_EQ(negative, nback);
  EXPECT_EQ(fvalue, fback);
  EXPECT_EQ(dvalue, dback);
  EXPECT_EQ(0, memcmp(opaque, oback, 5));
  remove(path.c_str());
}

// Data exported from the mbio DLL must be reachable from C++ code.
TEST(PlatformDll, SensorTypeTablesAreExported) {
  EXPECT_EQ(MB_SENSOR_TYPE_NONE, mb_sensor_type_id[0]);
  EXPECT_STREQ("Unknown sensor type", mb_sensor_type_string[0]);
}

// Times are doubles: dates beyond 2038 must not wrap on platforms where
// long and time_t were historically 32 bit.
TEST(PlatformTime, DatesBeyond2038RoundTrip) {
  int time_i[7] = {2100, 2, 28, 23, 59, 30, 250000};
  double time_d = 0.0;
  EXPECT_EQ(MB_SUCCESS, mb_get_time(0, time_i, &time_d));
  EXPECT_GT(time_d, 4.0e9);
  int back[7] = {0};
  EXPECT_EQ(MB_SUCCESS, mb_get_date(0, time_d, back));
  for (int i = 0; i < 6; i++)
    EXPECT_EQ(time_i[i], back[i]) << "field " << i;
  EXPECT_NEAR(time_i[6], back[6], 1);
}

// Windows path syntax: both separators, drive letters and spaces.
TEST(PlatformPaths, FormatFromWindowsStylePaths) {
  const char *paths[] = {"C:\\Survey Data\\line 001.mb21", "C:/Survey Data/line 001.mb21",
                         "\\\\server\\share\\line_001.mb21", "line_001.mb21"};
  for (const char *p : paths) {
    char path[MB_PATH_MAXLINE];
    char fileroot[MB_PATH_MAXLINE];
    strcpy(path, p);
    int format = 0;
    int error = MB_ERROR_NO_ERROR;
    mb_get_format(0, path, fileroot, &format, &error);
    EXPECT_EQ(21, format) << p;
    EXPECT_NE(nullptr, strstr(fileroot, "line")) << p;
    EXPECT_EQ(nullptr, strstr(fileroot, ".mb21")) << p;
  }
}

TEST(PlatformPaths, BasenameAcceptsForwardSlashes) {
  char path[MB_PATH_MAXLINE] = "C:/Survey Data/line_001.mb21";
  int error = MB_ERROR_NO_ERROR;
  EXPECT_EQ(MB_SUCCESS, mb_get_basename(0, path, &error));
  EXPECT_STREQ("line_001.mb21", path);
}

#ifdef _WIN32
TEST(PlatformPaths, RelativePathWithBackslashes) {
  char path[MB_PATH_MAXLINE] = "C:\\data\\survey\\line_001.mb21";
  char pwd[MB_PATH_MAXLINE] = "C:\\data";
  int error = MB_ERROR_NO_ERROR;
  EXPECT_EQ(MB_SUCCESS, mb_get_relative_path(0, path, pwd, &error));
  EXPECT_STREQ("survey/line_001.mb21", path);
}
#endif
