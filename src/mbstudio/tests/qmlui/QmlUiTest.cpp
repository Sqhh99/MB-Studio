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
#include "viewmodels/SceneRenderer.h"
#include "viewmodels/SoundingEditorModel.h"
#include "viewmodels/SwathViewModel.h"
#include "viewmodels/TerrainViewModel.h"
#include "viewmodels/ToolsViewModel.h"
#include "viewmodels/Translator.h"
#include "viewmodels/TrackPlot.h"
#include "viewmodels/VariantListModel.h"
#include "viewmodels/WindowChrome.h"
#include "viewmodels/WorkspaceModel.h"

using namespace mbstudio;

namespace {

QString testData() {
  return QStringLiteral(MBSTUDIO_SOURCE_DIR) + QStringLiteral("/test/utilities/testdata/mb21/TN136HS.309.snipped.mb21");
}

// the same pings in MB-System's processing format (editable, processable)
QString testData71() {
  return QStringLiteral(MBSTUDIO_SOURCE_DIR) + QStringLiteral("/test/utilities/testdata/mb71/TN136HS.309.snipped.mb71");
}

// A document object with a view state (for the workspace model).
class StatefulDocument : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString viewState READ viewState WRITE setViewState NOTIFY viewStateChanged)
 public:
  using QObject::QObject;
  QString viewState() const { return state_; }
  void setViewState(const QString &s) {
    state_ = s;
    emit viewStateChanged();
  }
 signals:
  void viewStateChanged();

 private:
  QString state_;
};

