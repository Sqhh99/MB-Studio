// Tests of the mbsystem plugin services against real MB-System data and
// programs (formats, swath inspection, grids, rendering, tools) and of the
// plugin through the kernel command interface.
#include <atomic>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <netcdf.h>

#include "MbSystemPlugin.h"
#include "MbioServices.h"
#include "mbstudio/kernel/Kernel.h"
#include "mbstudio/platform/Paths.h"

using namespace mbstudio;
using namespace mbstudio::mbsystem;

namespace {

std::string testData() {
  return platform::joinPath(MBSTUDIO_SOURCE_DIR, "test/utilities/testdata/mb21/TN136HS.309.snipped.mb21");
}

std::string scratch(const std::string &name) {
  // Next to the test program, in native path syntax (determined at run time).
  const std::string dir = platform::joinPath(platform::executableDirectory(), "mbstudio-test-scratch");
  platform::makeDirectories(dir);
  return platform::joinPath(dir, name);
}

std::vector<std::string> toolDirs() { return {MBSTUDIO_TOOL_DIR}; }

// Write a small COARDS grid with y ascending (as GMT does) and one NaN node.
std::string writeTestGrid() {
  const std::string path = scratch("coards.grd");
  int ncid, xdim, ydim, xvar, yvar, zvar;
  EXPECT_EQ(NC_NOERR, nc_create(path.c_str(), NC_CLOBBER, &ncid));
  nc_def_dim(ncid, "lon", 4, &xdim);
  nc_def_dim(ncid, "lat", 3, &ydim);
  nc_def_var(ncid, "lon", NC_DOUBLE, 1, &xdim, &xvar);
  nc_def_var(ncid, "lat", NC_DOUBLE, 1, &ydim, &yvar);
  int dims[2] = {ydim, xdim};
  nc_def_var(ncid, "z", NC_FLOAT, 2, dims, &zvar);
  nc_put_att_text(ncid, xvar, "units", 12, "degrees_east");
  nc_put_att_text(ncid, zvar, "units", 6, "meters");
  const float fill = -9999.0f;
  nc_put_att_float(ncid, zvar, "_FillValue", NC_FLOAT, 1, &fill);
  nc_put_att_text(ncid, NC_GLOBAL, "title", 4, "test");
  nc_enddef(ncid);
  const double lon[4] = {10.0, 10.5, 11.0, 11.5};
  const double lat[3] = {50.0, 50.5, 51.0};
  // row 0 = lat 50 (south)
  const float z[12] = {-100, -110, -120, -130, -200, -210, -9999, -230, -300, -310, -320, -330};
  nc_put_var_double(ncid, xvar, lon);
  nc_put_var_double(ncid, yvar, lat);
  nc_put_var_float(ncid, zvar, z);
  nc_close(ncid);
  return path;
}

}  // namespace

TEST(FormatService, ListsAndDescribesFormats) {
  FormatService formats;
  const auto list = formats.formats();
  EXPECT_GT(list.size(), 50u);
  FormatInfo info;
  ASSERT_TRUE(formats.format(21, info));
  EXPECT_EQ("MBF_HSATLRAW", info.name);
  EXPECT_EQ("Raw Hydrosweep", info.description);
  EXPECT_NE(std::string::npos, info.attributes.find("Atlas Electronik"));  // continuation line
  EXPECT_FALSE(formats.format(99999, info));
}

TEST(FormatService, ParsesDescriptions) {
  const FormatInfo f = FormatService::parseDescription(
      88, "Format name:          MBF_RESON7KR\nInformal Description: Reson 7K raw\nAttributes:           a,\n"
          "                      b.\n");
  EXPECT_EQ("MBF_RESON7KR", f.name);
  EXPECT_EQ("Reson 7K raw", f.description);
  EXPECT_EQ("a, b.", f.attributes);
}

TEST(FormatService, IdentifiesFiles) {
  FormatService formats;
  EXPECT_EQ(21, formats.identify(testData()));
  EXPECT_EQ(88, formats.identify("C:\\Survey Data\\line 1.mb88"));
  EXPECT_EQ(0, formats.identify("notes.txt"));
}

