// Tests of the project plugin: a project in a folder whose name has spaces
// and Chinese characters, import of real swath data (datalists written by the
// service, metadata read in a background job), gridding through the job
// service, product registration, persistence and the project.* commands.
#include <algorithm>
#include <map>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "MbSystemPlugin.h"
#include "ProjectPlugin.h"
#include "ProjectService.h"
#include "mbstudio/api/JobService.h"
#include "mbstudio/api/ProjectService.h"
#include "mbstudio/api/SoundingService.h"
#include "mbstudio/api/ToolService.h"
#include "mbstudio/kernel/Kernel.h"
#include "mbstudio/platform/Paths.h"

using namespace mbstudio;

namespace {

std::string testData() {
  return platform::joinPath(MBSTUDIO_SOURCE_DIR, "test/utilities/testdata/mb21/TN136HS.309.snipped.mb21");
}

// The same two pings in MB-System's processing format (written by mbprocess).
std::string testData71() {
  return platform::joinPath(MBSTUDIO_SOURCE_DIR, "test/utilities/testdata/mb71/TN136HS.309.snipped.mb71");
}

std::string scratch(const std::string &name) {
  const std::string dir = platform::joinPath(platform::executableDirectory(), "mbstudio-project-scratch");
  platform::makeDirectories(dir);
  return platform::joinPath(dir, name);
}

// a new, empty folder for each run (earlier runs are left alone)
std::string freshFolder(const std::string &name) {
  const std::string dir = scratch(name);
  std::string run;
  int n = 0;
  do
    run = platform::joinPath(dir, "run-" + std::to_string(++n));
  while (platform::fileExists(run));
  platform::makeDirectories(run);
  return run;
}

bool copyFile(const std::string &from, const std::string &to) {
  std::string data;
  return platform::readTextFile(from, data) && platform::writeTextFile(to, data);
}

bool contains(const std::vector<std::string> &list, const std::string &item) {
  return std::find(list.begin(), list.end(), item) != list.end();
}

const JobRecord *recordOf(const std::vector<JobRecord> &history, std::int64_t sequence) {
  for (const auto &r : history)
    if (r.sequence == sequence)
      return &r;
  return nullptr;
}

class ProjectTest : public ::testing::Test {
 protected:
  void SetUp() override {
    KernelConfig config;
    config.toolDirectories = {MBSTUDIO_TOOL_DIR};
    kernel = std::make_unique<Kernel>(config);
    kernel->plugins().addStatic([] { return std::make_unique<mbsystem::MbSystemPlugin>(); });
    kernel->plugins().addStatic([] { return std::make_unique<project::ProjectPlugin>(); });
    ASSERT_EQ(2u, kernel->start());
    service = kernel->services().get<IProjectService>();
    ASSERT_TRUE(service);
  }
  void TearDown() override {
    service.reset();
    kernel->stop();
  }
  // a project with the line of format 71 (or 21) in its "raw" folder
  std::string makeProject(const std::string &name, const std::string &data) {
    const std::string root = platform::joinPath(freshFolder(name), "survey");
    std::string error;
    EXPECT_TRUE(service->create(root, name, error)) << error;
    const std::string raw = platform::joinPath(root, "raw");
    platform::makeDirectories(raw);
    EXPECT_TRUE(copyFile(data, platform::joinPath(raw, "line" + data.substr(data.rfind('.')))));
    const ImportResult imported = service->addFiles({raw}, true);
    EXPECT_EQ(1, imported.added);
    EXPECT_TRUE(concrete()->waitForJobs(120000));
    return root;
  }
  project::ProjectService *concrete() { return dynamic_cast<project::ProjectService *>(service.get()); }
  ProcessingRequest gridRequest(const std::string &name) const {
    ProcessingRequest r;
    r.kind = "grid";
    r.name = name;
    r.grid.spacing = 20;
    r.grid.dataType = 2;
    r.grid.bounds = GeoBounds{-124.507, -124.496, 40.835, 40.842};
    return r;
  }
  std::unique_ptr<Kernel> kernel;
  std::shared_ptr<IProjectService> service;
};

}  // namespace

