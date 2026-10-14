// Platform adaptation tests of the MB-Studio kernel: paths, Windows command
// line quoting, argument splitting and process execution. They run on every
// platform; the Windows specific expectations are checked where they apply.
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "mbstudio/platform/Geodesy.h"
#include "mbstudio/platform/Paths.h"
#include "mbstudio/platform/Process.h"

using namespace mbstudio::platform;

namespace {

std::string childProgram() { return MBSTUDIO_CHILD_PROGRAM; }

std::vector<std::string> outputLines(const std::string &text) {
  std::vector<std::string> lines;
  std::string current;
  for (char c : text) {
    if (c == '\n') {
      if (!current.empty() && current.back() == '\r')
        current.pop_back();
      lines.push_back(current);
      current.clear();
    } else {
      current += c;
    }
  }
  if (!current.empty())
    lines.push_back(current);
  return lines;
}

}  // namespace

TEST(WindowsQuoting, PlainArgumentsAreUnchanged) {
  EXPECT_EQ("mbinfo", quoteWindowsArgument("mbinfo"));
  EXPECT_EQ("-IC:\\data\\file.mb88", quoteWindowsArgument("-IC:\\data\\file.mb88"));
}

TEST(WindowsQuoting, SpacesQuotesAndBackslashes) {
  EXPECT_EQ("\"\"", quoteWindowsArgument(""));
  EXPECT_EQ("\"C:\\Survey Data\\line 1.mb88\"", quoteWindowsArgument("C:\\Survey Data\\line 1.mb88"));
  // a trailing backslash must be doubled so it does not escape the quote
  EXPECT_EQ("\"C:\\Survey Data\\\\\"", quoteWindowsArgument("C:\\Survey Data\\"));
  EXPECT_EQ("\"say \\\"hi\\\"\"", quoteWindowsArgument("say \"hi\""));
  EXPECT_EQ("\"a\\\\\\\"b c\"", quoteWindowsArgument("a\\\"b c"));
}

TEST(WindowsQuoting, CommandLine) {
  EXPECT_EQ("\"C:\\Program Files\\MB-Studio\\bin\\mbinfo.exe\" \"-IC:\\My Data\\x.mb88\" -X1",
            buildWindowsCommandLine("C:\\Program Files\\MB-Studio\\bin\\mbinfo.exe", {"-IC:\\My Data\\x.mb88", "-X1"}));
}

TEST(SplitArguments, QuotesAndWindowsPaths) {
  const std::vector<std::string> expected = {"-I", "C:\\Survey Data\\line 1.mb88", "-F88", "it's", ""};
  EXPECT_EQ(expected, splitArguments("-I \"C:\\Survey Data\\line 1.mb88\"  -F88 \"it's\" ''"));
  EXPECT_EQ(std::vector<std::string>({"-Ia\\b", "x\"y"}), splitArguments("-Ia\\b x\\\"y"));
  EXPECT_TRUE(splitArguments("   ").empty());
}

TEST(SplitArguments, RoundTripThroughFormatCommand) {
  const std::vector<std::string> args = {"-I", "/data/with space/f.mb88", "plain"};
  const std::string shown = formatCommand("mbinfo", args);
  EXPECT_EQ("mbinfo -I \"/data/with space/f.mb88\" plain", shown);
  std::vector<std::string> back = splitArguments(shown);
  back.erase(back.begin());
  EXPECT_EQ(args, back);
}