// the number of opaque pixels of an image
int opaquePixels(const QImage &image) {
  int n = 0;
  for (int y = 0; y < image.height(); y++)
    for (int x = 0; x < image.width(); x++)
      n += qAlpha(image.pixel(x, y)) > 0 ? 1 : 0;
  return n;
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

    // the view state of the document (display and view) goes back into a new one
    model.setViewZoom(3.0);
    model.setViewPanX(25.0);
    model.setLayer(0);
    const QString state = model.viewState();
    QVERIFY(state.contains(QStringLiteral("\"zoom\":3")));
    ui::GridViewModel restored(std::make_shared<FakeGrids>(), std::make_shared<mbsystem::GridRenderService>());
    restored.setViewState(state);  // before the grid is loaded: kept until then
    restored.load(QStringLiteral("fake.grd"));
    QVERIFY(restored.waitForFinished(30000));
    QCOMPARE(restored.colorMap(), 3);
    QCOMPARE(restored.hillshade(), false);
    QCOMPARE(restored.viewZoom(), 3.0);
    QCOMPARE(restored.viewPanX(), 25.0);
    // each document serves its own image
    QVERIFY(restored.imageSource() != model.imageSource());
    QVERIFY(restored.imageKey() != model.imageKey());
    ui::GridImageProvider provider;
    QSize size;
    const QString id = restored.imageSource().mid(QStringLiteral("image://mbgrid/").size());
    QCOMPARE(provider.requestImage(id, &size, QSize()).size(), QSize(20, 10));
    model.probe(0.5, 0.5);
    QVERIFY(model.statusText().contains(QStringLiteral("lon")));
    model.clearProbe();
    QVERIFY(model.statusText().isEmpty());

    model.load(QStringLiteral("bad.grd"));
    QVERIFY(model.waitForFinished(30000));
    QCOMPARE(model.error(), QStringLiteral("cannot open grid"));
  }

  void sceneRendererDrawsAndPicks() {
    namespace sc = ui::scene;
    sc::Frame frame;
    frame.resize(100, 80);
    frame.clear(0u);
    const double x[3] = {10, 90, 50}, y[3] = {10, 10, 70}, depth[3] = {5, 5, 5};
    const std::uint32_t red[3] = {0xFFFF0000u, 0xFFFF0000u, 0xFFFF0000u};
    sc::drawTriangle(frame, x, y, depth, red, 7);
    QCOMPARE(frame.id[30 * 100 + 50], 7);
    QCOMPARE(frame.color[30 * 100 + 50], 0xFFFF0000u);
    QCOMPARE(frame.id[75 * 100 + 5], -1);
    // a nearer point wins, a farther one does not
    sc::drawPoint(frame, 50, 30, 1.0, 0xFF00FF00u, 3, 9);
    QCOMPARE(frame.id[30 * 100 + 50], 9);
    sc::drawPoint(frame, 50, 30, 10.0, 0xFF0000FFu, 3, 11);
    QCOMPARE(frame.id[30 * 100 + 50], 9);
    // the camera: the target in the middle of the view, unprojection inverts projection
    sc::Camera camera;
    camera.distance = 100.0;
    for (const bool perspective : {true, false}) {
      camera.perspective = perspective;
      const sc::View view(camera, 200, 100, 50.0);
      double sx = 0, sy = 0, d = 0;
      QVERIFY(view.project(sc::Vec3{0, 0, 0}, sx, sy, d));
      QVERIFY(std::fabs(sx - 100.0) < 1e-6 && std::fabs(sy - 50.0) < 1e-6);
      QVERIFY(view.project(sc::Vec3{10, -5, 3}, sx, sy, d));
      const sc::Vec3 back = view.unproject(sx, sy, d);
      QVERIFY(std::fabs(back.x - 10) < 1e-6 && std::fabs(back.y + 5) < 1e-6 && std::fabs(back.z - 3) < 1e-6);
    }
    QCOMPARE(sc::niceExaggeration(10000.0, 100.0), 20.0);
    QCOMPARE(sc::niceExaggeration(100.0, 1000.0), 1.0);
  }

  void terrainModelRendersPicksAndMeasures() {
    ui::TerrainViewModel model(std::make_shared<FakeGrids>(), std::make_shared<mbsystem::GridRenderService>());
    model.load(QStringLiteral("fake.grd"));
    QVERIFY(model.waitForFinished(30000));
    QVERIFY(model.loaded());
    QCOMPARE(model.depthsPositive(), false);  // elevations (negative below sea level)
    QVERIFY(model.exaggeration() >= 1.0);
    model.standardView(0);  // from above
    QImage image = model.render(400, 300, false);
    QCOMPARE(image.size(), QSize(400, 300));
    QVERIFY(opaquePixels(image) > 400 * 300 / 10);
    // the value under the centre comes from the grid
    const QVariantMap centre = model.pick(200, 150);
    QVERIFY(!centre.isEmpty());
    QVERIFY(centre.value(QStringLiteral("valid")).toBool());
    const double z = centre.value(QStringLiteral("z")).toDouble();
    QVERIFY(z >= -500.0 && z <= -265.0);
    QVERIFY(std::fabs(centre.value(QStringLiteral("lon")).toDouble() + 124.4) < 0.06);
    QVERIFY(model.pick(1, 1).isEmpty() || model.pick(1, 1).contains(QStringLiteral("lon")));
    // two points: distance, height difference, profile
    model.setTool(1);
    QVERIFY(model.addMarker(150, 150));
    QVERIFY(model.addMarker(250, 150));
    QCOMPARE(model.markerCount(), 2);
    const QVariantMap m = model.measurement();
    QVERIFY(m.value(QStringLiteral("horizontal")).toDouble() > 100.0);
    QVERIFY(model.profile().size() > 10);
    QCOMPARE(model.markerPositions().size(), 2);
    model.clearMarkers();
    QCOMPARE(model.markerCount(), 0);
    // camera: orbit, standard views, projection; the view state comes back
    model.orbit(40, 10);
    model.setPerspective(false);
    model.setExaggeration(5.0);
    const QString state = model.viewState();
    ui::TerrainViewModel other(std::make_shared<FakeGrids>(), std::make_shared<mbsystem::GridRenderService>());
    other.setViewState(state);
    other.load(QStringLiteral("fake.grd"));
    QVERIFY(other.waitForFinished(30000));
    QCOMPARE(other.perspective(), false);
    QCOMPARE(other.exaggeration(), 5.0);
    QVERIFY(std::fabs(other.yaw() - model.yaw()) < 1e-6);
    QVERIFY(opaquePixels(other.render(300, 200, true)) > 0);  // the coarse level while moving
    model.standardView(2);
    QCOMPARE(model.yaw(), 90.0);
  }

  // The editing loop through the document: select in 3D, flag, undo, save
  // as a project job; the line then needs processing.
  void soundingEditorSavesThroughTheProject() {
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
    const QString root = freshFolder(QStringLiteral("editor")) + QStringLiteral("/survey");
    QVERIFY(project->createProject(root, QStringLiteral("Survey")));
    QDir().mkpath(root + QStringLiteral("/raw"));
    QVERIFY(QFile::copy(testData71(), root + QStringLiteral("/raw/line.mb71")));
    QCOMPARE(project->addData({root + QStringLiteral("/raw")}), 1);
    QTRY_COMPARE_WITH_TIMEOUT(project->inspectedCount(), 1, 60000);

    const int tab = app.openEditor({QStringLiteral("L1")});
    QVERIFY(tab >= 0);
    QCOMPARE(app.workspaceModel()->currentWorkspace(), QStringLiteral("editor"));
    auto *editor = qobject_cast<ui::SoundingEditorModel *>(app.workspaceModel()->document(tab));
    QVERIFY(editor != nullptr);
    QVERIFY(editor->waitForFinished(60000));
    QVERIFY2(editor->loaded(), qPrintable(editor->error()));
    QCOMPARE(editor->pingCount(), 2);
    QVERIFY(editor->soundingCount() > 10);
    const int flaggedBefore = editor->flaggedCount();

    // a rectangle around everything in a top view
    editor->standardView(0);
    QVERIFY(opaquePixels(editor->render(400, 300, false)) > 0);
    QVERIFY(editor->selectRect(0, 0, 400, 300) > 0);
    const int flagged = editor->flagSelected();
    QVERIFY(flagged > 0);
    QCOMPARE(editor->editCount(), flagged);
    QCOMPARE(editor->selectedCount(), 0);
    QVERIFY(editor->canUndo());
    QVERIFY(editor->undo());
    QCOMPARE(editor->editCount(), 0);
    QVERIFY(editor->redo());
    QCOMPARE(editor->editCount(), flagged);
    // the identity of every edit: line, ping time and multiplicity, beam
    const auto edits = editor->pendingEdits();
    QCOMPARE(static_cast<int>(edits.size()), 1);
    QCOMPARE(static_cast<int>(edits.begin()->second.size()), flagged);
    QCOMPARE(QString::fromStdString(edits.begin()->first), QStringLiteral("L1"));
    // the sounding under a pixel and its ping
    const QVariantMap info = editor->soundingInfo(0);
    QCOMPARE(info.value(QStringLiteral("lineId")).toString(), QStringLiteral("L1"));
    QVERIFY(info.contains(QStringLiteral("beam")));
    QVERIFY(info.value(QStringLiteral("time")).toString().startsWith(QStringLiteral("19")) ||
            info.value(QStringLiteral("time")).toString().startsWith(QStringLiteral("20")));
    const QVariantList beams = editor->pingBeams(0);
    QVERIFY(!beams.isEmpty());

    QSignalSpy saved(editor, &ui::SoundingEditorModel::saved);
    const qint64 job = editor->save();
    QVERIFY2(job > 0, "the edits could not be saved");
    QVERIFY(editor->saving());
    QVERIFY(app.jobModel()->waitFor(job, 60000));
    QTRY_COMPARE_WITH_TIMEOUT(saved.count(), 1, 10000);
    QCOMPARE(editor->editCount(), 0);
    QVERIFY(!app.unsavedEdits());
    QTRY_COMPARE_WITH_TIMEOUT(project->lineInfo(QStringLiteral("L1")).value(QStringLiteral("status")).toString(),
                              QStringLiteral("processing-required"), 10000);
    QCOMPARE(project->nextAction().value(QStringLiteral("kind")).toString(), QStringLiteral("process"));

    // the saved edits come back when the lines are read again
    editor->reload();
    QVERIFY(editor->waitForFinished(60000));
    QCOMPARE(editor->flaggedCount(), flaggedBefore + flagged);
    QCOMPARE(editor->editCount(), 0);
    // a beam in the ping view
    const int index = editor->pingBeams(0).first().toMap().value(QStringLiteral("index")).toInt();
    editor->toggleBeam(index);
    QCOMPARE(editor->editCount(), 1);
    editor->revert();
    QCOMPARE(editor->editCount(), 0);
  }

  // The system title bar is the default (the frameless window of 0.3.0
  // could not be created on some Windows systems); MB-Studio's own title
  // bar is opt-in.
  void windowChromeDefaultsToTheSystemTitleBar() {
    qunsetenv("MBSTUDIO_CUSTOM_TITLEBAR");
    {
      QQuickWindow window;
      ui::WindowChrome chrome;
      chrome.setWindow(&window);
      QVERIFY(!chrome.active());
      QVERIFY(!chrome.nativeFrame());
      QVERIFY(!(window.flags() & Qt::FramelessWindowHint));
    }
    qputenv("MBSTUDIO_CUSTOM_TITLEBAR", "1");
    {
      QQuickWindow window;
      ui::WindowChrome chrome;
      chrome.setWindow(&window);
      QVERIFY(chrome.active());
    }
    qputenv("MBSTUDIO_NATIVE_TITLEBAR", "1");  // always wins
    {
      QQuickWindow window;
      ui::WindowChrome chrome;
      chrome.setWindow(&window);
      QVERIFY(!chrome.active());
    }
    qunsetenv("MBSTUDIO_NATIVE_TITLEBAR");
    qunsetenv("MBSTUDIO_CUSTOM_TITLEBAR");
  }

  void windowChromeHitTesting() {
    using C = ui::WindowChrome;
    const QSizeF size(1000, 700);
    QCOMPARE(C::hitTest(QPointF(2, 300), size, 6, false, false, false), C::Left);
    QCOMPARE(C::hitTest(QPointF(997, 300), size, 6, false, false, false), C::Right);
    QCOMPARE(C::hitTest(QPointF(500, 2), size, 6, false, true, false), C::Top);
    QCOMPARE(C::hitTest(QPointF(500, 698), size, 6, false, false, false), C::Bottom);
    QCOMPARE(C::hitTest(QPointF(3, 8), size, 6, false, true, false), C::TopLeft);  // corners are larger
    QCOMPARE(C::hitTest(QPointF(995, 695), size, 6, false, false, false), C::BottomRight);
    QCOMPARE(C::hitTest(QPointF(500, 15), size, 6, false, true, false), C::Caption);
    QCOMPARE(C::hitTest(QPointF(500, 15), size, 6, false, true, true), C::Client);  // a button in the title bar
    QCOMPARE(C::hitTest(QPointF(500, 300), size, 6, false, false, false), C::Client);
    // a maximised window is not resized from its edges
    QCOMPARE(C::hitTest(QPointF(2, 15), size, 6, true, true, false), C::Caption);
    QCOMPARE(C::hitTest(QPointF(2, 300), size, 6, true, false, false), C::Client);
    // restoring onto the screens that exist now
    const QRect primary(0, 0, 1920, 1040);
    const QList<QRect> screens{primary, QRect(1920, 0, 1280, 1000)};
    QCOMPARE(C::fitToScreens(QRect(100, 100, 1200, 800), screens, primary), QRect(100, 100, 1200, 800));
    QCOMPARE(C::fitToScreens(QRect(2000, 50, 1000, 700), screens, primary), QRect(2000, 50, 1000, 700));
    const QRect moved = C::fitToScreens(QRect(4000, 50, 1000, 700), screens, primary);  // a monitor that is gone
    QVERIFY(primary.contains(moved));
    QCOMPARE(moved.size(), QSize(1000, 700));
    const QRect big = C::fitToScreens(QRect(10, 10, 5000, 3000), screens, primary);
    QVERIFY(big.width() <= 1920 && big.height() <= 1040);
    QCOMPARE(C::fitToScreens(QRect(), screens, primary).size(), QSize(1440, 900));
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
    QCOMPARE(model.available().size(), 12);
    // every grid document gets its own document object with a view state
    int created = 0;
    model.setDocumentFactory([&created](const QString &workspace, const QString &argument, QObject *parent) -> QObject * {
      if (workspace != QLatin1String("grid"))
        return nullptr;
      created++;
      auto *doc = new StatefulDocument(parent);
      doc->setObjectName(argument);
      return doc;
    });
    QCOMPARE(model.open(QStringLiteral("nothing")), -1);
    QCOMPARE(model.open(QStringLiteral("start")), 0);
    QCOMPARE(model.open(QStringLiteral("map")), 1);
    QCOMPARE(model.open(QStringLiteral("start")), 0);  // singleton: activated again
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.open(QStringLiteral("grid"), QStringLiteral("/data/a.grd"), QStringLiteral("Grid: a.grd")), 2);
    QCOMPARE(model.open(QStringLiteral("grid"), QStringLiteral("/data/b.grd"), QStringLiteral("Grid: b.grd")), 3);
    QCOMPARE(model.open(QStringLiteral("grid"), QStringLiteral("/data/a.grd")), 2);  // the same document
    QCOMPARE(created, 2);
    QVERIFY(model.document(2) != model.document(3));
    QCOMPARE(model.document(3)->objectName(), QStringLiteral("/data/b.grd"));
    QCOMPARE(model.currentDocument(), model.document(2));
    qobject_cast<StatefulDocument *>(model.document(3))->setViewState(QStringLiteral("{\"zoom\":2}"));
    QCOMPARE(model.open(QStringLiteral("test.extra"), QStringLiteral("1")), 4);
    QCOMPARE(model.open(QStringLiteral("test.extra"), QStringLiteral("2")), 5);  // not a singleton
    QCOMPARE(model.currentWorkspace(), QStringLiteral("test.extra"));
    // split: grid b on the right, compared with grid a on the left
    QVERIFY(!model.split());
    model.moveToGroup(3, 1);
    QVERIFY(model.split());
    QCOMPARE(model.group(3), 1);
    QCOMPARE(model.secondaryIndex(), 3);
    QCOMPARE(model.currentIndex(), 3);
    model.setCurrentIndex(2);
    QCOMPARE(model.primaryIndex(), 2);
    QCOMPARE(model.secondaryIndex(), 3);  // both shown
    QCOMPARE(model.data(model.index(2), ui::WorkspaceModel::ShownRole).toBool(), true);
    QCOMPARE(model.data(model.index(3), ui::WorkspaceModel::ShownRole).toBool(), true);
    QCOMPARE(model.data(model.index(1), ui::WorkspaceModel::ShownRole).toBool(), false);
    QSignalSpy stateSpy(&model, &ui::WorkspaceModel::stateChanged);
    qobject_cast<StatefulDocument *>(model.document(2))->setViewState(QStringLiteral("{\"zoom\":5}"));
    QCOMPARE(stateSpy.count(), 1);
    const QString state = model.saveState();
    QVERIFY(state.contains(QStringLiteral("zoom")));
    model.close(1);  // the map cannot be closed
    QCOMPARE(model.count(), 6);
    model.close(5);
    QCOMPARE(model.count(), 5);
    // closing the last document on the right joins the split
    model.close(3);
    QVERIFY(!model.split());
    QCOMPARE(model.count(), 4);

    ui::WorkspaceModel restored(&kernel);
    created = 0;
    restored.setDocumentFactory([&created](const QString &workspace, const QString &, QObject *parent) -> QObject * {
      if (workspace != QLatin1String("grid"))
        return nullptr;
      created++;
      return new StatefulDocument(parent);
    });
    restored.restoreState(state);
    QCOMPARE(restored.count(), 6);
    QCOMPARE(created, 2);
    QVERIFY(restored.split());
    QCOMPARE(restored.group(3), 1);
    QCOMPARE(restored.argument(3), QStringLiteral("/data/b.grd"));
    // the documents get their view state back
    QCOMPARE(restored.document(2)->property("viewState").toString(), QStringLiteral("{\"zoom\":5}"));
    QCOMPARE(restored.document(3)->property("viewState").toString(), QStringLiteral("{\"zoom\":2}"));
    QCOMPARE(restored.primaryIndex(), 2);
    QCOMPARE(restored.secondaryIndex(), 3);
    restored.unsplit();
    QVERIFY(!restored.split());
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

    // a project job runs again as a new attempt of the project history
    const int historyBefore = project->history()->property("count").toInt();
    const qint64 again = jobs->retry(id);
    QVERIFY(again > 0);
    QVERIFY(jobs->waitFor(again, 120000));
    QTRY_COMPARE_WITH_TIMEOUT(project->history()->property("count").toInt(), historyBefore + 1, 10000);
    const QVariantMap newest = qobject_cast<ui::VariantListModel *>(project->history())->get(0);
    QCOMPARE(newest.value(QStringLiteral("attempt")).toInt(), 2);
    QVERIFY(newest.value(QStringLiteral("canRetry")).toBool());
    QCOMPARE(project->nextAction().value(QStringLiteral("kind")).toString(), QString());  // up to date
    QCOMPARE(project->lineVersions(QStringLiteral("L1")).size(), 1);
    // the recorded request fills the panel again
    processing->setKind(QStringLiteral("clean"));
    QVERIFY(processing->loadRequest(static_cast<qint64>(newest.value(QStringLiteral("sequence")).toDouble())));
    QCOMPARE(processing->kind(), QStringLiteral("grid"));
    QCOMPARE(processing->value(QStringLiteral("spacing")).toDouble(), 20.0);

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
    auto *swath = qobject_cast<ui::SwathViewModel *>(app.workspaceModel()->currentDocument());
    QVERIFY(swath != nullptr);
    QVERIFY(swath->waitForFinished(30000));
    QVERIFY(swath->loaded());
    QVERIFY(!app.colorMaps().isEmpty());
    QCOMPARE(app.openEditor({}), -1);  // no project: nothing to edit
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
    // the title bar drawn by MB-Studio (opt-in) with its window buttons
    qputenv("MBSTUDIO_CUSTOM_TITLEBAR", "1");
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
      engine.addImageProvider(QStringLiteral("mbgrid"), new ui::GridImageProvider());
      engine.load(QUrl(QStringLiteral("qrc:/mbstudio/qml/Main.qml")));
      QVERIFY2(!engine.rootObjects().isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
      auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
      QVERIFY(window != nullptr);
      QTRY_VERIFY_WITH_TIMEOUT(window->isVisible(), 5000);  // shown by the window chrome
      window->resize(1440, 900);
      const QString shots = QStringLiteral(MBSTUDIO_SCREENSHOT_DIR);
      QDir().mkpath(shots);
      auto shoot = [&](const QString &name) {
        QTest::qWait(200);
        const QImage shot = window->grabWindow();
        QVERIFY(!shot.isNull());
        shot.save(shots + QStringLiteral("/") + name + QStringLiteral(".png"));
      };
      auto item = [&](const QString &name) -> QQuickItem * {
        QList<QQuickItem *> found;
        findItems(window->contentItem(), name, found);
        return found.isEmpty() ? nullptr : found.first();
      };
      QCOMPARE(app.workspaceModel()->currentWorkspace(), QStringLiteral("start"));
      shoot(QStringLiteral("01-start"));

      // the title bar drawn by MB-Studio: its controls take the mouse, its
      // empty space moves the window
      QQuickItem *titleBar = item(QStringLiteral("titleBar"));
      QVERIFY(titleBar != nullptr);
      QQuickItem *closeButton = item(QStringLiteral("closeButton"));
      QVERIFY(closeButton != nullptr);
      const QPointF onClose = closeButton->mapToScene(QPointF(closeButton->width() / 2, closeButton->height() / 2));
      QVERIFY(ui::WindowChrome::onControl(titleBar, onClose));
      QQuickItem *title = item(QStringLiteral("windowTitle"));
      QVERIFY(title != nullptr);
      QVERIFY(!ui::WindowChrome::onControl(titleBar, title->mapToScene(QPointF(2, title->height() / 2))));
      QVERIFY(item(QStringLiteral("menuBar")) != nullptr);

      // a project with real data, gridded through the processing panel
      auto *project = app.projectModel();
      const QString root = freshFolder(QStringLiteral("workbench")) + QStringLiteral("/EM survey");
      QVERIFY(project->createProject(root, QStringLiteral("EM survey")));
      QDir().mkpath(root + QStringLiteral("/raw"));
      QVERIFY(QFile::copy(testData71(), root + QStringLiteral("/raw/TN136HS.309.snipped.mb71")));
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

      // a grid document with its quality layers; its settings are in the
      // properties panel, not in a sidebar of the view
      const QString grid = project->newestProductOfJob(id);
      QVERIFY(!grid.isEmpty());
      const int gridTab = app.openGrid(grid);
      auto *gridDoc = qobject_cast<ui::GridViewModel *>(app.workspaceModel()->document(gridTab));
      QVERIFY(gridDoc != nullptr);
      QVERIFY(gridDoc->waitForFinished(30000));
      QVERIFY2(gridDoc->loaded(), qPrintable(gridDoc->error()));
      QVERIFY(gridDoc->hasQualityGrids());
      QTest::qWait(100);
      QVERIFY(item(QStringLiteral("gridColorMap")) != nullptr);
      shoot(QStringLiteral("05-grid"));
      gridDoc->setLayer(1);
      QVERIFY(gridDoc->waitForFinished(30000));
      shoot(QStringLiteral("06-grid-count"));
      gridDoc->setLayer(0);

      // two grids side by side: the grid and its data count grid, independent documents
      QString countGrid = grid;
      countGrid.replace(QStringLiteral(".grd"), QStringLiteral("_num.grd"));
      const int countTab = app.openGrid(countGrid, 1);
      QVERIFY(app.workspaceModel()->split());
      auto *countDoc = qobject_cast<ui::GridViewModel *>(app.workspaceModel()->document(countTab));
      QVERIFY(countDoc != nullptr && countDoc != gridDoc);
      QVERIFY(countDoc->waitForFinished(30000));
      countDoc->setColorMap(2);
      QVERIFY(countDoc->waitForFinished(30000));
      QCOMPARE(gridDoc->colorMap(), 0);  // the other document is unchanged
      shoot(QStringLiteral("07-split-two-grids"));

      // the 3D terrain beside the grid
      app.workspaceModel()->close(countTab);
      const int terrainTab = app.openTerrain(grid, 1);
      auto *terrain = qobject_cast<ui::TerrainViewModel *>(app.workspaceModel()->document(terrainTab));
      QVERIFY(terrain != nullptr);
      QVERIFY(terrain->waitForFinished(30000));
      QVERIFY2(terrain->loaded(), qPrintable(terrain->error()));
      QTRY_VERIFY_WITH_TIMEOUT(item(QStringLiteral("terrainView")) != nullptr, 5000);
      QTRY_VERIFY_WITH_TIMEOUT(item(QStringLiteral("terrainToolbar")) != nullptr, 5000);
      QVERIFY(item(QStringLiteral("terrainView"))->isVisible());
      QVERIFY(item(QStringLiteral("terrainView"))->width() > 0);
      QVERIFY(item(QStringLiteral("terrainView"))->height() > 0);
      terrain->setTool(2);
      shoot(QStringLiteral("08-split-grid-and-3d"));
      app.workspaceModel()->unsplit();

      // every processing panel renders, including the GMT figure
      for (const QString &kind : ui::ProcessingViewModel::kinds()) {
        app.trigger(QStringLiteral("ui:panel:") + kind);
        QTest::qWait(60);
      }
      shoot(QStringLiteral("09-figure-panel"));

      // the line in a version, then the sounding editor
      const int lineTab = app.inspectLine(QStringLiteral("L1"), QString());
      auto *line = qobject_cast<ui::SwathViewModel *>(app.workspaceModel()->document(lineTab));
      QVERIFY(line != nullptr);
      QVERIFY(line->waitForFinished(30000));
      QCOMPARE(line->version(), QStringLiteral("original"));
      QCOMPARE(line->versions().size(), 1);
      shoot(QStringLiteral("10-inspect"));

      project->selectIds({QStringLiteral("L1")});
      const int editorTab = app.openEditor({});
      QVERIFY(editorTab >= 0);
      auto *editor = qobject_cast<ui::SoundingEditorModel *>(app.workspaceModel()->document(editorTab));
      QVERIFY(editor != nullptr);
      QVERIFY(editor->waitForFinished(60000));
      QVERIFY2(editor->loaded(), qPrintable(editor->error()));
      editor->setTool(1);
      QTest::qWait(150);
      QVERIFY(item(QStringLiteral("pointCloudView")) != nullptr);
      QVERIFY(item(QStringLiteral("pingView")) != nullptr);
      QVERIFY(item(QStringLiteral("editorToolbar")) != nullptr);
      editor->selectAll();
      QVERIFY(editor->selectedCount() > 0);
      QVERIFY(editor->flagSelected() > 0);
      QVERIFY(app.unsavedEdits());
      shoot(QStringLiteral("11-editor"));
      QVERIFY(editor->undo());
      QVERIFY(!app.unsavedEdits());

      // workspace presets change the docks, not the documents
      const int documents = app.workspaceModel()->count();
      for (const char *preset : {"editing", "modeling", "cartography", "delivery", "overview"}) {
        app.trigger(QStringLiteral("ui:preset:") + QLatin1String(preset));
        QTest::qWait(60);
      }
      QVERIFY(app.workspaceModel()->count() >= documents);
      app.trigger(QStringLiteral("ui:preset:cartography"));
      shoot(QStringLiteral("12-cartography-preset"));
      app.trigger(QStringLiteral("ui:preset:overview"));

      const char *others[] = {"history", "programs", "formats", "plugins", "start"};
      int n = 13;
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
      // (documents are created by Loaders: walk the item tree)
      QList<QQuickItem *> helpViews;
      findItems(window->contentItem(), QStringLiteral("helpView"), helpViews);
      QCOMPARE(helpViews.size(), 1);
      QQuickItem *helpView = helpViews.first();
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
