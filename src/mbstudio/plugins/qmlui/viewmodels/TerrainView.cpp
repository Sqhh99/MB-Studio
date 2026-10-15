#include "TerrainView.h"

#include <QHoverEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <cmath>

#include "TerrainViewModel.h"

namespace mbstudio::ui {

TerrainView::TerrainView(QQuickItem *parent) : QQuickPaintedItem(parent) {
  setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
  setAcceptHoverEvents(true);
  setFlag(ItemIsFocusScope, false);
  setActiveFocusOnTab(true);
  setAntialiasing(true);
  settle_.setSingleShot(true);
  settle_.setInterval(220);
  connect(&settle_, &QTimer::timeout, this, [this]() {
    interacting_ = false;
    emit interactingChanged();
    update();  // the fine level of detail
  });
}

QObject *TerrainView::model() const { return model_.data(); }

void TerrainView::setModel(QObject *model) {
  auto *m = qobject_cast<TerrainViewModel *>(model);
  if (m == model_.data())
    return;
  if (model_)
    disconnect(model_.data(), nullptr, this, nullptr);
  model_ = m;
  if (model_) {
    connect(model_.data(), &TerrainViewModel::sceneChanged, this, [this]() { update(); });
    connect(model_.data(), &TerrainViewModel::markersChanged, this, [this]() { update(); });
    connect(model_.data(), &TerrainViewModel::stateChanged, this, [this]() { update(); });
  }
  emit modelChanged();
  update();
}

void TerrainView::setMarkerColor(const QColor &c) {
  if (c == markerColor_)
    return;
  markerColor_ = c;
  emit styleChanged();
  update();
}

void TerrainView::paint(QPainter *painter) {
  if (!model_ || width() < 2 || height() < 2)
    return;
  const QImage image = model_->render(static_cast<int>(width()), static_cast<int>(height()), interacting_);
  painter->drawImage(QPointF(0, 0), image);
  const QVector<QPointF> markers = model_->markerPositions();
  if (markers.isEmpty())
    return;
  painter->setRenderHint(QPainter::Antialiasing, true);
  QPen pen(markerColor_, 2.0);
  painter->setPen(pen);
  if (markers.size() == 2)
    painter->drawLine(markers[0], markers[1]);
  painter->setBrush(markerColor_);
  for (int i = 0; i < markers.size(); i++) {
    painter->drawEllipse(markers[i], 4.5, 4.5);
    painter->drawText(markers[i] + QPointF(7, -7), i == 0 ? QStringLiteral("A") : QStringLiteral("B"));
  }
}

void TerrainView::startInteraction() {
  if (!interacting_) {
    interacting_ = true;
    emit interactingChanged();
  }
  settle_.start();
}

void TerrainView::mousePressEvent(QMouseEvent *event) {
  forceActiveFocus(Qt::MouseFocusReason);
  dragging_ = true;
  moved_ = false;
  button_ = event->button();
  pressPos_ = lastPos_ = event->position();
  event->accept();
}

void TerrainView::mouseMoveEvent(QMouseEvent *event) {
  if (!dragging_ || !model_)
    return;
  const QPointF p = event->position();
  const QPointF d = p - lastPos_;
  lastPos_ = p;
  if (!moved_ && (p - pressPos_).manhattanLength() < 3)
    return;
  moved_ = true;
  startInteraction();
  const bool panning = button_ == Qt::RightButton || button_ == Qt::MiddleButton ||
                       (event->modifiers() & Qt::ShiftModifier);
  if (panning)
    model_->pan(d.x(), d.y(), width(), height());
  else
    model_->orbit(d.x(), d.y());
  event->accept();
}

void TerrainView::mouseReleaseEvent(QMouseEvent *event) {
  if (dragging_ && !moved_ && model_ && button_ == Qt::LeftButton && model_->tool() > 0)
    model_->addMarker(event->position().x(), event->position().y());
  dragging_ = false;
  event->accept();
}

void TerrainView::mouseDoubleClickEvent(QMouseEvent *event) {
  if (model_ && model_->tool() == 0)
    model_->fit();
  event->accept();
}

void TerrainView::wheelEvent(QWheelEvent *event) {
  if (!model_)
    return;
  const double steps = event->angleDelta().y() / 120.0;
  if (steps != 0.0) {
    startInteraction();
    model_->zoom(std::pow(1.15, steps));
  }
  event->accept();
}

void TerrainView::hoverMoveEvent(QHoverEvent *event) {
  if (model_ && !dragging_)
    model_->hover(event->position().x(), event->position().y());
}

void TerrainView::hoverLeaveEvent(QHoverEvent *) {
  if (model_)
    model_->leave();
}

void TerrainView::keyPressEvent(QKeyEvent *event) {
  if (!model_) {
    event->ignore();
    return;
  }
  switch (event->key()) {
    case Qt::Key_1: model_->standardView(0); break;
    case Qt::Key_2: model_->standardView(1); break;
    case Qt::Key_3: model_->standardView(2); break;
    case Qt::Key_5: model_->setPerspective(!model_->perspective()); break;
    case Qt::Key_F: model_->fit(); break;
    case Qt::Key_Left: model_->orbit(-12.0, 0.0); break;
    case Qt::Key_Right: model_->orbit(12.0, 0.0); break;
    case Qt::Key_Up: model_->orbit(0.0, 10.0); break;
    case Qt::Key_Down: model_->orbit(0.0, -10.0); break;
    case Qt::Key_Plus:
    case Qt::Key_Equal: model_->zoom(1.25); break;
    case Qt::Key_Minus: model_->zoom(1.0 / 1.25); break;
    case Qt::Key_Escape: model_->clearMarkers(); break;
    default: event->ignore(); return;
  }
  event->accept();
}

}  // namespace mbstudio::ui
