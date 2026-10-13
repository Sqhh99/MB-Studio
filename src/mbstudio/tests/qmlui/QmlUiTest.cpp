// Tests of the MB-Studio user interface layer.
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QtTest>
#include <memory>

#include "MbSystemPlugin.h"
#include "MbioServices.h"
#include "ProjectPlugin.h"
#include "QmlFrontend.h"
#include "mbstudio/api/ToolService.h"
#include "mbstudio/kernel/Kernel.h"
#include "viewmodels/AppController.h"
#include "viewmodels/FormatListModel.h"
#include "viewmodels/GridViewModel.h"
#include "viewmodels/JobListModel.h"
#include "viewmodels/LogModel.h"
#include "viewmodels/MapView.h"
#include "viewmodels/ProcessingViewModel.h"
#include "viewmodels/ProjectViewModel.h"
#include "viewmodels/SwathViewModel.h"
#include "viewmodels/ToolsViewModel.h"
#include "viewmodels/Translator.h"
#include "viewmodels/TrackPlot.h"
#include "viewmodels/WorkspaceModel.h"

using namespace mbstudio;

namespace {

QString testData() {
  return QStringLiteral(MBSTUDIO_SOURCE_DIR) + QStringLiteral("/test/utilities/testdata/mb21/TN136HS.309.snipped.mb21");
}

// a new, empty folder below the screenshot directory for each run
QString freshFolder(const QString &name) {
  const QString base = QStringLiteral(MBSTUDIO_SCREENSHOT_DIR) + QStringLiteral("/projects/") + name;
  QString dir;
  int n = 0;
  do
    dir = base + QStringLiteral("/run-%1").arg(++n);
  while (QFileInfo::exists(dir));
  QDir().mkpath(dir);
  return dir;
}

class FakeFormats : public IFormatService {
 public:
  std::vector<FormatInfo> formats() const override {
    return {{21, "MBF_HSATLRAW", "Raw Hydrosweep", "Atlas Electronik"},
            {58, "MBF_EM710RAW", "Kongsberg 3rd generation multibeam", "EM710, EM302"},
            {88, "MBF_RESON7KR", "Reson 7K multibeam vendor format", "Teledyne Reson"}};
  }
  bool format(int id, FormatInfo &info) const override {
    for (const auto &f : formats())
      if (f.id == id) {
        info = f;
        return true;
      }
    return false;
  }
  int identify(const std::string &path) const override { return path.find(".mb88") != std::string::npos ? 88 : 0; }
};

class FakeTools : public IToolService {
 public:
  mutable std::vector<std::string> lastArgs;
  std::vector<ToolInfo> tools() const override {
    ToolInfo a{"mbinfo", "Inspection", "statistics", "-I file", "/bin/mbinfo", true};
    ToolInfo b{"mbgrid", "Gridding", "gridding", "-I datalist", "", false};
    return {a, b};
  }
  bool tool(const std::string &name, ToolInfo &info) const override {
    for (const auto &t : tools())
      if (t.name == name) {
        info = t;
        return true;
      }
    return false;
  }
  std::vector<std::string> categories() const override { return {"Inspection", "Gridding"}; }
  platform::ProcessResult run(const std::string &, const std::vector<std::string> &arguments, const std::string &,
                              const platform::OutputHandler &output, const std::atomic<bool> *) const override {
    lastArgs = arguments;
    output("line one\r\n");
    output("line two\n");
    platform::ProcessResult r;
    r.started = true;
    r.exitCode = 3;
    return r;
  }
};

class FakeGrids : public IGridService {
 public:
  GridLoadResult load(const std::string &path) const override {
    GridLoadResult r;
    if (path == "bad.grd") {
      r.error = "cannot open grid";
      return r;
    }
    Grid &g = r.grid;
    g.nx = 20;
    g.ny = 10;
    g.xmin = -124.5;
    g.xmax = -124.3;
    g.ymin = 40.8;
    g.ymax = 40.9;
    g.geographic = true;
    for (int iy = 0; iy < g.ny; iy++)
      for (int ix = 0; ix < g.nx; ix++)
        g.z.push_back(-500.0f + 10.0f * ix + 5.0f * iy);
    g.z[5] = std::numeric_limits<float>::quiet_NaN();
    g.zmin = -500;
    g.zmax = -265;
    r.ok = true;
    return r;
  }
  GridStatistics statistics(const Grid &grid) const override {
    GridStatistics st;
    st.nodes = static_cast<long long>(grid.z.size());
    for (const float z : grid.z)
      st.valid += std::isnan(z) ? 0 : 1;
    st.empty = st.nodes - st.valid;
    st.emptyRatio = st.nodes > 0 ? double(st.empty) / double(st.nodes) : 0.0;
    return st;
  }
};

// Items of the scene below 'item' with the given object name (Repeater
// delegates are not QObject children of the window, so walk the item tree).
void findItems(QQuickItem *item, const QString &name, QList<QQuickItem *> &out) {
  if (item->objectName() == name)
    out << item;
  for (auto *child : item->childItems())
    findItems(child, name, out);
}

}  // namespace

