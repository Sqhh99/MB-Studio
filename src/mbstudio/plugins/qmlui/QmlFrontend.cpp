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
#include "viewmodels/LogModel.h"
#include "viewmodels/SwathViewModel.h"
#include "viewmodels/ToolsViewModel.h"
#include "viewmodels/TrackPlot.h"
#include "mbstudio/kernel/Kernel.h"

namespace mbstudio::ui {

void registerQmlTypes() {
  static bool registered = false;
  if (registered)
    return;
  registered = true;
  const char *uri = "MBStudio.Backend";
  qmlRegisterType<TrackPlot>(uri, 1, 0, "TrackPlot");
  qmlRegisterUncreatableType<AppController>(uri, 1, 0, "AppController", QStringLiteral("use the App singleton"));
  qmlRegisterUncreatableType<FormatListModel>(uri, 1, 0, "FormatListModel", QString());
  qmlRegisterUncreatableType<SwathViewModel>(uri, 1, 0, "SwathViewModel", QString());
  qmlRegisterUncreatableType<GridViewModel>(uri, 1, 0, "GridViewModel", QString());
  qmlRegisterUncreatableType<ToolsViewModel>(uri, 1, 0, "ToolsViewModel", QString());
  qmlRegisterUncreatableType<LogModel>(uri, 1, 0, "LogModel", QString());
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

    status = QGuiApplication::exec();
  }
  return status;
}

}  // namespace mbstudio::ui
