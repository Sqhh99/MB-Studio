// MB-Studio QML UI - application level state exposed to QML as the "App"
// singleton: settings, recent files, plugin information and the view models
// of the individual pages.
#pragma once

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QUrl>
#include <QVariantList>

namespace mbstudio {
class Kernel;
}

namespace mbstudio::ui {

class FormatListModel;
class GridViewModel;
class LogModel;
class SwathViewModel;
class ToolsViewModel;

class AppController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString version READ version CONSTANT)
  Q_PROPERTY(QString mbsystemVersion READ mbsystemVersion CONSTANT)
  Q_PROPERTY(QString platformName READ platformName CONSTANT)
  Q_PROPERTY(bool darkTheme READ darkTheme WRITE setDarkTheme NOTIFY darkThemeChanged)
  Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
  Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged)
  Q_PROPERTY(QVariantList plugins READ plugins CONSTANT)
  Q_PROPERTY(QVariantList services READ services CONSTANT)
  Q_PROPERTY(QVariantList commands READ commands CONSTANT)
  Q_PROPERTY(QObject *formats READ formats CONSTANT)
  Q_PROPERTY(QObject *swath READ swath CONSTANT)
  Q_PROPERTY(QObject *grid READ grid CONSTANT)
  Q_PROPERTY(QObject *tools READ tools CONSTANT)
  Q_PROPERTY(QObject *log READ log CONSTANT)

 public:
  enum Page { HomePage = 0, InspectPage, GridPage, ToolsPage, FormatsPage, PluginsPage };
  Q_ENUM(Page)

  explicit AppController(Kernel *kernel, QObject *parent = nullptr);
  ~AppController() override;

  QString version() const;
  QString mbsystemVersion() const;
  QString platformName() const;
  bool darkTheme() const { return darkTheme_; }
  void setDarkTheme(bool dark);
  int currentPage() const { return currentPage_; }
  void setCurrentPage(int page);
  QStringList recentFiles() const { return recentFiles_; }
  QVariantList plugins() const;
  QVariantList services() const;
  QVariantList commands() const;
  QObject *formats() const;
  QObject *swath() const;
  QObject *grid() const;
  QObject *tools() const;
  QObject *log() const;

  FormatListModel *formatModel() const { return formats_; }
  SwathViewModel *swathModel() const { return swath_; }
  GridViewModel *gridModel() const { return grid_; }
  ToolsViewModel *toolsModel() const { return tools_; }
  LogModel *logModel() const { return log_; }

  // Open a file in the matching page: grids in the grid viewer, swath files
  // and datalists in the inspector. Returns the page used, or -1.
  Q_INVOKABLE int openFile(const QString &path);
  Q_INVOKABLE int openUrl(const QUrl &url);
  Q_INVOKABLE static QString toLocalFile(const QUrl &url);
  Q_INVOKABLE static QUrl toUrl(const QString &path);
  Q_INVOKABLE static bool isGridFile(const QString &path);
  Q_INVOKABLE void clearRecentFiles();
  Q_INVOKABLE void showInFolder(const QString &path) const;
  Q_INVOKABLE void openDocumentation() const;
  Q_INVOKABLE void copyToClipboard(const QString &text) const;
  // Run an MB-System program on a file from the inspector (switches to Tools).
  Q_INVOKABLE void runToolOn(const QString &tool, const QString &path);

 signals:
  void darkThemeChanged();
  void currentPageChanged();
  void recentFilesChanged();
  void notify(const QString &message);

 private:
  void addRecent(const QString &path);
  Kernel *kernel_;
  QSettings settings_;
  bool darkTheme_ = true;
  int currentPage_ = HomePage;
  QStringList recentFiles_;
  FormatListModel *formats_ = nullptr;
  SwathViewModel *swath_ = nullptr;
  GridViewModel *grid_ = nullptr;
  ToolsViewModel *tools_ = nullptr;
  LogModel *log_ = nullptr;
};

}  // namespace mbstudio::ui
