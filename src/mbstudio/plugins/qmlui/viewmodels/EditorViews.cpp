#include "EditorViews.h"

#include <QHoverEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

#include "SoundingEditorModel.h"
#include "TrackPlot.h"

namespace mbstudio::ui {

// ---------------------------------------------------------------------------
// PointCloudView

PointCloudView::PointCloudView(QQuickItem *parent) : QQuickPaintedItem(parent) {
  setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
  setAcceptHoverEvents(true);
  setActiveFocusOnTab(true);
  settle_.setSingleShot(true);
  settle_.setInterval(220);
  connect(&settle_, &QTimer::timeout, this, [this]() {
    interacting_ = false;
    update();
  });
}

QObject *PointCloudView::model() const { return model_.data(); }

void PointCloudView::setModel(QObject *model) {
  auto *m = qobject_cast<SoundingEditorModel *>(model);
  if (m == model_.data())
    return;
  if (model_)
    disconnect(model_.data(), nullptr, this, nullptr);
  model_ = m;
  if (model_) {
    connect(model_.data(), &SoundingEditorModel::sceneChanged, this, [this]() { update(); });
    connect(model_.data(), &SoundingEditorModel::stateChanged, this, [this]() { update(); });
    connect(model_.data(), &SoundingEditorModel::toolChanged, this, [this]() { update(); });
  }
  emit modelChanged();
  update();
}

void PointCloudView::setToolColor(const QColor &c) {
  if (c == toolColor_)
    return;
  toolColor_ = c;
  emit styleChanged();
  update();
}

void PointCloudView::paint(QPainter *painter) {
  if (!model_ || width() < 2 || height() < 2)
    return;
  painter->drawImage(QPointF(0, 0), model_->render(static_cast<int>(width()), static_cast<int>(height()), interacting_));
  painter->setRenderHint(QPainter::Antialiasing, true);
  QPen pen(toolColor_, 1.5, Qt::DashLine);
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);
  const int tool = model_->tool();
  if (selecting_ && tool == 1)
    painter->drawRect(QRectF(pressPos_, lastPos_).normalized());
  if (selecting_ && tool == 2 && lasso_.size() > 1)
    painter->drawPolyline(lasso_);
  if (tool == 3 && cursorInside_) {
    painter->setPen(QPen(toolColor_, 1.5));
    painter->drawEllipse(cursor_, model_->brushRadius(), model_->brushRadius());
  }
}

void PointCloudView::startInteraction() {
  interacting_ = true;
  settle_.start();
}

int PointCloudView::gestureMode(Qt::KeyboardModifiers modifiers) const {
  if (modifiers & Qt::ControlModifier)
    return 2;
  if (modifiers & Qt::ShiftModifier)
    return 1;
  return savedMode_;
}

void PointCloudView::mousePressEvent(QMouseEvent *event) {
  forceActiveFocus(Qt::MouseFocusReason);
  if (!model_)
    return;
  dragging_ = true;
  moved_ = false;
  button_ = event->button();
  pressPos_ = lastPos_ = event->position();
  const int tool = model_->tool();
  selecting_ = button_ == Qt::LeftButton && tool >= 1;
  if (selecting_) {
    savedMode_ = model_->selectionMode();
    model_->setSelectionMode(gestureMode(event->modifiers()));
    lasso_.clear();
    lasso_ << pressPos_;
    if (tool == 3) {
      // a brush stroke replaces the selection when it starts
      if (model_->selectionMode() == 0)
        model_->clearSelection();
      model_->selectBrush(pressPos_.x(), pressPos_.y());
    }
  }
  event->accept();
}

void PointCloudView::mouseMoveEvent(QMouseEvent *event) {
  if (!dragging_ || !model_)
    return;
  const QPointF p = event->position();
  const QPointF d = p - lastPos_;
  lastPos_ = p;
  cursor_ = p;
  if (!moved_ && (p - pressPos_).manhattanLength() < 3)
    return;
  moved_ = true;
  if (selecting_) {
    const int tool = model_->tool();
    if (tool == 2)
      lasso_ << p;
    else if (tool == 3)
      model_->selectBrush(p.x(), p.y());
    update();
    event->accept();
    return;
  }
  startInteraction();
  if (button_ == Qt::RightButton || button_ == Qt::MiddleButton || (event->modifiers() & Qt::ShiftModifier))
    model_->pan(d.x(), d.y(), width(), height());
  else
    model_->orbit(d.x(), d.y());
  event->accept();
}