TEST(ProjectService, SafeNames) {
  EXPECT_EQ("EM122_survey_2011", project::ProjectService::safeName("EM122 survey/2011"));
  EXPECT_EQ("product", project::ProjectService::safeName("  "));
  EXPECT_EQ("测量", project::ProjectService::safeName("测量"));
}

TEST_F(ProjectTest, ImportGridSaveAndReopen) {
  // a folder name with spaces and Chinese characters
  const std::string root = platform::joinPath(freshFolder("projects"), "Survey 2011 测线");
  std::string error;
  ASSERT_TRUE(service->create(root, "Survey 测线", error)) << error;
  EXPECT_TRUE(service->isOpen());
  EXPECT_TRUE(platform::fileExists(service->file()));
  EXPECT_NE(std::string::npos, service->file().find(".mbsproj"));
  EXPECT_FALSE(service->create(root, "again", error));  // one project per folder

  // raw data inside the project folder: relative datalist paths, no spaces
  const std::string raw = platform::joinPath(root, "raw");
  platform::makeDirectories(raw);
  ASSERT_TRUE(copyFile(testData(), platform::joinPath(raw, "TN136HS.309.snipped.mb21")));
  platform::writeTextFile(platform::joinPath(raw, "notes.txt"), "not a swath file");

  ImportResult imported = service->addFiles({raw}, true);
  EXPECT_EQ(1, imported.added);
  ASSERT_NE(0, imported.job);
  auto *concrete = dynamic_cast<project::ProjectService *>(service.get());
  ASSERT_NE(nullptr, concrete);
  ASSERT_TRUE(concrete->waitForJobs(120000));
  auto lines = service->lines();
  ASSERT_EQ(1u, lines.size());
  EXPECT_EQ(21, lines[0].format);
  EXPECT_EQ("MBF_HSATLRAW", lines[0].formatName);
  EXPECT_TRUE(lines[0].inspected);
  EXPECT_EQ(2, lines[0].records);
  EXPECT_TRUE(lines[0].bounds.valid());
  EXPECT_TRUE(service->bounds().valid());
  // the same file again is skipped
  imported = service->addFiles({platform::joinPath(raw, "TN136HS.309.snipped.mb21")}, false);
  EXPECT_EQ(0, imported.added);
  ASSERT_EQ(1u, imported.skipped.size());

  std::string datalist;
  ASSERT_TRUE(platform::readTextFile(service->datalist("raw"), datalist));
  EXPECT_NE(std::string::npos, datalist.find("../raw/TN136HS.309.snipped.mb21 21"));

  // preview and run a grid
  ProcessingRequest request;
  request.kind = "grid";
  request.name = "bathy 20m";
  request.grid.spacing = 20;
  request.grid.dataType = 2;
  request.grid.bounds = GeoBounds{-124.507, -124.496, 40.835, 40.842};
  const CommandPlan preview = service->plan(request);
  ASSERT_TRUE(preview.ok) << preview.error;
  ASSERT_FALSE(preview.commandLines().empty());
  EXPECT_NE(std::string::npos, preview.commandLines().front().find("mbgrid"));
  EXPECT_NE(std::string::npos, preview.outputs.front().find("bathy_20m.grd"));
  const auto id = service->run(request, error);
  ASSERT_NE(0, id) << error;
  ASSERT_TRUE(concrete->waitForJobs(120000));
  const auto history = service->history();
  ASSERT_EQ(2u, history.size());
  EXPECT_EQ("import", history[0].kind);
  EXPECT_EQ("completed", history[1].state) << history[1].error;
  EXPECT_FALSE(history[1].commands.empty());
  EXPECT_TRUE(platform::fileExists(history[1].logPath));
  auto assets = service->assets();
  ASSERT_GE(assets.size(), 1u);
  EXPECT_EQ("grid", assets[0].type);
  EXPECT_TRUE(platform::fileExists(assets[0].path));
  EXPECT_EQ(history[1].sequence, assets[0].job);
  ASSERT_FALSE(service->layers().empty());
  EXPECT_EQ(assets[0].id, service->layers()[0].asset);

  // a second grid with the same name does not overwrite the first
  const CommandPlan again = service->plan(request);
  ASSERT_TRUE(again.ok);
  EXPECT_NE(std::string::npos, again.outputs.front().find("bathy_20m-2.grd"));

  // layer style and workspace state persist
  ProjectLayer layer = service->layers()[0];
  layer.opacity = 0.5;
  layer.colorMap = 2;
  EXPECT_TRUE(service->setLayer(layer));
  service->setWorkspaceState(R"({"tabs":["map"]})");
  const std::string file = service->file();
  service->close();
  EXPECT_FALSE(service->isOpen());

  ASSERT_TRUE(service->open(root, error)) << error;  // the folder finds the project file
  EXPECT_EQ(file, service->file());
  EXPECT_EQ("Survey 测线", service->name());
  ASSERT_EQ(1u, service->lines().size());
  EXPECT_TRUE(service->lines()[0].inspected);
  EXPECT_EQ(assets.size(), service->assets().size());
  ASSERT_FALSE(service->layers().empty());
  EXPECT_DOUBLE_EQ(0.5, service->layers()[0].opacity);
  EXPECT_EQ(2, service->layers()[0].colorMap);
  EXPECT_EQ(2u, service->history().size());
  EXPECT_NE(std::string::npos, service->workspaceState().find("map"));

  // removing a product removes its layer
  EXPECT_TRUE(service->removeAsset(assets[0].id, false));
  EXPECT_TRUE(service->layers().empty());
  EXPECT_EQ(1, service->removeLines({service->lines()[0].id}));
  EXPECT_TRUE(service->lines().empty());
}

