// MB-Studio QML UI - the views of a SoundingEditorModel.
//
// PointCloudView: the soundings in 3D with the selection tools of the
// model. Left button: the active tool (navigate: orbit and pick a ping;
// rectangle, lasso, brush, point: select); right or middle button: pan;
// wheel: zoom. During a selection Shift adds to and Ctrl removes from the
// selection. Keys: Delete or F flags, U unflags, Escape clears the
// selection, Home fits, 1/2/3 standard views, 5 projection.
//
// PingView: the beams of the current ping across the track (depth down);
// a click flags or unflags a beam, the wheel steps through the pings.
#pragma once

#include <QColor>
#include <QPointer>
#include <QPolygonF>
#include <QQuickPaintedItem>
#include <QTimer>

namespace mbstudio::ui {

class SoundingEditorModel;

class PointCloudView : public QQuickPaintedItem {
  Q_OBJECT
  Q_PROPERTY(QObject *model READ model WRITE setModel NOTIFY modelChanged)
  Q_PROPERTY(QColor toolColor READ toolColor WRITE setToolColor NOTIFY styleChanged)

 public:
  explicit PointCloudView(QQuickItem *parent = nullptr);
  void paint(QPainter *painter) override;
  QObject *model() const;
  void setModel(QObject *model);
  QColor toolColor() const { return toolColor_; }
  void setToolColor(const QColor &c);

 signals:
  void modelChanged();
  void styleChanged();

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
  int gestureMode(Qt::KeyboardModifiers modifiers) const;
  QPointer<SoundingEditorModel> model_;
  QColor toolColor_ = QColor(0xFF, 0xD5, 0x4F);
  QTimer settle_;
  bool interacting_ = false;
  bool dragging_ = false;
  bool moved_ = false;
  bool selecting_ = false;
  int savedMode_ = 0;
  Qt::MouseButton button_ = Qt::NoButton;
  QPointF pressPos_, lastPos_, cursor_;
  bool cursorInside_ = false;
  QPolygonF lasso_;
};

class PingView : public QQuickPaintedItem {
  Q_OBJECT
  Q_PROPERTY(QObject *model READ model WRITE setModel NOTIFY modelChanged)
  Q_PROPERTY(QColor goodColor READ goodColor WRITE setGoodColor NOTIFY styleChanged)
  Q_PROPERTY(QColor flaggedColor READ flaggedColor WRITE setFlaggedColor NOTIFY styleChanged)
  Q_PROPERTY(QColor axisColor READ axisColor WRITE setAxisColor NOTIFY styleChanged)
  Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY styleChanged)

 public:
  explicit PingView(QQuickItem *parent = nullptr);
  void paint(QPainter *painter) override;
  QObject *model() const;
  void setModel(QObject *model);
  QColor goodColor() const { return goodColor_; }
  void setGoodColor(const QColor &c);
  QColor flaggedColor() const { return flaggedColor_; }
  void setFlaggedColor(const QColor &c);
  QColor axisColor() const { return axisColor_; }
  void setAxisColor(const QColor &c);
  QColor textColor() const { return textColor_; }
  void setTextColor(const QColor &c);

 signals:
  void modelChanged();
  void styleChanged();

 protected:
  void mousePressEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

 private:
  struct Layout {
    QRectF plot;
    double xmin = 0, xmax = 1, ymin = 0, ymax = 1;
    QPointF map(double x, double y) const;
  };
  Layout layout() const;
  QPointer<SoundingEditorModel> model_;
  QColor goodColor_ = QColor(0x9D, 0xB4, 0xC8);
  QColor flaggedColor_ = QColor(0xE5, 0x53, 0x4B);
  QColor axisColor_ = QColor(128, 128, 128, 90);
  QColor textColor_ = QColor(160, 160, 160);
};

}  // namespace mbstudio::ui
