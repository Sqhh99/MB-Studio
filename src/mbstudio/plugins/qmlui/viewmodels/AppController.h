// MB-Studio QML UI - application level state exposed to QML as the "App"
// singleton: settings, recent files, plugin information, the open project,
// background jobs, the processing panels, the workbench documents and the
// actions contributed by plugins.
#pragma once

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

namespace mbstudio {
class Kernel;
}

namespace mbstudio::ui {

class FormatListModel;
class JobListModel;
class JsonTranslator;
class GridViewModel;
class LogModel;
class ProcessingViewModel;
class ProjectViewModel;
class SwathViewModel;
class ToolsViewModel;
class WorkspaceModel;

class AppController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString version READ version CONSTANT)
  Q_PROPERTY(QString mbsystemVersion READ mbsystemVersion CONSTANT)
  Q_PROPERTY(QString platformName READ platformName CONSTANT)
  Q_PROPERTY(bool darkTheme READ darkTheme WRITE setDarkTheme NOTIFY darkThemeChanged)
  // user interface language: "" follows the system, otherwise "en", "ja", "zh_CN"
  Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
  Q_PROPERTY(QString effectiveLanguage READ effectiveLanguage NOTIFY languageChanged)
  Q_PROPERTY(QVariantList languages READ languages CONSTANT)
  Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged)
  Q_PROPERTY(QVariantList plugins READ plugins CONSTANT)
  Q_PROPERTY(QVariantList services READ services CONSTANT)
  Q_PROPERTY(QVariantList commands READ commands CONSTANT)
  // menu actions contributed by plugins: [{id, title, menu, command, shortcut, icon}]
  Q_PROPERTY(QVariantList actions READ actions NOTIFY languageChanged)
  Q_PROPERTY(QObject *formats READ formats CONSTANT)
  Q_PROPERTY(QObject *swath READ swath CONSTANT)
  Q_PROPERTY(QObject *grid READ grid CONSTANT)
  Q_PROPERTY(QObject *tools READ tools CONSTANT)
  Q_PROPERTY(QObject *log READ log CONSTANT)
  Q_PROPERTY(QObject *project READ project CONSTANT)
  Q_PROPERTY(QObject *jobs READ jobs CONSTANT)
  Q_PROPERTY(QObject *processing READ processing CONSTANT)
  Q_PROPERTY(QObject *workspaces READ workspaces CONSTANT)

 public:
  explicit AppController(Kernel *kernel, QObject *parent = nullptr);
  ~AppController() override;

  QString version() const;
  QString mbsystemVersion() const;
  QString platformName() const;
  bool darkTheme() const { return darkTheme_; }
  void setDarkTheme(bool dark);
  QString language() const { return language_; }
  void setLanguage(const QString &language);
  QString effectiveLanguage() const { return effectiveLanguage_; }
  QVariantList languages() const;
  QStringList recentFiles() const { return recentFiles_; }
  QVariantList plugins() const;
  QVariantList services() const;
  QVariantList commands() const;
  QVariantList actions() const;
  QObject *formats() const;
  QObject *swath() const;
  QObject *grid() const;
  QObject *tools() const;
  QObject *log() const;
  QObject *project() const;
  QObject *jobs() const;
  QObject *processing() const;
  QObject *workspaces() const;

  FormatListModel *formatModel() const { return formats_; }
  SwathViewModel *swathModel() const { return swath_; }
  GridViewModel *gridModel() const { return grid_; }
  ToolsViewModel *toolsModel() const { return tools_; }
  LogModel *logModel() const { return log_; }
  ProjectViewModel *projectModel() const { return project_; }
  JobListModel *jobModel() const { return jobs_; }
  ProcessingViewModel *processingModel() const { return processing_; }
  WorkspaceModel *workspaceModel() const { return workspaces_; }

  // Open a file where it belongs: projects (.mbsproj or a project folder)
  // are opened, grids in the grid view, swath files in the line inspector,
  // datalists summarised with mbinfo. Returns the workspace used, or "".
  Q_INVOKABLE QString openFile(const QString &path);
  Q_INVOKABLE QString openUrl(const QUrl &url);
  Q_INVOKABLE static QString toLocalFile(const QUrl &url);
  Q_INVOKABLE static QUrl toUrl(const QString &path);
  Q_INVOKABLE static bool isGridFile(const QString &path);
  Q_INVOKABLE static bool isProjectFile(const QString &path);
  Q_INVOKABLE void openGrid(const QString &path);
  Q_INVOKABLE void openWorkspace(const QString &workspace);
  Q_INVOKABLE void inspectLine(const QString &lineId);
  Q_INVOKABLE void clearRecentFiles();
  Q_INVOKABLE void showInFolder(const QString &path) const;
  Q_INVOKABLE void openPath(const QString &path) const;
  // The HTML help of the MB-System programs in the interface language
  // (the MB-System web site if it is not installed).
  Q_INVOKABLE void openDocumentation() const;
  Q_INVOKABLE void openWebSite() const;
  Q_INVOKABLE void copyToClipboard(const QString &text) const;
  // Run an MB-System program on a file from the inspector (opens Programs).
  Q_INVOKABLE void runToolOn(const QString &tool, const QString &path);
  // Trigger a contributed action or a command: "ui:..." commands are
  // handled by the QML shell (uiAction), others run as kernel commands.
  Q_INVOKABLE void trigger(const QString &command);
  // Small persistent values of the shell (panel sizes, visibility).
  Q_INVOKABLE QVariant uiValue(const QString &key, const QVariant &fallback = QVariant()) const;
  Q_INVOKABLE void setUiValue(const QString &key, const QVariant &value);

 signals:
  void darkThemeChanged();
  void languageChanged();
  void recentFilesChanged();
  void notify(const QString &message);
  // a front end action ("newProject", "panel:grid", ...) for the QML shell
  void uiAction(const QString &name);

 private:
  void addRecent(const QString &path);
  void applyLanguage();
  void onProjectChanged();
  void onJobFinished(qint64 id, const QString &kind, const QString &state, const QString &title, const QString &error);
  Kernel *kernel_;
  QSettings settings_;
  bool darkTheme_ = true;
  QString language_;
  QString effectiveLanguage_ = QStringLiteral("en");
  JsonTranslator *translator_ = nullptr;
  QStringList recentFiles_;
  FormatListModel *formats_ = nullptr;
  SwathViewModel *swath_ = nullptr;
  GridViewModel *grid_ = nullptr;
  ToolsViewModel *tools_ = nullptr;
  LogModel *log_ = nullptr;
  ProjectViewModel *project_ = nullptr;
  JobListModel *jobs_ = nullptr;
  ProcessingViewModel *processing_ = nullptr;
  WorkspaceModel *workspaces_ = nullptr;
  QString openProjectFile_;
  QTimer saveWorkspaceTimer_;
  bool restoringWorkspace_ = false;
};

}  // namespace mbstudio::ui
