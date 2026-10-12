// MB-Studio QML UI - plot of a navigation track (or any x/y polyline) with
// axes, drawn with QPainter. Presentation helper for the QML layer.
#pragma once

#include <QColor>
#include <QQuickPaintedItem>
#include <QVariantList>
#include <QVector>
#include <QPointF>

namespace mbstudio::ui {

class TrackPlot : public QQuickPaintedItem {
  Q_OBJECT
  Q_PROPERTY(QVariantList points READ points WRITE setPoints NOTIFY pointsChanged)
  Q_PROPERTY(bool geographic READ geographic WRITE setGeographic NOTIFY styleChanged)
  Q_PROPERTY(bool invertY READ invertY WRITE setInvertY NOTIFY styleChanged)
  Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY styleChanged)
  Q_PROPERTY(QColor axisColor READ axisColor WRITE setAxisColor NOTIFY styleChanged)
  Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY styleChanged)
  Q_PROPERTY(QString xLabel READ xLabel WRITE setXLabel NOTIFY styleChanged)
  Q_PROPERTY(QString yLabel READ yLabel WRITE setYLabel NOTIFY styleChanged)
  Q_PROPERTY(int pointCount READ pointCount NOTIFY pointsChanged)

 public:
  explicit TrackPlot(QQuickItem *parent = nullptr);

  void paint(QPainter *painter) override;

  QVariantList points() const { return pointsVariant_; }
  void setPoints(const QVariantList &points);
  bool geographic() const { return geographic_; }
  void setGeographic(bool value);
  bool invertY() const { return invertY_; }
  void setInvertY(bool value);
  QColor lineColor() const { return lineColor_; }
  void setLineColor(const QColor &c);
  QColor axisColor() const { return axisColor_; }
  void setAxisColor(const QColor &c);
  QColor textColor() const { return textColor_; }
  void setTextColor(const QColor &c);
  QString xLabel() const { return xLabel_; }
  void setXLabel(const QString &s);
  QString yLabel() const { return yLabel_; }
  void setYLabel(const QString &s);
  int pointCount() const { return static_cast<int>(points_.size()); }

  // "Nice" axis tick values covering [lo, hi] (exposed for tests).
  static QVector<double> niceTicks(double lo, double hi, int maxTicks);

 signals:
  void pointsChanged();
  void styleChanged();

 private:
  QVariantList pointsVariant_;
  QVector<QPointF> points_;
  bool geographic_ = true;
  bool invertY_ = false;
  QColor lineColor_ = QColor(0x26, 0xC6, 0xDA);
  QColor axisColor_ = QColor(128, 128, 128, 90);
  QColor textColor_ = QColor(160, 160, 160);
  QString xLabel_;
  QString yLabel_;
};

}  // namespace mbstudio::ui
