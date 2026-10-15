// MB-Studio QML UI - the interactive 3D view of a TerrainViewModel.
//
// Mouse: left drag orbits, right or middle drag (or Shift + left) pans, the
// wheel zooms, a double click fits the surface; with the measure or profile
// tool a click picks a point. Keys: 1 top, 2 front, 3 side, 5 orthographic
// or perspective, F fit, arrows rotate, + and - zoom, Escape clears points.
#pragma once

#include <QColor>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QTimer>

namespace mbstudio::ui {

class TerrainViewModel;

class TerrainView : public QQuickPaintedItem {
  Q_OBJECT
  Q_PROPERTY(QObject *model READ model WRITE setModel NOTIFY modelChanged)
  Q_PROPERTY(QColor markerColor READ markerColor WRITE setMarkerColor NOTIFY styleChanged)
  Q_PROPERTY(bool interacting READ interacting NOTIFY interactingChanged)

 public:
  explicit TerrainView(QQuickItem *parent = nullptr);
  void paint(QPainter *painter) override;

  QObject *model() const;
  void setModel(QObject *model);
  QColor markerColor() const { return markerColor_; }
  void setMarkerColor(const QColor &c);
  bool interacting() const { return interacting_; }

 signals:
  void modelChanged();
  void styleChanged();
  void interactingChanged();

 protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void hoverMoveEvent(QHoverEvent *event) override;
  void hoverLeaveEvent(QHoverEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

 private:
  void startInteraction();
  QPointer<TerrainViewModel> model_;
  QColor markerColor_ = QColor(0xFF, 0xC1, 0x07);
  QTimer settle_;
  bool interacting_ = false;
  bool dragging_ = false;
  bool moved_ = false;
  Qt::MouseButton button_ = Qt::NoButton;
  QPointF pressPos_, lastPos_;
};

}  // namespace mbstudio::ui
