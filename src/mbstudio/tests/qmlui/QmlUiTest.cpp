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
#include "QmlFrontend.h"
#include "mbstudio/api/ToolService.h"
#include "mbstudio/kernel/Kernel.h"
#include "viewmodels/AppController.h"
#include "viewmodels/FormatListModel.h"
#include "viewmodels/GridViewModel.h"
#include "viewmodels/LogModel.h"
#include "viewmodels/SwathViewModel.h"
#include "viewmodels/ToolsViewModel.h"
#include "viewmodels/Translator.h"
#include "viewmodels/TrackPlot.h"

using namespace mbstudio;

namespace {

QString testData() {
  return QStringLiteral(MBSTUDIO_SOURCE_DIR) + QStringLiteral("/test/utilities/testdata/mb21/TN136HS.309.snipped.mb21");
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
      QVERIFY(!translator.translate("Main", "Home").isEmpty());
      QVERIFY(!translator.translate("ToolCategory", "Gridding").isEmpty());
      QVERIFY(translator.translate("Main", "no such text").isEmpty());
    }
    ui::JsonTranslator ja;
    QVERIFY(ja.loadLanguage(QStringLiteral("ja")));
    QCOMPARE(ja.translate("Main", "Home"), QStringLiteral("ホーム"));
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
    QCOMPARE(QCoreApplication::translate("Main", "Home"), QStringLiteral("ホーム"));
    QCOMPARE(tools->helpLanguage(), QStringLiteral("ja"));
    QVERIFY(tools->helpText().contains(QStringLiteral("## 書式")));
    QCOMPARE(tools->current().value(QStringLiteral("category")).toString(), QStringLiteral("グリッド作成"));

    app.setLanguage(QStringLiteral("zh_CN"));
    QCOMPARE(QCoreApplication::translate("Main", "Home"), QStringLiteral("首页"));
    QVERIFY(tools->helpText().contains(QStringLiteral("## 语法")));
    tools->setFilter(QStringLiteral("网格"));  // localized summaries are searchable
    QVERIFY(tools->count() >= 1);
    tools->setFilter(QString());

