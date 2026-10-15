#include "TerrainViewModel.h"

#include <QColor>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <cmath>
#include <limits>

namespace mbstudio::ui {

namespace {

constexpr int kFineMax = 640;    // vertices per side of the still view
constexpr int kCoarseMax = 180;  // while the camera moves
constexpr double kMetersPerDegree = 111319.49;
constexpr double kDeg = 3.14159265358979323846 / 180.0;
constexpr int kProfileSamples = 400;

QString fmt(double v, int digits) { return QString::number(v, 'f', digits); }

}  // namespace

TerrainViewModel::TerrainViewModel(std::shared_ptr<IGridService> grids, std::shared_ptr<IGridRenderService> renderer,
                                   QObject *parent)
    : QObject(parent), grids_(std::move(grids)), renderer_(std::move(renderer)) {
  connect(&watcher_, &QFutureWatcher<Loaded>::finished, this, [this]() {
    Loaded loaded = watcher_.result();
    loading_ = false;
    if (!loaded.main.ok) {
      error_ = QString::fromStdString(loaded.main.error);
      emit stateChanged();
      return;
    }
    error_.clear();
    grid_ = std::make_shared<const Grid>(std::move(loaded.main.grid));
    num_ = loaded.num;
    const Grid &g = *grid_;
    projection_ = g.geographic ? platform::MapProjection::parse("") : platform::MapProjection::parse(g.projection);
    originX_ = 0.5 * (g.xmin + g.xmax);
    originY_ = 0.5 * (g.ymin + g.ymax);
    if (g.geographic) {
      metersY_ = kMetersPerDegree;
      metersX_ = kMetersPerDegree * std::max(0.01, std::cos(originY_ * kDeg));
    } else {
      metersX_ = metersY_ = 1.0;
    }
    // grids of depths (mbgrid -A1) are positive down
    depthsPositive_ = g.zmin >= 0.0;
    const double extent = std::max((g.xmax - g.xmin) * metersX_, (g.ymax - g.ymin) * metersY_);
    exaggeration_ = scene::niceExaggeration(extent, g.zmax - g.zmin);
    info_.clear();
    info_[QStringLiteral("nx")] = g.nx;
    info_[QStringLiteral("ny")] = g.ny;
    info_[QStringLiteral("zmin")] = g.zmin;
    info_[QStringLiteral("zmax")] = g.zmax;
    info_[QStringLiteral("zUnits")] = QString::fromStdString(g.zUnits);
    info_[QStringLiteral("geographic")] = g.geographic;
    info_[QStringLiteral("projection")] =
        g.geographic ? QStringLiteral("WGS 84")
                     : projection_.valid() ? QString::fromStdString(projection_.name()) : QStringLiteral("?");
    info_[QStringLiteral("widthKm")] = (g.xmax - g.xmin) * metersX_ / 1000.0;
    info_[QStringLiteral("heightKm")] = (g.ymax - g.ymin) * metersY_ / 1000.0;
    camera_ = scene::Camera();
    buildLods();
    fit();
    if (!pendingState_.isEmpty()) {
      const QString state = pendingState_;
      pendingState_.clear();
      setViewState(state);
    }
    emit stateChanged();
    changed();
  });
}

TerrainViewModel::~TerrainViewModel() { watcher_.waitForFinished(); }

QString TerrainViewModel::fileName() const { return QFileInfo(path_).fileName(); }

QStringList TerrainViewModel::colorMaps() const {
  QStringList out;
  if (renderer_)
    for (const auto &name : renderer_->colorMapNames())
      out << QString::fromStdString(name);
  return out;
}

void TerrainViewModel::load(const QString &path) {
  if (!grids_ || path.isEmpty())
    return;
  watcher_.waitForFinished();
  path_ = path;
  loading_ = true;
  error_.clear();
  emit stateChanged();
  auto grids = grids_;
  const std::string file = path.toStdString();
  QString root = path;
  if (root.endsWith(QLatin1String(".grd"), Qt::CaseInsensitive))
    root.chop(4);
  const std::string numFile = (root + QStringLiteral("_num.grd")).toStdString();
  watcher_.setFuture(QtConcurrent::run([grids, file, numFile]() {
    Loaded loaded;
    loaded.main = grids->load(file);
    if (!loaded.main.ok)
      return loaded;
    GridLoadResult num = grids->load(numFile);
    if (num.ok && num.grid.nx == loaded.main.grid.nx && num.grid.ny == loaded.main.grid.ny)
      loaded.num = std::make_shared<const Grid>(std::move(num.grid));
    return loaded;
  }));
}

bool TerrainViewModel::waitForFinished(int timeoutMs) {
  QElapsedTimer timer;
  timer.start();
  while (loading_ && timer.elapsed() < timeoutMs)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
  return !loading_;
}

void TerrainViewModel::buildLods() {
  const Grid &g = *grid_;
  auto build = [this, &g](Lod &lod, int maxSide) {
    lod.step = std::max(1, static_cast<int>(std::ceil(std::max(g.nx, g.ny) / static_cast<double>(maxSide))));
    lod.nx = (g.nx - 1) / lod.step + 1;
    lod.ny = (g.ny - 1) / lod.step + 1;
    const std::size_t n = static_cast<std::size_t>(lod.nx) * lod.ny;
    lod.z.assign(n, std::numeric_limits<float>::quiet_NaN());
    lod.interpolated.assign(n, 0);
    for (int j = 0; j < lod.ny; j++)
      for (int i = 0; i < lod.nx; i++) {
        const int gx = std::min(i * lod.step, g.nx - 1), gy = std::min(j * lod.step, g.ny - 1);
        const float v = g.at(gx, gy);
        const std::size_t k = static_cast<std::size_t>(j) * lod.nx + i;
        lod.z[k] = v;
        if (num_ && std::isfinite(v) && !(num_->at(gx, gy) > 0.0f))
          lod.interpolated[k] = 1;
      }
  };
  build(fine_, kFineMax);
  build(coarse_, kCoarseMax);
  recolor();
}

void TerrainViewModel::recolor() {
  if (!grid_)
    return;
  colorLod(fine_);
  colorLod(coarse_);
}

scene::Vec3 TerrainViewModel::vertex(const Lod &lod, int i, int j) const {
  const Grid &g = *grid_;
  const int gx = std::min(i * lod.step, g.nx - 1), gy = std::min(j * lod.step, g.ny - 1);
  const float v = lod.z[static_cast<std::size_t>(j) * lod.nx + i];
  return scene::Vec3{(g.xOf(gx) - originX_) * metersX_, (g.yOf(gy) - originY_) * metersY_, worldZ(v)};
}

void TerrainViewModel::colorLod(Lod &lod) const {
  const Grid &g = *grid_;
  const std::size_t n = lod.z.size();
  lod.color.assign(n, 0xFF808080u);
  const double range = g.zmax > g.zmin ? g.zmax - g.zmin : 1.0;
  const scene::Vec3 light = scene::lightDirection(315.0, 45.0);
  const auto map = static_cast<ColorMap>(std::clamp(colorMap_, 0, 3));
  auto at = [&lod](int i, int j) { return lod.z[static_cast<std::size_t>(j) * lod.nx + i]; };
  for (int j = 0; j < lod.ny; j++)
    for (int i = 0; i < lod.nx; i++) {
      const std::size_t k = static_cast<std::size_t>(j) * lod.nx + i;
      const float v = lod.z[k];
      if (!std::isfinite(v))
        continue;
      const double t = std::clamp((v - g.zmin) / range, 0.0, 1.0);
      std::uint32_t c = renderer_ ? renderer_->color(map, t) : 0xFF808080u;
      // normal from the neighbours (world units, exaggerated)
      const scene::Vec3 p = vertex(lod, i, j);
      auto neighbour = [&](int di, int dj, scene::Vec3 &q) {
        const int a = i + di, b = j + dj;
        if (a < 0 || b < 0 || a >= lod.nx || b >= lod.ny || !std::isfinite(at(a, b)))
          return false;
        q = vertex(lod, a, b);
        return true;
      };
      scene::Vec3 e, w, s, nn;
      const bool he = neighbour(1, 0, e), hw = neighbour(-1, 0, w), hs = neighbour(0, 1, s), hn = neighbour(0, -1, nn);
      const scene::Vec3 dx = he && hw ? e - w : he ? e - p : hw ? p - w : scene::Vec3{1, 0, 0};
      // rows run southwards: north is j - 1
      const scene::Vec3 dy = hn && hs ? nn - s : hn ? nn - p : hs ? p - s : scene::Vec3{0, 1, 0};
      const scene::Vec3 normal = scene::normalized(scene::cross(dx, dy));
      c = scene::shade(c | 0xFF000000u, normal.z < 0 ? normal * -1.0 : normal, light);
      if (showInterpolated_ && lod.interpolated[k])
        c = (i + j) % 2 == 0 ? scene::mix(c, 0xFFF0F0F0u, 0.6) : scene::mix(c, 0xFFFFFFFFu, 0.2);
      lod.color[k] = c;
    }
}

double TerrainViewModel::sceneRadius() const {
  if (!grid_)
    return 1.0;
  const Grid &g = *grid_;
  const double w = (g.xmax - g.xmin) * metersX_, h = (g.ymax - g.ymin) * metersY_;
  const double z = (g.zmax - g.zmin) * exaggeration_;
  return std::max(1e-3, 0.5 * std::sqrt(w * w + h * h + z * z));
}

void TerrainViewModel::changed() {
  revision_++;
  emit sceneChanged();
  emit viewStateChanged();
}

void TerrainViewModel::setColorMap(int value) {
  if (value == colorMap_ || value < 0 || value > 3)
    return;
  colorMap_ = value;
  recolor();
  changed();
}

void TerrainViewModel::setExaggeration(double value) {
  value = std::clamp(value, 0.1, 200.0);
  if (std::fabs(value - exaggeration_) < 1e-9)
    return;
  camera_.target.z *= value / exaggeration_;
  exaggeration_ = value;
  recolor();
  changed();
}

void TerrainViewModel::setPerspective(bool value) {
  if (value == camera_.perspective)
    return;
  camera_.perspective = value;
  changed();
}

void TerrainViewModel::setYaw(double value) {
  value = std::fmod(std::fmod(value, 360.0) + 360.0, 360.0);
  if (std::fabs(value - camera_.yaw) < 1e-9)
    return;
  camera_.yaw = value;
  changed();
}

void TerrainViewModel::setPitch(double value) {
  value = std::clamp(value, 1.0, 89.9);
  if (std::fabs(value - camera_.pitch) < 1e-9)
    return;
  camera_.pitch = value;
  changed();
}

void TerrainViewModel::setDepthsPositive(bool value) {
  if (value == depthsPositive_)
    return;
  depthsPositive_ = value;
  camera_.target.z = -camera_.target.z;
  recolor();
  changed();
}

void TerrainViewModel::setShowInterpolated(bool value) {
  if (value == showInterpolated_)
    return;
  showInterpolated_ = value;
  recolor();
  changed();
}

void TerrainViewModel::setTool(int value) {
  value = std::clamp(value, 0, 2);
  if (value == tool_)
    return;
  tool_ = value;
  clearMarkers();
  emit toolChanged();
}

void TerrainViewModel::fit() {
  if (!grid_)
    return;
  const Grid &g = *grid_;
  camera_.target = scene::Vec3{0.0, 0.0, 0.5 * (worldZ(g.zmin) + worldZ(g.zmax))};
  camera_.distance = sceneRadius() / std::tan(0.5 * camera_.fov * kDeg) * 1.1;
  changed();
}

void TerrainViewModel::standardView(int which) {
  if (which == 0) {
    camera_.yaw = 0.0;
    camera_.pitch = 89.9;
  } else if (which == 1) {
    camera_.yaw = 0.0;
    camera_.pitch = 10.0;
  } else {
    camera_.yaw = 90.0;
    camera_.pitch = 10.0;
  }
  fit();
}

void TerrainViewModel::orbit(double dx, double dy) {
  camera_.yaw = std::fmod(std::fmod(camera_.yaw - 0.4 * dx, 360.0) + 360.0, 360.0);
  camera_.pitch = std::clamp(camera_.pitch + 0.3 * dy, 1.0, 89.9);
  changed();
}

void TerrainViewModel::pan(double dx, double dy, double width, double height) {
  const scene::View view(camera_, std::max(1, static_cast<int>(width)), std::max(1, static_cast<int>(height)),
                         sceneRadius());
  const double ppu = view.pixelsPerUnit(camera_.distance);
  if (!(ppu > 0.0))
    return;
  camera_.target = camera_.target + view.right() * (-dx / ppu) + view.up() * (dy / ppu);
  changed();
}

void TerrainViewModel::zoom(double factor) {
  if (!(factor > 0.0))
    return;
  const double r = sceneRadius();
  camera_.distance = std::clamp(camera_.distance / factor, r * 0.005, r * 50.0);
  changed();
}

QImage TerrainViewModel::render(int width, int height, bool fast) {
  frame_.resize(width, height);
  frame_.clear(0x00000000u);
  frameLod_ = nullptr;
  if (!grid_) {
    view_.reset();
    QImage empty(std::max(1, width), std::max(1, height), QImage::Format_ARGB32_Premultiplied);
    empty.fill(Qt::transparent);
    return empty;
  }
  view_ = std::make_unique<scene::View>(camera_, frame_.width, frame_.height, sceneRadius());
  const Lod &lod = fast ? coarse_ : fine_;
  frameLod_ = &lod;
  const std::size_t n = lod.z.size();
  std::vector<double> sx(n), sy(n), depth(n);
  std::vector<std::uint8_t> ok(n, 0);
  for (int j = 0; j < lod.ny; j++)
    for (int i = 0; i < lod.nx; i++) {
      const std::size_t k = static_cast<std::size_t>(j) * lod.nx + i;
      if (!std::isfinite(lod.z[k]))
        continue;
      ok[k] = view_->project(vertex(lod, i, j), sx[k], sy[k], depth[k]) ? 1 : 0;
    }
  for (int j = 0; j + 1 < lod.ny; j++)
    for (int i = 0; i + 1 < lod.nx; i++) {
      const std::size_t a = static_cast<std::size_t>(j) * lod.nx + i, b = a + 1, c = a + lod.nx, d = c + 1;
      const auto id = static_cast<std::int32_t>(a);
      auto triangle = [&](std::size_t p, std::size_t q, std::size_t r) {
        if (!ok[p] || !ok[q] || !ok[r])
          return;
        const double x[3] = {sx[p], sx[q], sx[r]}, y[3] = {sy[p], sy[q], sy[r]}, z[3] = {depth[p], depth[q], depth[r]};
        const std::uint32_t col[3] = {lod.color[p], lod.color[q], lod.color[r]};
        scene::drawTriangle(frame_, x, y, z, col, id);
      };
      triangle(a, b, c);
      triangle(b, d, c);
    }
  return QImage(reinterpret_cast<const uchar *>(frame_.color.data()), frame_.width, frame_.height,
                frame_.width * 4, QImage::Format_ARGB32_Premultiplied)
      .copy();
}

bool TerrainViewModel::worldToGrid(double x, double y, double &gx, double &gy) const {
  if (!grid_)
    return false;
  const Grid &g = *grid_;
  const double cx = x / metersX_ + originX_, cy = y / metersY_ + originY_;
  if (g.dx() <= 0.0 || g.dy() <= 0.0)
    return false;
  gx = (cx - g.xmin) / g.dx();
  gy = (g.ymax - cy) / g.dy();
  return gx > -0.5 && gy > -0.5 && gx < g.nx - 0.5 && gy < g.ny - 0.5;
}

bool TerrainViewModel::sample(double gx, double gy, double &value) const {
  const Grid &g = *grid_;
  const int ix = std::clamp(static_cast<int>(std::lround(gx)), 0, g.nx - 1);
  const int iy = std::clamp(static_cast<int>(std::lround(gy)), 0, g.ny - 1);
  const float v = g.at(ix, iy);
  if (!std::isfinite(v))
    return false;
  value = v;
  return true;
}

void TerrainViewModel::gridToGeographic(double gx, double gy, double &lon, double &lat) const {
  const Grid &g = *grid_;
  const double x = g.xmin + gx * g.dx(), y = g.ymax - gy * g.dy();
  if (g.geographic || !projection_.valid() || !projection_.inverse(x, y, lon, lat)) {
    lon = x;
    lat = y;
  }
}

QVariantMap TerrainViewModel::pick(double x, double y) const {
  QVariantMap out;
  if (!grid_ || !view_ || frameLod_ == nullptr)
    return out;
  const int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
  if (!frame_.inside(ix, iy))
    return out;
  const std::size_t index = static_cast<std::size_t>(iy) * frame_.width + ix;
  if (frame_.id[index] < 0)
    return out;
  const scene::Vec3 w = view_->unproject(ix + 0.5, iy + 0.5, frame_.depth[index]);
  double gx = 0.0, gy = 0.0, value = 0.0;
  if (!worldToGrid(w.x, w.y, gx, gy))
    return out;
  const Grid &g = *grid_;
  const bool valid = sample(gx, gy, value);
  out[QStringLiteral("valid")] = valid;
  out[QStringLiteral("worldX")] = w.x;
  out[QStringLiteral("worldY")] = w.y;
  out[QStringLiteral("x")] = g.xmin + gx * g.dx();
  out[QStringLiteral("y")] = g.ymax - gy * g.dy();
  double lon = 0.0, lat = 0.0;
  gridToGeographic(gx, gy, lon, lat);
  out[QStringLiteral("lon")] = lon;
  out[QStringLiteral("lat")] = lat;
  out[QStringLiteral("geographic")] = g.geographic || projection_.valid();
  if (valid)
    out[QStringLiteral("z")] = value;
  if (num_) {
    const int cx = std::clamp(static_cast<int>(std::lround(gx)), 0, g.nx - 1);
    const int cy = std::clamp(static_cast<int>(std::lround(gy)), 0, g.ny - 1);
    const float n = num_->at(cx, cy);
    out[QStringLiteral("count")] = std::isfinite(n) ? static_cast<double>(n) : 0.0;
    out[QStringLiteral("interpolated")] = valid && !(n > 0.0f);
  }
  return out;
}

void TerrainViewModel::hover(double x, double y) {
  probe_ = pick(x, y);
  emit probeChanged();
}

void TerrainViewModel::leave() {
  if (probe_.isEmpty())
    return;
  probe_.clear();
  emit probeChanged();
}

bool TerrainViewModel::addMarker(double x, double y) {
  const QVariantMap p = pick(x, y);
  if (p.isEmpty() || !p.value(QStringLiteral("valid")).toBool())
    return false;
  if (markers_.size() >= 2)
    markers_.clear();
  markers_.push_back(scene::Vec3{p.value(QStringLiteral("worldX")).toDouble(), p.value(QStringLiteral("worldY")).toDouble(),
                                 p.value(QStringLiteral("z")).toDouble()});
  updateProfile();
  emit markersChanged();
  return true;
}

void TerrainViewModel::clearMarkers() {
  if (markers_.empty() && profile_.isEmpty())
    return;
  markers_.clear();
  profile_.clear();
  emit markersChanged();
}

void TerrainViewModel::updateProfile() {
  profile_.clear();
  if (markers_.size() < 2 || !grid_)
    return;
  const scene::Vec3 a = markers_[0], b = markers_[1];
  const double length = std::hypot(b.x - a.x, b.y - a.y);
  for (int k = 0; k <= kProfileSamples; k++) {
    const double t = static_cast<double>(k) / kProfileSamples;
    double gx = 0.0, gy = 0.0, value = 0.0;
    if (!worldToGrid(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), gx, gy) || !sample(gx, gy, value))
      continue;
    profile_.push_back(QPointF(t * length, value));
  }
}

