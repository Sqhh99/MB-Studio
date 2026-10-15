// MB-Studio QML UI - a terrain document: a grid as a lit 3D surface with an
// orbit camera, standard views, orthographic or perspective projection,
// vertical exaggeration, picking of positions and depths, distance
// measurement and profiles. Cells without data are holes; cells filled by
// interpolation (data count 0 in the "_num" quality grid) are stippled.
//
// The surface is drawn by the software renderer (SceneRenderer.h) from two
// levels of detail: a coarse mesh while the camera moves, a finer one when
// it stops. Picking reads the depth buffer and samples the full resolution
// grid, so reported values are those of the grid, not of the display.
#pragma once

#include <QFutureWatcher>
#include <QImage>
#include <QObject>
#include <QPointF>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <memory>

#include "SceneRenderer.h"
#include "mbstudio/api/GridService.h"
#include "mbstudio/platform/Geodesy.h"

namespace mbstudio::ui {

class TerrainViewModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
  Q_PROPERTY(bool loaded READ loaded NOTIFY stateChanged)
  Q_PROPERTY(QString error READ error NOTIFY stateChanged)
  Q_PROPERTY(QString path READ path NOTIFY stateChanged)
  Q_PROPERTY(QString fileName READ fileName NOTIFY stateChanged)
  Q_PROPERTY(QVariantMap info READ info NOTIFY stateChanged)
  Q_PROPERTY(QStringList colorMaps READ colorMaps CONSTANT)
  Q_PROPERTY(int colorMap READ colorMap WRITE setColorMap NOTIFY sceneChanged)
  Q_PROPERTY(double exaggeration READ exaggeration WRITE setExaggeration NOTIFY sceneChanged)
  Q_PROPERTY(bool perspective READ perspective WRITE setPerspective NOTIFY sceneChanged)
  Q_PROPERTY(double yaw READ yaw WRITE setYaw NOTIFY sceneChanged)
  Q_PROPERTY(double pitch READ pitch WRITE setPitch NOTIFY sceneChanged)
  // the grid holds depths (positive down) rather than elevations
  Q_PROPERTY(bool depthsPositive READ depthsPositive WRITE setDepthsPositive NOTIFY sceneChanged)
  Q_PROPERTY(bool showInterpolated READ showInterpolated WRITE setShowInterpolated NOTIFY sceneChanged)
  Q_PROPERTY(bool hasQualityGrids READ hasQualityGrids NOTIFY stateChanged)
  // 0 navigate (hover shows the value), 1 measure, 2 profile
  Q_PROPERTY(int tool READ tool WRITE setTool NOTIFY toolChanged)
  Q_PROPERTY(QVariantMap probe READ probe NOTIFY probeChanged)
  Q_PROPERTY(QVariantMap measurement READ measurement NOTIFY markersChanged)
  Q_PROPERTY(QVariantList profile READ profile NOTIFY markersChanged)
  Q_PROPERTY(int markerCount READ markerCount NOTIFY markersChanged)
  Q_PROPERTY(QVariantList legend READ legend NOTIFY sceneChanged)
  Q_PROPERTY(int revision READ revision NOTIFY sceneChanged)
  Q_PROPERTY(QString viewState READ viewState WRITE setViewState NOTIFY viewStateChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY probeChanged)

 public:
  TerrainViewModel(std::shared_ptr<IGridService> grids, std::shared_ptr<IGridRenderService> renderer,
                   QObject *parent = nullptr);
  ~TerrainViewModel() override;

  bool busy() const { return loading_; }
  bool loaded() const { return grid_ != nullptr; }
  QString error() const { return error_; }
  QString path() const { return path_; }
  QString fileName() const;
  QVariantMap info() const { return info_; }
  QStringList colorMaps() const;
  int colorMap() const { return colorMap_; }
  void setColorMap(int value);
  double exaggeration() const { return exaggeration_; }
  void setExaggeration(double value);
  bool perspective() const { return camera_.perspective; }
  void setPerspective(bool value);
  double yaw() const { return camera_.yaw; }
  void setYaw(double value);
  double pitch() const { return camera_.pitch; }
  void setPitch(double value);
  bool depthsPositive() const { return depthsPositive_; }
  void setDepthsPositive(bool value);
  bool showInterpolated() const { return showInterpolated_; }
  void setShowInterpolated(bool value);
  bool hasQualityGrids() const { return num_ != nullptr; }
  int tool() const { return tool_; }
  void setTool(int value);
  QVariantMap probe() const { return probe_; }
  QVariantMap measurement() const;
  QVariantList profile() const { return profile_; }
  int markerCount() const { return static_cast<int>(markers_.size()); }
  QVariantList legend() const;
  int revision() const { return revision_; }
  QString viewState() const;
  void setViewState(const QString &json);
  QString statusText() const;

