// MB-Studio QML UI - application level state exposed to QML as the "App"
// singleton: settings, recent files, plugin information, the open project,
// background jobs, the processing panels, the workbench documents and the
// actions contributed by plugins.
//
// Documents: every grid, 3D terrain view, line inspector, sounding editor
// and figure tab owns its own document object (GridViewModel,
// TerrainViewModel, SwathViewModel, SoundingEditorModel), created by the
// document factory of the workspace model, so documents of the same kind
// are independent and can be compared side by side.
#pragma once

#include <QHash>
#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <memory>

namespace mbstudio {
class Kernel;
class IGridService;
class IGridRenderService;
class ISwathService;
class ISoundingService;
class IProjectService;
}  // namespace mbstudio

namespace mbstudio::ui {

class FormatListModel;
class JobListModel;
class JsonTranslator;
class LogModel;
class ProcessingViewModel;
class ProjectViewModel;
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
  Q_PROPERTY(QStringList colorMaps READ colorMaps CONSTANT)
  Q_PROPERTY(QObject *formats READ formats CONSTANT)
  Q_PROPERTY(QObject *tools READ tools CONSTANT)
  Q_PROPERTY(QObject *log READ log CONSTANT)
  Q_PROPERTY(QObject *project READ project CONSTANT)
  Q_PROPERTY(QObject *jobs READ jobs CONSTANT)
  Q_PROPERTY(QObject *processing READ processing CONSTANT)
  Q_PROPERTY(QObject *workspaces READ workspaces CONSTANT)
  // sounding edits that are not saved yet (in any editor)
  Q_PROPERTY(bool unsavedEdits READ unsavedEdits NOTIFY unsavedEditsChanged)
  // cursor position of the active view, for the status bar
  Q_PROPERTY(QString cursorText READ cursorText WRITE setCursorText NOTIFY cursorTextChanged)

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
  QStringList colorMaps() const;
  QObject *formats() const;
  QObject *tools() const;
  QObject *log() const;
  QObject *project() const;
  QObject *jobs() const;
  QObject *processing() const;
  QObject *workspaces() const;
  bool unsavedEdits() const;
  QString cursorText() const { return cursorText_; }
  void setCursorText(const QString &text);

  FormatListModel *formatModel() const { return formats_; }
  ToolsViewModel *toolsModel() const { return tools_; }
  LogModel *logModel() const { return log_; }
  ProjectViewModel *projectModel() const { return project_; }
  JobListModel *jobModel() const { return jobs_; }
  ProcessingViewModel *processingModel() const { return processing_; }
  WorkspaceModel *workspaceModel() const { return workspaces_; }

  // Open a file where it belongs: projects (.mbsproj or a project folder)
  // are opened, grids in a grid document, swath files in a line document,
  // datalists summarised with mbinfo. Returns the workspace used, or "".
  Q_INVOKABLE QString openFile(const QString &path);
  Q_INVOKABLE QString openUrl(const QUrl &url);
  Q_INVOKABLE static QString toLocalFile(const QUrl &url);
  Q_INVOKABLE static QUrl toUrl(const QString &path);
  Q_INVOKABLE static bool isGridFile(const QString &path);
  Q_INVOKABLE static bool isProjectFile(const QString &path);
  // Documents; each returns its tab index (-1 on failure). group -1: the
  // active group, 1: beside the active document (split view).
  Q_INVOKABLE int openGrid(const QString &path, int group = -1);
  Q_INVOKABLE int openTerrain(const QString &path, int group = -1);
  // A survey line in a version (original, preprocessed, processed; "": the
  // most processed one).
  Q_INVOKABLE int inspectLine(const QString &lineId, const QString &version = QString());
  // The sounding editor for survey lines (empty: the selected lines).
  Q_INVOKABLE int openEditor(const QStringList &lineIds = QStringList());
  Q_INVOKABLE int openFigure(const QString &assetId);
  Q_INVOKABLE void openWorkspace(const QString &workspace);
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
  void unsavedEditsChanged();
  void cursorTextChanged();

 private:
  QObject *createDocument(const QString &workspace, const QString &argument, QObject *parent);
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
  std::shared_ptr<IGridService> gridService_;
  std::shared_ptr<IGridRenderService> renderService_;
  std::shared_ptr<ISwathService> swathService_;
  std::shared_ptr<ISoundingService> soundingService_;
  std::shared_ptr<IProjectService> projectService_;
  FormatListModel *formats_ = nullptr;
  ToolsViewModel *tools_ = nullptr;
  LogModel *log_ = nullptr;
  ProjectViewModel *project_ = nullptr;
  JobListModel *jobs_ = nullptr;
  ProcessingViewModel *processing_ = nullptr;
  WorkspaceModel *workspaces_ = nullptr;
  QString openProjectFile_;
  QTimer saveWorkspaceTimer_;
  bool restoringWorkspace_ = false;
  QString cursorText_;
  // map layers of finished jobs whose completion is reported when drawn
  QHash<QString, QString> pendingLayerNotices_;
};

}  // namespace mbstudio::ui
