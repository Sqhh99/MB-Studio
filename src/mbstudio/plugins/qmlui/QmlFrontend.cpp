#include "QmlFrontend.h"

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QtQml>

#include "viewmodels/AppController.h"
#include "viewmodels/EditorViews.h"
#include "viewmodels/FormatListModel.h"
#include "viewmodels/GridViewModel.h"
#include "viewmodels/JobListModel.h"
#include "viewmodels/LogModel.h"
#include "viewmodels/MapView.h"
#include "viewmodels/ProcessingViewModel.h"
#include "viewmodels/ProjectViewModel.h"
#include "viewmodels/SoundingEditorModel.h"
#include "viewmodels/SwathViewModel.h"
#include "viewmodels/TerrainView.h"
#include "viewmodels/TerrainViewModel.h"
#include "viewmodels/ToolsViewModel.h"
#include "viewmodels/TrackPlot.h"
#include "viewmodels/VariantListModel.h"
#include "viewmodels/WindowChrome.h"
#include "viewmodels/WorkspaceModel.h"
#include "mbstudio/kernel/Kernel.h"

namespace mbstudio::ui {

namespace {

const char *kOwner = "qmlui";

WorkspaceContribution workspace(const char *id, const char *title, const char *icon, const char *category,
                                const char *file, int order, bool singleton = true) {
  WorkspaceContribution w;
  w.id = id;
  w.title = title;
  w.icon = icon;
  w.category = category;
  w.qmlUrl = std::string("qrc:/mbstudio/qml/workspaces/") + file;
  w.owner = kOwner;
  w.order = order;
  w.singleton = singleton;
  return w;
}

ActionContribution action(const char *id, const char *title, const char *menu, const char *command,
                          const char *shortcut, const char *icon, int order) {
  return ActionContribution{id, title, menu, command, shortcut, icon, kOwner, order};
}

}  // namespace

bool registerContributions(Kernel &kernel, std::string &error) {
  const std::vector<WorkspaceContribution> workspaces = {
      workspace("start", "Start", "home", "view", "StartWorkspace.qml", 0),
      workspace("map", "Map", "map", "view", "MapWorkspace.qml", 10),
      // documents: one tab per grid, line, editor and figure
      workspace("grid", "Grid", "terrain", "view", "GridWorkspace.qml", 20, false),
      workspace("terrain", "3D terrain", "cube", "analysis", "TerrainWorkspace.qml", 25, false),
      workspace("inspect", "Line inspector", "waves", "view", "InspectWorkspace.qml", 30, false),
      workspace("editor", "Sounding editor", "edit", "edit", "EditorWorkspace.qml", 35, false),
      workspace("figure", "Figure", "image", "analysis", "FigureWorkspace.qml", 38, false),
      workspace("history", "Processing history", "history", "view", "HistoryWorkspace.qml", 40),
      workspace("programs", "Programs", "code", "tools", "ProgramsWorkspace.qml", 60),
      workspace("formats", "Formats", "list", "tools", "FormatsWorkspace.qml", 70),
      workspace("plugins", "Plugins", "extension", "tools", "PluginsWorkspace.qml", 80),
  };
  const std::vector<ActionContribution> actions = {
      action("app.openFile", "Open file…", "file", "ui:openFile", "Ctrl+O", "folderOpen", 30),
      action("app.closeProject", "Close project", "file", "ui:closeProject", "", "close", 40),
      action("app.quit", "Quit", "file", "ui:quit", "Ctrl+Q", "close", 99),
      action("edit.undo", "Undo", "edit", "ui:edit:undo", "Ctrl+Z", "undo", 10),
      action("edit.redo", "Redo", "edit", "ui:edit:redo", "Ctrl+Y", "redo", 11),
      action("edit.flag", "Flag the selected soundings", "edit", "ui:edit:flag", "Del", "flag", 20),
      action("edit.unflag", "Unflag the selected soundings", "edit", "ui:edit:unflag", "Ctrl+U", "check", 21),
      action("edit.selectAll", "Select all soundings", "edit", "ui:edit:selectAll", "Ctrl+A", "check", 30),
      action("edit.clearSelection", "Clear the selection", "edit", "ui:edit:clearSelection", "", "close", 31),
      action("edit.save", "Save the edits", "edit", "ui:edit:save", "Ctrl+S", "save", 40),
      action("edit.editor", "Sounding editor", "edit", "ui:editor", "Ctrl+Shift+E", "edit", 50),
      action("view.start", "Start", "view", "ui:workspace:start", "Ctrl+1", "home", 10),
      action("view.map", "Map", "view", "ui:workspace:map", "Ctrl+2", "map", 20),
      action("view.history", "Processing history", "view", "ui:workspace:history", "Ctrl+3", "history", 30),
      action("view.splitRight", "Split: move the document to the right", "view", "ui:splitRight", "Ctrl+\\", "split", 40),
      action("view.unsplit", "Join the split view", "view", "ui:unsplit", "", "close", 41),
      action("view.theme", "Light or dark theme", "view", "ui:toggleTheme", "", "moon", 80),
      action("analysis.terrain", "3D terrain view", "analysis", "ui:terrain", "Ctrl+T", "cube", 10),
      action("analysis.grid", "Open the grid", "analysis", "ui:gridView", "", "terrain", 20),
      action("window.overview", "Workspace: Overview", "window", "ui:preset:overview", "", "dashboard", 10),
      action("window.editing", "Workspace: Bathymetry editing", "window", "ui:preset:editing", "", "edit", 11),
      action("window.modeling", "Workspace: Modelling", "window", "ui:preset:modeling", "", "terrain", 12),
      action("window.cartography", "Workspace: Cartography", "window", "ui:preset:cartography", "", "map", 13),
      action("window.delivery", "Workspace: Delivery", "window", "ui:preset:delivery", "", "download", 14),
      action("window.saveLayout", "Save the panel layout", "window", "ui:preset:save", "", "save", 20),
      action("window.resetLayout", "Reset the panel layout", "window", "ui:preset:reset", "", "refresh", 21),
      action("tools.palette", "Command palette…", "window", "ui:palette", "Ctrl+Shift+P", "search", 30),
      action("tools.programs", "Programs", "window", "ui:workspace:programs", "", "code", 40),
      action("tools.formats", "Formats", "window", "ui:workspace:formats", "", "list", 41),
      action("tools.plugins", "Plugins", "window", "ui:workspace:plugins", "", "extension", 42),
      action("help.programs", "Program help", "help", "ui:help", "F1", "help", 10),
      action("help.website", "MB-System web site", "help", "ui:website", "", "openInNew", 20),
      action("help.about", "About MB-Studio", "help", "ui:about", "", "info", 30),
  };
  return kernel.contributions().addAll(workspaces, actions, {}, &error);
}

void registerQmlTypes() {
  static bool registered = false;
  if (registered)
    return;
  registered = true;
  const char *uri = "MBStudio.Backend";
  qmlRegisterType<TrackPlot>(uri, 1, 0, "TrackPlot");
  qmlRegisterType<MapView>(uri, 1, 0, "MapView");
  qmlRegisterType<TerrainView>(uri, 1, 0, "TerrainView");
  qmlRegisterType<PointCloudView>(uri, 1, 0, "PointCloudView");
  qmlRegisterType<PingView>(uri, 1, 0, "PingView");
  qmlRegisterType<WindowChrome>(uri, 1, 0, "WindowChrome");
  qmlRegisterUncreatableType<TerrainViewModel>(uri, 1, 0, "TerrainViewModel", QString());
  qmlRegisterUncreatableType<SoundingEditorModel>(uri, 1, 0, "SoundingEditorModel", QString());
  qmlRegisterUncreatableType<AppController>(uri, 1, 0, "AppController", QStringLiteral("use the App singleton"));
  qmlRegisterUncreatableType<FormatListModel>(uri, 1, 0, "FormatListModel", QString());
  qmlRegisterUncreatableType<SwathViewModel>(uri, 1, 0, "SwathViewModel", QString());
  qmlRegisterUncreatableType<GridViewModel>(uri, 1, 0, "GridViewModel", QString());
  qmlRegisterUncreatableType<ToolsViewModel>(uri, 1, 0, "ToolsViewModel", QString());
  qmlRegisterUncreatableType<LogModel>(uri, 1, 0, "LogModel", QString());
  qmlRegisterUncreatableType<ProjectViewModel>(uri, 1, 0, "ProjectViewModel", QString());
  qmlRegisterUncreatableType<JobListModel>(uri, 1, 0, "JobListModel", QString());
  qmlRegisterUncreatableType<ProcessingViewModel>(uri, 1, 0, "ProcessingViewModel", QString());
  qmlRegisterUncreatableType<WorkspaceModel>(uri, 1, 0, "WorkspaceModel", QString());
  qmlRegisterUncreatableType<VariantListModel>(uri, 1, 0, "VariantListModel", QString());
}

int QmlFrontend::run(Kernel &kernel, int argc, char **argv) {
  // High DPI rounding suited to fractional Windows scale factors (125%, 150%).
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
  QGuiApplication app(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("MB-Studio"));
  QGuiApplication::setOrganizationName(QStringLiteral("MB-Studio"));
  QGuiApplication::setApplicationVersion(QString::fromStdString(kernel.config().version));
  QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/mbstudio/images/mbstudio.png")));

  if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
    QQuickStyle::setStyle(QStringLiteral("Material"));
  // compact desktop controls
  if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_MATERIAL_VARIANT"))
    qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", "Dense");

  registerQmlTypes();
  AppController controller(&kernel);
  qmlRegisterSingletonInstance("MBStudio.Backend", 1, 0, "App", &controller);

  int status = 0;
  {
    QQmlApplicationEngine engine;
    // Installed layout: QML modules in <bin>/qml (see MBWindowsDeploy.cmake).
    const QString deployedQml = QCoreApplication::applicationDirPath() + QStringLiteral("/qml");
    if (QFileInfo(deployedQml).isDir())
      engine.addImportPath(deployedQml);
    // QML errors also go to the kernel log (shown on the Log page).
    QObject::connect(&engine, &QQmlEngine::warnings, &engine, [&kernel](const QList<QQmlError> &warnings) {
      for (const QQmlError &w : warnings)
        kernel.log(LogLevel::Warning, "qmlui", w.toString().toStdString());
    });
    // language switch at run time: re-evaluate every qsTr() binding
    QObject::connect(&controller, &AppController::languageChanged, &engine, &QQmlEngine::retranslate);
    engine.addImageProvider(QStringLiteral("mbgrid"), new GridImageProvider());
    engine.load(QUrl(QStringLiteral("qrc:/mbstudio/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
      kernel.log(LogLevel::Error, "qmlui", "the user interface could not be loaded");
      qCritical("MB-Studio: the user interface could not be loaded (QML import paths: %s)",
                qPrintable(engine.importPathList().join(QLatin1Char(';'))));
      return 3;
    }
    kernel.log(LogLevel::Info, "qmlui",
               std::string("user interface started (Qt ") + qVersion() + ", " +
                   QQuickWindow::sceneGraphBackend().toStdString() + ")");

    // Files given on the command line (or by "Open with" in Explorer).
    const QStringList args = QGuiApplication::arguments();
    for (int i = 1; i < args.size(); i++)
      if (!args[i].startsWith(QLatin1Char('-')) && QFileInfo::exists(args[i]))
        controller.openFile(args[i]);
    if (controller.workspaceModel()->count() == 0)
      controller.openWorkspace(QStringLiteral("start"));

    status = QGuiApplication::exec();
  }
  return status;
}

}  // namespace mbstudio::ui
