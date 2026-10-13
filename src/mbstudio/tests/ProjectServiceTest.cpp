// Tests of the project plugin: a project in a folder whose name has spaces
// and Chinese characters, import of real swath data (datalists written by the
// service, metadata read in a background job), gridding through the job
// service, product registration, persistence and the project.* commands.
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "MbSystemPlugin.h"
#include "ProjectPlugin.h"
#include "ProjectService.h"
#include "mbstudio/api/ProjectService.h"
#include "mbstudio/kernel/Kernel.h"
#include "mbstudio/platform/Paths.h"

using namespace mbstudio;

namespace {

std::string testData() {
  return platform::joinPath(MBSTUDIO_SOURCE_DIR, "test/utilities/testdata/mb21/TN136HS.309.snipped.mb21");
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