TEST_F(ProjectTest, RejectsBadRequests) {
  std::string error;
  ProcessingRequest request;
  request.kind = "grid";
  EXPECT_FALSE(service->plan(request).ok);  // no project
  EXPECT_EQ(0, service->run(request, error));
  ASSERT_TRUE(service->create(freshFolder("empty"), "empty", error)) << error;
  EXPECT_FALSE(service->plan(request).ok);  // no lines
  request.kind = "nonsense";
  EXPECT_FALSE(service->plan(request).ok);
  const ImportResult r = service->addFiles({scratch("does-not-exist.mb21")}, false);
  EXPECT_EQ(0, r.added);
  EXPECT_EQ(1u, r.skipped.size());
  EXPECT_FALSE(service->open(scratch("nothing-here"), error));
}

TEST_F(ProjectTest, CommandsForTheCommandLine) {
  const std::string root = platform::joinPath(freshFolder("commands"), "cli");
  auto r = kernel->commands().execute("project.create", {{"dir", root}, {"name", std::string("cli")}});
  ASSERT_TRUE(r.ok) << r.message;
  const std::string file = getString(r.data, "file");
  const std::string raw = platform::joinPath(root, "raw");
  platform::makeDirectories(raw);
  ASSERT_TRUE(copyFile(testData(), platform::joinPath(raw, "line.mb21")));
  service->close();
  r = kernel->commands().execute("project.add", {{"project", file}, {"path", raw}});
  ASSERT_TRUE(r.ok) << r.message;
  EXPECT_EQ(1, getInt(r.data, "added"));
  EXPECT_EQ(1, getInt(r.data, "inspected"));
  r = kernel->commands().execute("project.info", {{"project", root}});
  ASSERT_TRUE(r.ok) << r.message;
  EXPECT_EQ(1, getInt(r.data, "lines"));
  EXPECT_FALSE(kernel->commands().execute("project.info", {{"project", scratch("none")}}).ok);
  EXPECT_FALSE(kernel->contributions().actions().empty());
}