class QmlUiTest : public QObject {
  Q_OBJECT

 private slots:
  void formatModelFilters() {
    ui::FormatListModel model(std::make_shared<FakeFormats>());
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.totalCount(), 3);
    model.setFilter(QStringLiteral("kongsberg"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.get(0).value(QStringLiteral("formatId")).toInt(), 58);
    model.setFilter(QStringLiteral("88"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), ui::FormatListModel::NameRole).toString(), QStringLiteral("MBF_RESON7KR"));
    model.setFilter(QString());
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.identify(QStringLiteral("C:/data/line.mb88")), 88);
  }

  void toolsModelRunsAndCollectsOutput() {
    auto fake = std::make_shared<FakeTools>();
    ui::ToolsViewModel model(fake);
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.availableCount(), 1);
    model.setFilter(QStringLiteral("grid"));
    QCOMPARE(model.count(), 1);
    model.setFilter(QString());
    QVERIFY(model.selectTool(QStringLiteral("mbinfo")));
    QCOMPARE(model.current().value(QStringLiteral("name")).toString(), QStringLiteral("mbinfo"));
    model.setArguments(QStringLiteral("-I \"C:\\Survey Data\\line 1.mb88\" -X1"));
    QCOMPARE(model.commandPreview(), QStringLiteral("mbinfo -I \"C:\\Survey Data\\line 1.mb88\" -X1"));
    QSignalSpy finished(&model, &ui::ToolsViewModel::finished);
    model.run();
    QVERIFY(model.waitForFinished(10000));
    QCOMPARE(finished.count(), 1);
    QCOMPARE(model.exitCode(), 3);
    const std::vector<std::string> expected = {"-I", "C:\\Survey Data\\line 1.mb88", "-X1"};
    QCOMPARE(fake->lastArgs, expected);
    QVERIFY(model.output().contains(QStringLiteral("line one\nline two\n")));  // CR removed
    QVERIFY(model.output().contains(QStringLiteral("[exit code 3]")));
    model.clearOutput();
    QVERIFY(model.output().isEmpty());
    model.useExample();
    QCOMPARE(model.arguments(), QStringLiteral("-I file"));
  }

  void toolsModelRunsRealPrograms() {
    auto tools = std::make_shared<mbsystem::ToolService>(std::vector<std::string>{MBSTUDIO_TOOL_DIR}, nullptr);
    ui::ToolsViewModel model(tools);
    QVERIFY(model.selectTool(QStringLiteral("mbformat")));
    model.setArguments(QStringLiteral("-F21"));
    model.run();
    QVERIFY(model.waitForFinished(60000));
    QCOMPARE(model.exitCode(), 0);
    QVERIFY2(model.output().contains(QStringLiteral("MBF_HSATLRAW")), qPrintable(model.output()));
  }

  void swathModelInspectsRealData() {
    auto swath = std::make_shared<mbsystem::SwathService>(std::make_shared<mbsystem::FormatService>());
    ui::SwathViewModel model(swath);
    QSignalSpy finished(&model, &ui::SwathViewModel::finished);
    model.inspect(testData());
    QVERIFY(model.busy());
    QVERIFY(model.waitForFinished(30000));
    QCOMPARE(finished.count(), 1);
    QVERIFY2(model.loaded(), qPrintable(model.error()));
    QCOMPARE(model.summary().value(QStringLiteral("records")).toLongLong(), 2LL);
    QCOMPARE(model.summary().value(QStringLiteral("formatName")).toString(), QStringLiteral("MBF_HSATLRAW"));
    QCOMPARE(model.track().size(), 2);
    QCOMPARE(model.fileName(), QStringLiteral("TN136HS.309.snipped.mb21"));
    model.clear();
    QVERIFY(!model.loaded());
  }

  void swathModelReportsErrors() {
    auto swath = std::make_shared<mbsystem::SwathService>(std::make_shared<mbsystem::FormatService>());
    ui::SwathViewModel model(swath);
    model.inspect(QStringLiteral("does-not-exist.mb21"));
    QVERIFY(model.waitForFinished(30000));
    QVERIFY(!model.loaded());
    QVERIFY(!model.error().isEmpty());
  }

  void gridModelLoadsRendersAndProbes() {
    ui::GridViewModel model(std::make_shared<FakeGrids>(), std::make_shared<mbsystem::GridRenderService>());
    QCOMPARE(model.colorMaps().size(), 4);
    model.load(QStringLiteral("fake.grd"));
    QVERIFY(model.waitForFinished(30000));
    QVERIFY(model.loaded());
    QCOMPARE(model.info().value(QStringLiteral("nx")).toInt(), 20);
    QCOMPARE(model.imageWidth(), 20);
    QCOMPARE(model.imageHeight(), 10);
    QVERIFY(model.imageSource().startsWith(QStringLiteral("image://mbgrid/")));
    QCOMPARE(qAlpha(model.image().pixel(5, 0)), 0);  // the NaN node is transparent
    const QString first = model.imageSource();

    const QRgb before = model.image().pixel(10, 5);
    model.setColorMap(3);  // grayscale
    model.setHillshade(false);
    QVERIFY(model.waitForFinished(30000));
    QVERIFY(model.imageSource() != first);
    const QRgb after = model.image().pixel(10, 5);
    QVERIFY(before != after);
    QCOMPARE(qRed(after), qGreen(after));  // grey

    const QVariantMap p = model.probe(0.0, 0.0);
    QCOMPARE(p.value(QStringLiteral("x")).toDouble(), -124.5);
    QCOMPARE(p.value(QStringLiteral("y")).toDouble(), 40.9);
    QCOMPARE(p.value(QStringLiteral("z")).toDouble(), -500.0);
    QVERIFY(model.probe(1.5, 0.5).isEmpty());
    QCOMPARE(model.legend().size(), 11);

    model.setAzimuth(-45);
    QCOMPARE(model.azimuth(), 315.0);
    model.setElevation(200);
    QCOMPARE(model.elevation(), 90.0);

    model.load(QStringLiteral("bad.grd"));
    QVERIFY(model.waitForFinished(30000));
    QCOMPARE(model.error(), QStringLiteral("cannot open grid"));
  }

  void trackPlotTicks() {
    const auto ticks = ui::TrackPlot::niceTicks(-124.5072, -124.4961, 5);
    QVERIFY(ticks.size() >= 2 && ticks.size() <= 5);
    for (double t : ticks)
      QVERIFY(t >= -124.5072 && t <= -124.4961);
    QCOMPARE(ui::TrackPlot::niceTicks(0, 100, 6), QVector<double>({0, 20, 40, 60, 80, 100}));
    QVERIFY(ui::TrackPlot::niceTicks(1, 1, 5).isEmpty());
  }

  void logModelFollowsKernelEvents() {
    Kernel kernel;
    kernel.log(LogLevel::Info, "test", "before");
    ui::LogModel model(&kernel);
    QCOMPARE(model.count(), 1);
    kernel.log(LogLevel::Warning, "test", "careful");
    QTRY_COMPARE(model.count(), 2);  // delivered through the event loop
    QCOMPARE(model.warningCount(), 1);
    QVERIFY(model.text().contains(QStringLiteral("careful")));
    model.clear();
    QCOMPARE(model.count(), 0);
  }

  void translatorUsesTheCatalogues() {
    QCOMPARE(ui::resolveLanguage(QStringLiteral("ja")), QStringLiteral("ja"));
    QCOMPARE(ui::resolveLanguage(QStringLiteral("ja-JP")), QStringLiteral("ja"));
    QCOMPARE(ui::resolveLanguage(QStringLiteral("zh_TW")), QStringLiteral("zh_CN"));
    QCOMPARE(ui::resolveLanguage(QStringLiteral("zh-Hans-CN")), QStringLiteral("zh_CN"));
    QCOMPARE(ui::resolveLanguage(QStringLiteral("fr_FR")), QStringLiteral("en"));
    QCOMPARE(ui::supportedLanguages().size(), 3);
    for (const auto &l : ui::supportedLanguages()) {
      if (l.code == QLatin1String("en"))
        continue;
      ui::JsonTranslator translator;
      QVERIFY2(translator.loadLanguage(l.code), qPrintable(l.code));
      QCOMPARE(translator.languageCode(), l.code);
      QVERIFY(translator.size() > 100);
      QVERIFY(!translator.translate("Main", "Log").isEmpty());
      QVERIFY(!translator.translate("Contribution", "Grid bathymetry…").isEmpty());
      QVERIFY(!translator.translate("ToolCategory", "Gridding").isEmpty());
      QVERIFY(translator.translate("Main", "no such text").isEmpty());
    }
    ui::JsonTranslator ja;
    QVERIFY(ja.loadLanguage(QStringLiteral("ja")));
    QCOMPARE(ja.translate("Main", "Log"), QStringLiteral("ログ"));
    QVERIFY(!ja.loadLanguage(QStringLiteral("xx")));
    QVERIFY(ja.isEmpty());
  }

  void languageSwitchTranslatesTheModels() {
    KernelConfig config;
    config.toolDirectories = {MBSTUDIO_TOOL_DIR};
    Kernel kernel(config);
    kernel.plugins().addStatic([] { return std::make_unique<mbsystem::MbSystemPlugin>(); });
    QCOMPARE(kernel.start(), std::size_t(1));
    ui::AppController app(&kernel);
    app.setLanguage(QStringLiteral("en"));
    QCOMPARE(app.effectiveLanguage(), QStringLiteral("en"));
    QCOMPARE(app.languages().size(), 3);
    auto *tools = app.toolsModel();
    QVERIFY(tools->selectTool(QStringLiteral("mbgrid")));
    QVERIFY(tools->helpText().startsWith(QStringLiteral("# mbgrid ")));
    QCOMPARE(tools->helpLanguage(), QStringLiteral("en"));

    QSignalSpy changed(&app, &ui::AppController::languageChanged);
    app.setLanguage(QStringLiteral("ja"));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(app.language(), QStringLiteral("ja"));
    QCOMPARE(app.effectiveLanguage(), QStringLiteral("ja"));
    QCOMPARE(QCoreApplication::translate("Main", "Log"), QStringLiteral("ログ"));
    QCOMPARE(tools->helpLanguage(), QStringLiteral("ja"));
    QVERIFY(tools->helpText().contains(QStringLiteral("## 書式")));
    QCOMPARE(tools->current().value(QStringLiteral("category")).toString(), QStringLiteral("グリッド作成"));

    app.setLanguage(QStringLiteral("zh_CN"));
    QCOMPARE(QCoreApplication::translate("Main", "Log"), QStringLiteral("日志"));
    QVERIFY(tools->helpText().contains(QStringLiteral("## 语法")));
    tools->setFilter(QStringLiteral("网格"));  // localized summaries are searchable
    QVERIFY(tools->count() >= 1);
    tools->setFilter(QString());

    app.setLanguage(QStringLiteral("system"));
    QCOMPARE(app.language(), QString());
    app.setLanguage(QStringLiteral("en"));
    QCOMPARE(QCoreApplication::translate("Main", "Log"), QStringLiteral("Log"));
  }

  void workspacesComeFromContributions() {
    Kernel kernel;
    std::string error;
    QVERIFY2(ui::registerContributions(kernel, error), error.c_str());
    // a plugin's workspace appears without any change to the shell
    WorkspaceContribution extra;
    extra.id = "test.extra";
    extra.title = "Extra";
    extra.qmlUrl = "qrc:/test/Extra.qml";
    extra.owner = "test";
    extra.singleton = false;
    QVERIFY(kernel.contributions().addWorkspace(extra, &error));
    ui::WorkspaceModel model(&kernel);
    QCOMPARE(model.available().size(), 9);
    QCOMPARE(model.open(QStringLiteral("nothing")), -1);
    QCOMPARE(model.open(QStringLiteral("start")), 0);
    QCOMPARE(model.open(QStringLiteral("map")), 1);
    QCOMPARE(model.open(QStringLiteral("start")), 0);  // singleton: activated again
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.open(QStringLiteral("grid"), QStringLiteral("/data/a.grd"), QStringLiteral("Grid: a.grd")), 2);
    QCOMPARE(model.open(QStringLiteral("grid"), QStringLiteral("/data/b.grd"), QStringLiteral("Grid: b.grd")), 2);
    QCOMPARE(model.argument(2), QStringLiteral("/data/b.grd"));
    QCOMPARE(model.open(QStringLiteral("test.extra"), QStringLiteral("1")), 3);
    QCOMPARE(model.open(QStringLiteral("test.extra"), QStringLiteral("2")), 4);  // not a singleton
    QCOMPARE(model.currentWorkspace(), QStringLiteral("test.extra"));
    const QString state = model.saveState();
    model.close(1);  // the map cannot be closed
    QCOMPARE(model.count(), 5);
    model.close(4);
    QCOMPARE(model.count(), 4);
    QCOMPARE(model.currentIndex(), 3);
    ui::WorkspaceModel restored(&kernel);
    restored.restoreState(state);
    QCOMPARE(restored.count(), 5);
    QCOMPARE(restored.currentIndex(), 4);
    QCOMPARE(restored.argument(2), QStringLiteral("/data/b.grd"));
  }

  void projectProcessingAndJobsWithRealData() {
    KernelConfig config;
    config.toolDirectories = {MBSTUDIO_TOOL_DIR};
    Kernel kernel(config);
    kernel.plugins().addStatic([] { return std::make_unique<mbsystem::MbSystemPlugin>(); });
    kernel.plugins().addStatic([] { return std::make_unique<project::ProjectPlugin>(); });
    QCOMPARE(kernel.start(), std::size_t(2));
    std::string error;
    QVERIFY(ui::registerContributions(kernel, error));
    ui::AppController app(&kernel);
    auto *project = app.projectModel();
    auto *processing = app.processingModel();
    auto *jobs = app.jobModel();
    QVERIFY(project->available());
    QVERIFY(!project->isOpen());
    QVERIFY(!processing->ready());  // no project yet

    const QString root = freshFolder(QStringLiteral("viewmodels")) + QStringLiteral("/survey");
    QVERIFY(project->createProject(root, QStringLiteral("Survey")));
    QVERIFY(project->isOpen());
    QVERIFY(project->recentProjects().contains(project->file()));
    QDir().mkpath(root + QStringLiteral("/raw"));
    QVERIFY(QFile::copy(testData(), root + QStringLiteral("/raw/line.mb21")));
    QCOMPARE(project->addData({QUrl::fromLocalFile(root + QStringLiteral("/raw"))}), 1);
    QTRY_COMPARE_WITH_TIMEOUT(project->inspectedCount(), 1, 60000);
    QCOMPARE(project->lineCount(), 1);
    QVERIFY(project->summary().value(QStringLiteral("hasBounds")).toBool());

    // selection
    const QString lineId = project->lineInfo(QStringLiteral("L1")).value(QStringLiteral("lineId")).toString();
    QCOMPARE(lineId, QStringLiteral("L1"));
    project->select(lineId);
    QCOMPARE(project->selectedCount(), 1);
    project->select(lineId, 1);
    QCOMPARE(project->selectedCount(), 0);

    // the grid panel: schema, visibility rules, preview, estimate
    processing->setKind(QStringLiteral("grid"));
    QVERIFY(processing->fields().size() > 8);
    QVERIFY(!processing->fieldVisible(QStringLiteral("clipMode")));
    processing->setValue(QStringLiteral("clip"), 3);
    QVERIFY(processing->fieldVisible(QStringLiteral("clipMode")));
    processing->setValue(QStringLiteral("clip"), 0);
    QVERIFY(!processing->fieldVisible(QStringLiteral("threshold")));
    processing->setValue(QStringLiteral("algorithm"), 3);
    QVERIFY(processing->fieldVisible(QStringLiteral("threshold")));
    processing->setValue(QStringLiteral("algorithm"), 1);
    processing->setValue(QStringLiteral("spacing"), 20.0);
    processing->setValue(QStringLiteral("name"), QStringLiteral("bathy"));
    processing->updatePreview();
    QVERIFY2(processing->ready(), qPrintable(processing->error()));
    QVERIFY(processing->preview().contains(QStringLiteral("mbgrid")));
    QVERIFY(processing->preview().contains(QStringLiteral("-E20/20/meters!")));
    QVERIFY(processing->outputs().first().endsWith(QStringLiteral("bathy.grd")));
    QVERIFY(!processing->estimate().isEmpty());

    // run it as a background job
    const qint64 id = processing->run();
    QVERIFY(id > 0);
    QVERIFY(jobs->waitFor(id, 120000));
    const QVariantMap job = jobs->job(id);
    QCOMPARE(job.value(QStringLiteral("state")).toString(), QStringLiteral("completed"));
    QVERIFY(!jobs->log(id).isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(project->products()->property("count").toInt(), 3, 10000);  // grid, count, std. dev.
    QVERIFY(!project->newestProductOfJob(id).isEmpty());
    QTRY_VERIFY_WITH_TIMEOUT(project->history()->property("count").toInt() >= 2, 10000);
    QVERIFY(project->waitForMap(30000));
    QVERIFY(!project->mapLayers().isEmpty());
    QVERIFY(!project->mapLayers().first().image.isNull());

    // other panels produce valid plans too
    processing->setKind(QStringLiteral("clean"));
    processing->updatePreview();
    QVERIFY2(processing->ready(), qPrintable(processing->error()));
    QVERIFY(processing->preview().contains(QStringLiteral("mbclean")));
    processing->setKind(QStringLiteral("export"));
    QVERIFY(processing->options(QStringLiteral("source")).size() >= 2);  // soundings and the grid
    processing->setKind(QStringLiteral("corrections"));
    processing->updatePreview();
    QVERIFY(!processing->ready());  // nothing chosen yet
    processing->setValue(QStringLiteral("useDraft"), true);
    processing->setValue(QStringLiteral("draft"), 4.5);
    processing->updatePreview();
    QVERIFY2(processing->preview().contains(QStringLiteral("-PDRAFT:4.5")), qPrintable(processing->preview()));

    // reopening restores the documents of the project
    app.openWorkspace(QStringLiteral("history"));
    QTest::qWait(700);  // the workspace state is saved after a short delay
    const QString file = project->file();
    project->closeProject();
    QVERIFY(!project->isOpen());
    QCOMPARE(app.openFile(file), QStringLiteral("map"));
    QVERIFY(project->isOpen());
    QVERIFY(app.workspaceModel()->indexOf(QStringLiteral("history")) >= 0);
    QCOMPARE(project->lineCount(), 1);
  }

  void appControllerRoutesFiles() {
    KernelConfig config;
    config.toolDirectories = {MBSTUDIO_TOOL_DIR};
    Kernel kernel(config);
    kernel.plugins().addStatic([] { return std::make_unique<mbsystem::MbSystemPlugin>(); });
    QCOMPARE(kernel.start(), std::size_t(1));
    std::string error;
    QVERIFY(ui::registerContributions(kernel, error));
    ui::AppController app(&kernel);
    QVERIFY(ui::AppController::isGridFile(QStringLiteral("C:/x/bathy.GRD")));
    QVERIFY(!ui::AppController::isGridFile(QStringLiteral("line.mb88")));
    QVERIFY(ui::AppController::isProjectFile(QStringLiteral("D:/s/Survey.mbsproj")));
    QVERIFY(!app.projectModel()->available());  // no project plugin: the rest still works
    QSignalSpy notify(&app, &ui::AppController::notify);
    QCOMPARE(app.openFile(QStringLiteral("does-not-exist.mb21")), QString());
    QCOMPARE(notify.count(), 1);
    QCOMPARE(app.openFile(testData()), QStringLiteral("inspect"));
    QCOMPARE(app.workspaceModel()->currentWorkspace(), QStringLiteral("inspect"));
    QVERIFY(app.recentFiles().contains(QFileInfo(testData()).absoluteFilePath()));
    QVERIFY(app.swathModel()->waitForFinished(30000));
    QVERIFY(app.swathModel()->loaded());
    app.runToolOn(QStringLiteral("mbinfo"), testData());
    QCOMPARE(app.workspaceModel()->currentWorkspace(), QStringLiteral("programs"));
    QVERIFY(app.toolsModel()->arguments().startsWith(QStringLiteral("-I ")));
    QVERIFY(!app.plugins().isEmpty());
    QVERIFY(!app.services().isEmpty());
    QVERIFY(!app.commands().isEmpty());
    QVERIFY(!app.actions().isEmpty());
    QSignalSpy uiActions(&app, &ui::AppController::uiAction);
    app.trigger(QStringLiteral("ui:panel:grid"));
    QCOMPARE(uiActions.count(), 1);
    QCOMPARE(uiActions.first().first().toString(), QStringLiteral("panel:grid"));
    app.setUiValue(QStringLiteral("test"), 42);
    QCOMPARE(app.uiValue(QStringLiteral("test")).toInt(), 42);
    app.clearRecentFiles();
    QVERIFY(app.recentFiles().isEmpty());
  }

  void mapViewSteps() {
    QCOMPARE(ui::MapView::niceStep(10.0, 6), 2.0);
    QCOMPARE(ui::MapView::niceStep(0.01, 6), 10.0 / 3600);
    QCOMPARE(ui::MapView::niceStep(1000.0, 6), 90.0);
  }

  // Load the complete QML workbench offscreen against real services and a
  // real project, visit every workspace and panel, and fail on any warning.
  void qmlLoadsWithoutWarnings() {
    KernelConfig config;
    config.toolDirectories = {MBSTUDIO_TOOL_DIR};
    config.version = MBSTUDIO_VERSION;
    Kernel kernel(config);
    kernel.plugins().addStatic([] { return std::make_unique<mbsystem::MbSystemPlugin>(); });
    kernel.plugins().addStatic([] { return std::make_unique<project::ProjectPlugin>(); });
    kernel.start();
    std::string error;
    QVERIFY(ui::registerContributions(kernel, error));

    ui::registerQmlTypes();
    ui::AppController app(&kernel);
    app.setLanguage(QStringLiteral("en"));
    qmlRegisterSingletonInstance("MBStudio.Backend", 1, 0, "App", &app);
    QStringList warnings;
    {
      QQmlApplicationEngine engine;
      QObject::connect(&engine, &QQmlEngine::warnings, [&warnings](const QList<QQmlError> &list) {
        for (const auto &w : list)
          warnings << w.toString();
      });
      engine.addImageProvider(QStringLiteral("mbgrid"), new ui::GridImageProvider(app.gridModel()));
      engine.load(QUrl(QStringLiteral("qrc:/mbstudio/qml/Main.qml")));
      QVERIFY2(!engine.rootObjects().isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
      auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
      QVERIFY(window != nullptr);
      window->resize(1440, 900);
      const QString shots = QStringLiteral(MBSTUDIO_SCREENSHOT_DIR);
      QDir().mkpath(shots);
      auto shoot = [&](const QString &name) {
        QTest::qWait(200);
        const QImage shot = window->grabWindow();
        QVERIFY(!shot.isNull());
        shot.save(shots + QStringLiteral("/") + name + QStringLiteral(".png"));
      };
      QCOMPARE(app.workspaceModel()->currentWorkspace(), QStringLiteral("start"));
      shoot(QStringLiteral("01-start"));

      // a project with real data, gridded through the processing panel
      auto *project = app.projectModel();
      const QString root = freshFolder(QStringLiteral("workbench")) + QStringLiteral("/EM survey");
      QVERIFY(project->createProject(root, QStringLiteral("EM survey")));
      QDir().mkpath(root + QStringLiteral("/raw"));
      QVERIFY(QFile::copy(testData(), root + QStringLiteral("/raw/TN136HS.309.snipped.mb21")));
      QCOMPARE(project->addData({root + QStringLiteral("/raw")}), 1);
      QTRY_COMPARE_WITH_TIMEOUT(project->inspectedCount(), 1, 60000);
      app.openWorkspace(QStringLiteral("map"));
      QVERIFY(project->waitForMap(30000));
      shoot(QStringLiteral("02-map-lines"));

      app.trigger(QStringLiteral("ui:panel:grid"));
      auto *processing = app.processingModel();
      processing->setValue(QStringLiteral("spacing"), 20.0);
      processing->setValue(QStringLiteral("clip"), 2);
      processing->updatePreview();
      shoot(QStringLiteral("03-grid-panel"));
      const qint64 id = processing->run();
      QVERIFY2(id > 0, qPrintable(processing->error()));
      QVERIFY(app.jobModel()->waitFor(id, 120000));
      QTRY_VERIFY_WITH_TIMEOUT(project->products()->property("count").toInt() >= 3, 10000);
      QVERIFY(project->waitForMap(30000));
      shoot(QStringLiteral("04-map-grid-and-jobs"));

      // the grid view with its quality layers
      const QString grid = project->newestProductOfJob(id);
      QVERIFY(!grid.isEmpty());
      app.openGrid(grid);
      QVERIFY(app.gridModel()->waitForFinished(30000));
      QVERIFY2(app.gridModel()->loaded(), qPrintable(app.gridModel()->error()));
      QVERIFY(app.gridModel()->hasQualityGrids());
      shoot(QStringLiteral("05-grid"));
      app.gridModel()->setLayer(1);
      QVERIFY(app.gridModel()->waitForFinished(30000));
      shoot(QStringLiteral("06-grid-count"));
      app.gridModel()->setLayer(0);

      // every processing panel renders
      for (const QString &kind : ui::ProcessingViewModel::kinds()) {
        app.trigger(QStringLiteral("ui:panel:") + kind);
        QTest::qWait(60);
      }
      shoot(QStringLiteral("07-export-panel"));

      app.inspectLine(QStringLiteral("L1"));
      QVERIFY(app.swathModel()->waitForFinished(30000));
      shoot(QStringLiteral("08-inspect"));
      const char *others[] = {"history", "programs", "formats", "plugins", "start"};
      int n = 9;
      for (const char *w : others) {
        app.openWorkspace(QString::fromLatin1(w));
        if (QLatin1String(w) == QLatin1String("programs")) {
          app.toolsModel()->selectTool(QStringLiteral("mbgrid"));
          QTest::qWait(100);
        }
        shoot(QStringLiteral("%1-%2").arg(n++, 2, 10, QLatin1Char('0')).arg(QLatin1String(w)));
      }
      // the help tab shows the page of the selected program
      app.openWorkspace(QStringLiteral("programs"));
      QTest::qWait(150);
      auto *helpView = window->findChild<QObject *>(QStringLiteral("helpView"));
      QVERIFY(helpView != nullptr);
      QVERIFY(helpView->property("text").toString().contains(QStringLiteral("mbgrid")));

      app.openWorkspace(QStringLiteral("map"));
      app.setDarkTheme(false);
      shoot(QStringLiteral("light-theme"));
      app.setDarkTheme(true);

      app.trigger(QStringLiteral("ui:palette"));
      shoot(QStringLiteral("command-palette"));
      QTest::keyClick(window, Qt::Key_Escape);

      // switch the language at run time: QML bindings are retranslated
      QObject::connect(&app, &ui::AppController::languageChanged, &engine, &QQmlEngine::retranslate);
      for (const QString lang : {QStringLiteral("ja"), QStringLiteral("zh_CN")}) {
        app.setLanguage(lang);
        QTest::qWait(200);
        app.trigger(QStringLiteral("ui:panel:grid"));
        app.openWorkspace(QStringLiteral("map"));
        shoot(QStringLiteral("language-%1").arg(lang));
        app.openWorkspace(QStringLiteral("start"));
        QTest::qWait(100);
      }
      app.setLanguage(QStringLiteral("en"));
      project->closeProject();
      QTest::qWait(100);
    }
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
  }
};

int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  // keep the user's real settings (recent files, theme) untouched
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                     QStringLiteral(MBSTUDIO_SCREENSHOT_DIR "/settings"));
  QQuickStyle::setStyle(QStringLiteral("Material"));
  QmlUiTest test;
  return QTest::qExec(&test, argc, argv);
}

#include "QmlUiTest.moc"
