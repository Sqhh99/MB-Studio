#include "GridViewModel.h"

#include <QColor>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QMutexLocker>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace mbstudio::ui {

GridViewModel::GridViewModel(std::shared_ptr<IGridService> grids, std::shared_ptr<IGridRenderService> renderer,
                             QObject *parent)
    : QObject(parent), grids_(std::move(grids)), renderer_(std::move(renderer)) {
  renderTimer_.setSingleShot(true);
  renderTimer_.setInterval(40);  // coalesce slider movements
  connect(&renderTimer_, &QTimer::timeout, this, &GridViewModel::startRender);
  connect(&loadWatcher_, &QFutureWatcher<GridLoadResult>::finished, this, [this]() {
    GridLoadResult result = loadWatcher_.result();
    loading_ = false;
    if (result.ok) {
      grid_ = std::make_shared<const Grid>(std::move(result.grid));
      const Grid &g = *grid_;
      info_.clear();
      info_[QStringLiteral("nx")] = g.nx;
      info_[QStringLiteral("ny")] = g.ny;
      info_[QStringLiteral("xmin")] = g.xmin;
      info_[QStringLiteral("xmax")] = g.xmax;
      info_[QStringLiteral("ymin")] = g.ymin;
      info_[QStringLiteral("ymax")] = g.ymax;
      info_[QStringLiteral("zmin")] = g.zmin;
      info_[QStringLiteral("zmax")] = g.zmax;
      info_[QStringLiteral("dx")] = g.dx();
      info_[QStringLiteral("dy")] = g.dy();
      info_[QStringLiteral("geographic")] = g.geographic;
      info_[QStringLiteral("title")] = QString::fromStdString(g.title);
      info_[QStringLiteral("zUnits")] = QString::fromStdString(g.zUnits);
      info_[QStringLiteral("xName")] = QString::fromStdString(g.xName);
      info_[QStringLiteral("yName")] = QString::fromStdString(g.yName);
      error_.clear();
      startRender();
    } else {
      error_ = QString::fromStdString(result.error);
    }
    emit stateChanged();
  });
  connect(&renderWatcher_, &QFutureWatcher<QImage>::finished, this, [this]() {
    {
      QMutexLocker lock(&imageMutex_);
      image_ = renderWatcher_.result();
    }
    rendering_ = false;
    revision_++;
    emit imageChanged();
    emit stateChanged();
    if (renderAgain_) {
      renderAgain_ = false;
      startRender();
    }
  });
}

GridViewModel::~GridViewModel() {
  loadWatcher_.waitForFinished();
  renderWatcher_.waitForFinished();
}

QString GridViewModel::fileName() const { return QFileInfo(path_).fileName(); }

QStringList GridViewModel::colorMaps() const {
  QStringList out;
  if (renderer_)
    for (const auto &name : renderer_->colorMapNames())
      out << QString::fromStdString(name);
  return out;
}

void GridViewModel::setColorMap(int value) {
  if (value == colorMap() || value < 0 || value >= colorMaps().size())
    return;
  options_.colorMap = static_cast<ColorMap>(value);
  emit optionsChanged();
  scheduleRender();
}

void GridViewModel::setHillshade(bool value) {
  if (value == options_.hillshade)
    return;
  options_.hillshade = value;
  emit optionsChanged();
  scheduleRender();
}

void GridViewModel::setAzimuth(double value) {
  value = std::fmod(std::fmod(value, 360.0) + 360.0, 360.0);
  if (qFuzzyCompare(value + 1.0, options_.azimuth + 1.0))
    return;
  options_.azimuth = value;
  emit optionsChanged();
  scheduleRender();
}

void GridViewModel::setElevation(double value) {
  value = std::clamp(value, 1.0, 90.0);
  if (qFuzzyCompare(value, options_.elevation))
    return;
  options_.elevation = value;
  emit optionsChanged();
  scheduleRender();
}

void GridViewModel::setExaggeration(double value) {
  value = std::clamp(value, 0.1, 100.0);
  if (qFuzzyCompare(value, options_.exaggeration))
    return;
  options_.exaggeration = value;
  emit optionsChanged();
  scheduleRender();
}

