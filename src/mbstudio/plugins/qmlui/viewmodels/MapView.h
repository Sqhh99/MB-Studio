// MB-Studio QML UI - the project map: grid layers placed by their
// geographic bounds, the navigation tracks of all survey lines (selected ones
// highlighted), graticule and scale bar, with pan, zoom, picking of lines and
// a cursor readout. Drawn with QPainter from the data of a ProjectViewModel.
#pragma once

#include <QColor>
#include <QPointF>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QVariantMap>

#include "ProjectViewModel.h"

namespace mbstudio::ui {

class MapView : public QQuickPaintedItem {
  Q_OBJECT
  Q_PROPERTY(QObject *project READ project WRITE setProject NOTIFY projectChanged)
  Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY styleChanged)
  Q_PROPERTY(QColor trackColor READ trackColor WRITE setTrackColor NOTIFY styleChanged)
  Q_PROPERTY(QColor selectedColor READ selectedColor WRITE setSelectedColor NOTIFY styleChanged)
  Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY styleChanged)
  Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor NOTIFY styleChanged)
  Q_PROPERTY(bool showTracks READ showTracks WRITE setShowTracks NOTIFY styleChanged)
  Q_PROPERTY(bool showLayers READ showLayers WRITE setShowLayers NOTIFY styleChanged)
  Q_PROPERTY(bool hasContent READ hasContent NOTIFY contentChanged)
  Q_PROPERTY(bool cursorInside READ cursorInside NOTIFY cursorChanged)
  Q_PROPERTY(double cursorLon READ cursorLon NOTIFY cursorChanged)
  Q_PROPERTY(double cursorLat READ cursorLat NOTIFY cursorChanged)
  Q_PROPERTY(QVariantMap cursorValue READ cursorValue NOTIFY cursorChanged)
  Q_PROPERTY(QString hoverLine READ hoverLine NOTIFY cursorChanged)

 public:
  explicit MapView(QQuickItem *parent = nullptr);

  void paint(QPainter *painter) override;

  QObject *project() const;
  void setProject(QObject *project);
  QColor backgroundColor() const { return background_; }
  void setBackgroundColor(const QColor &c);
  QColor trackColor() const { return track_; }
  void setTrackColor(const QColor &c);
  QColor selectedColor() const { return selected_; }
  void setSelectedColor(const QColor &c);
  QColor textColor() const { return text_; }
  void setTextColor(const QColor &c);
  QColor gridColor() const { return grid_; }
  void setGridColor(const QColor &c);
  bool showTracks() const { return showTracks_; }
  void setShowTracks(bool value);
  bool showLayers() const { return showLayers_; }
  void setShowLayers(bool value);
  bool hasContent() const;
  bool cursorInside() const { return cursorInside_; }
  double cursorLon() const { return cursorLon_; }
  double cursorLat() const { return cursorLat_; }
  QVariantMap cursorValue() const { return cursorValue_; }
  QString hoverLine() const { return hoverLine_; }

  Q_INVOKABLE void fitAll();
  Q_INVOKABLE void zoomBy(double factor);
  // Graticule spacing for a span (exposed for tests).
  static double niceStep(double span, int maxLines);

 signals:
  void projectChanged();
  void styleChanged();
  void contentChanged();
  void cursorChanged();
  // a click on a track (id empty for a click elsewhere); modifiers: Qt::KeyboardModifiers
  void lineClicked(const QString &id, int modifiers);

 protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void hoverMoveEvent(QHoverEvent *event) override;
  void hoverLeaveEvent(QHoverEvent *event) override;
  void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

 private:
  QPointF toScreen(double lon, double lat) const;
  QPointF toGeo(const QPointF &p) const;
  bool contentBounds(double &w, double &e, double &s, double &n) const;
  QString pick(const QPointF &p, double tolerance) const;
  void onProjectChanged();
  void updateCursor(const QPointF &p);
  QPointer<ProjectViewModel> project_;
  QColor background_ = QColor(0x08, 0x10, 0x18);
  QColor track_ = QColor(0xE3, 0xEA, 0xF1, 170);
  QColor selected_ = QColor(0x1F, 0xB6, 0xC9);
  QColor text_ = QColor(0x84, 0x97, 0xAA);
  QColor grid_ = QColor(128, 128, 128, 60);
  bool showTracks_ = true;
  bool showLayers_ = true;
  double centerLon_ = 0.0, centerLat_ = 0.0;
  double ppd_ = 0.0;  // pixels per degree of latitude; 0: not yet fitted
  bool userMoved_ = false;
  QPointF pressPos_;
  QPointF lastPos_;
  bool dragging_ = false;
  bool cursorInside_ = false;
  double cursorLon_ = 0.0, cursorLat_ = 0.0;
  QVariantMap cursorValue_;
  QString hoverLine_;
  QString hoverId_;
  bool hadContent_ = false;
};

}  // namespace mbstudio::ui
