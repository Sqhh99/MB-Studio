#include "MapView.h"

#include <QFontMetricsF>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

#include "ProjectViewModel.h"

namespace mbstudio::ui {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kMetersPerDegree = 111319.49;

double distanceToSegment(const QPointF &p, const QPointF &a, const QPointF &b) {
  const QPointF ab = b - a;
  const double len2 = ab.x() * ab.x() + ab.y() * ab.y();
  double t = len2 > 0 ? ((p.x() - a.x()) * ab.x() + (p.y() - a.y()) * ab.y()) / len2 : 0.0;
  t = std::clamp(t, 0.0, 1.0);
  const QPointF c = a + t * ab;
  return std::hypot(p.x() - c.x(), p.y() - c.y());
}

QString formatDegrees(double v, bool lon) {
  const QChar hemi = lon ? (v < 0 ? QLatin1Char('W') : QLatin1Char('E')) : (v < 0 ? QLatin1Char('S') : QLatin1Char('N'));
  const double a = std::fabs(v);
  int deg = static_cast<int>(a);
  double min = (a - deg) * 60.0;
  if (min >= 59.995) {
    deg++;
    min = 0.0;
  }
  return QStringLiteral("%1°%2′%3").arg(deg).arg(min, 0, 'f', min == std::floor(min) ? 0 : 2).arg(hemi);
}

}  // namespace

MapView::MapView(QQuickItem *parent) : QQuickPaintedItem(parent) {
  setAcceptedMouseButtons(Qt::LeftButton);
  setAcceptHoverEvents(true);
  setAntialiasing(true);
  setFlag(ItemHasContents, true);
}

QObject *MapView::project() const { return project_.data(); }

void MapView::setProject(QObject *project) {
  auto *model = qobject_cast<ProjectViewModel *>(project);
  if (model == project_.data())
    return;
  if (project_)
    disconnect(project_.data(), nullptr, this, nullptr);
  project_ = model;
  if (project_) {
    connect(project_.data(), &ProjectViewModel::mapChanged, this, &MapView::onProjectChanged);
    connect(project_.data(), &ProjectViewModel::projectChanged, this, [this]() {
      userMoved_ = false;
      ppd_ = 0.0;
      onProjectChanged();
    });
  }
  emit projectChanged();
  onProjectChanged();
}

void MapView::onProjectChanged() {
  const bool content = hasContent();
  // fit the view when the first data arrive, until the user moves the map
  if (content && (!userMoved_ || ppd_ <= 0.0))
    fitAll();
  if (content != hadContent_) {
    hadContent_ = content;
    emit contentChanged();
  }
  update();
}

#define MAPVIEW_SETTER(name, member, type)    \
  void MapView::name(type value) {            \
    if (member == value)                      \
      return;                                 \
    member = value;                           \
    emit styleChanged();                      \
    update();                                 \
  }
MAPVIEW_SETTER(setBackgroundColor, background_, const QColor &)
MAPVIEW_SETTER(setTrackColor, track_, const QColor &)
MAPVIEW_SETTER(setSelectedColor, selected_, const QColor &)
MAPVIEW_SETTER(setTextColor, text_, const QColor &)
MAPVIEW_SETTER(setGridColor, grid_, const QColor &)
MAPVIEW_SETTER(setShowTracks, showTracks_, bool)
MAPVIEW_SETTER(setShowLayers, showLayers_, bool)
#undef MAPVIEW_SETTER

bool MapView::hasContent() const {
  double w, e, s, n;
  return contentBounds(w, e, s, n);
}

bool MapView::contentBounds(double &w, double &e, double &s, double &n) const {
  if (!project_)
    return false;
  bool any = false;
  auto add = [&](double lon, double lat) {
    if (!any) {
      w = e = lon;
      s = n = lat;
      any = true;
    } else {
      w = std::min(w, lon);
      e = std::max(e, lon);
      s = std::min(s, lat);
      n = std::max(n, lat);
    }
  };
  for (const auto &t : project_->tracks())
    for (const auto &p : t.points)
      add(p.x(), p.y());
  for (const auto &l : project_->mapLayers())
    if (l.visible && !l.image.isNull()) {
      add(l.bounds.west, l.bounds.south);
      add(l.bounds.east, l.bounds.north);
    }
  return any;
}

QPointF MapView::toScreen(double lon, double lat) const {
  const double kx = std::cos(centerLat_ * kPi / 180.0);
  return QPointF(width() / 2.0 + (lon - centerLon_) * ppd_ * kx, height() / 2.0 - (lat - centerLat_) * ppd_);
}

