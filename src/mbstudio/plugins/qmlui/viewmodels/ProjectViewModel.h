// MB-Studio QML UI - the open survey project: survey lines and their
// selection, products and map layers, processing history, and the data the
// map view draws (navigation tracks and rendered grid layers).
#pragma once

#include <QFutureWatcher>
#include <QImage>
#include <QObject>
#include <QPointF>
#include <QSet>
#include <QSettings>
#include <QStringList>
#include <QVector>
#include <memory>

#include "mbstudio/api/GridService.h"
#include "mbstudio/api/ProjectService.h"
#include "mbstudio/kernel/EventBus.h"

namespace mbstudio {
class Kernel;
}

namespace mbstudio::ui {

class VariantListModel;

struct MapTrack {
  QString id;
  QString name;
  QVector<QPointF> points;  // lon, lat
};

struct MapLayer {
  QString asset;
  QString label;
  QImage image;
  GeoBounds bounds;  // of the image (cell edges)
  double opacity = 1.0;
  bool visible = true;
  std::shared_ptr<const Grid> grid;
  QString style;     // colour map and shading the image was rendered with
};

class ProjectViewModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool available READ available CONSTANT)
  Q_PROPERTY(bool isOpen READ isOpen NOTIFY projectChanged)
  Q_PROPERTY(QString name READ name NOTIFY projectChanged)
  Q_PROPERTY(QString file READ file NOTIFY projectChanged)
  Q_PROPERTY(QString root READ root NOTIFY projectChanged)
  Q_PROPERTY(QObject *lines READ lines CONSTANT)
  Q_PROPERTY(QObject *products READ products CONSTANT)
  Q_PROPERTY(QObject *history READ history CONSTANT)
  Q_PROPERTY(int lineCount READ lineCount NOTIFY linesChanged)
  Q_PROPERTY(int inspectedCount READ inspectedCount NOTIFY linesChanged)
  Q_PROPERTY(QStringList selectedIds READ selectedIds NOTIFY selectionChanged)
  Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
  // "All 14 survey lines" / "3 of 14 survey lines"
  Q_PROPERTY(QString selectionText READ selectionText NOTIFY selectionChanged)
  Q_PROPERTY(QVariantMap summary READ summary NOTIFY linesChanged)
  Q_PROPERTY(QStringList recentProjects READ recentProjects NOTIFY recentProjectsChanged)
  // incremented whenever tracks or layers of the map change
  Q_PROPERTY(int mapRevision READ mapRevision NOTIFY mapChanged)
  Q_PROPERTY(bool mapBusy READ mapBusy NOTIFY mapChanged)
  // what the processing chain has done so far (for the workflow guide)
  Q_PROPERTY(QVariantMap progress READ progress NOTIFY progressChanged)

 public:
  ProjectViewModel(Kernel *kernel, std::shared_ptr<IProjectService> project, std::shared_ptr<IGridService> grids,
                   std::shared_ptr<IGridRenderService> renderer, QObject *parent = nullptr);
  ~ProjectViewModel() override;

  bool available() const { return project_ != nullptr; }
  bool isOpen() const { return open_; }
  QString name() const { return name_; }
  QString file() const { return file_; }
  QString root() const { return root_; }
  QObject *lines() const;
  QObject *products() const;
  QObject *history() const;
  int lineCount() const;
  int inspectedCount() const;
  QStringList selectedIds() const;
  int selectedCount() const { return static_cast<int>(selected_.size()); }
  QString selectionText() const;
  QVariantMap summary() const;
  QStringList recentProjects() const { return recent_; }
  int mapRevision() const { return mapRevision_; }
  bool mapBusy() const { return pendingLayers_ > 0 || tracksLoading_; }
  QVariantMap progress() const;
  std::shared_ptr<IProjectService> service() const { return project_; }
  // ids of the selected lines, or empty for "all lines"
  std::vector<std::string> selection() const;
  GeoBounds selectionBounds() const;

  const QVector<MapTrack> &tracks() const { return tracks_; }
  const QVector<MapLayer> &mapLayers() const { return layers_; }
  bool isSelected(const QString &id) const { return selected_.contains(id); }

  Q_INVOKABLE bool createProject(const QString &folder, const QString &name);
  Q_INVOKABLE bool openProject(const QString &path);
  Q_INVOKABLE void closeProject();
  Q_INVOKABLE void clearRecentProjects();
  // Files and folders (paths or file URLs); returns the number of lines added.
  Q_INVOKABLE int addData(const QVariantList &items, bool recursive = true);
  Q_INVOKABLE int removeSelectedLines();
  // mode 0: select only this line, 1: toggle it, 2: extend the range to it
  Q_INVOKABLE void select(const QString &id, int mode = 0);
  Q_INVOKABLE void selectAll();
  Q_INVOKABLE void clearSelection();
  Q_INVOKABLE bool lineSelected(const QString &id) const { return selected_.contains(id); }
  Q_INVOKABLE QVariantMap lineInfo(const QString &id) const;
  Q_INVOKABLE QVariantMap productInfo(const QString &id) const;
  Q_INVOKABLE QString productPath(const QString &id) const;
  Q_INVOKABLE void setLayerVisible(const QString &asset, bool visible);
  Q_INVOKABLE void setLayerOpacity(const QString &asset, double opacity);
  Q_INVOKABLE void setLayerColorMap(const QString &asset, int colorMap);
  Q_INVOKABLE void setLayerHillshade(const QString &asset, bool hillshade);
  Q_INVOKABLE bool removeProduct(const QString &asset, bool deleteFiles);
  Q_INVOKABLE QString addGrid(const QString &path);
  // Value of the topmost visible grid layer at a position.
  Q_INVOKABLE QVariantMap valueAt(double lon, double lat) const;
  Q_INVOKABLE QString workspaceState() const;
  Q_INVOKABLE void setWorkspaceState(const QString &json);
  Q_INVOKABLE QString datalistPath(const QString &which) const;
  Q_INVOKABLE QString newestProductOfJob(qint64 jobId) const;
  Q_INVOKABLE void refresh();
  // Process events until the map tracks and layers are loaded (tests).
  Q_INVOKABLE bool waitForMap(int timeoutMs = 30000);

 signals:
  void projectChanged();
  void linesChanged();
  void selectionChanged();
  void recentProjectsChanged();
  void mapChanged();
  void progressChanged();
  void notify(const QString &message);

 private:
  void onChanged(const QString &what);
  void refreshLines();
  void refreshProducts();
  void refreshHistory();
  void loadTracks();
  void updateLayers();
  void addRecent(const QString &file);
  Kernel *kernel_;
  std::shared_ptr<IProjectService> project_;
  std::shared_ptr<IGridService> grids_;
  std::shared_ptr<IGridRenderService> renderer_;
  EventBus::SubscriptionId subscription_ = 0;
  QSettings settings_;
  bool open_ = false;
  QString name_, file_, root_;
  VariantListModel *lines_ = nullptr;
  VariantListModel *products_ = nullptr;
  VariantListModel *history_ = nullptr;
  std::vector<SurveyLine> lineData_;
  std::vector<ProjectAsset> assetData_;
  std::vector<ProjectLayer> layerData_;
  std::vector<JobRecord> historyData_;
  QSet<QString> selected_;
  QString anchor_;  // for range selection
  QStringList recent_;
  QVector<MapTrack> tracks_;
  QVector<MapLayer> layers_;
  int mapRevision_ = 0;
  int pendingLayers_ = 0;
  bool tracksLoading_ = false;
  int tracksGeneration_ = 0;
  int layerGeneration_ = 0;
};

}  // namespace mbstudio::ui