QVariantMap TerrainViewModel::measurement() const {
  QVariantMap out;
  if (markers_.size() < 2 || !grid_)
    return out;
  const scene::Vec3 a = markers_[0], b = markers_[1];
  double horizontal = std::hypot(b.x - a.x, b.y - a.y);
  if (grid_->geographic) {
    // geodesic distance between the two positions
    const double lon1 = a.x / metersX_ + originX_, lat1 = a.y / metersY_ + originY_;
    const double lon2 = b.x / metersX_ + originX_, lat2 = b.y / metersY_ + originY_;
    horizontal = platform::geodesicDistance(lon1, lat1, lon2, lat2);
  }
  const double dz = b.z - a.z;
  out[QStringLiteral("horizontal")] = horizontal;
  out[QStringLiteral("dz")] = dz;
  out[QStringLiteral("slopeDistance")] = std::sqrt(horizontal * horizontal + dz * dz);
  out[QStringLiteral("slopeDegrees")] = horizontal > 0.0 ? std::atan(std::fabs(dz) / horizontal) / kDeg : 90.0;
  return out;
}

QVector<QPointF> TerrainViewModel::markerPositions() const {
  QVector<QPointF> out;
  if (!view_)
    return out;
  for (const auto &m : markers_) {
    double sx = 0.0, sy = 0.0, depth = 0.0;
    if (view_->project(scene::Vec3{m.x, m.y, worldZ(m.z)}, sx, sy, depth))
      out.push_back(QPointF(sx, sy));
  }
  return out;
}

