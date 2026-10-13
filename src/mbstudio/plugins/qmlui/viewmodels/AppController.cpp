#include "AppController.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QFutureWatcher>
#include <QPointer>
#include <QProcess>
#include <QSysInfo>

#include <QtConcurrent/QtConcurrentRun>

#include "FormatListModel.h"
#include "GridViewModel.h"
#include "JobListModel.h"
#include "LogModel.h"
#include "ProcessingViewModel.h"
#include "ProjectViewModel.h"
#include "SwathViewModel.h"
#include "ToolsViewModel.h"
#include "Translator.h"
#include "WorkspaceModel.h"
#include "mbstudio/api/FormatService.h"
#include "mbstudio/api/GridService.h"
#include "mbstudio/api/HelpService.h"
#include "mbstudio/api/JobService.h"
#include "mbstudio/api/ProcessingService.h"
#include "mbstudio/api/ProjectService.h"
#include "mbstudio/api/SwathService.h"
#include "mbstudio/api/ToolService.h"
#include "mbstudio/kernel/Kernel.h"
#include "mbstudio/platform/Paths.h"

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
  language_ = settings_.value(QStringLiteral("ui/language")).toString();

  std::shared_ptr<IFormatService> formatService;
  std::shared_ptr<ISwathService> swathService;
  std::shared_ptr<IGridService> gridService;
  std::shared_ptr<IGridRenderService> renderService;
  std::shared_ptr<IToolService> toolService;
  std::shared_ptr<IHelpService> helpService;
  std::shared_ptr<IJobService> jobService;
  std::shared_ptr<IProjectService> projectService;
  std::shared_ptr<IProcessingService> processingService;
  if (kernel_ != nullptr) {
    jobService = kernel_->services().get<IJobService>();
    projectService = kernel_->services().get<IProjectService>();
    processingService = kernel_->services().get<IProcessingService>();
    formatService = kernel_->services().get<IFormatService>();
    swathService = kernel_->services().get<ISwathService>();
    gridService = kernel_->services().get<IGridService>();
    renderService = kernel_->services().get<IGridRenderService>();
    toolService = kernel_->services().get<IToolService>();
    helpService = kernel_->services().get<IHelpService>();
  }
  formats_ = new FormatListModel(formatService, this);
  swath_ = new SwathViewModel(swathService, this);
  grid_ = new GridViewModel(gridService, renderService, this);
  tools_ = new ToolsViewModel(toolService, helpService, this);
  log_ = new LogModel(kernel_, this);
  jobs_ = new JobListModel(kernel_, jobService, this);
  project_ = new ProjectViewModel(kernel_, projectService, gridService, renderService, this);
  processing_ = new ProcessingViewModel(projectService, processingService, project_, this);
  workspaces_ = new WorkspaceModel(kernel_, this);
  connect(project_, &ProjectViewModel::notify, this, &AppController::notify);
  connect(processing_, &ProcessingViewModel::notify, this, &AppController::notify);
  connect(project_, &ProjectViewModel::projectChanged, this, &AppController::onProjectChanged);
  connect(jobs_, &JobListModel::jobFinished, this, &AppController::onJobFinished);
  // the open documents are part of the project
  saveWorkspaceTimer_.setSingleShot(true);
  saveWorkspaceTimer_.setInterval(500);
  connect(&saveWorkspaceTimer_, &QTimer::timeout, this, [this]() {
    if (project_->isOpen())
      project_->setWorkspaceState(workspaces_->saveState());
  });
  auto scheduleSave = [this]() {
    if (!restoringWorkspace_ && project_->isOpen())
      saveWorkspaceTimer_.start();
  };
  connect(workspaces_, &WorkspaceModel::countChanged, this, scheduleSave);
  connect(workspaces_, &WorkspaceModel::currentChanged, this, scheduleSave);
  tools_->setWorkingDirectory(settings_.value(QStringLiteral("tools/workingDirectory"), QDir::homePath()).toString());
  connect(tools_, &ToolsViewModel::argumentsChanged, this, [this]() {
    settings_.setValue(QStringLiteral("tools/workingDirectory"), tools_->workingDirectory());
  });

  connect(swath_, &SwathViewModel::finished, this, [this](bool ok) {
    if (!ok)
      emit notify(tr("Could not read %1: %2").arg(swath_->fileName(), swath_->error()));
  });

  translator_ = new JsonTranslator(this);
  applyLanguage();
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

