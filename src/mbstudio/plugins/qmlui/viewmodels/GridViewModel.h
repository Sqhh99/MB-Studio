// MB-Studio QML UI - a grid document: a gridded bathymetry (GMT grid) as
// colour shaded relief with its quality layers. Loading and rendering run
// off the GUI thread; the rendered image is served to QML through
// GridImageProvider ("image://mbgrid/<document>/<revision>"), so every open
// grid document has its own image. The display settings and the view
// (zoom, pan) are the document's view state, saved with the project.
#pragma once

#include <QFutureWatcher>
#include <QHash>
#include <QImage>
#include <QMutex>
#include <QPointer>
#include <QObject>
#include <QQuickImageProvider>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <memory>

#include "mbstudio/api/GridService.h"
#include "mbstudio/platform/Geodesy.h"

namespace mbstudio::ui {

class GridViewModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
  Q_PROPERTY(bool loaded READ loaded NOTIFY stateChanged)
  Q_PROPERTY(QString error READ error NOTIFY stateChanged)
  Q_PROPERTY(QString path READ path NOTIFY stateChanged)
  Q_PROPERTY(QString fileName READ fileName NOTIFY stateChanged)
  Q_PROPERTY(QVariantMap info READ info NOTIFY stateChanged)
  Q_PROPERTY(QStringList colorMaps READ colorMaps CONSTANT)
  Q_PROPERTY(int colorMap READ colorMap WRITE setColorMap NOTIFY optionsChanged)
  Q_PROPERTY(bool hillshade READ hillshade WRITE setHillshade NOTIFY optionsChanged)
  Q_PROPERTY(double azimuth READ azimuth WRITE setAzimuth NOTIFY optionsChanged)
  Q_PROPERTY(double elevation READ elevation WRITE setElevation NOTIFY optionsChanged)
  Q_PROPERTY(double exaggeration READ exaggeration WRITE setExaggeration NOTIFY optionsChanged)
  Q_PROPERTY(QString imageSource READ imageSource NOTIFY imageChanged)
  Q_PROPERTY(QVariantList legend READ legend NOTIFY optionsChanged)
  Q_PROPERTY(int imageWidth READ imageWidth NOTIFY imageChanged)
  Q_PROPERTY(int imageHeight READ imageHeight NOTIFY imageChanged)
  // valid / empty node counts, emptyPercent, mean, stddev of the main grid
  Q_PROPERTY(QVariantMap statistics READ statistics NOTIFY stateChanged)
  // what is shown: 0 the grid, 1 data count per cell (_num), 2 standard deviation (_sd)
  Q_PROPERTY(int layer READ layer WRITE setLayer NOTIFY optionsChanged)
  Q_PROPERTY(bool hasQualityGrids READ hasQualityGrids NOTIFY stateChanged)
  // hatch cells that hold a value but no data (filled by interpolation)
  Q_PROPERTY(bool showInterpolated READ showInterpolated WRITE setShowInterpolated NOTIFY optionsChanged)
  Q_PROPERTY(double interpolatedPercent READ interpolatedPercent NOTIFY stateChanged)
  Q_PROPERTY(bool autoRange READ autoRange WRITE setAutoRange NOTIFY optionsChanged)
  Q_PROPERTY(double rangeMin READ rangeMin WRITE setRangeMin NOTIFY optionsChanged)
  Q_PROPERTY(double rangeMax READ rangeMax WRITE setRangeMax NOTIFY optionsChanged)
  // the view of the document (zoom factor and pan in pixels)
  Q_PROPERTY(double viewZoom READ viewZoom WRITE setViewZoom NOTIFY viewChanged)
  Q_PROPERTY(double viewPanX READ viewPanX WRITE setViewPanX NOTIFY viewChanged)
  Q_PROPERTY(double viewPanY READ viewPanY WRITE setViewPanY NOTIFY viewChanged)
  Q_PROPERTY(QString viewState READ viewState WRITE setViewState NOTIFY viewStateChanged)
  // the value under the cursor (see probe())
  Q_PROPERTY(QVariantMap lastProbe READ lastProbe NOTIFY probeChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY probeChanged)

 public:
  GridViewModel(std::shared_ptr<IGridService> grids, std::shared_ptr<IGridRenderService> renderer,
                QObject *parent = nullptr);
  ~GridViewModel() override;

