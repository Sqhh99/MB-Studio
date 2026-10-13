#include "AppController.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QSysInfo>

#include "FormatListModel.h"
#include "GridViewModel.h"
#include "LogModel.h"
#include "SwathViewModel.h"
#include "ToolsViewModel.h"
#include "mbstudio/api/FormatService.h"
#include "mbstudio/api/GridService.h"
#include "mbstudio/api/SwathService.h"
#include "mbstudio/api/ToolService.h"
#include "mbstudio/kernel/Kernel.h"

namespace mbstudio::ui {

namespace {
constexpr int kMaxRecent = 12;
}

AppController::AppController(Kernel *kernel, QObject *parent)
    : QObject(parent),
      kernel_(kernel),
      settings_(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("MB-Studio"), QStringLiteral("mbstudio")) {
  darkTheme_ = settings_.value(QStringLiteral("ui/darkTheme"), true).toBool();
  recentFiles_ = settings_.value(QStringLiteral("files/recent")).toStringList();

  std::shared_ptr<IFormatService> formatService;
  std::shared_ptr<ISwathService> swathService;
  std::shared_ptr<IGridService> gridService;
  std::shared_ptr<IGridRenderService> renderService;
  std::shared_ptr<IToolService> toolService;
  if (kernel_ != nullptr) {
    formatService = kernel_->services().get<IFormatService>();
    swathService = kernel_->services().get<ISwathService>();
    gridService = kernel_->services().get<IGridService>();
    renderService = kernel_->services().get<IGridRenderService>();
    toolService = kernel_->services().get<IToolService>();
  }
  formats_ = new FormatListModel(formatService, this);
  swath_ = new SwathViewModel(swathService, this);
  grid_ = new GridViewModel(gridService, renderService, this);
  tools_ = new ToolsViewModel(toolService, this);
  log_ = new LogModel(kernel_, this);
  tools_->setWorkingDirectory(settings_.value(QStringLiteral("tools/workingDirectory"), QDir::homePath()).toString());
  connect(tools_, &ToolsViewModel::argumentsChanged, this, [this]() {
    settings_.setValue(QStringLiteral("tools/workingDirectory"), tools_->workingDirectory());
  });

  connect(swath_, &SwathViewModel::finished, this, [this](bool ok) {
    if (!ok)
      emit notify(tr("Could not read %1: %2").arg(swath_->fileName(), swath_->error()));
  });
}

AppController::~AppController() = default;

QString AppController::version() const {
  return kernel_ != nullptr ? QString::fromStdString(kernel_->config().version) : QString();
}

QString AppController::mbsystemVersion() const {
  // The mbsystem plugin reports "<plugin version> (MB-System <version>)".
  if (kernel_ != nullptr)
    for (const auto &p : kernel_->plugins().plugins())
      if (p.info.id == "mbsystem") {
        const QString v = QString::fromStdString(p.info.version);
        const int at = v.indexOf(QLatin1String("MB-System "));
        return at >= 0 ? v.mid(at + 10).remove(QLatin1Char(')')) : v;
      }
  return QString();
}

QString AppController::platformName() const {
  return QStringLiteral("%1 (%2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
}

void AppController::setDarkTheme(bool dark) {
  if (dark == darkTheme_)
    return;
  darkTheme_ = dark;
  settings_.setValue(QStringLiteral("ui/darkTheme"), dark);
  emit darkThemeChanged();
}

void AppController::setCurrentPage(int page) {
  if (page == currentPage_ || page < HomePage || page > PluginsPage)
    return;
  currentPage_ = page;
  emit currentPageChanged();
}

QVariantList AppController::plugins() const {
  QVariantList out;
  if (kernel_ == nullptr)
    return out;
  for (const auto &p : kernel_->plugins().plugins()) {
    QVariantMap m;
    m[QStringLiteral("id")] = QString::fromStdString(p.info.id);
    m[QStringLiteral("name")] = QString::fromStdString(p.info.name);
    m[QStringLiteral("version")] = QString::fromStdString(p.info.version);
    m[QStringLiteral("description")] = QString::fromStdString(p.info.description);
    m[QStringLiteral("state")] = QString::fromUtf8(pluginStateName(p.state));
    m[QStringLiteral("error")] = QString::fromStdString(p.error);
    m[QStringLiteral("path")] = QString::fromStdString(p.path);
    out.push_back(m);
  }
  return out;
}

QVariantList AppController::services() const {
  QVariantList out;
  if (kernel_ == nullptr)
    return out;
  for (const auto &s : kernel_->services().list()) {
    QVariantMap m;
    m[QStringLiteral("id")] = QString::fromStdString(s.id);
    m[QStringLiteral("provider")] = QString::fromStdString(s.provider);
    out.push_back(m);
  }
  return out;
}

QVariantList AppController::commands() const {
  QVariantList out;
  if (kernel_ == nullptr)
    return out;
  for (const auto &c : kernel_->commands().list()) {
    QVariantMap m;
    m[QStringLiteral("id")] = QString::fromStdString(c.id);
    m[QStringLiteral("owner")] = QString::fromStdString(c.owner);
    m[QStringLiteral("description")] = QString::fromStdString(c.description);
    out.push_back(m);
  }
  return out;
}

QObject *AppController::formats() const { return formats_; }
QObject *AppController::swath() const { return swath_; }
QObject *AppController::grid() const { return grid_; }
QObject *AppController::tools() const { return tools_; }
QObject *AppController::log() const { return log_; }

bool AppController::isGridFile(const QString &path) {
  const QString suffix = QFileInfo(path).suffix().toLower();
  return suffix == QLatin1String("grd") || suffix == QLatin1String("nc") || suffix == QLatin1String("nc4");
}

int AppController::openFile(const QString &path) {
  if (path.isEmpty())
    return -1;
  const QFileInfo info(path);
  if (!info.exists()) {
    emit notify(tr("File not found: %1").arg(QDir::toNativeSeparators(path)));
    return -1;
  }
  int page = -1;
  if (isGridFile(path)) {
    grid_->load(info.absoluteFilePath());
    page = GridPage;
  } else if (const int format = formats_->identify(path); format > 0) {
    swath_->inspect(info.absoluteFilePath(), format);
    page = InspectPage;
  } else if (format < 0) {
    // a datalist: summarise all of its files with mbinfo
    runToolOn(QStringLiteral("mbinfo"), info.absoluteFilePath());
    tools_->run();
    page = ToolsPage;
  } else {
    emit notify(tr("%1 is not a recognised swath data file or GMT grid").arg(info.fileName()));
    return -1;
  }
  addRecent(info.absoluteFilePath());
  setCurrentPage(page);
  return page;
}

int AppController::openUrl(const QUrl &url) { return openFile(toLocalFile(url)); }

QString AppController::toLocalFile(const QUrl &url) { return url.isLocalFile() ? url.toLocalFile() : url.toString(); }

QUrl AppController::toUrl(const QString &path) { return QUrl::fromLocalFile(path); }

void AppController::addRecent(const QString &path) {
  recentFiles_.removeAll(path);
  recentFiles_.prepend(path);
  while (recentFiles_.size() > kMaxRecent)
    recentFiles_.removeLast();
  settings_.setValue(QStringLiteral("files/recent"), recentFiles_);
  emit recentFilesChanged();
}

void AppController::clearRecentFiles() {
  recentFiles_.clear();
  settings_.remove(QStringLiteral("files/recent"));
  emit recentFilesChanged();
}

void AppController::showInFolder(const QString &path) const {
  const QFileInfo info(path);
#ifdef Q_OS_WIN
  if (info.exists()) {
    QProcess::startDetached(QStringLiteral("explorer.exe"),
                            {QStringLiteral("/select,"), QDir::toNativeSeparators(info.absoluteFilePath())});
    return;
  }
#endif
  QDesktopServices::openUrl(QUrl::fromLocalFile(info.isDir() ? info.absoluteFilePath() : info.absolutePath()));
}

void AppController::openDocumentation() const {
  QDesktopServices::openUrl(QUrl(QStringLiteral("https://www.mbari.org/technology/mb-system/")));
}

void AppController::copyToClipboard(const QString &text) const {
  if (QGuiApplication::clipboard() != nullptr)
    QGuiApplication::clipboard()->setText(text);
}

void AppController::runToolOn(const QString &tool, const QString &path) {
  if (!tools_->selectTool(tool))
    return;
  QString arg = QDir::toNativeSeparators(path);
  if (arg.contains(QLatin1Char(' ')))
    arg = QLatin1Char('"') + arg + QLatin1Char('"');
  tools_->setArguments(QStringLiteral("-I ") + arg);
  tools_->setWorkingDirectory(QFileInfo(path).absolutePath());
  setCurrentPage(ToolsPage);
}

}  // namespace mbstudio::ui
