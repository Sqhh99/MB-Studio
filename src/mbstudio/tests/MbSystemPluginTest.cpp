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