void PointCloudView::mouseReleaseEvent(QMouseEvent *event) {
  if (!model_ || !dragging_)
    return;
  const QPointF p = event->position();
  if (selecting_) {
    const int tool = model_->tool();
    if (tool == 1 && moved_)
      model_->selectRect(pressPos_.x(), pressPos_.y(), p.x(), p.y());
    else if (tool == 2 && lasso_.size() > 2)
      model_->selectLasso([this]() {
        QVariantList points;
        for (const auto &q : lasso_)
          points << q;
        return points;
      }());
    else if (tool == 4 || (!moved_ && tool != 3))
      model_->selectAt(p.x(), p.y());
    model_->setSelectionMode(savedMode_);
    lasso_.clear();
  } else if (!moved_ && button_ == Qt::LeftButton) {
    // navigate: a click shows the ping of the sounding in the ping view
    const QVariantMap info = model_->soundingInfo(model_->soundingAt(p.x(), p.y()));
    if (!info.isEmpty())
      model_->setCurrentPing(info.value(QStringLiteral("pingIndex")).toInt());
  }
  selecting_ = false;
  dragging_ = false;
  update();
  event->accept();
}

void PointCloudView::mouseDoubleClickEvent(QMouseEvent *event) {
  if (model_ && model_->tool() == 0)
    model_->fit();
  event->accept();
}

void PointCloudView::wheelEvent(QWheelEvent *event) {
  if (!model_)
    return;
  const double steps = event->angleDelta().y() / 120.0;
  if (steps != 0.0) {
    startInteraction();
    model_->zoom(std::pow(1.15, steps));
  }
  event->accept();
}

void PointCloudView::hoverMoveEvent(QHoverEvent *event) {
  cursor_ = event->position();
  cursorInside_ = true;
  if (model_ && !dragging_)
    model_->hover(cursor_.x(), cursor_.y());
  if (model_ && model_->tool() == 3)
    update();
}

void PointCloudView::hoverLeaveEvent(QHoverEvent *) {
  cursorInside_ = false;
  if (model_)
    model_->leave();
  update();
}

void PointCloudView::keyPressEvent(QKeyEvent *event) {
  if (!model_) {
    event->ignore();
    return;
  }
  switch (event->key()) {
    case Qt::Key_Delete:
    case Qt::Key_F: model_->flagSelected(); break;
    case Qt::Key_U: model_->unflagSelected(); break;
    case Qt::Key_Escape: model_->clearSelection(); break;
    case Qt::Key_Home: model_->fit(); break;
    case Qt::Key_1: model_->standardView(0); break;
    case Qt::Key_2: model_->standardView(1); break;
    case Qt::Key_3: model_->standardView(2); break;
    case Qt::Key_5: model_->setPerspective(!model_->perspective()); break;
    case Qt::Key_PageDown: model_->stepPing(1); break;
    case Qt::Key_PageUp: model_->stepPing(-1); break;
    default: event->ignore(); return;
  }
  event->accept();
}

// ---------------------------------------------------------------------------
// PingView

PingView::PingView(QQuickItem *parent) : QQuickPaintedItem(parent) {
  setAcceptedMouseButtons(Qt::LeftButton);
  setAntialiasing(true);
}

QObject *PingView::model() const { return model_.data(); }

void PingView::setModel(QObject *model) {
  auto *m = qobject_cast<SoundingEditorModel *>(model);
  if (m == model_.data())
    return;
  if (model_)
    disconnect(model_.data(), nullptr, this, nullptr);
  model_ = m;
  if (model_) {
    connect(model_.data(), &SoundingEditorModel::pingChanged, this, [this]() { update(); });
    connect(model_.data(), &SoundingEditorModel::countsChanged, this, [this]() { update(); });
    connect(model_.data(), &SoundingEditorModel::selectionChanged, this, [this]() { update(); });
  }
  emit modelChanged();
  update();
}

#define MB_PING_SETTER(Name, member)    \
  void PingView::Name(const QColor &c) { \
    if (c == member)                     \
      return;                            \
    member = c;                          \
    emit styleChanged();                 \
    update();                            \
  }
MB_PING_SETTER(setGoodColor, goodColor_)
MB_PING_SETTER(setFlaggedColor, flaggedColor_)
MB_PING_SETTER(setAxisColor, axisColor_)
MB_PING_SETTER(setTextColor, textColor_)
#undef MB_PING_SETTER

QPointF PingView::Layout::map(double x, double y) const {
  const double u = (x - xmin) / (xmax - xmin), v = (y - ymin) / (ymax - ymin);
  return QPointF(plot.left() + u * plot.width(), plot.top() + v * plot.height());  // depth grows downwards
}

