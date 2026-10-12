// MB-Studio QML UI - display of a gridded bathymetry (GMT grid) as colour
// shaded relief. Loading and rendering run off the GUI thread; the rendered
// image is served to QML through GridImageProvider ("image://mbgrid/...").
#pragma once

#include <QFutureWatcher>
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QQuickImageProvider>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <memory>

#include "mbstudio/api/GridService.h"

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

  Q_INVOKABLE void load(const QString &path);
  Q_INVOKABLE void clear();
  // Value under a point given in normalised image coordinates (0..1).
  Q_INVOKABLE QVariantMap probe(double u, double v) const;
  Q_INVOKABLE bool waitForFinished(int timeoutMs = 30000);

  QImage image() const;  // thread safe (used by the image provider)

 signals:
  void stateChanged();
  void optionsChanged();
  void imageChanged();

 private:
  void scheduleRender();
  void startRender();
  std::shared_ptr<IGridService> grids_;
  std::shared_ptr<IGridRenderService> renderer_;
  std::shared_ptr<const Grid> grid_;
  RenderOptions options_;
  QFutureWatcher<GridLoadResult> loadWatcher_;
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
};

class GridImageProvider : public QQuickImageProvider {
 public:
  explicit GridImageProvider(GridViewModel *model);
  QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

 private:
  GridViewModel *model_;
};

}  // namespace mbstudio::ui