TEST(Paths, JoinParentAndFileName) {
#ifdef _WIN32
  EXPECT_EQ("C:\\a\\b", joinPath("C:\\a", "b"));
  EXPECT_EQ("C:/a/b", joinPath("C:/a/", "b"));
  EXPECT_EQ("D:\\x", joinPath("C:\\a", "D:\\x"));
  EXPECT_EQ("C:\\a", parentPath("C:\\a\\b.mb88"));
  EXPECT_EQ("C:\\", parentPath("C:\\b.mb88"));
  EXPECT_EQ("b.mb88", fileName("C:/a\\b.mb88"));
  EXPECT_TRUE(isAbsolutePath("C:\\x"));
  EXPECT_TRUE(isAbsolutePath("c:/x"));
  EXPECT_TRUE(isAbsolutePath("\\\\server\\share"));
  EXPECT_EQ("C:/a/b", toGenericSeparators("C:\\a\\b"));
  EXPECT_EQ("C:\\a\\b", toNativeSeparators("C:/a/b"));
  EXPECT_EQ(';', pathListSeparator());
  EXPECT_STREQ(".exe", executableSuffix());
#else
  EXPECT_EQ("/a/b", joinPath("/a", "b"));
  EXPECT_EQ("/a", parentPath("/a/b.mb88"));
  EXPECT_EQ("/", parentPath("/b.mb88"));
  EXPECT_EQ("b.mb88", fileName("/a/b.mb88"));
  EXPECT_EQ(':', pathListSeparator());
  EXPECT_STREQ("", executableSuffix());
#endif
  EXPECT_FALSE(isAbsolutePath("relative/file"));
  EXPECT_EQ(std::vector<std::string>({"a", "b"}),
            splitPathList(std::string("a") + pathListSeparator() + pathListSeparator() + "b"));
}

TEST(Paths, ExecutableAndInstallLocations) {
  const std::string exe = executablePath();
  ASSERT_FALSE(exe.empty());
  EXPECT_TRUE(fileExists(exe));
  EXPECT_TRUE(isDirectory(executableDirectory()));
  EXPECT_EQ(parentPath(executableDirectory()), installPrefix());
  EXPECT_FALSE(userConfigDirectory().empty());
#ifdef _WIN32
  EXPECT_NE(std::string::npos, userConfigDirectory().find("MB-Studio"));
#endif
}

TEST(Paths, EnvironmentRoundTripsUnicode) {
  const std::string value = "C:\\\xE6\xB5\x8B\xE7\xBA\xBF\\data";  // contains Chinese characters
  ASSERT_TRUE(setEnv("MBSTUDIO_TEST_UNICODE", value));
  EXPECT_EQ(value, getEnv("MBSTUDIO_TEST_UNICODE"));
}

TEST(Paths, UnicodeDirectories) {
  const std::string base = joinPath(executableDirectory(), "mbstudio-\xE6\xB5\x8B\xE8\xAF\x95 dir");
  ASSERT_TRUE(makeDirectories(joinPath(base, "sub")));
  EXPECT_TRUE(isDirectory(base));
  const auto entries = listDirectory(base);
  ASSERT_EQ(1u, entries.size());
  EXPECT_EQ("sub", entries[0]);
}

TEST(Process, ArgumentsArriveIntact) {
  const std::vector<std::string> args = {"plain", "with space", "C:\\path\\", "quote\"inside", "",
                                         "\xE6\xB5\x8B\xE7\xBA\xBF"};
  std::string output;
  ProcessSpec spec;
  spec.program = childProgram();
  spec.arguments = args;
  const ProcessResult r = runProcess(spec, [&](const std::string &chunk) { output += chunk; });
  ASSERT_TRUE(r.started) << r.error;
  EXPECT_EQ(0, r.exitCode);
  // stderr is unbuffered, so only the order of the argument lines is fixed
  std::vector<std::string> lines;
  for (const auto &line : outputLines(output))
    if (!line.empty() && line[0] == '<')
      lines.push_back(line);
  ASSERT_EQ(args.size(), lines.size()) << output;
  for (size_t i = 0; i < args.size(); i++)
    EXPECT_EQ("<" + args[i] + ">", lines[i]);
}

TEST(Process, ExitCodeEnvironmentWorkingDirectoryAndStderr) {
  std::string output;
  ProcessSpec spec;
  spec.program = childProgram();
  spec.arguments = {"--exit=7"};
  spec.environment = {{"MBSTUDIO_TEST_VAR", "value with space"}};
  spec.workingDirectory = installPrefix();
  const ProcessResult r = runProcess(spec, [&](const std::string &chunk) { output += chunk; });
  ASSERT_TRUE(r.started) << r.error;
  EXPECT_EQ(7, r.exitCode);
  EXPECT_NE(std::string::npos, output.find("var=value with space"));
  EXPECT_NE(std::string::npos, output.find("stderr line"));
  const std::string cwdLine = "cwd=" + installPrefix();
  EXPECT_NE(std::string::npos, toGenericSeparators(output).find(toGenericSeparators(cwdLine))) << output;
}