  Q_INVOKABLE void load(const QString &path);
  Q_INVOKABLE bool waitForFinished(int timeoutMs = 30000);
  // Camera: fit the surface, standard views (0 top, 1 front: from the south,
  // 2 side: from the east), orbit and pan by screen pixels, zoom by a factor.
  Q_INVOKABLE void fit();
  Q_INVOKABLE void standardView(int which);
  Q_INVOKABLE void orbit(double dx, double dy);
  Q_INVOKABLE void pan(double dx, double dy, double width, double height);
  Q_INVOKABLE void zoom(double factor);
  // Value under a pixel of the last rendered frame (empty map: no surface).
  Q_INVOKABLE QVariantMap pick(double x, double y) const;
  Q_INVOKABLE void hover(double x, double y);
  Q_INVOKABLE void leave();
  // Add a point under a pixel to the measurement or profile (two points).
  Q_INVOKABLE bool addMarker(double x, double y);
  Q_INVOKABLE void clearMarkers();

  // Render the surface (fast: the coarse level of detail).
  QImage render(int width, int height, bool fast);
  // Screen positions of the markers in the last rendered frame.
  QVector<QPointF> markerPositions() const;

 signals:
  void stateChanged();
  void sceneChanged();
  void toolChanged();
  void probeChanged();
  void markersChanged();
  void viewStateChanged();

 private:
  struct Lod {
    int step = 1;
    int nx = 0, ny = 0;
    std::vector<float> z;           // grid value per vertex (NaN: no data)
    std::vector<std::uint8_t> interpolated;
    std::vector<std::uint32_t> color;  // lit colour per vertex
  };
  struct Loaded {
    GridLoadResult main;
    std::shared_ptr<const Grid> num;
  };
  void buildLods();
  void colorLod(Lod &lod) const;
  void recolor();
  double worldZ(double value) const { return (depthsPositive_ ? -value : value) * exaggeration_; }
  scene::Vec3 vertex(const Lod &lod, int i, int j) const;
  // grid coordinates of a world point (x, y in metres from the origin)
  bool worldToGrid(double x, double y, double &gx, double &gy) const;
  bool sample(double gx, double gy, double &value) const;
  void gridToGeographic(double gx, double gy, double &lon, double &lat) const;
  double sceneRadius() const;
  void changed();
  void updateProfile();

  std::shared_ptr<IGridService> grids_;
  std::shared_ptr<IGridRenderService> renderer_;
  std::shared_ptr<const Grid> grid_;
  std::shared_ptr<const Grid> num_;
  platform::MapProjection projection_;
  QFutureWatcher<Loaded> watcher_;
  bool loading_ = false;
  QString error_;
  QString path_;
  QVariantMap info_;
  // metres per grid unit (x, y) and the origin (grid centre)
  double metersX_ = 1.0, metersY_ = 1.0;
  double originX_ = 0.0, originY_ = 0.0;
  Lod fine_, coarse_;
  int colorMap_ = 0;
  double exaggeration_ = 1.0;
  bool depthsPositive_ = false;
  bool showInterpolated_ = true;
  scene::Camera camera_;
  int tool_ = 0;
  QVariantMap probe_;
  std::vector<scene::Vec3> markers_;  // world points (z without exaggeration)
  QVariantList profile_;
  scene::Frame frame_;
  std::unique_ptr<scene::View> view_;  // of the last frame
  const Lod *frameLod_ = nullptr;
  int revision_ = 0;
  QString pendingState_;  // view state given before the grid was loaded
};

}  // namespace mbstudio::ui