PingView::Layout PingView::layout() const {
  Layout l;
  l.plot = QRectF(52, 10, std::max(10.0, width() - 64), std::max(10.0, height() - 36));
  if (!model_)
    return l;
  const QVariantList beams = model_->pingBeams(model_->currentPing());
  bool first = true;
  for (const auto &b : beams) {
    const QVariantMap m = b.toMap();
    const double x = m.value(QStringLiteral("x")).toDouble(), y = m.value(QStringLiteral("y")).toDouble();
    if (first) {
      l.xmin = l.xmax = x;
      l.ymin = l.ymax = y;
      first = false;
    }
    l.xmin = std::min(l.xmin, x);
    l.xmax = std::max(l.xmax, x);
    l.ymin = std::min(l.ymin, y);
    l.ymax = std::max(l.ymax, y);
  }
  // symmetric across track, a little room around the depths
  const double half = std::max({std::fabs(l.xmin), std::fabs(l.xmax), 1.0}) * 1.05;
  l.xmin = -half;
  l.xmax = half;
  const double pad = std::max(1.0, 0.08 * (l.ymax - l.ymin));
  l.ymin -= pad;
  l.ymax += pad;
  return l;
}

void PingView::paint(QPainter *painter) {
  if (!model_ || model_->currentPing() < 0)
    return;
  painter->setRenderHint(QPainter::Antialiasing, true);
  const Layout l = layout();
  QFont font = painter->font();
  font.setPixelSize(10);
  painter->setFont(font);
  painter->setPen(axisColor_);
  painter->drawRect(l.plot);
  for (const double t : TrackPlot::niceTicks(l.xmin, l.xmax, 7)) {
    const QPointF p = l.map(t, l.ymax);
    painter->setPen(axisColor_);
    painter->drawLine(QPointF(p.x(), l.plot.top()), QPointF(p.x(), l.plot.bottom()));
    painter->setPen(textColor_);
    painter->drawText(QRectF(p.x() - 40, l.plot.bottom() + 2, 80, 14), Qt::AlignHCenter | Qt::AlignTop,
                      QString::number(t, 'f', 0));
  }
  for (const double t : TrackPlot::niceTicks(l.ymin, l.ymax, 5)) {
    const QPointF p = l.map(l.xmin, t);
    painter->setPen(axisColor_);
    painter->drawLine(QPointF(l.plot.left(), p.y()), QPointF(l.plot.right(), p.y()));
    painter->setPen(textColor_);
    painter->drawText(QRectF(0, p.y() - 7, l.plot.left() - 4, 14), Qt::AlignRight | Qt::AlignVCenter,
                      QString::number(t, 'f', t == std::floor(t) ? 0 : 1));
  }
  // the seafloor of the good beams, then every beam
  const QVariantList beams = model_->pingBeams(model_->currentPing());
  QPolygonF floor;
  for (const auto &b : beams) {
    const QVariantMap m = b.toMap();
    if (m.value(QStringLiteral("good")).toBool())
      floor << l.map(m.value(QStringLiteral("x")).toDouble(), m.value(QStringLiteral("y")).toDouble());
  }
  painter->setPen(QPen(goodColor_, 1.0));
  painter->drawPolyline(floor);
  for (const auto &b : beams) {
    const QVariantMap m = b.toMap();
    const QPointF p = l.map(m.value(QStringLiteral("x")).toDouble(), m.value(QStringLiteral("y")).toDouble());
    const bool good = m.value(QStringLiteral("good")).toBool();
    const bool selected = m.value(QStringLiteral("selected")).toBool();
    if (good) {
      painter->setPen(Qt::NoPen);
      painter->setBrush(selected ? QColor(0xFF, 0xD5, 0x4F) : goodColor_);
      painter->drawEllipse(p, 2.6, 2.6);
    } else {
      painter->setPen(QPen(selected ? QColor(0xFF, 0xD5, 0x4F) : flaggedColor_, 1.4));
      painter->drawLine(p + QPointF(-3, -3), p + QPointF(3, 3));
      painter->drawLine(p + QPointF(-3, 3), p + QPointF(3, -3));
    }
  }
  painter->setPen(textColor_);
  painter->drawText(QRectF(l.plot.left(), l.plot.bottom() + 14, l.plot.width(), 14), Qt::AlignHCenter,
                    tr("across track (m)"));
}

void PingView::mousePressEvent(QMouseEvent *event) {
  if (!model_)
    return;
  const Layout l = layout();
  const QVariantList beams = model_->pingBeams(model_->currentPing());
  int best = -1;
  double bestDistance = 64.0;  // 8 pixels
  for (const auto &b : beams) {
    const QVariantMap m = b.toMap();
    const QPointF p = l.map(m.value(QStringLiteral("x")).toDouble(), m.value(QStringLiteral("y")).toDouble());
    const QPointF d = p - event->position();
    const double d2 = d.x() * d.x() + d.y() * d.y();
    if (d2 < bestDistance) {
      bestDistance = d2;
      best = m.value(QStringLiteral("index")).toInt();
    }
  }
  if (best >= 0)
    model_->toggleBeam(best);
  event->accept();
}

void PingView::wheelEvent(QWheelEvent *event) {
  if (model_ && event->angleDelta().y() != 0)
    model_->stepPing(event->angleDelta().y() > 0 ? -1 : 1);
  event->accept();
}

}  // namespace mbstudio::ui