TEST(Process, CancellationTerminatesTheChild) {
  std::atomic<bool> cancel{false};
  std::thread canceller([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cancel = true;
  });
  ProcessSpec spec;
  spec.program = childProgram();
  spec.arguments = {"--sleep"};
  const auto start = std::chrono::steady_clock::now();
  const ProcessResult r = runProcess(spec, nullptr, &cancel);
  canceller.join();
  EXPECT_TRUE(r.started);
  EXPECT_TRUE(r.cancelled);
  EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(20));
}

TEST(Process, MissingProgramIsReported) {
  ProcessSpec spec;
  spec.program = joinPath(executableDirectory(), "no-such-program-mbstudio");
  const ProcessResult r = runProcess(spec, nullptr);
  EXPECT_FALSE(r.started && r.exitCode == 0);
  EXPECT_FALSE(r.error.empty());
}

// Transverse Mercator (UTM) against reference values: the WGS 84 meridian
// arc is 3 320 113.398 m at 30° and 4 984 944.378 m at 45° (GeographicLib).
TEST(Geodesy, UtmOnTheCentralMeridian) {
  const MapProjection north = MapProjection::parse("UTM13N");
  ASSERT_TRUE(north.valid());
  EXPECT_FALSE(north.geographic());
  double x = 0.0, y = 0.0;
  ASSERT_TRUE(north.forward(-105.0, 45.0, x, y));
  EXPECT_NEAR(500000.0, x, 0.01);
  EXPECT_NEAR(0.9996 * 4984944.378, y, 0.5);
  const MapProjection south = MapProjection::parse("UTM33S");
  ASSERT_TRUE(south.forward(15.0, -30.0, x, y));
  EXPECT_NEAR(500000.0, x, 0.01);
  EXPECT_NEAR(10000000.0 - 0.9996 * 3320113.398, y, 0.5);
  EXPECT_EQ("UTM zone 33 S", south.name());
}

TEST(Geodesy, RoundTripsWithinAZone) {
  const MapProjection utm = MapProjection::parse("EPSG:32610");  // UTM 10 N
  ASSERT_TRUE(utm.valid());
  for (const double lon : {-125.9, -124.5, -123.0, -120.1})
    for (const double lat : {0.5, 36.8, 40.84, 71.0}) {
      double x = 0.0, y = 0.0, lon2 = 0.0, lat2 = 0.0;
      ASSERT_TRUE(utm.forward(lon, lat, x, y));
      ASSERT_TRUE(utm.inverse(x, y, lon2, lat2));
      EXPECT_NEAR(lon, lon2, 2e-8) << lon << " " << lat;  // about 2 mm
      EXPECT_NEAR(lat, lat2, 2e-8) << lon << " " << lat;
    }
  // a local transverse Mercator has its origin at the reference point
  const MapProjection ltm = MapProjection::parse("LTM-122.5/36.8");
  ASSERT_TRUE(ltm.valid());
  double x = 1.0, y = 1.0;
  ASSERT_TRUE(ltm.forward(-122.5, 36.8, x, y));
  EXPECT_NEAR(0.0, x, 1e-6);
  EXPECT_NEAR(0.0, y, 1e-6);
}

TEST(Geodesy, NamesAndDistances) {
  EXPECT_TRUE(MapProjection::parse("").geographic());
  EXPECT_TRUE(MapProjection::parse("EPSG:4326").geographic());
  EXPECT_TRUE(MapProjection::parse("Geographic").geographic());
  EXPECT_TRUE(MapProjection::parse("UTM10").valid());  // north by default
  EXPECT_FALSE(MapProjection::parse("UTM99N").valid());
  EXPECT_FALSE(MapProjection::parse("EPSG:3857").valid());
  EXPECT_FALSE(MapProjection::parse("nonsense").valid());
  EXPECT_EQ(10, MapProjection::utmZone(-124.5));
  EXPECT_EQ(31, MapProjection::utmZone(0.5));
  EXPECT_EQ(60, MapProjection::utmZone(179.9));
  // one degree of latitude on the mean sphere
  EXPECT_NEAR(111195.08, geodesicDistance(10.0, 0.0, 10.0, 1.0), 0.5);
  double x = 0.0, y = 0.0;
  EXPECT_FALSE(MapProjection::parse("UTM10N").forward(60.0, 40.0, x, y));  // far outside the zone
}
