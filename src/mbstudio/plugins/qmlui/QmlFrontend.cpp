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
#include "viewmodels/FormatListModel.h"
#include "viewmodels/GridViewModel.h"
#include "viewmodels/JobListModel.h"
#include "viewmodels/LogModel.h"
#include "viewmodels/MapView.h"
#include "viewmodels/ProcessingViewModel.h"
#include "viewmodels/ProjectViewModel.h"
#include "viewmodels/SwathViewModel.h"
#include "viewmodels/ToolsViewModel.h"
#include "viewmodels/TrackPlot.h"
#include "viewmodels/VariantListModel.h"
#include "viewmodels/WorkspaceModel.h"
#include "mbstudio/kernel/Kernel.h"

namespace mbstudio::ui {

namespace {

const char *kOwner = "qmlui";

WorkspaceContribution workspace(const char *id, const char *title, const char *icon, const char *category,
                                const char *file, int order) {
  WorkspaceContribution w;
  w.id = id;
  w.title = title;
  w.icon = icon;
  w.category = category;
  w.qmlUrl = std::string("qrc:/mbstudio/qml/workspaces/") + file;
  w.owner = kOwner;
  w.order = order;
  w.singleton = true;
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
      workspace("grid", "Grid", "terrain", "view", "GridWorkspace.qml", 20),
      workspace("inspect", "Line inspector", "waves", "view", "InspectWorkspace.qml", 30),
      workspace("history", "Processing history", "history", "view", "HistoryWorkspace.qml", 40),
      workspace("programs", "Programs", "code", "tools", "ProgramsWorkspace.qml", 60),
      workspace("formats", "Formats", "list", "tools", "FormatsWorkspace.qml", 70),
      workspace("plugins", "Plugins", "extension", "tools", "PluginsWorkspace.qml", 80),
  };
  const std::vector<ActionContribution> actions = {
      action("app.openFile", "Open file…", "file", "ui:openFile", "Ctrl+O", "folderOpen", 30),
      action("app.closeProject", "Close project", "file", "ui:closeProject", "", "close", 40),
      action("app.quit", "Quit", "file", "ui:quit", "Ctrl+Q", "close", 99),
      action("view.start", "Start", "view", "ui:workspace:start", "Ctrl+1", "home", 10),
      action("view.map", "Map", "view", "ui:workspace:map", "Ctrl+2", "map", 20),
      action("view.history", "Processing history", "view", "ui:workspace:history", "Ctrl+3", "history", 30),
      action("view.theme", "Light or dark theme", "view", "ui:toggleTheme", "", "moon", 80),
      action("tools.palette", "Command palette…", "tools", "ui:palette", "Ctrl+Shift+P", "search", 10),
      action("tools.programs", "Programs", "tools", "ui:workspace:programs", "", "code", 20),
      action("tools.formats", "Formats", "tools", "ui:workspace:formats", "", "list", 30),
      action("tools.plugins", "Plugins", "tools", "ui:workspace:plugins", "", "extension", 40),
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
    engine.addImageProvider(QStringLiteral("mbgrid"), new GridImageProvider(controller.gridModel()));
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