// Products never share a file, jobs that write a survey line run before the
// jobs that read it (and never at the same time), and repeating a job is a
// new attempt with its own record, folder and outputs.
TEST_F(ProjectTest, ReservesOutputsSerialisesAndRetries) {
  makeProject("correctness", testData71());
  const std::string lineId = service->lines().at(0).id;
  std::string error;
  ProcessingRequest clean;
  clean.kind = "clean";
  const auto cleanJob = service->run(clean, error);
  ASSERT_NE(0, cleanJob) << error;
  const auto first = service->run(gridRequest("bathy"), error);
  ASSERT_NE(0, first) << error;
  const auto second = service->run(gridRequest("bathy"), error);
  ASSERT_NE(0, second) << error;
  auto jobs = kernel->services().get<IJobService>();
  JobInfo c, g1, g2;
  ASSERT_TRUE(jobs->job(first, g1));
  ASSERT_TRUE(jobs->job(second, g2));
  // reserved while the first job is queued: a different file
  ASSERT_FALSE(g1.outputs.empty());
  EXPECT_NE(g1.outputs.front(), g2.outputs.front());
  EXPECT_NE(std::string::npos, g2.outputs.front().find("bathy-2.grd"));
  ASSERT_TRUE(concrete()->waitForJobs(120000));
  ASSERT_TRUE(jobs->job(cleanJob, c));
  ASSERT_TRUE(jobs->job(first, g1));
  ASSERT_TRUE(jobs->job(second, g2));
  EXPECT_EQ(JobState::Completed, c.state) << c.error;
  EXPECT_EQ(JobState::Completed, g1.state) << g1.error;
  EXPECT_EQ(JobState::Completed, g2.state) << g2.error;
  EXPECT_TRUE(contains(c.writes, "line:" + lineId));
  EXPECT_TRUE(contains(g1.reads, "line:" + lineId));
  EXPECT_GE(g1.startedAt, c.finishedAt);  // gridding waited for the cleaning
  int grids = 0;
  for (const auto &a : service->assets())
    grids += a.type == "grid" ? 1 : 0;
  EXPECT_EQ(2, grids);
  // cleaning set edits that are not processed yet
  EXPECT_EQ(1, service->lines().at(0).editRevision);
  EXPECT_EQ("processing-required", service->lines().at(0).processingStatus());

  // the job service does not repeat project jobs on its own
  EXPECT_EQ(0, jobs->retry(first));
  const std::int64_t sequence = service->sequenceOfJob(first);
  ASSERT_GT(sequence, 0);
  ProcessingRequest recorded;
  ASSERT_TRUE(service->request(sequence, recorded));
  EXPECT_EQ("grid", recorded.kind);
  EXPECT_DOUBLE_EQ(20.0, recorded.grid.spacing);
  EXPECT_EQ(std::vector<std::string>{lineId}, recorded.lines);
  const auto again = service->retry(sequence, error);
  ASSERT_NE(0, again) << error;
  ASSERT_TRUE(concrete()->waitForJobs(120000));
  auto history = service->history();
  const JobRecord *original = recordOf(history, sequence);
  const JobRecord *attempt = recordOf(history, service->sequenceOfJob(again));
  ASSERT_NE(nullptr, original);
  ASSERT_NE(nullptr, attempt);
  EXPECT_EQ("completed", attempt->state) << attempt->error;
  EXPECT_EQ(2, attempt->attempt);
  EXPECT_EQ(sequence, attempt->retryOf);
  EXPECT_NE(original->workingDirectory, attempt->workingDirectory);
  ASSERT_FALSE(attempt->outputs.empty());
  EXPECT_NE(original->outputs.front(), attempt->outputs.front());
  EXPECT_EQ("completed", original->state);  // the first attempt keeps its record
  const std::int64_t attemptSequence = attempt->sequence;
  // the import can be repeated as well
  EXPECT_NE(0, service->retry(history.front().sequence, error)) << error;
  ASSERT_TRUE(concrete()->waitForJobs(120000));

  // attempts and requests persist
  const std::string file = service->file();
  service->close();
  ASSERT_TRUE(service->open(file, error)) << error;
  const auto reopened = service->history();
  const JobRecord *restored = recordOf(reopened, attemptSequence);
  ASSERT_NE(nullptr, restored);
  EXPECT_EQ(2, restored->attempt);
  EXPECT_EQ(sequence, restored->retryOf);
  EXPECT_TRUE(service->request(sequence, recorded));
  EXPECT_EQ(1, service->lines().at(0).editRevision);
}