QPointF MapView::toGeo(const QPointF &p) const {
  const double kx = std::max(0.01, std::cos(centerLat_ * kPi / 180.0));
  if (ppd_ <= 0.0)
    return QPointF();
  return QPointF(centerLon_ + (p.x() - width() / 2.0) / (ppd_ * kx), centerLat_ - (p.y() - height() / 2.0) / ppd_);
}

void MapView::fitAll() {
  double w, e, s, n;
  if (!contentBounds(w, e, s, n) || width() < 10 || height() < 10)
    return;
  centerLon_ = 0.5 * (w + e);
  centerLat_ = 0.5 * (s + n);
  const double kx = std::max(0.01, std::cos(centerLat_ * kPi / 180.0));
  const double spanX = std::max((e - w) * kx, 1e-6);
  const double spanY = std::max(n - s, 1e-6);
  const double margin = 0.88;
  ppd_ = std::min(width() * margin / spanX, height() * margin / spanY);
  update();
}

void MapView::zoomBy(double factor) {
  if (ppd_ <= 0.0)
    return;
  ppd_ = std::clamp(ppd_ * factor, 1e-3, 1e9);
  userMoved_ = true;
  update();
}

double MapView::niceStep(double span, int maxLines) {
  static const double steps[] = {1.0 / 3600, 2.0 / 3600, 5.0 / 3600, 10.0 / 3600, 30.0 / 3600, 1.0 / 60,
                                 2.0 / 60,   5.0 / 60,   10.0 / 60,  15.0 / 60,   30.0 / 60,   1.0,
                                 2.0,        5.0,        10.0,       15.0,        30.0,        45.0};
  for (const double s : steps)
    if (span / s <= maxLines)
      return s;
  return 90.0;
}