QString GridViewModel::imageSource() const {
  return revision_ > 0 ? QStringLiteral("image://mbgrid/%1").arg(revision_) : QString();
}

QVariantList GridViewModel::legend() const {
  QVariantList out;
  if (!renderer_)
    return out;
  for (int i = 0; i <= 10; i++) {
    const double t = i / 10.0;
    QVariantMap stop;
    stop[QStringLiteral("position")] = t;
    stop[QStringLiteral("color")] = QColor::fromRgba(renderer_->color(options_.colorMap, t)).name();
    out.push_back(stop);
  }
  return out;
}

int GridViewModel::imageWidth() const {
  QMutexLocker lock(&imageMutex_);
  return image_.width();
}

int GridViewModel::imageHeight() const {
  QMutexLocker lock(&imageMutex_);
  return image_.height();
}

void GridViewModel::load(const QString &path) {
  if (!grids_ || path.isEmpty())
    return;
  loadWatcher_.waitForFinished();
  path_ = path;
  loading_ = true;
  error_.clear();
  emit stateChanged();
  auto grids = grids_;
  const std::string file = path.toStdString();
  loadWatcher_.setFuture(QtConcurrent::run([grids, file]() { return grids->load(file); }));
}

void GridViewModel::clear() {
  loadWatcher_.waitForFinished();
  renderWatcher_.waitForFinished();
  grid_.reset();
  info_.clear();
  path_.clear();
  error_.clear();
  {
    QMutexLocker lock(&imageMutex_);
    image_ = QImage();
  }
  revision_ = 0;
  emit stateChanged();
  emit imageChanged();
}

void GridViewModel::scheduleRender() {
  if (grid_)
    renderTimer_.start();
}

void GridViewModel::startRender() {
  if (!grid_ || !renderer_)
    return;
  if (rendering_) {
    renderAgain_ = true;  // options changed while rendering
    return;
  }
  rendering_ = true;
  emit stateChanged();
  auto grid = grid_;
  auto renderer = renderer_;
  const RenderOptions options = options_;
  renderWatcher_.setFuture(QtConcurrent::run([grid, renderer, options]() {
    const Image image = renderer->render(*grid, options);
    QImage out(image.width, image.height, QImage::Format_ARGB32);
    for (int y = 0; y < image.height; y++)
      memcpy(out.scanLine(y), image.pixels.data() + static_cast<size_t>(y) * image.width,
             static_cast<size_t>(image.width) * 4);
    return out;
  }));
}

QVariantMap GridViewModel::probe(double u, double v) const {
  QVariantMap out;
  if (!grid_ || u < 0.0 || u > 1.0 || v < 0.0 || v > 1.0)
    return out;
  const Grid &g = *grid_;
  const int ix = std::clamp(static_cast<int>(std::lround(u * (g.nx - 1))), 0, g.nx - 1);
  const int iy = std::clamp(static_cast<int>(std::lround(v * (g.ny - 1))), 0, g.ny - 1);
  out[QStringLiteral("x")] = g.xOf(ix);
  out[QStringLiteral("y")] = g.yOf(iy);
  const float z = g.at(ix, iy);
  out[QStringLiteral("valid")] = !std::isnan(z);
  out[QStringLiteral("z")] = std::isnan(z) ? 0.0 : static_cast<double>(z);
  return out;
}

bool GridViewModel::waitForFinished(int timeoutMs) {
  QElapsedTimer timer;
  timer.start();
  while ((busy() || renderTimer_.isActive()) && timer.elapsed() < timeoutMs)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
  return !busy();
}

QImage GridViewModel::image() const {
  QMutexLocker lock(&imageMutex_);
  return image_;
}

GridImageProvider::GridImageProvider(GridViewModel *model)
    : QQuickImageProvider(QQuickImageProvider::Image), model_(model) {}

QImage GridImageProvider::requestImage(const QString &, QSize *size, const QSize &requestedSize) {
  QImage image = model_ != nullptr ? model_->image() : QImage();
  if (image.isNull()) {
    image = QImage(1, 1, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
  }
  if (size != nullptr)
    *size = image.size();
  if (requestedSize.isValid() && !requestedSize.isEmpty())
    return image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::FastTransformation);
  return image;
}

}  // namespace mbstudio::ui