void AppController::setLanguage(const QString &language) {
  const QString value = language == QLatin1String("system") ? QString() : language;
  if (value == language_)
    return;
  language_ = value;
  settings_.setValue(QStringLiteral("ui/language"), language_);
  applyLanguage();
}

QVariantList AppController::languages() const {
  QVariantList out;
  for (const auto &l : supportedLanguages())
    out << QVariantMap{{QStringLiteral("code"), l.code}, {QStringLiteral("name"), l.nativeName}};
  return out;
}

void AppController::applyLanguage() {
  // MBSTUDIO_LANG overrides the setting (tests, screenshots, support)
  const QString forced = qEnvironmentVariable("MBSTUDIO_LANG");
  const QString effective = resolveLanguage(forced.isEmpty() ? language_ : forced);
  if (QCoreApplication::instance() != nullptr) {
    QCoreApplication::removeTranslator(translator_);
    if (effective != QLatin1String("en") && translator_->loadLanguage(effective))
      QCoreApplication::installTranslator(translator_);
  }
  effectiveLanguage_ = effective;
  tools_->setLanguage(effective);
  workspaces_->reload();
  if (kernel_ != nullptr)
    kernel_->log(LogLevel::Info, "qmlui", "user interface language: " + effective.toStdString());
  emit languageChanged();
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
QObject *AppController::project() const { return project_; }
QObject *AppController::jobs() const { return jobs_; }
QObject *AppController::processing() const { return processing_; }
QObject *AppController::workspaces() const { return workspaces_; }

QVariantList AppController::actions() const {
  QVariantList out;
  if (kernel_ == nullptr)
    return out;
  for (const auto &a : kernel_->contributions().actions()) {
    QVariantMap m;
    m[QStringLiteral("id")] = QString::fromStdString(a.id);
    m[QStringLiteral("title")] = QCoreApplication::translate("Contribution", a.title.c_str());
    m[QStringLiteral("menu")] = QString::fromStdString(a.menu);
    m[QStringLiteral("command")] = QString::fromStdString(a.commandId);
    m[QStringLiteral("shortcut")] = QString::fromStdString(a.shortcut);
    m[QStringLiteral("icon")] = QString::fromStdString(a.icon);
    m[QStringLiteral("owner")] = QString::fromStdString(a.owner);
    out.push_back(m);
  }
  return out;
}


bool AppController::isGridFile(const QString &path) {
  const QString suffix = QFileInfo(path).suffix().toLower();
  return suffix == QLatin1String("grd") || suffix == QLatin1String("nc") || suffix == QLatin1String("nc4");
}

bool AppController::isProjectFile(const QString &path) {
  return path.endsWith(QLatin1String(".mbsproj"), Qt::CaseInsensitive);
}

QString AppController::openFile(const QString &path) {
  if (path.isEmpty())
    return QString();
  const QFileInfo info(path);
  if (!info.exists()) {
    emit notify(tr("File not found: %1").arg(QDir::toNativeSeparators(path)));
    return QString();
  }
  if (info.isDir()) {
    // a project folder opens the project; any other folder is added to it
    if (!QDir(info.absoluteFilePath()).entryList({QStringLiteral("*.mbsproj")}, QDir::Files).isEmpty()) {
      if (!project_->openProject(info.absoluteFilePath()))
        return QString();
      openWorkspace(QStringLiteral("map"));
      return QStringLiteral("map");
    }
    if (project_->isOpen()) {
      project_->addData({info.absoluteFilePath()});
      openWorkspace(QStringLiteral("map"));
      return QStringLiteral("map");
    }
    emit notify(tr("Create or open a project to add the survey data in %1").arg(info.fileName()));
    return QString();
  }
  if (isProjectFile(path)) {
    if (!project_->openProject(info.absoluteFilePath()))
      return QString();
    openWorkspace(QStringLiteral("map"));
    return QStringLiteral("map");
  }
  QString workspace;
  if (isGridFile(path)) {
    openGrid(info.absoluteFilePath());
    workspace = QStringLiteral("grid");
  } else if (const int format = formats_->identify(path); format > 0) {
    swath_->inspect(info.absoluteFilePath(), format);
    workspaces_->open(QStringLiteral("inspect"), QString(), tr("Line: %1").arg(info.fileName()));
    workspace = QStringLiteral("inspect");
  } else if (format < 0) {
    // a datalist: summarise all of its files with mbinfo
    runToolOn(QStringLiteral("mbinfo"), info.absoluteFilePath());
    tools_->run();
    workspace = QStringLiteral("programs");
  } else {
    emit notify(tr("%1 is not a recognised swath data file or GMT grid").arg(info.fileName()));
    return QString();
  }
  addRecent(info.absoluteFilePath());
  return workspace;
}

QString AppController::openUrl(const QUrl &url) { return openFile(toLocalFile(url)); }

void AppController::openGrid(const QString &path) {
  if (path.isEmpty())
    return;
  grid_->load(path);
  workspaces_->open(QStringLiteral("grid"), path, tr("Grid: %1").arg(QFileInfo(path).fileName()));
}

void AppController::openWorkspace(const QString &workspace) {
  if (workspaces_->open(workspace) < 0)
    emit notify(tr("The view %1 is not available").arg(workspace));
}

void AppController::inspectLine(const QString &lineId) {
  const QVariantMap line = project_->lineInfo(lineId);
  const QString path = line.value(QStringLiteral("path")).toString();
  if (path.isEmpty())
    return;
  swath_->inspect(QDir::fromNativeSeparators(path), line.value(QStringLiteral("format")).toInt());
  workspaces_->open(QStringLiteral("inspect"), QString(),
                    tr("Line: %1").arg(line.value(QStringLiteral("name")).toString()));
}

void AppController::onProjectChanged() {
  const QString file = project_->file();
  if (file == openProjectFile_)
    return;
  openProjectFile_ = file;
  if (file.isEmpty())
    return;
  // the documents that were open when the project was saved
  restoringWorkspace_ = true;
  workspaces_->restoreState(project_->workspaceState());
  const int map = workspaces_->indexOf(QStringLiteral("map"));
  if (map < 0)
    workspaces_->open(QStringLiteral("map"));
  else
    workspaces_->setCurrentIndex(map);
  restoringWorkspace_ = false;
}

void AppController::onJobFinished(qint64 id, const QString &kind, const QString &state, const QString &title,
                                  const QString &error) {
  if (state == QLatin1String("failed")) {
    emit notify(tr("%1 failed: %2").arg(title, error));
    return;
  }
  if (state != QLatin1String("completed"))
    return;
  if ((kind == QLatin1String("grid") || kind == QLatin1String("mosaic")) &&
      !project_->newestProductOfJob(id).isEmpty()) {
    emit notify(tr("%1 finished; the result is on the map").arg(title));
    return;
  }
  emit notify(tr("%1 finished").arg(title));
}

void AppController::trigger(const QString &command) {
  if (command.startsWith(QLatin1String("ui:"))) {
    emit uiAction(command.mid(3));
    return;
  }
  if (kernel_ == nullptr || command.isEmpty())
    return;
  // kernel commands may run programs: keep the interface responsive
  Kernel *kernel = kernel_;
  const std::string id = command.toStdString();
  auto *watcher = new QFutureWatcher<CommandResult>(this);
  connect(watcher, &QFutureWatcher<CommandResult>::finished, this, [this, watcher, command]() {
    const CommandResult r = watcher->result();
    watcher->deleteLater();
    if (kernel_ != nullptr)
      kernel_->log(r.ok ? LogLevel::Info : LogLevel::Warning, "qmlui",
                   command.toStdString() + (r.ok ? " done" : " failed") + (r.message.empty() ? "" : ": " + r.message));
    emit notify(r.ok ? tr("%1 done").arg(command) : tr("%1 failed: %2").arg(command, QString::fromStdString(r.message)));
  });
  watcher->setFuture(QtConcurrent::run([kernel, id]() { return kernel->commands().execute(id); }));
}

QVariant AppController::uiValue(const QString &key, const QVariant &fallback) const {
  return settings_.value(QStringLiteral("layout/") + key, fallback);
}

void AppController::setUiValue(const QString &key, const QVariant &value) {
  settings_.setValue(QStringLiteral("layout/") + key, value);
}

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

void AppController::openPath(const QString &path) const {
  QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::fromNativeSeparators(path)));
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
  const QString root = QString::fromStdString(platform::joinPath(platform::installPrefix(), "share/doc/mbstudio/help"));
  const QString index = root + QLatin1Char('/') + effectiveLanguage_ + QStringLiteral("/index.html");
  if (QFileInfo::exists(index))
    QDesktopServices::openUrl(QUrl::fromLocalFile(index));
  else
    openWebSite();
}

void AppController::openWebSite() const {
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
  workspaces_->open(QStringLiteral("programs"));
}

}  // namespace mbstudio::ui