    app.setLanguage(QStringLiteral("system"));
    QCOMPARE(app.language(), QString());
    app.setLanguage(QStringLiteral("en"));
    QCOMPARE(QCoreApplication::translate("Main", "Home"), QStringLiteral("Home"));
  }

  void appControllerRoutesFiles() {
    KernelConfig config;
    config.toolDirectories = {MBSTUDIO_TOOL_DIR};
    Kernel kernel(config);
    kernel.plugins().addStatic([] { return std::make_unique<mbsystem::MbSystemPlugin>(); });
    QCOMPARE(kernel.start(), std::size_t(1));
    ui::AppController app(&kernel);
    QVERIFY(ui::AppController::isGridFile(QStringLiteral("C:/x/bathy.GRD")));
    QVERIFY(!ui::AppController::isGridFile(QStringLiteral("line.mb88")));
    QSignalSpy notify(&app, &ui::AppController::notify);
    QCOMPARE(app.openFile(QStringLiteral("does-not-exist.mb21")), -1);
    QCOMPARE(notify.count(), 1);
    QCOMPARE(app.openFile(testData()), int(ui::AppController::InspectPage));
    QCOMPARE(app.currentPage(), int(ui::AppController::InspectPage));
    QVERIFY(app.recentFiles().contains(QFileInfo(testData()).absoluteFilePath()));
    QVERIFY(app.swathModel()->waitForFinished(30000));
    QVERIFY(app.swathModel()->loaded());
    app.runToolOn(QStringLiteral("mbinfo"), testData());
    QCOMPARE(app.currentPage(), int(ui::AppController::ToolsPage));
    QVERIFY(app.toolsModel()->arguments().startsWith(QStringLiteral("-I ")));
    QVERIFY(!app.plugins().isEmpty());
    QVERIFY(!app.services().isEmpty());
    QVERIFY(!app.commands().isEmpty());
    app.clearRecentFiles();
    QVERIFY(app.recentFiles().isEmpty());
  }

  // Load the complete QML user interface offscreen against real services,
  // visit every page and fail on any QML warning.
  void qmlLoadsWithoutWarnings() {
    KernelConfig config;
    config.toolDirectories = {MBSTUDIO_TOOL_DIR};
    config.version = MBSTUDIO_VERSION;
    Kernel kernel(config);
    kernel.plugins().addStatic([] { return std::make_unique<mbsystem::MbSystemPlugin>(); });
    kernel.start();

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
      window->resize(1360, 860);

      QDir().mkpath(QStringLiteral(MBSTUDIO_SCREENSHOT_DIR));
      // a real grid made by mbgrid from the sample data
      const QString dir = QStringLiteral(MBSTUDIO_SCREENSHOT_DIR);
      QFile datalist(dir + QStringLiteral("/datalist.mb-1"));
      QVERIFY(datalist.open(QIODevice::WriteOnly | QIODevice::Text));
      datalist.write((testData() + QStringLiteral(" 21\n")).toUtf8());
      datalist.close();
      auto tools = kernel.services().get<IToolService>();
      const auto gridded = tools->run("mbgrid",
                                      {"-I" + datalist.fileName().toStdString(), "-O" + (dir + "/bathy").toStdString(),
                                       "-E10/0/meters!", "-A2", "-N", "-C3", "-R-124.507/-124.496/40.835/40.842"},
                                      dir.toStdString(), nullptr);
      QCOMPARE(gridded.exitCode, 0);
      app.openFile(dir + QStringLiteral("/bathy.grd"));
      QVERIFY(app.gridModel()->waitForFinished(30000));
      QVERIFY2(app.gridModel()->loaded(), qPrintable(app.gridModel()->error()));
      app.openFile(testData());
      QVERIFY(app.swathModel()->waitForFinished(30000));
      const char *names[] = {"home", "inspect", "grids", "programs", "formats", "plugins"};
      for (int page = 0; page <= ui::AppController::PluginsPage; page++) {
        app.setCurrentPage(page);
        if (page == ui::AppController::ToolsPage) {
          app.toolsModel()->selectTool(QStringLiteral("mbinfo"));
          app.toolsModel()->showHelp();
          app.toolsModel()->waitForFinished(30000);
        }
        QTest::qWait(150);
        const QImage shot = window->grabWindow();
        QVERIFY(!shot.isNull());
        shot.save(QStringLiteral(MBSTUDIO_SCREENSHOT_DIR "/%1-%2.png").arg(page).arg(QLatin1String(names[page])));
      }
      app.setDarkTheme(false);
      QTest::qWait(150);
      window->grabWindow().save(QStringLiteral(MBSTUDIO_SCREENSHOT_DIR "/light-theme.png"));
      app.setDarkTheme(true);

      // the help tab shows the page of the selected program
      app.setCurrentPage(ui::AppController::ToolsPage);
      app.toolsModel()->selectTool(QStringLiteral("mbgrid"));
      QTest::qWait(150);
      auto *helpView = window->findChild<QObject *>(QStringLiteral("helpView"));
      QVERIFY(helpView != nullptr);
      QVERIFY(helpView->property("text").toString().contains(QStringLiteral("mbgrid")));

      // hover and selection are separate layers that only fade their opacity,
      // so the sidebar never blends through a dark transparent colour
      QList<QQuickItem *> hovers;
      findItems(window->contentItem(), QStringLiteral("navHover"), hovers);
      QVERIFY2(hovers.size() >= 6, qPrintable(QString::number(hovers.size())));
      for (auto *h : hovers)
        QCOMPARE(h->property("opacity").toDouble(), 0.0);

      // switch the language at run time: QML bindings are retranslated
      QObject::connect(&app, &ui::AppController::languageChanged, &engine, &QQmlEngine::retranslate);
      for (const QString lang : {QStringLiteral("ja"), QStringLiteral("zh_CN")}) {
        app.setLanguage(lang);
        QTest::qWait(200);
        QVERIFY(helpView->property("text").toString().contains(QStringLiteral("mbgrid")));
        for (int page = 0; page <= ui::AppController::PluginsPage; page++) {
          app.setCurrentPage(page);
          QTest::qWait(100);
        }
        app.setCurrentPage(ui::AppController::ToolsPage);
        QList<QQuickItem *> tabs;
        findItems(window->contentItem(), QStringLiteral("toolTabs"), tabs);
        QCOMPARE(tabs.size(), 1);
        tabs.first()->setProperty("currentIndex", 0);  // the help tab
        QTest::qWait(150);
        window->grabWindow().save(QStringLiteral(MBSTUDIO_SCREENSHOT_DIR "/language-%1.png").arg(lang));
      }
      app.setLanguage(QStringLiteral("en"));
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