void MapView::paint(QPainter *painter) {
  painter->fillRect(QRectF(0, 0, width(), height()), background_);
  if (!project_ || ppd_ <= 0.0)
    return;
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

  // grid layers, bottom first
  if (showLayers_) {
    for (const auto &l : project_->mapLayers()) {
      if (!l.visible || l.image.isNull() || !l.bounds.valid())
        continue;
      const QPointF tl = toScreen(l.bounds.west, l.bounds.north);
      const QPointF br = toScreen(l.bounds.east, l.bounds.south);
      // grid cells stay sharp when magnified (each pixel is a cell)
      painter->setRenderHint(QPainter::SmoothPixmapTransform, (br.x() - tl.x()) < l.image.width());
      painter->setOpacity(std::clamp(l.opacity, 0.0, 1.0));
      painter->drawImage(QRectF(tl, br), l.image);
    }
    painter->setOpacity(1.0);
  }

  // graticule with labels
  const QPointF g0 = toGeo(QPointF(0, height()));
  const QPointF g1 = toGeo(QPointF(width(), 0));
  const double step = niceStep(std::max(g1.x() - g0.x(), g1.y() - g0.y()), 6);
  QFont font = painter->font();
  font.setPixelSize(10);
  painter->setFont(font);
  painter->setPen(QPen(grid_, 1.0));
  for (double lon = std::ceil(g0.x() / step) * step; lon <= g1.x(); lon += step) {
    const double x = toScreen(lon, 0).x();
    painter->setPen(QPen(grid_, 1.0));
    painter->drawLine(QPointF(x, 0), QPointF(x, height()));
    painter->setPen(text_);
    painter->drawText(QPointF(x + 3, height() - 4), formatDegrees(lon, true));
  }
  for (double lat = std::ceil(g0.y() / step) * step; lat <= g1.y(); lat += step) {
    const double y = toScreen(0, lat).y();
    painter->setPen(QPen(grid_, 1.0));
    painter->drawLine(QPointF(0, y), QPointF(width(), y));
    if (y > height() - 44)
      continue;  // keep the scale bar and the longitude labels readable
    painter->setPen(text_);
    painter->drawText(QPointF(4, y - 3), formatDegrees(lat, false));
  }

  // navigation tracks: unselected first, selected on top
  if (showTracks_) {
    const bool anySelected = project_->selectedCount() > 0;
    for (int pass = 0; pass < 2; pass++) {
      for (const auto &t : project_->tracks()) {
        const bool selected = project_->isSelected(t.id);
        if ((pass == 1) != selected || t.points.size() < 1)
          continue;
        QPolygonF polygon;
        polygon.reserve(t.points.size());
        for (const auto &p : t.points)
          polygon << toScreen(p.x(), p.y());
        QColor c = selected ? selected_ : track_;
        if (anySelected && !selected)
          c.setAlphaF(c.alphaF() * 0.55);
        const bool hovered = t.id == hoverId_;
        painter->setPen(QPen(c, selected ? 3.0 : hovered ? 2.6 : 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPolyline(polygon);
        // start of the line
        painter->setBrush(c);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(polygon.first(), selected ? 3.5 : 2.5, selected ? 3.5 : 2.5);
        painter->setBrush(Qt::NoBrush);
      }
    }
  }

  // scale bar
  const double metersPerPixel = kMetersPerDegree / ppd_;
  const double target = metersPerPixel * 120.0;
  const double magnitude = std::pow(10.0, std::floor(std::log10(target)));
  double length = magnitude;
  for (const double m : {1.0, 2.0, 5.0, 10.0})
    if (m * magnitude <= target)
      length = m * magnitude;
  const double pixels = length / metersPerPixel;
  const QPointF origin(16, height() - 26);
  painter->setPen(QPen(text_, 2.0));
  painter->drawLine(origin, origin + QPointF(pixels, 0));
  painter->drawLine(origin + QPointF(0, -4), origin + QPointF(0, 2));
  painter->drawLine(origin + QPointF(pixels, -4), origin + QPointF(pixels, 2));
  painter->drawText(origin + QPointF(0, -7),
                    length >= 1000 ? QStringLiteral("%1 km").arg(length / 1000.0) : QStringLiteral("%1 m").arg(length));
}

QString MapView::pick(const QPointF &p, double tolerance) const {
  if (!project_ || !showTracks_)
    return QString();
  QString best;
  double bestDistance = tolerance;
  for (const auto &t : project_->tracks()) {
    QPointF previous;
    for (int i = 0; i < t.points.size(); i++) {
      const QPointF s = toScreen(t.points[i].x(), t.points[i].y());
      const double d = i == 0 ? std::hypot(p.x() - s.x(), p.y() - s.y()) : distanceToSegment(p, previous, s);
      if (d < bestDistance) {
        bestDistance = d;
        best = t.id;
      }
      previous = s;
    }
  }
  return best;
}

void MapView::mousePressEvent(QMouseEvent *event) {
  pressPos_ = lastPos_ = event->position();
  dragging_ = false;
  event->accept();
}

void MapView::mouseMoveEvent(QMouseEvent *event) {
  const QPointF p = event->position();
  if (!dragging_ && (p - pressPos_).manhattanLength() > 4)
    dragging_ = true;
  if (dragging_ && ppd_ > 0.0) {
    const QPointF a = toGeo(lastPos_);
    const QPointF b = toGeo(p);
    centerLon_ -= b.x() - a.x();
    centerLat_ -= b.y() - a.y();
    userMoved_ = true;
    update();
  }
  lastPos_ = p;
  updateCursor(p);
  event->accept();
}

void MapView::mouseReleaseEvent(QMouseEvent *event) {
  if (!dragging_)
    emit lineClicked(pick(event->position(), 8.0), event->modifiers().toInt());
  dragging_ = false;
  event->accept();
}

void MapView::mouseDoubleClickEvent(QMouseEvent *event) {
  userMoved_ = false;
  fitAll();
  event->accept();
}

void MapView::wheelEvent(QWheelEvent *event) {
  if (ppd_ <= 0.0)
    return;
  const QPointF p = event->position();
  const QPointF before = toGeo(p);
  const double factor = std::pow(1.0015, event->angleDelta().y());
  ppd_ = std::clamp(ppd_ * factor, 1e-3, 1e9);
  const QPointF after = toGeo(p);
  centerLon_ += before.x() - after.x();
  centerLat_ += before.y() - after.y();
  userMoved_ = true;
  updateCursor(p);
  update();
  event->accept();
}

void MapView::hoverMoveEvent(QHoverEvent *event) { updateCursor(event->position()); }

void MapView::hoverLeaveEvent(QHoverEvent *) {
  cursorInside_ = false;
  hoverLine_.clear();
  if (!hoverId_.isEmpty()) {
    hoverId_.clear();
    update();
  }
  emit cursorChanged();
}

void MapView::updateCursor(const QPointF &p) {
  if (ppd_ <= 0.0 || !project_)
    return;
  const QPointF g = toGeo(p);
  cursorInside_ = true;
  cursorLon_ = g.x();
  cursorLat_ = g.y();
  cursorValue_ = project_->valueAt(g.x(), g.y());
  const QString id = pick(p, 6.0);
  if (id != hoverId_) {
    hoverId_ = id;
    hoverLine_.clear();
    for (const auto &t : project_->tracks())
      if (t.id == id)
        hoverLine_ = t.name;
    update();
  }
  emit cursorChanged();
}

void MapView::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
  QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
  if (!userMoved_ && newGeometry.size() != oldGeometry.size())
    fitAll();
}

}  // namespace mbstudio::ui
