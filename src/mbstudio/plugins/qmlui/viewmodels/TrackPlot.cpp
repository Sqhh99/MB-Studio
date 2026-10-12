#include "TrackPlot.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace mbstudio::ui {

TrackPlot::TrackPlot(QQuickItem *parent) : QQuickPaintedItem(parent) {
  setAntialiasing(true);
  connect(this, &TrackPlot::styleChanged, this, [this]() { update(); });
  connect(this, &TrackPlot::pointsChanged, this, [this]() { update(); });
}

void TrackPlot::setPoints(const QVariantList &points) {
  pointsVariant_ = points;
  points_.clear();
  points_.reserve(points.size());
  for (const QVariant &v : points) {
    if (v.canConvert<QPointF>()) {
      points_.push_back(v.toPointF());
    } else {
      const QVariantMap m = v.toMap();
      points_.push_back(QPointF(m.value(QStringLiteral("x")).toDouble(), m.value(QStringLiteral("y")).toDouble()));
    }
  }
  emit pointsChanged();
}

#define MB_SETTER(Name, member, Type)   \
  void TrackPlot::Name(Type value) {    \
    if (member == value)                \
      return;                           \
    member = value;                     \
    emit styleChanged();                \
  }
MB_SETTER(setGeographic, geographic_, bool)
MB_SETTER(setInvertY, invertY_, bool)
MB_SETTER(setLineColor, lineColor_, const QColor &)
MB_SETTER(setAxisColor, axisColor_, const QColor &)
MB_SETTER(setTextColor, textColor_, const QColor &)
MB_SETTER(setXLabel, xLabel_, const QString &)
MB_SETTER(setYLabel, yLabel_, const QString &)
#undef MB_SETTER

QVector<double> TrackPlot::niceTicks(double lo, double hi, int maxTicks) {
  QVector<double> ticks;
  if (!(hi > lo) || maxTicks < 2)
    return ticks;
  const double raw = (hi - lo) / (maxTicks - 1);
  const double magnitude = std::pow(10.0, std::floor(std::log10(raw)));
  double step = magnitude;
  for (double m : {1.0, 2.0, 2.5, 5.0, 10.0}) {
    step = m * magnitude;
    if (step >= raw)
      break;
  }
  for (double t = std::ceil(lo / step) * step; t <= hi + step * 1e-9; t += step)
    ticks.push_back(std::fabs(t) < step * 1e-9 ? 0.0 : t);
  return ticks;
}

void TrackPlot::paint(QPainter *painter) {
  const QRectF bounds = boundingRect();
  QFont font = painter->font();
  font.setPixelSize(11);
  painter->setFont(font);
  const QFontMetrics fm(font);
  const double left = fm.horizontalAdvance(QStringLiteral("-000.0000")) + 12;
  const double bottom = fm.height() * (xLabel_.isEmpty() ? 1.6 : 2.8);
  const QRectF plot(bounds.left() + left, bounds.top() + 10, bounds.width() - left - 14, bounds.height() - bottom - 10);
  if (plot.width() < 20 || plot.height() < 20)
    return;

  painter->setPen(QPen(axisColor_, 1));
  painter->drawRect(plot);

  if (points_.isEmpty()) {
    painter->setPen(textColor_);
    painter->drawText(plot, Qt::AlignCenter, tr("No data"));
    return;
  }

  double xmin = points_[0].x(), xmax = xmin, ymin = points_[0].y(), ymax = ymin;
  for (const QPointF &p : points_) {
    xmin = std::min(xmin, p.x());
    xmax = std::max(xmax, p.x());
    ymin = std::min(ymin, p.y());
    ymax = std::max(ymax, p.y());
  }
  // pad, and keep a minimum extent for (nearly) stationary tracks
  const double minExtent = geographic_ ? 1e-4 : 1e-6;
  if (xmax - xmin < minExtent) {
    xmin -= minExtent;
    xmax += minExtent;
  }
  if (ymax - ymin < minExtent) {
    ymin -= minExtent;
    ymax += minExtent;
  }
  double padX = 0.06 * (xmax - xmin), padY = 0.06 * (ymax - ymin);
  xmin -= padX;
  xmax += padX;
  ymin -= padY;
  ymax += padY;

  // Equal metric scale in both directions for geographic tracks.
  double sx = plot.width() / (xmax - xmin);
  double sy = plot.height() / (ymax - ymin);
  if (geographic_) {
    const double k = std::cos(0.5 * (ymin + ymax) * M_PI / 180.0);
    const double s = std::min(sx / std::max(k, 0.05), sy);
    sx = s * std::max(k, 0.05);
    sy = s;
    const double cx = 0.5 * (xmin + xmax), cy = 0.5 * (ymin + ymax);
    xmin = cx - plot.width() / sx / 2;
    xmax = cx + plot.width() / sx / 2;
    ymin = cy - plot.height() / sy / 2;
    ymax = cy + plot.height() / sy / 2;
  }
  auto map = [&](const QPointF &p) {
    const double x = plot.left() + (p.x() - xmin) * sx;
    const double y = invertY_ ? plot.top() + (p.y() - ymin) * sy : plot.bottom() - (p.y() - ymin) * sy;
    return QPointF(x, y);
  };

  // grid and tick labels
  const int precision = geographic_ ? 4 : 1;
  painter->setPen(QPen(axisColor_, 1, Qt::DotLine));
  for (double t : niceTicks(xmin, xmax, 5)) {
    const double x = map(QPointF(t, ymin)).x();
    painter->drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
  }
  for (double t : niceTicks(ymin, ymax, 5)) {
    const double y = map(QPointF(xmin, t)).y();
    painter->drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
  }
  painter->setPen(textColor_);
  for (double t : niceTicks(xmin, xmax, 5)) {
    const double x = map(QPointF(t, ymin)).x();
    painter->drawText(QRectF(x - 50, plot.bottom() + 2, 100, fm.height()), Qt::AlignHCenter | Qt::AlignTop,
                      QString::number(t, 'f', precision));
  }
  for (double t : niceTicks(ymin, ymax, 5)) {
    const double y = map(QPointF(xmin, t)).y();
    painter->drawText(QRectF(bounds.left(), y - fm.height() / 2.0, left - 6, fm.height()),
                      Qt::AlignRight | Qt::AlignVCenter, QString::number(t, 'f', precision));
  }
  if (!xLabel_.isEmpty())
    painter->drawText(QRectF(plot.left(), plot.bottom() + fm.height() + 2, plot.width(), fm.height()),
                      Qt::AlignHCenter, xLabel_);

  // the track
  painter->setClipRect(plot);
  QPainterPath path(map(points_[0]));
  for (int i = 1; i < points_.size(); i++)
    path.lineTo(map(points_[i]));
  painter->setPen(QPen(lineColor_, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter->drawPath(path);
  // start (filled) and end (ring) markers
  painter->setBrush(lineColor_);
  painter->drawEllipse(map(points_.front()), 4.5, 4.5);
  painter->setBrush(Qt::NoBrush);
  painter->setPen(QPen(lineColor_, 2));
  painter->drawEllipse(map(points_.back()), 5.5, 5.5);
}

}  // namespace mbstudio::ui