TEST(SwathService, InspectsSampleFile) {
  auto formats = std::make_shared<FormatService>();
  SwathService swath(formats);
  const SwathSummary s = swath.inspect(testData(), 0, 100, nullptr);
  ASSERT_TRUE(s.ok) << s.error;
  EXPECT_EQ(21, s.format);
  EXPECT_EQ("MBF_HSATLRAW", s.formatName);
  EXPECT_EQ(2, s.records);
  EXPECT_EQ(59, s.maxBeams);
  EXPECT_EQ(118, s.beamsTotal);
  EXPECT_EQ(115, s.beamsGood);
  EXPECT_EQ("2001-11-05T00:01:44.000Z", s.timeStartIso);
  EXPECT_EQ("2001-11-05T00:01:49.000Z", s.timeEndIso);
  EXPECT_NEAR(-124.5016021, s.lonMin, 1e-3);
  EXPECT_NEAR(40.8385, s.latMin, 1e-3);
  EXPECT_NEAR(448.2, s.depthMin, 0.05);
  EXPECT_NEAR(468.3, s.depthMax, 0.05);
  EXPECT_NEAR(0.0036, s.trackLengthKm, 0.0005);
  EXPECT_EQ(2u, s.track.size());
}

TEST(SwathService, ReportsErrors) {
  SwathService swath(std::make_shared<FormatService>());
  EXPECT_FALSE(swath.inspect("unknown.xyz", 0, 10, nullptr).ok);
  const SwathSummary s = swath.inspect(scratch("missing.mb21"), 0, 10, nullptr);
  EXPECT_FALSE(s.ok);
  EXPECT_FALSE(s.error.empty());
}

TEST(GridService, LoadsCoardsGridNorthUp) {
  GridService grids;
  const GridLoadResult r = grids.load(writeTestGrid());
  ASSERT_TRUE(r.ok) << r.error;
  const Grid &g = r.grid;
  EXPECT_EQ(4, g.nx);
  EXPECT_EQ(3, g.ny);
  EXPECT_TRUE(g.geographic);
  EXPECT_DOUBLE_EQ(10.0, g.xmin);
  EXPECT_DOUBLE_EQ(51.0, g.ymax);
  EXPECT_EQ("meters", g.zUnits);
  EXPECT_FLOAT_EQ(-300.0f, g.at(0, 0));  // north-west corner
  EXPECT_FLOAT_EQ(-130.0f, g.at(3, 2));  // south-east corner
  EXPECT_TRUE(std::isnan(g.at(2, 1)));   // _FillValue
  EXPECT_DOUBLE_EQ(-330.0, g.zmin);
  EXPECT_DOUBLE_EQ(-100.0, g.zmax);
  EXPECT_DOUBLE_EQ(51.0, g.yOf(0));
}

TEST(GridService, RejectsNonGrids) {
  GridService grids;
  EXPECT_FALSE(grids.load(testData()).ok);
  EXPECT_FALSE(grids.load(scratch("missing.grd")).ok);
}

TEST(GridRenderService, ColorMapsAndShading) {
  GridRenderService render;
  EXPECT_EQ(4u, render.colorMapNames().size());
  EXPECT_EQ(0xFF0A0079u, render.color(ColorMap::Haxby, 0.0));
  EXPECT_EQ(0xFFFFBD57u, render.color(ColorMap::Haxby, 1.0));
  EXPECT_EQ(0xFF0000FFu, render.color(ColorMap::Rainbow, 0.0));
  EXPECT_EQ(0xFFFF0000u, render.color(ColorMap::Rainbow, 1.0));

  GridService grids;
  const Grid g = grids.load(writeTestGrid()).grid;
  RenderOptions options;
  options.hillshade = false;
  const Image flat = render.render(g, options);
  ASSERT_EQ(4, flat.width);
  ASSERT_EQ(3, flat.height);
  EXPECT_EQ(render.color(ColorMap::Haxby, 0.0), flat.pixels[3 + 0 * 4]);  // -330 is the minimum
  EXPECT_EQ(render.color(ColorMap::Haxby, 1.0), flat.pixels[0 + 2 * 4]);  // -100 is the maximum
  EXPECT_EQ(0u, flat.pixels[2 + 1 * 4]);                                  // no data is transparent

  options.hillshade = true;
  const Image shaded = render.render(g, options);
  EXPECT_EQ(flat.pixels.size(), shaded.pixels.size());
  EXPECT_NE(flat.pixels, shaded.pixels);
  for (size_t i = 0; i < shaded.pixels.size(); i++)
    EXPECT_EQ(flat.pixels[i] >> 24, shaded.pixels[i] >> 24);  // shading keeps alpha
}