  bool busy() const { return loading_ || rendering_; }
  bool loaded() const { return grid_ != nullptr; }
  QString error() const { return error_; }
  QString path() const { return path_; }
  QString fileName() const;
  QVariantMap info() const { return info_; }
  QStringList colorMaps() const;
  int colorMap() const { return static_cast<int>(options_.colorMap); }
  void setColorMap(int value);
  bool hillshade() const { return options_.hillshade; }
  void setHillshade(bool value);
  double azimuth() const { return options_.azimuth; }
  void setAzimuth(double value);
  double elevation() const { return options_.elevation; }
  void setElevation(double value);
  double exaggeration() const { return options_.exaggeration; }
  void setExaggeration(double value);
  QString imageSource() const;
  QVariantList legend() const;  // [{position: 0..1, color: "#rrggbb"}, ...]
  int imageWidth() const;
  int imageHeight() const;
  QVariantMap statistics() const { return statistics_; }
  int layer() const { return layer_; }
  void setLayer(int value);
  bool hasQualityGrids() const { return num_ != nullptr || sd_ != nullptr; }
  bool showInterpolated() const { return showInterpolated_; }
  void setShowInterpolated(bool value);
  double interpolatedPercent() const { return interpolatedPercent_; }
  bool autoRange() const { return options_.autoRange; }
  void setAutoRange(bool value);
  double rangeMin() const { return options_.zmin; }
  void setRangeMin(double value);
  double rangeMax() const { return options_.zmax; }
  void setRangeMax(double value);
  double viewZoom() const { return viewZoom_; }
  void setViewZoom(double value);
  double viewPanX() const { return viewPanX_; }
  void setViewPanX(double value);
  double viewPanY() const { return viewPanY_; }
  void setViewPanY(double value);
  QString viewState() const;
  void setViewState(const QString &json);
  QVariantMap lastProbe() const { return lastProbe_; }
  QString statusText() const;

  Q_INVOKABLE void load(const QString &path);
  Q_INVOKABLE void reload() { load(path_); }
  Q_INVOKABLE void clear();
  // Value under a point given in normalised image coordinates (0..1); it
  // becomes lastProbe (an empty map for a point outside the grid).
  Q_INVOKABLE QVariantMap probe(double u, double v);
  Q_INVOKABLE void clearProbe();
  Q_INVOKABLE bool waitForFinished(int timeoutMs = 30000);

  QImage image() const;  // thread safe (used by the image provider)
  int imageKey() const { return key_; }

 signals:
  void stateChanged();
  void optionsChanged();
  void imageChanged();
  void viewChanged();
  void viewStateChanged();
  void probeChanged();

 private:
  struct Loaded {
    GridLoadResult main;
    std::shared_ptr<const Grid> num, sd;
    GridStatistics statistics;
    long long interpolated = 0;
  };
  void scheduleRender();
  void startRender();
  std::shared_ptr<const Grid> displayed() const;
  std::shared_ptr<IGridService> grids_;
  std::shared_ptr<IGridRenderService> renderer_;
  std::shared_ptr<const Grid> grid_;
  std::shared_ptr<const Grid> num_;
  std::shared_ptr<const Grid> sd_;
  RenderOptions options_;
  int layer_ = 0;
  bool showInterpolated_ = true;
  double interpolatedPercent_ = 0.0;
  QVariantMap statistics_;
  QFutureWatcher<Loaded> loadWatcher_;
  QFutureWatcher<QImage> renderWatcher_;
  QTimer renderTimer_;
  bool loading_ = false;
  bool rendering_ = false;
  bool renderAgain_ = false;
  QString error_;
  QString path_;
  QVariantMap info_;
  mutable QMutex imageMutex_;
  QImage image_;
  int revision_ = 0;
  int key_ = 0;
  double viewZoom_ = 1.0, viewPanX_ = 0.0, viewPanY_ = 0.0;
  QVariantMap lastProbe_;
  platform::MapProjection projection_;
  QString pendingState_;
};

// Serves the rendered images of every grid document ("image://mbgrid/<key>/<revision>").
class GridImageProvider : public QQuickImageProvider {
 public:
  GridImageProvider();
  QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

  static void add(int key, GridViewModel *model);
  static void remove(int key);

 private:
  static QMutex &mutex();
  static QHash<int, QPointer<GridViewModel>> &models();
};

}  // namespace mbstudio::ui
