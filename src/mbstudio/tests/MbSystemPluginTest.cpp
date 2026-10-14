// Tests of the mbsystem plugin services against real MB-System data and
// programs (formats, swath inspection, grids, rendering, tools) and of the
// plugin through the kernel command interface.
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <netcdf.h>

#include "MbSystemPlugin.h"
#include "MbioServices.h"
#include "mbstudio/kernel/Kernel.h"
#include "mbstudio/platform/Geodesy.h"
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

// The same two pings in MB-System's own processing format (writable by mbprocess).
std::string testData71() {
  return platform::joinPath(MBSTUDIO_SOURCE_DIR, "test/utilities/testdata/mb71/TN136HS.309.snipped.mb71");
}

bool copyFile(const std::string &from, const std::string &to) {
  std::string data;
  return platform::readTextFile(from, data) && platform::writeTextFile(to, data);
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

TEST(ToolService, RunsProgramsAndPublishesToolEvents) {
  EventBus events;
  std::vector<std::string> topics;
  events.subscribe("tool.*", [&](const Event &e) { topics.push_back(e.topic); });
  ToolService tools(toolDirs(), &events);
  std::string output;
  const auto r = tools.run("mbinfo", {"-I" + testData()}, std::string(),
                           [&](const std::string &chunk) { output += chunk; });
  ASSERT_TRUE(r.started) << r.error;
  EXPECT_EQ(0, r.exitCode);
  EXPECT_NE(std::string::npos, output.find("MBIO Data Format ID:  21"));
  ASSERT_GE(topics.size(), 3u);
  EXPECT_EQ("tool.started", topics.front());
  EXPECT_EQ("tool.finished", topics.back());
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

  // the same data in UTM: the grid records its projection
  output.clear();
  const auto u = tools.run("mbgrid",
                           {"-I" + datalist, "-O" + scratch("bathy-utm"), "-E20/0/meters!", "-A2", "-N", "-JU",
                            "-R-124.507/-124.496/40.835/40.842"},
                           std::string(), [&](const std::string &chunk) { output += chunk; });
  ASSERT_TRUE(u.started) << u.error;
  ASSERT_EQ(0, u.exitCode) << output;
  const GridLoadResult projected = grids.load(scratch("bathy-utm.grd"));
  ASSERT_TRUE(projected.ok) << projected.error;
  EXPECT_FALSE(projected.grid.geographic);
  EXPECT_EQ("UTM10N", projected.grid.projection);
  // its corner lies where the geographic grid is
  const platform::MapProjection utm = platform::MapProjection::parse(projected.grid.projection);
  ASSERT_TRUE(utm.valid());
  double lon = 0.0, lat = 0.0;
  ASSERT_TRUE(utm.inverse(projected.grid.xmin, projected.grid.ymin, lon, lat));
  EXPECT_NEAR(-124.507, lon, 0.01);
  EXPECT_NEAR(40.835, lat, 0.01);
}

TEST(ProcessingService, GridArgumentsAreExplicit) {
  ProcessingService processing;
  GridParameters p;
  p.outputRoot = "/data/survey/products/bathy";
  p.spacing = 10;
  p.algorithm = 2;
  p.clip = 3;
  p.clipMode = 1;
  p.projection = "U";
  const CommandPlan plan = processing.grid("/data/survey/datalist.mb-1", p);
  ASSERT_TRUE(plan.ok) << plan.error;
  ASSERT_EQ(1u, plan.steps.size());
  EXPECT_EQ("mbgrid", plan.steps[0].tool);
  EXPECT_EQ((std::vector<std::string>{"-I/data/survey/datalist.mb-1", "-O/data/survey/products/bathy", "-A1",
                                      "-E10/10/meters!", "-F2", "-C3/1", "-JU", "-M", "-N", "-V"}),
            plan.steps[0].arguments);
  EXPECT_EQ((std::vector<std::string>{"/data/survey/products/bathy.grd", "/data/survey/products/bathy_num.grd",
                                      "/data/survey/products/bathy_sd.grd"}),
            plan.outputs);
  EXPECT_FALSE(plan.warnings.empty());  // interpolation is not measured data

  // minimum filter: no quality grids, threshold appended
  p.algorithm = 3;
  p.threshold = 0.5;
  p.clip = 0;
  p.projection.clear();
  p.bounds = GeoBounds{-124.507, -124.496, 40.835, 40.842};
  const CommandPlan min = processing.grid("dl.mb-1", p);
  ASSERT_TRUE(min.ok);
  EXPECT_EQ((std::vector<std::string>{"-Idl.mb-1", "-O/data/survey/products/bathy", "-A1", "-E10/10/meters!",
                                      "-R-124.507/-124.496/40.835/40.842", "-F3/0.5", "-N", "-V"}),
            min.steps[0].arguments);
  EXPECT_EQ(1u, min.outputs.size());

  p.algorithm = 5;
  p.dataType = 4;
  EXPECT_FALSE(processing.grid("dl.mb-1", p).ok);  // footprint gridding of sidescan
  p.outputRoot.clear();
  EXPECT_FALSE(processing.grid("dl.mb-1", p).ok);

  GridParameters e;
  e.spacing = 100;
  const GridEstimate est = processing.estimateGrid(e, GeoBounds{0.0, 1.0, 0.0, 1.0});
  EXPECT_NEAR(1115, est.nx, 3);
  EXPECT_NEAR(1115, est.ny, 3);
  EXPECT_GT(est.megabytes, 1.0);
}

TEST(ProcessingService, CleaningCorrectionsAndExports) {
  ProcessingService processing;
  CleanParameters c;
  c.outerBeams = 5;
  c.useDepthRange = true;
  c.depthMin = 100;
  c.depthMax = 6000;
  c.areaClean = true;
  CommandPlan plan = processing.clean("dl.mb-1", c);
  ASSERT_TRUE(plan.ok) << plan.error;
  ASSERT_EQ(2u, plan.steps.size());
  EXPECT_EQ((std::vector<std::string>{"-Idl.mb-1", "-M1", "-C1/0", "-B100/6000", "-X5", "-V"}),
            plan.steps[0].arguments);
  EXPECT_EQ("mbareaclean", plan.steps[1].tool);
  c = CleanParameters();
  c.useSlope = false;
  EXPECT_FALSE(processing.clean("dl.mb-1", c).ok);

  CorrectionParameters k;
  k.svpMode = 2;
  k.longitude = -124.4;
  k.latitude = 40.8;
  k.useRollBias = true;
  k.rollBias = -0.25;
  k.useDraft = true;
  k.draft = 5.5;
  k.workingDirectory = "/w";
  plan = processing.corrections("/d/dl.mb-1", k);
  ASSERT_TRUE(plan.ok) << plan.error;
  ASSERT_EQ(2u, plan.steps.size());
  EXPECT_EQ("mblevitus", plan.steps[0].tool);
  EXPECT_EQ("-R-124/41", plan.steps[0].arguments[0]);
  EXPECT_EQ("mbset", plan.steps[1].tool);
  EXPECT_EQ((std::vector<std::string>{"-I/d/dl.mb-1", "-PSVPFILE:" + platform::joinPath("/w", "levitus.svp"),
                                      "-PROLLBIAS:-0.25", "-PDRAFT:5.5", "-V"}),
            plan.steps[1].arguments);
  EXPECT_FALSE(processing.corrections("dl.mb-1", CorrectionParameters()).ok);

  ProcessParameters pp;
  pp.threads = 4;
  pp.processedDatalist = "dlp.mb-1";
  plan = processing.process("dl.mb-1", pp);
  ASSERT_EQ(2u, plan.steps.size());
  EXPECT_EQ((std::vector<std::string>{"-Idl.mb-1", "-C4"}), plan.steps[0].arguments);
  EXPECT_TRUE(plan.steps[1].allowFailure);

  ExportParameters x;
  x.kind = "geotiff";
  x.output = "/o/bathy.tif";
  plan = processing.exportGrid("/o/bathy.grd", x);
  ASSERT_TRUE(plan.ok);
  EXPECT_EQ((std::vector<std::string>{"grdconvert", "/o/bathy.grd", "/o/bathy.tif=gd:GTiff"}), plan.steps[0].arguments);
  x.kind = "las";
  x.output = "/o/points.las";
  plan = processing.exportSoundings("dlp.mb-1", x);
  ASSERT_TRUE(plan.ok);
  EXPECT_EQ("mbswath2las", plan.steps[0].tool);
  x.kind = "pdf";
  EXPECT_FALSE(processing.exportGrid("g.grd", x).ok);

  MosaicParameters m;
  m.outputRoot = "/o/ss";
  m.prioritySource = 3;
  m.angleCorrection = true;
  m.processedDatalist = "dlp.mb-1";
  plan = processing.mosaic("dl.mb-1", m);
  ASSERT_TRUE(plan.ok) << plan.error;
  ASSERT_EQ(3u, plan.steps.size());
  EXPECT_EQ("mbbackangle", plan.steps[0].tool);
  EXPECT_EQ("mbmosaic", plan.steps[2].tool);
  EXPECT_EQ("-Idlp.mb-1", plan.steps[2].arguments[0]);
}

TEST(ProcessingService, DatalistsAndFileNames) {
  ProcessingService processing;
  const std::string base = scratch("project");
  platform::makeDirectories(platform::joinPath(base, "datalists"));
  std::string text, error;
  const std::vector<DatalistEntry> entries = {{platform::joinPath(base, "raw/0001_line.all"), 58, 1.0},
                                              {platform::joinPath(base, "raw/0002_line.all"), 58, 1.0}};
  ASSERT_TRUE(processing.datalistText(entries, platform::joinPath(base, "datalists"), false, text, error)) << error;
  EXPECT_NE(std::string::npos, text.find("../raw/0001_line.all 58 1\n"));
  ASSERT_TRUE(processing.datalistText(entries, platform::joinPath(base, "datalists"), true, text, error));
  EXPECT_NE(std::string::npos, text.find("$PROCESSED\n"));
  EXPECT_FALSE(processing.datalistText({{"/elsewhere/with space/a.all", 58, 1.0}}, base, false, text, error));
  EXPECT_NE(std::string::npos, error.find("spaces"));
  EXPECT_FALSE(processing.datalistText({{"/x/a.xyz", 0, 1.0}}, base, false, text, error));
  EXPECT_NE(std::string::npos, processing.processedDatalistText("datalist.mb-1").find("datalist.mb-1 -1 1"));

  EXPECT_EQ(59, processing.preprocessOutputFormat(58));
  EXPECT_EQ(21, processing.preprocessOutputFormat(21));
  EXPECT_TRUE(processing.recommendPreprocess(58));
  EXPECT_FALSE(processing.recommendPreprocess(21));
  EXPECT_EQ("0000_20111219_154640_METEOR_EM122.mb59",
            processing.preprocessOutputName("D:/s/0000_20111219_154640_METEOR_EM122.all", 58));
  EXPECT_EQ("liner.mb59", processing.preprocessOutputName("/s/line.mb59", 59));
  EXPECT_EQ("/s/linep.mb59", processing.processedName("/s/line.mb59"));
  CommandPlan pre = processing.preprocess("dl.mb-1", PreprocessParameters());
  EXPECT_FALSE(pre.ok);
  PreprocessParameters pp;
  pp.outputDirectory = "/p/processed";
  pre = processing.preprocess("dl.mb-1", pp);
  ASSERT_TRUE(pre.ok) << pre.error;
  EXPECT_EQ("--output-directory=/p/processed", pre.steps[0].arguments[1]);
  pp.outputDirectory = "/p/with space";
  EXPECT_FALSE(processing.preprocess("dl.mb-1", pp).ok);
}

TEST(SwathService, ParsesFastNavigation) {
  const std::string fnv =
      "2011 12 19 15 46 40.123000\t1324309600.123000\t-25.1234567890\t37.0000000000\t90.0\t10.0\t5.0\n"
      "2011 12 19 15 46 41.123000\t1324309601.123000\t-25.1230000000\t37.0010000000\t90.0\t10.0\t5.0\n"
      "garbage line\n"
      "2011 12 19 15 46 42.123000\t1324309602.123000\t0.0\t0.0\t90.0\t10.0\t5.0\n"
      "2011 12 19 15 46 43.123000\t1324309603.123000\t-25.1220000000\t37.0020000000\t90.0\t10.0\t5.0\n";
  auto track = SwathService::parseFnv(fnv, 100);
  ASSERT_EQ(3u, track.size());
  EXPECT_NEAR(-25.1234567890, track[0].lon, 1e-9);
  EXPECT_NEAR(37.002, track[2].lat, 1e-9);
  EXPECT_NEAR(1324309600.123, track[0].time, 1e-3);
  EXPECT_EQ(2u, SwathService::parseFnv(fnv, 2).size());
  SwathService swath(nullptr);
  EXPECT_TRUE(swath.readNavigation(scratch("missing.fnv"), 10).empty());
}

TEST(GridService, Statistics) {
  GridService grids;
  const GridLoadResult r = grids.load(writeTestGrid());
  ASSERT_TRUE(r.ok) << r.error;
  const GridStatistics st = grids.statistics(r.grid);
  EXPECT_EQ(12, st.nodes);
  EXPECT_EQ(11, st.valid);
  EXPECT_EQ(1, st.empty);
  EXPECT_NEAR(1.0 / 12.0, st.emptyRatio, 1e-9);
}

TEST(JobService, RunsStepsCancelsAndRetries) {
  EventBus events;
  std::vector<std::string> topics;
  std::mutex topicsMutex;
  events.subscribe("job.*", [&](const Event &e) {
    std::lock_guard<std::mutex> lock(topicsMutex);
    topics.push_back(e.topic);
  });
  auto tools = std::make_shared<ToolService>(toolDirs(), &events);
  JobService jobs(tools, &events, 1);
  const std::string dir = scratch("jobs dir");

  // a program step and an action step
  bool actionRan = false;
  JobSpec spec;
  spec.title = "Inspect";
  spec.kind = "test";
  spec.workingDirectory = dir;
  JobStep info;
  info.title = "mbinfo";
  info.tool = "mbinfo";
  info.arguments = {"-I" + testData()};
  spec.steps.push_back(info);
  JobStep action;
  action.title = "action";
  action.action = [&actionRan](std::string &log) {
    actionRan = true;
    log = "action done";
    return true;
  };
  spec.steps.push_back(action);
  std::string error;
  const auto id = jobs.submit(spec, &error);
  ASSERT_NE(0, id) << error;
  ASSERT_TRUE(jobs.wait(id, 60000));
  JobInfo done;
  ASSERT_TRUE(jobs.job(id, done));
  EXPECT_EQ(JobState::Completed, done.state) << done.error;
  EXPECT_TRUE(actionRan);
  EXPECT_EQ(1u, done.commandLines.size());
  EXPECT_NE(std::string::npos, jobs.log(id).find("MBIO Data Format ID:  21"));
  EXPECT_NE(std::string::npos, jobs.log(id).find("action done"));
  EXPECT_TRUE(platform::fileExists(done.logPath));
  {
    std::lock_guard<std::mutex> lock(topicsMutex);
    ASSERT_FALSE(topics.empty());
    EXPECT_EQ("job.queued", topics.front());
    EXPECT_EQ("job.finished", topics.back());
  }

  // a failing program: actionable error with the exit code
  JobSpec bad = spec;
  bad.steps = {info};
  bad.steps[0].arguments = {"-I" + scratch("does-not-exist.mb21")};
  const auto badId = jobs.submit(bad);
  ASSERT_TRUE(jobs.wait(badId, 60000));
  ASSERT_TRUE(jobs.job(badId, done));
  EXPECT_EQ(JobState::Failed, done.state);
  EXPECT_NE(std::string::npos, done.error.find("mbinfo"));
  const auto retried = jobs.retry(badId);
  ASSERT_NE(0, retried);
  ASSERT_TRUE(jobs.wait(retried, 60000));
  ASSERT_TRUE(jobs.job(retried, done));
  EXPECT_EQ(badId, done.retryOf);

  // cancel a queued job while another one blocks the only worker
  std::atomic<bool> release{false};
  JobSpec blocking;
  blocking.workingDirectory = dir;
  JobStep wait;
  wait.title = "wait";
  wait.action = [&release](std::string &) {
    while (!release.load())
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return true;
  };
  blocking.steps = {wait};
  const auto first = jobs.submit(blocking);
  const auto second = jobs.submit(spec);
  EXPECT_TRUE(jobs.cancel(second));
  release = true;
  ASSERT_TRUE(jobs.wait(first, 60000));
  ASSERT_TRUE(jobs.wait(second, 1000));
  ASSERT_TRUE(jobs.job(second, done));
  EXPECT_EQ(JobState::Cancelled, done.state);
  EXPECT_FALSE(jobs.cancel(second));

  EXPECT_EQ(0, jobs.submit(JobSpec(), &error));
  JobSpec unknown;
  JobStep u;
  u.tool = "not-a-program";
  unknown.steps = {u};
  EXPECT_EQ(0, jobs.submit(unknown, &error));
  jobs.clearFinished();
  EXPECT_TRUE(jobs.jobs().empty());
}

namespace {

// A step that records when it ran.
struct Interval {
  std::atomic<double> start{0.0}, end{0.0};
};

double seconds() {
  return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

JobSpec timedJob(Interval &interval, std::vector<std::string> writes, std::vector<std::string> reads = {},
                 int milliseconds = 300) {
  JobSpec spec;
  spec.workingDirectory = scratch("timed jobs");
  spec.writes = std::move(writes);
  spec.reads = std::move(reads);
  JobStep step;
  step.title = "sleep";
  step.action = [&interval, milliseconds](std::string &) {
    interval.start = seconds();
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    interval.end = seconds();
    return true;
  };
  spec.steps = {step};
  return spec;
}

// Records the environment a program was started with.
class EnvironmentTools : public IToolService {
 public:
  mutable std::vector<std::pair<std::string, std::string>> environment;
  std::vector<ToolInfo> tools() const override { return {ToolInfo{"gmt", "Plotting", "", "", "/bin/gmt", true}}; }
  bool tool(const std::string &name, ToolInfo &info) const override {
    info = tools().front();
    return name == "gmt";
  }
  std::vector<std::string> categories() const override { return {"Plotting"}; }
  platform::ProcessResult run(const std::string &, const std::vector<std::string> &, const std::string &,
                              const platform::OutputHandler &, const std::atomic<bool> *) const override {
    platform::ProcessResult r;
    r.started = true;
    r.exitCode = 0;
    return r;
  }
  platform::ProcessResult runWithEnvironment(const std::string &name, const std::vector<std::string> &arguments,
                                             const std::string &cwd,
                                             const std::vector<std::pair<std::string, std::string>> &env,
                                             const platform::OutputHandler &output,
                                             const std::atomic<bool> *cancel) const override {
    environment = env;
    return run(name, arguments, cwd, output, cancel);
  }
};

}  // namespace

// Jobs that write the same survey line or file never run at the same time
// and keep their order; independent jobs run in parallel.
TEST(JobService, SerialisesConflictingJobs) {
  JobService jobs(nullptr, nullptr, 3);
  Interval a, b, c, d;
  const auto ia = jobs.submit(timedJob(a, {"line:L1"}));
  const auto ib = jobs.submit(timedJob(b, {"line:L1"}));             // waits for a
  const auto ic = jobs.submit(timedJob(c, {"line:L2"}));             // independent
  const auto id = jobs.submit(timedJob(d, {}, {"line:L1"}, 50));     // reads what a and b write
  ASSERT_NE(0, ia);
  JobInfo info;
  ASSERT_TRUE(jobs.job(ib, info));
  EXPECT_EQ(ia, info.blockedBy);
  ASSERT_TRUE(jobs.job(id, info));
  EXPECT_NE(0, info.blockedBy);
  for (const auto job : {ia, ib, ic, id})
    ASSERT_TRUE(jobs.wait(job, 30000));
  EXPECT_GE(b.start.load(), a.end.load());   // never at the same time
  EXPECT_GE(d.start.load(), b.end.load());   // in the order they were submitted
  EXPECT_LT(c.start.load(), a.end.load());   // independent work went on
  ASSERT_TRUE(jobs.job(ib, info));
  EXPECT_EQ(JobState::Completed, info.state);
  EXPECT_EQ(std::vector<std::string>{"line:L1"}, info.writes);
  EXPECT_TRUE(jobsConflict(timedJob(a, {"file:x"}), timedJob(b, {}, {"file:x"})));
  EXPECT_FALSE(jobsConflict(timedJob(a, {}, {"file:x"}), timedJob(b, {}, {"file:x"})));  // readers share
}

TEST(JobService, OwnersDecideAboutRetriesAndEnvironment) {
  auto tools = std::make_shared<EnvironmentTools>();
  JobService jobs(tools, nullptr, 1);
  JobSpec spec;
  spec.workingDirectory = scratch("env jobs");
  spec.retryable = false;
  JobStep step;
  step.title = "gmt";
  step.tool = "gmt";
  step.arguments = {"--version"};
  step.environment = {{"GMT_SESSION_NAME", "mbstudio-job-7"}};
  spec.steps = {step};
  const auto id = jobs.submit(spec);
  ASSERT_NE(0, id);
  ASSERT_TRUE(jobs.wait(id, 30000));
  JobInfo info;
  ASSERT_TRUE(jobs.job(id, info));
  EXPECT_EQ(JobState::Completed, info.state) << info.error;
  ASSERT_EQ(1u, tools->environment.size());
  EXPECT_EQ("mbstudio-job-7", tools->environment[0].second);
  EXPECT_FALSE(info.retryable);
  EXPECT_EQ(0, jobs.retry(id));  // the owner repeats it with new outputs
}

TEST(GridService, ReadsTheProjectionOfProjectedGrids) {
  const std::string path = scratch("projected.grd");
  int ncid, xdim, ydim, xvar, yvar, zvar;
  ASSERT_EQ(NC_NOERR, nc_create(path.c_str(), NC_CLOBBER, &ncid));
  nc_def_dim(ncid, "x", 3, &xdim);
  nc_def_dim(ncid, "y", 2, &ydim);
  nc_def_var(ncid, "x", NC_DOUBLE, 1, &xdim, &xvar);
  nc_def_var(ncid, "y", NC_DOUBLE, 1, &ydim, &yvar);
  int dims[2] = {ydim, xdim};
  nc_def_var(ncid, "z", NC_FLOAT, 2, dims, &zvar);
  const std::string remark = "\n\tProjection: UTM10N\n\tGrid created by program mbgrid\n";
  nc_put_att_text(ncid, NC_GLOBAL, "description", remark.size(), remark.c_str());
  nc_enddef(ncid);
  const double x[3] = {500000.0, 500100.0, 500200.0};
  const double y[2] = {4500000.0, 4500100.0};
  const float z[6] = {-10, -11, -12, -13, -14, -15};
  nc_put_var_double(ncid, xvar, x);
  nc_put_var_double(ncid, yvar, y);
  nc_put_var_float(ncid, zvar, z);
  nc_close(ncid);
  GridService grids;
  const GridLoadResult r = grids.load(path);
  ASSERT_TRUE(r.ok) << r.error;
  EXPECT_FALSE(r.grid.geographic);
  EXPECT_EQ("UTM10N", r.grid.projection);
  // a geographic grid has none
  EXPECT_TRUE(grids.load(writeTestGrid()).grid.projection.empty());
}

// Sounding edits: the saved edits of the edit save file are applied when the
// soundings are read, new edits are appended to it, and the mbprocess
// parameter file is set to use it.
TEST(SoundingService, AppliesAndSavesEditEvents) {
  const std::string dir = scratch("soundings");
  platform::makeDirectories(dir);
  const std::string file = platform::joinPath(dir, "line.mb71");
  platform::removeFile(file + ".esf");
  platform::removeFile(file + ".par");
  ASSERT_TRUE(copyFile(testData71(), file));
  SoundingService service(std::make_shared<FormatService>());
  SoundingSet set = service.load(file, 71, 0);
  ASSERT_TRUE(set.ok) << set.error;
  EXPECT_EQ(71, set.format);
  ASSERT_EQ(2u, set.pings.size());
  ASSERT_GT(set.size(), 10u);
  EXPECT_TRUE(set.esfPath.empty());  // nothing saved yet
  EXPECT_EQ(set.size(), set.lon.size());
  EXPECT_EQ(set.size(), set.depth.size());
  // the first good sounding of each ping
  std::vector<BeamEdit> edits;
  std::vector<std::size_t> indices;
  for (const auto &ping : set.pings)
    for (std::uint32_t k = 0; k < ping.beams; k++)
      if (beamflag::isGood(set.flag[ping.first + k])) {
        edits.push_back(BeamEdit{ping.time, ping.multiplicity, static_cast<int>(k), BeamAction::Flag});
        indices.push_back(ping.first + k);
        break;
      }
  ASSERT_EQ(2u, edits.size());
  std::string error, esf;
  ASSERT_TRUE(service.saveEdits(file, 71, edits, error, &esf)) << error;
  EXPECT_NE(std::string::npos, esf.find("line.mb71.esf"));
  EXPECT_TRUE(platform::fileExists(file + ".esf"));
  std::string par;
  ASSERT_TRUE(platform::readTextFile(file + ".par", par));
  EXPECT_NE(std::string::npos, par.find("EDITSAVEMODE"));
  EXPECT_NE(std::string::npos, par.find("EDITSAVEFILE"));

  set = service.load(file, 71, 0);
  ASSERT_TRUE(set.ok) << set.error;
  EXPECT_FALSE(set.esfPath.empty());
  EXPECT_EQ(2, set.savedEdits);
  for (const auto i : indices) {
    EXPECT_TRUE(beamflag::isFlagged(set.flag[i]));
    EXPECT_TRUE(set.flag[i] & beamflag::kManual);
  }
  // a later event overrides an earlier one
  for (auto &e : edits)
    e.action = BeamAction::Unflag;
  ASSERT_TRUE(service.saveEdits(file, 71, edits, error)) << error;
  set = service.load(file, 71, 0);
  for (const auto i : indices)
    EXPECT_TRUE(beamflag::isGood(set.flag[i]));
  // the limit keeps whole pings
  const SoundingSet limited = service.load(file, 71, set.pings[0].beams);
  EXPECT_TRUE(limited.truncated);
  EXPECT_EQ(1u, limited.pings.size());
  EXPECT_FALSE(service.load(scratch("missing.mb71"), 71, 0).ok);
}

TEST(ProcessingService, GmtFigureRecipes) {
  ProcessingService processing;
  FigureParameters p;
  p.grid = "../../products/grids/bathy.grd";
  p.outputRoot = "../../products/figures/bathy";
  p.formats = {"ps"};
  p.contourInterval = 50;
  p.annotationInterval = 250;
  p.title = "Survey";
  p.session = "mbstudio-job-3";
  p.temporaryDirectory = "/survey/jobs/3";
  p.gridBounds = GeoBounds{-124.507, -124.496, 40.835, 40.842};
  CommandPlan plan = processing.figure(p);
  ASSERT_TRUE(plan.ok) << plan.error;
  ASSERT_GE(plan.steps.size(), 5u);
  EXPECT_EQ("gmt", plan.steps.front().tool);
  EXPECT_EQ((std::vector<std::string>{"begin", "../../products/figures/bathy", "ps"}), plan.steps.front().arguments);
  EXPECT_EQ(std::vector<std::string>{"end"}, plan.steps.back().arguments);
  std::string all;
  for (const auto &step : plan.steps) {
    // every call joins the private session of the job
    ASSERT_EQ(2u, step.environment.size());
    EXPECT_EQ("GMT_SESSION_NAME", step.environment[0].first);
    EXPECT_EQ("mbstudio-job-3", step.environment[0].second);
    EXPECT_EQ("GMT_TMPDIR", step.environment[1].first);
    for (const auto &a : step.arguments)
      all += a + " ";
    all += "\n";
  }
  EXPECT_NE(std::string::npos, all.find("grdimage ../../products/grids/bathy.grd -JM16c"));
  EXPECT_NE(std::string::npos, all.find("grdcontour ../../products/grids/bathy.grd -C50"));
  EXPECT_NE(std::string::npos, all.find("-A250"));
  EXPECT_NE(std::string::npos, all.find("-BWSne+tSurvey"));
  EXPECT_NE(std::string::npos, all.find("colorbar"));
  EXPECT_NE(std::string::npos, all.find("-LjBL+w0.1k"));  // about a fifth of a 0.93 km wide map
  EXPECT_EQ(std::vector<std::string>{"../../products/figures/bathy.ps"}, plan.outputs);

  // projected grids: a Cartesian map in metres, no scale bar
  p.geographic = false;
  plan = processing.figure(p);
  ASSERT_TRUE(plan.ok);
  all.clear();
  for (const auto &step : plan.steps)
    for (const auto &a : step.arguments)
      all += a + " ";
  EXPECT_NE(std::string::npos, all.find("-JX16c/0"));
  EXPECT_EQ(std::string::npos, all.find("-Lj"));

  // perspective view and survey overview
  p.kind = "perspective";
  plan = processing.figure(p);
  ASSERT_TRUE(plan.ok);
  bool view = false;
  for (const auto &step : plan.steps)
    view = view || (!step.arguments.empty() && step.arguments[0] == "grdview");
  EXPECT_TRUE(view);
  FigureParameters overview;
  overview.kind = "overview";
  overview.outputRoot = "overview";
  overview.formats = {"ps"};
  overview.tracks = {"line1.mb71.fnv", "line2.mb71.fnv"};
  EXPECT_FALSE(processing.figure(overview).ok);  // extent not known yet
  overview.region = GeoBounds{-124.51, -124.49, 40.83, 40.85};
  plan = processing.figure(overview);
  ASSERT_TRUE(plan.ok) << plan.error;
  all.clear();
  for (const auto &step : plan.steps)
    for (const auto &a : step.arguments)
      all += a + " ";
  EXPECT_NE(std::string::npos, all.find("plot line1.mb71.fnv line2.mb71.fnv -i7,8"));

  // errors and dependencies
  FigureParameters bad = p;
  bad.kind = "poster";
  EXPECT_FALSE(processing.figure(bad).ok);
  bad = p;
  bad.grid.clear();
  bad.kind = "bathymetry";
  EXPECT_FALSE(processing.figure(bad).ok);
  bad = p;
  bad.formats = {"gif"};
  EXPECT_FALSE(processing.figure(bad).ok);
  bad = p;
  bad.formats = {"png"};
  bad.ghostscript.clear();
  const bool ghostscript = !ProcessingService::findGhostscript({}).empty();
  EXPECT_EQ(ghostscript, processing.figure(bad).ok);  // PNG needs Ghostscript
  EXPECT_DOUBLE_EQ(5.0, ProcessingService::scaleBarKm(37.0));
  EXPECT_DOUBLE_EQ(0.5, ProcessingService::scaleBarKm(3.0));
  EXPECT_DOUBLE_EQ(2.0, ProcessingService::scaleBarKm(10.0));
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
  EXPECT_TRUE(kernel.services().has<IJobService>());
  EXPECT_TRUE(kernel.services().has<IProcessingService>());
  EXPECT_TRUE(kernel.services().has<ISoundingService>());

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
  r = kernel.commands().execute("grid.stats", {{"path", writeTestGrid()}});
  EXPECT_EQ(1, getInt(r.data, "empty"));
  r = kernel.commands().execute("processing.grid", {{"datalist", std::string("dl.mb-1")},
                                                    {"output", std::string("bathy")},
                                                    {"spacing", std::int64_t(50)}});
  ASSERT_TRUE(r.ok) << r.message;
  EXPECT_NE(std::string::npos, getString(r.data, "command").find("-E50/50/meters!"));
  EXPECT_TRUE(kernel.commands().execute("jobs.list").ok);
  r = kernel.commands().execute("soundings.info", {{"path", testData71()}});
  ASSERT_TRUE(r.ok) << r.message;
  EXPECT_EQ(2, getInt(r.data, "pings"));
  EXPECT_GT(getInt(r.data, "good"), 0);
  EXPECT_TRUE(kernel.commands().execute("figure.check").ok);
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