// The editing loop: sounding edits are saved to the edit save file, the line
// needs processing, mbprocess applies the edits, products made before are
// outdated and can be made again from their recipe.
TEST_F(ProjectTest, EditsProcessingAndOutdatedProducts) {
  makeProject("editing", testData71());
  SurveyLine line = service->lines().at(0);
  EXPECT_EQ("unprocessed", line.processingStatus());
  auto versions = service->lineVersions(line.id);
  ASSERT_EQ(1u, versions.size());
  EXPECT_EQ("original", versions[0].version);
  EXPECT_TRUE(versions[0].exists);

  std::string error;
  ASSERT_NE(0, service->run(gridRequest("before"), error)) << error;
  ASSERT_TRUE(concrete()->waitForJobs(120000));
  std::string before;
  for (const auto &a : service->assets())
    if (a.type == "grid") {
      before = a.id;
      EXPECT_EQ("current", a.status);
      EXPECT_EQ(1u, a.inputs.size());
    }
  ASSERT_FALSE(before.empty());

  // flag the first good sounding of each ping
  auto soundings = kernel->services().get<ISoundingService>();
  ASSERT_TRUE(soundings);
  const SoundingSet set = soundings->load(line.workPath(), line.workFormat(), 0);
  ASSERT_TRUE(set.ok) << set.error;
  std::vector<BeamEdit> edits;
  for (const auto &ping : set.pings)
    for (std::uint32_t k = 0; k < ping.beams; k++)
      if (beamflag::isGood(set.flag[ping.first + k])) {
        edits.push_back(BeamEdit{ping.time, ping.multiplicity, static_cast<int>(k), BeamAction::Flag});
        break;
      }
  ASSERT_EQ(2u, edits.size());
  const auto saveJob = service->saveEdits({{line.id, edits}}, error);
  ASSERT_NE(0, saveJob) << error;
  ASSERT_TRUE(concrete()->waitForJobs(120000));
  line = service->lines().at(0);
  EXPECT_EQ(1, line.editRevision);
  EXPECT_EQ("processing-required", line.processingStatus());
  for (const auto &a : service->assets())
    if (a.id == before)
      EXPECT_EQ("pending", a.status);  // its line waits for processing
  EXPECT_EQ("process", service->nextAction().kind);
  EXPECT_EQ("edit", service->history().back().kind);
  EXPECT_EQ("completed", service->history().back().state) << service->history().back().error;
  EXPECT_EQ(0, service->retry(service->history().back().sequence, error));  // edits come from the editor

  ProcessingRequest process;
  process.kind = "process";
  ASSERT_NE(0, service->run(process, error)) << error;
  ASSERT_TRUE(concrete()->waitForJobs(180000));
  ASSERT_EQ("completed", service->history().back().state) << service->history().back().error;
  line = service->lines().at(0);
  EXPECT_EQ("processed", line.processingStatus());
  versions = service->lineVersions(line.id);
  ASSERT_EQ(2u, versions.size());
  EXPECT_EQ("processed", versions[1].version);
  ASSERT_TRUE(versions[1].exists);
  // mbprocess applied the edits: the processed file has the flags
  const SoundingSet processed = soundings->load(versions[1].path, versions[1].format, 0);
  ASSERT_TRUE(processed.ok) << processed.error;
  ASSERT_EQ(set.pings.size(), processed.pings.size());
  for (std::size_t p = 0; p < edits.size(); p++) {
    const auto &ping = processed.pings[p];
    ASSERT_LT(static_cast<std::uint32_t>(edits[p].beam), ping.beams);
    EXPECT_TRUE(beamflag::isFlagged(processed.flag[ping.first + edits[p].beam])) << "ping " << p;
  }
  // the grid made before is outdated now
  for (const auto &a : service->assets())
    if (a.id == before) {
      EXPECT_EQ("outdated", a.status);
      EXPECT_EQ(1, a.staleInputs);
    }
  EXPECT_EQ("regenerate", service->nextAction().kind);
  ASSERT_NE(0, service->regenerate(before, error)) << error;
  ASSERT_TRUE(concrete()->waitForJobs(120000));
  int current = 0;
  for (const auto &a : service->assets())
    if (a.type == "grid" && a.status == "current")
      current++;
  EXPECT_EQ(1, current);  // the new grid; the old one stays outdated

  // revisions persist
  const std::string file = service->file();
  service->close();
  ASSERT_TRUE(service->open(file, error)) << error;
  line = service->lines().at(0);
  EXPECT_EQ(1, line.editRevision);
  EXPECT_EQ(1, line.processedEdit);
  EXPECT_EQ("processed", line.processingStatus());
}