QVariantList TerrainViewModel::legend() const {
  QVariantList out;
  if (!renderer_)
    return out;
  for (int i = 0; i <= 10; i++) {
    const double t = i / 10.0;
    out.push_back(QVariantMap{{QStringLiteral("position"), t},
                              {QStringLiteral("color"),
                               QColor::fromRgba(renderer_->color(static_cast<ColorMap>(colorMap_), t)).name()}});
  }
  return out;
}

QString TerrainViewModel::viewState() const {
  const double r = sceneRadius();
  QJsonObject o{{QStringLiteral("yaw"), camera_.yaw},
                {QStringLiteral("pitch"), camera_.pitch},
                {QStringLiteral("distance"), camera_.distance / r},
                {QStringLiteral("tx"), camera_.target.x / r},
                {QStringLiteral("ty"), camera_.target.y / r},
                {QStringLiteral("tz"), camera_.target.z / r},
                {QStringLiteral("perspective"), camera_.perspective},
                {QStringLiteral("exaggeration"), exaggeration_},
                {QStringLiteral("colorMap"), colorMap_},
                {QStringLiteral("depthsPositive"), depthsPositive_},
                {QStringLiteral("showInterpolated"), showInterpolated_}};
  return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

void TerrainViewModel::setViewState(const QString &json) {
  if (json.isEmpty())
    return;
  if (!grid_) {
    pendingState_ = json;
    return;
  }
  const QJsonObject o = QJsonDocument::fromJson(json.toUtf8()).object();
  if (o.isEmpty())
    return;
  exaggeration_ = std::clamp(o.value(QStringLiteral("exaggeration")).toDouble(exaggeration_), 0.1, 200.0);
  colorMap_ = std::clamp(o.value(QStringLiteral("colorMap")).toInt(colorMap_), 0, 3);
  depthsPositive_ = o.value(QStringLiteral("depthsPositive")).toBool(depthsPositive_);
  showInterpolated_ = o.value(QStringLiteral("showInterpolated")).toBool(showInterpolated_);
  const double r = sceneRadius();
  camera_.yaw = o.value(QStringLiteral("yaw")).toDouble(camera_.yaw);
  camera_.pitch = std::clamp(o.value(QStringLiteral("pitch")).toDouble(camera_.pitch), 1.0, 89.9);
  camera_.distance = std::max(1e-6, o.value(QStringLiteral("distance")).toDouble(camera_.distance / r) * r);
  camera_.target = scene::Vec3{o.value(QStringLiteral("tx")).toDouble() * r, o.value(QStringLiteral("ty")).toDouble() * r,
                               o.value(QStringLiteral("tz")).toDouble(camera_.target.z / r) * r};
  camera_.perspective = o.value(QStringLiteral("perspective")).toBool(camera_.perspective);
  recolor();
  changed();
}

QString TerrainViewModel::statusText() const {
  if (probe_.isEmpty())
    return QString();
  const bool geo = probe_.value(QStringLiteral("geographic")).toBool();
  QString text = geo ? tr("lon %1  lat %2").arg(fmt(probe_.value(QStringLiteral("lon")).toDouble(), 6),
                                               fmt(probe_.value(QStringLiteral("lat")).toDouble(), 6))
                     : tr("x %1  y %2").arg(fmt(probe_.value(QStringLiteral("x")).toDouble(), 1),
                                           fmt(probe_.value(QStringLiteral("y")).toDouble(), 1));
  if (probe_.contains(QStringLiteral("z")))
    text += QStringLiteral("  z %1 %2").arg(fmt(probe_.value(QStringLiteral("z")).toDouble(), 2),
                                           info_.value(QStringLiteral("zUnits")).toString());
  else
    text += QStringLiteral("  ") + tr("no data");
  if (probe_.value(QStringLiteral("interpolated")).toBool())
    text += QStringLiteral("  ") + tr("(interpolated)");
  return text;
}

}  // namespace mbstudio::ui