TEST(ToolService, CatalogIsCompleteAndInstalled) {
  ToolService tools(toolDirs(), nullptr);
  const auto list = tools.tools();
  EXPECT_GE(list.size(), 45u);
  std::set<std::string> names;
  for (const auto &t : list) {
    EXPECT_TRUE(names.insert(t.name).second) << "duplicate " << t.name;
    EXPECT_FALSE(t.summary.empty()) << t.name;
    if (t.name != "gmt")  // GMT is installed next to the programs, not built here
      EXPECT_TRUE(t.available) << t.name << " was not found in the build tree";
  }
  EXPECT_FALSE(tools.categories().empty());
}

TEST(ToolService, RunsProgramsAndPublishesJobEvents) {
  EventBus events;
  std::vector<std::string> topics;
  events.subscribe("job.*", [&](const Event &e) { topics.push_back(e.topic); });
  ToolService tools(toolDirs(), &events);
  std::string output;
  const auto r = tools.run("mbinfo", {"-I" + testData()}, std::string(),
                           [&](const std::string &chunk) { output += chunk; });
  ASSERT_TRUE(r.started) << r.error;
  EXPECT_EQ(0, r.exitCode);
  EXPECT_NE(std::string::npos, output.find("MBIO Data Format ID:  21"));
  ASSERT_GE(topics.size(), 3u);
  EXPECT_EQ("job.started", topics.front());
  EXPECT_EQ("job.finished", topics.back());
  EXPECT_FALSE(tools.run("not-a-tool", {}, std::string(), nullptr).started);
}

TEST(ToolService, GridsSwathDataWithMbgrid) {
  // End to end: datalist -> mbgrid -> grid service -> renderer.
  const std::string datalist = scratch("datalist.mb-1");
  FILE *fp = std::fopen(datalist.c_str(), "w");
  ASSERT_NE(nullptr, fp);
  std::fprintf(fp, "%s 21\n", testData().c_str());
  std::fclose(fp);
  ToolService tools(toolDirs(), nullptr);
  std::string output;
  const auto r = tools.run("mbgrid",
                           {"-I" + datalist, "-O" + scratch("bathy"), "-E20/0/meters!", "-A2", "-N",
                            "-R-124.507/-124.496/40.835/40.842"},
                           std::string(), [&](const std::string &chunk) { output += chunk; });
  ASSERT_TRUE(r.started) << r.error;
  ASSERT_EQ(0, r.exitCode) << output;
  GridService grids;
  const GridLoadResult g = grids.load(scratch("bathy.grd"));
  ASSERT_TRUE(g.ok) << g.error << "\n" << output;
  EXPECT_EQ(47, g.grid.nx);
  EXPECT_EQ(31, g.grid.ny);
  EXPECT_TRUE(g.grid.geographic);
  EXPECT_LT(g.grid.zmin, g.grid.zmax);
  EXPECT_GT(g.grid.zmin, -470.0);  // mbgrid writes topography (negative depth)
  GridRenderService render;
  EXPECT_EQ(47 * 31, static_cast<int>(render.render(g.grid, RenderOptions()).pixels.size()));
}