// GMT figures are products: isolated GMT sessions, recipes, attempts.
TEST_F(ProjectTest, MapFiguresWithGmt) {
  makeProject("figures", testData());
  std::string error;
  ASSERT_NE(0, service->run(gridRequest("bathy"), error)) << error;
  ASSERT_TRUE(concrete()->waitForJobs(120000));
  std::string grid;
  for (const auto &a : service->assets())
    if (a.type == "grid" && a.parent.empty())
      grid = a.id;
  ASSERT_FALSE(grid.empty());
  ProcessingRequest figure;
  figure.kind = "figure";
  figure.asset = grid;
  figure.name = "map";
  figure.figure.formats = {"ps"};
  figure.figure.contourInterval = 10;
  figure.figure.title = "Test";
  // GMT finishes every figure with psconvert, which needs Ghostscript
  const CommandResult check = kernel->commands().execute("figure.check");
  if (!getBool(check.data, "ghostscript")) {
    const CommandPlan refused = service->plan(figure);
    EXPECT_FALSE(refused.ok);
    EXPECT_NE(std::string::npos, refused.error.find("Ghostscript")) << refused.error;
    GTEST_SKIP() << "Ghostscript is not installed";
  }
  const CommandPlan plan = service->plan(figure);
  ASSERT_TRUE(plan.ok) << plan.error;
  ASSERT_FALSE(plan.steps.empty());
  for (const auto &step : plan.steps)
    if (!step.tool.empty()) {
      ASSERT_EQ(3u, step.environment.size());
      EXPECT_EQ(0u, step.environment[0].second.find("mbstudio-job-"));
      EXPECT_EQ("GMT_SESSIONDIR", step.environment[2].first);
      EXPECT_EQ(step.environment[1].second, step.environment[2].second);
    }
  EXPECT_NE(std::string::npos, plan.outputs.front().find("map.ps"));

  ToolInfo gmt;
  auto tools = kernel->services().get<IToolService>();
  if (!tools->tool("gmt", gmt) || !gmt.available)
    GTEST_SKIP() << "GMT is not installed where the tests can find it";
  // two figures at once: each in its own GMT session
  const auto a = service->run(figure, error);
  ASSERT_NE(0, a) << error;
  figure.figure.kind = "overview";
  figure.name = "overview";
  const auto b = service->run(figure, error);
  ASSERT_NE(0, b) << error;
  ASSERT_TRUE(concrete()->waitForJobs(180000));
  const auto history = service->history();
  for (const auto job : {a, b}) {
    const JobRecord *r = recordOf(history, service->sequenceOfJob(job));
    ASSERT_NE(nullptr, r);
    std::string log;
    platform::readTextFile(r->logPath, log);
    ASSERT_EQ("completed", r->state) << r->error << "\n" << log;
    ASSERT_FALSE(r->outputs.empty());
    EXPECT_TRUE(platform::fileExists(r->outputs.front()));
  }
  int figures = 0;
  for (const auto &asset : service->assets())
    if (asset.type == "figure") {
      figures++;
      EXPECT_EQ("current", asset.status);
    }
  EXPECT_EQ(2, figures);
}