TEST(HelpService, EveryProgramHasPagesInAllLanguages) {
  HelpService help(std::vector<std::string>{MBSTUDIO_HELP_DIR});
  ASSERT_EQ(MBSTUDIO_HELP_DIR, help.directory());
  const auto languages = help.languages();
  ASSERT_EQ((std::vector<std::string>{"en", "ja", "zh_CN"}), languages);
  ToolService tools(toolDirs(), nullptr);
  for (const auto &tool : tools.tools()) {
    for (const auto &lang : languages) {
      const HelpPage page = help.page(tool.name, lang);
      EXPECT_TRUE(page.found) << tool.name << " " << lang;
      EXPECT_EQ(lang, page.language) << tool.name;
      EXPECT_FALSE(page.summary.empty()) << tool.name << " " << lang;
      EXPECT_EQ(0u, page.markdown.find("# " + tool.name + " ")) << tool.name << " " << lang;
    }
  }
}

TEST(HelpService, FallsBackToRelatedLanguagesAndEnglish) {
  HelpService help(std::vector<std::string>{"does-not-exist", MBSTUDIO_HELP_DIR});
  EXPECT_EQ(MBSTUDIO_HELP_DIR, help.directory());
  EXPECT_EQ("zh_CN", help.page("mbinfo", "zh").language);
  EXPECT_EQ("zh_CN", help.page("mbinfo", "zh_TW").language);
  EXPECT_EQ("ja", help.page("mbinfo", "ja_JP").language);
  EXPECT_EQ("en", help.page("mbinfo", "fr").language);
  EXPECT_EQ("en", help.page("mbinfo", "").language);
  EXPECT_FALSE(help.page("no-such-program", "en").found);
  EXPECT_FALSE(help.page("../en/mbinfo", "en").found);  // no path components

  HelpService none(std::vector<std::string>{"does-not-exist"});
  EXPECT_TRUE(none.programs().empty());
  EXPECT_FALSE(none.page("mbinfo", "en").found);
}

TEST(MbSystemPlugin, CommandsThroughTheKernel) {
  KernelConfig config;
  config.toolDirectories = toolDirs();
  Kernel kernel(config);
  kernel.plugins().addStatic([] { return std::make_unique<MbSystemPlugin>(); });
  ASSERT_EQ(1u, kernel.start());
  EXPECT_TRUE(kernel.services().has<ISwathService>());
  EXPECT_TRUE(kernel.services().has<IToolService>());

  auto r = kernel.commands().execute("swath.inspect", {{"path", testData()}});
  ASSERT_TRUE(r.ok) << r.message;
  EXPECT_EQ(2, getInt(r.data, "records"));
  r = kernel.commands().execute("formats.identify", {{"path", std::string("x.mb88")}});
  EXPECT_EQ(88, getInt(r.data, "format"));
  r = kernel.commands().execute("tools.run", {{"tool", std::string("mbformat")}, {"args", std::string("-F21")}});
  EXPECT_TRUE(r.ok) << r.message;
  EXPECT_NE(std::string::npos, getString(r.data, "output").find("MBF_HSATLRAW"));
  r = kernel.commands().execute("grid.info", {{"path", writeTestGrid()}});
  EXPECT_EQ(4, getInt(r.data, "nx"));
  ASSERT_TRUE(kernel.services().has<IHelpService>());
  r = kernel.commands().execute("help.show", {{"name", std::string("mbgrid")}, {"lang", std::string("ja")}});
  ASSERT_TRUE(r.ok) << r.message;
  EXPECT_EQ("ja", getString(r.data, "language"));
  EXPECT_EQ(0u, getString(r.data, "markdown").find("# mbgrid "));
  r = kernel.commands().execute("help.programs");
  EXPECT_TRUE(r.ok) << r.message;
  EXPECT_FALSE(getString(r.data, "program.mbinfo").empty());
  EXPECT_FALSE(kernel.commands().execute("help.show", {{"name", std::string("nothing")}}).ok);
  kernel.stop();
  EXPECT_FALSE(kernel.services().has<ISwathService>());
}
