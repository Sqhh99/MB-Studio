#include "SoundingEditorModel.h"

#include <QColor>
#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPolygonF>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <cmath>

namespace mbstudio::ui {

namespace {

constexpr std::size_t kMaxSoundings = 6000000;   // memory bound of one editor
constexpr std::size_t kFastPoints = 400000;      // drawn while the camera moves
constexpr double kMetersPerDegree = 111319.49;
constexpr double kDeg = 3.14159265358979323846 / 180.0;

const std::uint32_t kLineColors[] = {0xFF4FC3F7, 0xFFFFB74D, 0xFF81C784, 0xFFE57373, 0xFFBA68C8,
                                     0xFF4DB6AC, 0xFFFFF176, 0xFF90A4AE, 0xFFF06292, 0xFFAED581};
constexpr std::uint32_t kSelected = 0xFFFFD54F;
constexpr std::uint32_t kCurrentPing = 0xFFFF4081;
constexpr std::uint32_t kGood = 0xFF9DB4C8;
constexpr std::uint32_t kManual = 0xFFE5534B;
constexpr std::uint32_t kFilter = 0xFFF2A93B;
constexpr std::uint32_t kSonar = 0xFFA371F7;
constexpr std::uint8_t kManualFlag = beamflag::kFlag | beamflag::kManual;

QString flagText(std::uint8_t f) {
  if (beamflag::isGood(f))
    return QCoreApplication::translate("SoundingEditorModel", "good");
  if (f & beamflag::kManual)
    return QCoreApplication::translate("SoundingEditorModel", "flagged (editor)");
  if (f & beamflag::kSonar)
    return QCoreApplication::translate("SoundingEditorModel", "flagged (sonar)");
  return QCoreApplication::translate("SoundingEditorModel", "flagged (filter)");
}

}  // namespace

SoundingEditorModel::SoundingEditorModel(std::shared_ptr<IProjectService> project,
                                         std::shared_ptr<ISoundingService> soundings,
                                         std::shared_ptr<IGridRenderService> renderer, QObject *parent)
    : QObject(parent), project_(std::move(project)), soundings_(std::move(soundings)), renderer_(std::move(renderer)) {
  connect(&watcher_, &QFutureWatcher<std::shared_ptr<Data>>::finished, this, [this]() {
    const std::shared_ptr<Data> d = watcher_.result();
    loading_ = false;
    error_ = d->error;
    warning_ = d->warning;
    lines_ = d->lines;
    pings_ = d->pings;
    x_ = std::move(d->x);
    y_ = std::move(d->y);
    z_ = std::move(d->z);
    across_ = std::move(d->across);
    flag_ = std::move(d->flag);
    ping_ = std::move(d->ping);
    beam_ = std::move(d->beam);
    lon0_ = d->lon0;
    lat0_ = d->lat0;
    metersX_ = d->metersX;
    metersY_ = d->metersY;
    baseline_ = flag_;
    selected_.assign(flag_.size(), 0);
    selectedCount_ = 0;
    undo_.clear();
    undoPosition_ = 0;
    loaded_ = !flag_.empty();
    if (!loaded_ && error_.isEmpty())
      error_ = tr("The survey lines have no soundings");
    depthMin_ = depthMax_ = 0.0;
    maxBeams_ = 1;
    bool first = true;
    for (std::size_t i = 0; i < z_.size(); i++) {
      const double depth = -z_[i];
      if (first) {
        depthMin_ = depthMax_ = depth;
        first = false;
      }
      depthMin_ = std::min(depthMin_, depth);
      depthMax_ = std::max(depthMax_, depth);
      maxBeams_ = std::max(maxBeams_, static_cast<int>(beam_[i]) + 1);
    }
    double minX = 0, maxX = 0, minY = 0, maxY = 0;
    for (std::size_t i = 0; i < x_.size(); i++) {
      minX = std::min(minX, static_cast<double>(x_[i]));
      maxX = std::max(maxX, static_cast<double>(x_[i]));
      minY = std::min(minY, static_cast<double>(y_[i]));
      maxY = std::max(maxY, static_cast<double>(y_[i]));
    }
    exaggeration_ = scene::niceExaggeration(std::max(maxX - minX, maxY - minY), depthMax_ - depthMin_);
    currentPing_ = pings_.empty() ? -1 : 0;
    camera_ = scene::Camera();
    fit();
    if (!pendingState_.isEmpty()) {
      const QString state = pendingState_;
      pendingState_.clear();
      setViewState(state);
    }
    emit stateChanged();
    emit selectionChanged();
    emit historyChanged();
    emit pingChanged();
    countsUpdated();
    changed();
  });
}

SoundingEditorModel::~SoundingEditorModel() { watcher_.waitForFinished(); }

void SoundingEditorModel::load(const QStringList &lineIds) {
  if (!project_ || !soundings_ || lineIds.isEmpty())
    return;
  watcher_.waitForFinished();
  lineIds_ = lineIds;
  loading_ = true;
  error_.clear();
  emit stateChanged();
  auto project = project_;
  auto soundings = soundings_;
  std::vector<std::string> ids;
  for (const auto &id : lineIds)
    ids.push_back(id.toStdString());
  watcher_.setFuture(QtConcurrent::run([project, soundings, ids]() {
    auto d = std::make_shared<Data>();
    std::vector<SurveyLine> selected;
    for (const auto &line : project->lines())
      if (std::find(ids.begin(), ids.end(), line.id) != ids.end() && !line.missing)
        selected.push_back(line);
    if (selected.empty()) {
      d->error = QCoreApplication::translate("SoundingEditorModel", "The survey lines are not in the project");
      return d;
    }
    // local metric coordinates around the centre of the lines
    GeoBounds b;
    bool first = true;
    for (const auto &line : selected) {
      if (!line.bounds.valid())
        continue;
      if (first) {
        b = line.bounds;
        first = false;
      }
      b.west = std::min(b.west, line.bounds.west);
      b.east = std::max(b.east, line.bounds.east);
      b.south = std::min(b.south, line.bounds.south);
      b.north = std::max(b.north, line.bounds.north);
    }
    bool haveOrigin = b.valid();
    d->lon0 = 0.5 * (b.west + b.east);
    d->lat0 = 0.5 * (b.south + b.north);
    for (const auto &line : selected) {
      const std::size_t budget = kMaxSoundings > d->flag.size() ? kMaxSoundings - d->flag.size() : 0;
      if (budget == 0) {
        d->warning = QCoreApplication::translate(
            "SoundingEditorModel",
            "Only part of the soundings were loaded (limit of %1 soundings); edit fewer lines at a time.")
                         .arg(kMaxSoundings);
        break;
      }
      // edits apply to the file mbprocess reads: the working file
      const SoundingSet set = soundings->load(line.workPath(), line.workFormat(), budget, nullptr);
      if (!set.ok) {
        d->error = QStringLiteral("%1: %2").arg(QFileInfo(QString::fromStdString(line.path)).fileName(),
                                               QString::fromStdString(set.error));
        continue;
      }
      if (set.truncated)
        d->warning = QCoreApplication::translate(
            "SoundingEditorModel",
            "Only part of the soundings were loaded (limit of %1 soundings); edit fewer lines at a time.")
                         .arg(kMaxSoundings);
      if (!haveOrigin && !set.pings.empty()) {
        d->lon0 = set.pings.front().navLon;
        d->lat0 = set.pings.front().navLat;
        haveOrigin = true;
      }
      d->metersY = kMetersPerDegree;
      d->metersX = kMetersPerDegree * std::max(0.01, std::cos(d->lat0 * kDeg));
      Line l;
      l.id = QString::fromStdString(line.id);
      l.name = QFileInfo(QString::fromStdString(line.path)).fileName();
      l.path = QString::fromStdString(set.path);
      l.format = set.format;
      l.esf = QString::fromStdString(set.esfPath);
      l.savedEdits = set.savedEdits;
      l.firstPing = static_cast<std::uint32_t>(d->pings.size());
      l.firstPoint = static_cast<std::uint32_t>(d->flag.size());
      const auto lineIndex = static_cast<std::uint16_t>(d->lines.size());
      for (const auto &p : set.pings) {
        Ping ping;
        ping.time = p.time;
        ping.multiplicity = p.multiplicity;
        ping.line = lineIndex;
        ping.first = static_cast<std::uint32_t>(d->flag.size());
        const auto pingIndex = static_cast<std::uint32_t>(d->pings.size());
        for (std::uint32_t k = 0; k < p.beams; k++) {
          const std::size_t i = p.first + k;
          const std::uint8_t f = set.flag[i];
          if (beamflag::isNull(f) || !std::isfinite(set.depth[i]) || !std::isfinite(set.lon[i]) ||
              !std::isfinite(set.lat[i]))
            continue;  // no sounding to show or edit
          d->x.push_back(static_cast<float>((set.lon[i] - d->lon0) * d->metersX));
          d->y.push_back(static_cast<float>((set.lat[i] - d->lat0) * d->metersY));
          d->z.push_back(-set.depth[i]);
          d->across.push_back(set.acrossTrack[i]);
          d->flag.push_back(f);
          d->ping.push_back(pingIndex);
          d->beam.push_back(static_cast<std::uint16_t>(k));
        }
        ping.count = static_cast<std::uint32_t>(d->flag.size()) - ping.first;
        d->pings.push_back(ping);
      }
      l.pingCount = static_cast<std::uint32_t>(d->pings.size()) - l.firstPing;
      l.pointCount = static_cast<std::uint32_t>(d->flag.size()) - l.firstPoint;
      d->lines.push_back(l);
    }
    return d;
  }));
}

bool SoundingEditorModel::waitForFinished(int timeoutMs) {
  QElapsedTimer timer;
  timer.start();
  while (loading_ && timer.elapsed() < timeoutMs)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
  return !loading_;
}

QVariantList SoundingEditorModel::lines() const {
  QVariantList out;
  for (int i = 0; i < lines_.size(); i++) {
    const Line &l = lines_[i];
    out.push_back(QVariantMap{{QStringLiteral("lineId"), l.id},
                              {QStringLiteral("name"), l.name},
                              {QStringLiteral("path"), l.path},
                              {QStringLiteral("pings"), static_cast<int>(l.pingCount)},
                              {QStringLiteral("soundings"), static_cast<int>(l.pointCount)},
                              {QStringLiteral("color"), QColor::fromRgba(kLineColors[i % 10]).name()},
                              {QStringLiteral("esf"), l.esf},
                              {QStringLiteral("savedEdits"), static_cast<double>(l.savedEdits)}});
  }
  return out;
}

int SoundingEditorModel::flaggedCount() const {
  return static_cast<int>(std::count_if(flag_.begin(), flag_.end(), [](std::uint8_t f) { return beamflag::isFlagged(f); }));
}

int SoundingEditorModel::editCount() const {
  int n = 0;
  for (std::size_t i = 0; i < flag_.size(); i++)
    n += flag_[i] != baseline_[i] ? 1 : 0;
  return n;
}

void SoundingEditorModel::countsUpdated() { emit countsChanged(); }

void SoundingEditorModel::changed() {
  revision_++;
  emit sceneChanged();
  emit viewStateChanged();
}

double SoundingEditorModel::sceneRadius() const {
  if (x_.empty())
    return 1.0;
  double minX = x_[0], maxX = x_[0], minY = y_[0], maxY = y_[0];
  for (std::size_t i = 0; i < x_.size(); i += 16) {
    minX = std::min(minX, static_cast<double>(x_[i]));
    maxX = std::max(maxX, static_cast<double>(x_[i]));
    minY = std::min(minY, static_cast<double>(y_[i]));
    maxY = std::max(maxY, static_cast<double>(y_[i]));
  }
  const double w = maxX - minX, h = maxY - minY, z = (depthMax_ - depthMin_) * exaggeration_;
  return std::max(1.0, 0.5 * std::sqrt(w * w + h * h + z * z));
}

void SoundingEditorModel::fit() {
  if (x_.empty())
    return;
  double sx = 0.0, sy = 0.0;
  for (std::size_t i = 0; i < x_.size(); i++) {
    sx += x_[i];
    sy += y_[i];
  }
  const double n = static_cast<double>(x_.size());
  camera_.target = scene::Vec3{sx / n, sy / n, -0.5 * (depthMin_ + depthMax_) * exaggeration_};
  camera_.distance = sceneRadius() / std::tan(0.5 * camera_.fov * kDeg) * 1.1;
  changed();
}

void SoundingEditorModel::standardView(int which) {
  camera_.yaw = which == 2 ? 90.0 : 0.0;
  camera_.pitch = which == 0 ? 89.9 : 8.0;
  fit();
}

void SoundingEditorModel::orbit(double dx, double dy) {
  camera_.yaw = std::fmod(std::fmod(camera_.yaw - 0.4 * dx, 360.0) + 360.0, 360.0);
  camera_.pitch = std::clamp(camera_.pitch + 0.3 * dy, 1.0, 89.9);
  changed();
}

void SoundingEditorModel::pan(double dx, double dy, double width, double height) {
  const scene::View view(camera_, std::max(1, static_cast<int>(width)), std::max(1, static_cast<int>(height)),
                         sceneRadius());
  const double ppu = view.pixelsPerUnit(camera_.distance);
  if (!(ppu > 0.0))
    return;
  camera_.target = camera_.target + view.right() * (-dx / ppu) + view.up() * (dy / ppu);
  changed();
}

void SoundingEditorModel::zoom(double factor) {
  if (!(factor > 0.0))
    return;
  const double r = sceneRadius();
  camera_.distance = std::clamp(camera_.distance / factor, r * 0.0005, r * 50.0);
  changed();
}

void SoundingEditorModel::setColorMode(int value) {
  value = std::clamp(value, 0, 3);
  if (value == colorMode_)
    return;
  colorMode_ = value;
  changed();
}

void SoundingEditorModel::setShowFlagged(bool value) {
  if (value == showFlagged_)
    return;
  showFlagged_ = value;
  changed();
}

void SoundingEditorModel::setPointSize(int value) {
  value = std::clamp(value, 1, 8);
  if (value == pointSize_)
    return;
  pointSize_ = value;
  changed();
}

void SoundingEditorModel::setExaggeration(double value) {
  value = std::clamp(value, 0.1, 200.0);
  if (std::fabs(value - exaggeration_) < 1e-9)
    return;
  camera_.target.z *= value / exaggeration_;
  exaggeration_ = value;
  changed();
}

void SoundingEditorModel::setPerspective(bool value) {
  if (value == camera_.perspective)
    return;
  camera_.perspective = value;
  changed();
}

void SoundingEditorModel::setTool(int value) {
  value = std::clamp(value, 0, 4);
  if (value == tool_)
    return;
  tool_ = value;
  emit toolChanged();
  emit viewStateChanged();
}

void SoundingEditorModel::setSelectionMode(int value) {
  value = std::clamp(value, 0, 2);
  if (value == selectionMode_)
    return;
  selectionMode_ = value;
  emit toolChanged();
}

void SoundingEditorModel::setBrushRadius(int value) {
  value = std::clamp(value, 2, 200);
  if (value == brushRadius_)
    return;
  brushRadius_ = value;
  emit toolChanged();
}

void SoundingEditorModel::setCurrentPing(int value) {
  value = pings_.empty() ? -1 : std::clamp(value, 0, static_cast<int>(pings_.size()) - 1);
  if (value == currentPing_)
    return;
  currentPing_ = value;
  emit pingChanged();
  changed();
}

void SoundingEditorModel::stepPing(int delta) { setCurrentPing(std::max(0, currentPing_ + delta)); }

QVariantMap SoundingEditorModel::pingInfo() const {
  QVariantMap out;
  if (currentPing_ < 0 || currentPing_ >= static_cast<int>(pings_.size()))
    return out;
  const Ping &p = pings_[static_cast<std::size_t>(currentPing_)];
  const Line &l = lines_[p.line];
  out[QStringLiteral("index")] = currentPing_;
  out[QStringLiteral("line")] = l.name;
  out[QStringLiteral("lineId")] = l.id;
  out[QStringLiteral("number")] = static_cast<int>(static_cast<std::uint32_t>(currentPing_) - l.firstPing) + 1;
  out[QStringLiteral("pings")] = static_cast<int>(l.pingCount);
  out[QStringLiteral("time")] =
      QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(p.time * 1000.0)).toUTC().toString(Qt::ISODateWithMs);
  out[QStringLiteral("multiplicity")] = p.multiplicity;
  out[QStringLiteral("soundings")] = static_cast<int>(p.count);
  return out;
}

QVariantList SoundingEditorModel::pingBeams(int ping) const {
  QVariantList out;
  if (ping < 0 || ping >= static_cast<int>(pings_.size()))
    return out;
  const Ping &p = pings_[static_cast<std::size_t>(ping)];
  for (std::uint32_t k = 0; k < p.count; k++) {
    const std::size_t i = p.first + k;
    out.push_back(QVariantMap{{QStringLiteral("x"), across_[i]},
                              {QStringLiteral("y"), -z_[i]},
                              {QStringLiteral("good"), beamflag::isGood(flag_[i])},
                              {QStringLiteral("flag"), static_cast<int>(flag_[i])},
                              {QStringLiteral("beam"), static_cast<int>(beam_[i])},
                              {QStringLiteral("index"), static_cast<int>(i)},
                              {QStringLiteral("selected"), selected_[i] != 0}});
  }
  return out;
}

std::uint32_t SoundingEditorModel::colorOf(std::size_t i) const {
  const std::uint8_t f = flag_[i];
  if (colorMode_ == 2 || (!beamflag::isGood(f) && colorMode_ != 1)) {
    if (beamflag::isGood(f))
      return kGood;
    if (f & beamflag::kManual)
      return kManual;
    if (f & beamflag::kSonar)
      return kSonar;
    return kFilter;
  }
  if (colorMode_ == 1)
    return kLineColors[pings_[ping_[i]].line % 10];
  if (colorMode_ == 3 && renderer_)
    return renderer_->color(ColorMap::Rainbow, static_cast<double>(beam_[i]) / std::max(1, maxBeams_ - 1)) | 0xFF000000u;
  const double range = depthMax_ > depthMin_ ? depthMax_ - depthMin_ : 1.0;
  // shallow red, deep blue (as the grid colour tables)
  const double t = std::clamp((-z_[i] - depthMin_) / range, 0.0, 1.0);
  return renderer_ ? renderer_->color(ColorMap::Haxby, 1.0 - t) | 0xFF000000u : kGood;
}

QImage SoundingEditorModel::render(int width, int height, bool fast) {
  frame_.resize(width, height);
  frame_.clear(0x00000000u);
  if (flag_.empty()) {
    view_.reset();
    QImage empty(std::max(1, width), std::max(1, height), QImage::Format_ARGB32_Premultiplied);
    empty.fill(Qt::transparent);
    return empty;
  }
  view_ = std::make_unique<scene::View>(camera_, frame_.width, frame_.height, sceneRadius());
  const std::size_t n = flag_.size();
  const std::size_t stride = fast ? std::max<std::size_t>(1, n / kFastPoints) : 1;
  const Ping *current = currentPing_ >= 0 && currentPing_ < static_cast<int>(pings_.size())
                            ? &pings_[static_cast<std::size_t>(currentPing_)]
                            : nullptr;
  for (std::size_t i = 0; i < n; i += stride) {
    if (!visible(i))
      continue;
    double sx = 0.0, sy = 0.0, depth = 0.0;
    if (!view_->project(world(i), sx, sy, depth))
      continue;
    std::uint32_t c = colorOf(i);
    int size = pointSize_;
    if (selected_[i]) {
      c = kSelected;
      size += 1;
    } else if (current != nullptr && i >= current->first && i < current->first + current->count) {
      c = kCurrentPing;
      size += 1;
    }
    scene::drawPoint(frame_, sx, sy, depth, c, size, static_cast<std::int32_t>(i));
  }
  return QImage(reinterpret_cast<const uchar *>(frame_.color.data()), frame_.width, frame_.height,
                frame_.width * 4, QImage::Format_ARGB32_Premultiplied)
      .copy();
}

template <typename Inside>
int SoundingEditorModel::select(Inside inside) {
  if (!view_ || flag_.empty())
    return selectedCount_;
  if (selectionMode_ == 0)
    std::fill(selected_.begin(), selected_.end(), 0);
  const std::uint8_t value = selectionMode_ == 2 ? 0 : 1;
  for (std::size_t i = 0; i < flag_.size(); i++) {
    if (!visible(i))
      continue;
    double sx = 0.0, sy = 0.0, depth = 0.0;
    if (!view_->project(world(i), sx, sy, depth))
      continue;
    if (inside(sx, sy))
      selected_[i] = value;
  }
  selectedCount_ = static_cast<int>(std::count(selected_.begin(), selected_.end(), 1));
  emit selectionChanged();
  changed();
  return selectedCount_;
}

int SoundingEditorModel::selectRect(double x0, double y0, double x1, double y1) {
  const double left = std::min(x0, x1), right = std::max(x0, x1), top = std::min(y0, y1), bottom = std::max(y0, y1);
  return select([=](double x, double y) { return x >= left && x <= right && y >= top && y <= bottom; });
}

int SoundingEditorModel::selectLasso(const QVariantList &points) {
  QPolygonF polygon;
  for (const auto &p : points) {
    if (p.canConvert<QPointF>()) {
      polygon << p.toPointF();
    } else {
      const QVariantMap m = p.toMap();
      polygon << QPointF(m.value(QStringLiteral("x")).toDouble(), m.value(QStringLiteral("y")).toDouble());
    }
  }
  if (polygon.size() < 3)
    return selectedCount_;
  const QRectF box = polygon.boundingRect();
  return select([&polygon, &box](double x, double y) {
    return box.contains(x, y) && polygon.containsPoint(QPointF(x, y), Qt::OddEvenFill);
  });
}

int SoundingEditorModel::selectBrush(double cx, double cy) {
  const double r2 = static_cast<double>(brushRadius_) * brushRadius_;
  // a brush stroke adds (or removes) without clearing what it already painted
  const int mode = selectionMode_;
  if (selectionMode_ == 0)
    selectionMode_ = 1;
  const int n = select([=](double x, double y) { return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r2; });
  selectionMode_ = mode;
  return n;
}

int SoundingEditorModel::soundingAt(double x, double y) const {
  if (!view_)
    return -1;
  const int cx = static_cast<int>(std::floor(x)), cy = static_cast<int>(std::floor(y));
  // the nearest drawn sounding within a few pixels
  int best = -1;
  double bestDistance = 1e9;
  for (int dy = -4; dy <= 4; dy++)
    for (int dx = -4; dx <= 4; dx++) {
      if (!frame_.inside(cx + dx, cy + dy))
        continue;
      const std::int32_t id = frame_.id[static_cast<std::size_t>(cy + dy) * frame_.width + cx + dx];
      const double d = dx * dx + dy * dy;
      if (id >= 0 && d < bestDistance) {
        best = id;
        bestDistance = d;
      }
    }
  return best;
}

int SoundingEditorModel::selectAt(double x, double y) {
  const int index = soundingAt(x, y);
  if (selectionMode_ == 0)
    std::fill(selected_.begin(), selected_.end(), 0);
  if (index >= 0) {
    selected_[static_cast<std::size_t>(index)] = selectionMode_ == 2 ? 0 : 1;
    setCurrentPing(static_cast<int>(ping_[static_cast<std::size_t>(index)]));
  }
  selectedCount_ = static_cast<int>(std::count(selected_.begin(), selected_.end(), 1));
  emit selectionChanged();
  changed();
  return selectedCount_;
}

void SoundingEditorModel::selectAll() {
  for (std::size_t i = 0; i < flag_.size(); i++)
    selected_[i] = visible(i) ? 1 : 0;
  selectedCount_ = static_cast<int>(std::count(selected_.begin(), selected_.end(), 1));
  emit selectionChanged();
  changed();
}

void SoundingEditorModel::clearSelection() {
  if (selectedCount_ == 0)
    return;
  std::fill(selected_.begin(), selected_.end(), 0);
  selectedCount_ = 0;
  emit selectionChanged();
  changed();
}

int SoundingEditorModel::applyFlags(bool flag) {
  Step step;
  for (std::size_t i = 0; i < flag_.size(); i++) {
    if (!selected_[i])
      continue;
    const std::uint8_t f = flag_[i];
    if (flag && beamflag::isGood(f)) {
      step.indices.push_back(static_cast<std::uint32_t>(i));
      step.before.push_back(f);
      step.after.push_back(kManualFlag);
    } else if (!flag && beamflag::isFlagged(f)) {
      step.indices.push_back(static_cast<std::uint32_t>(i));
      step.before.push_back(f);
      step.after.push_back(0);
    }
  }
  std::fill(selected_.begin(), selected_.end(), 0);
  selectedCount_ = 0;
  emit selectionChanged();
  if (step.indices.empty()) {
    changed();
    return 0;
  }
  apply(step, true);
  undo_.resize(static_cast<std::size_t>(undoPosition_));
  undo_.push_back(std::move(step));
  undoPosition_ = static_cast<int>(undo_.size());
  emit historyChanged();
  return static_cast<int>(undo_.back().indices.size());
}

int SoundingEditorModel::flagSelected() { return applyFlags(true); }
int SoundingEditorModel::unflagSelected() { return applyFlags(false); }

void SoundingEditorModel::apply(const Step &step, bool forward) {
  const auto &values = forward ? step.after : step.before;
  for (std::size_t k = 0; k < step.indices.size(); k++)
    flag_[step.indices[k]] = values[k];
  countsUpdated();
  changed();
}

bool SoundingEditorModel::undo() {
  if (!canUndo())
    return false;
  undoPosition_--;
  apply(undo_[static_cast<std::size_t>(undoPosition_)], false);
  emit historyChanged();
  return true;
}

bool SoundingEditorModel::redo() {
  if (!canRedo())
    return false;
  apply(undo_[static_cast<std::size_t>(undoPosition_)], true);
  undoPosition_++;
  emit historyChanged();
  return true;
}

void SoundingEditorModel::revert() {
  flag_ = baseline_;
  undo_.clear();
  undoPosition_ = 0;
  emit historyChanged();
  countsUpdated();
  changed();
}

void SoundingEditorModel::toggleBeam(int index) {
  if (index < 0 || index >= static_cast<int>(flag_.size()))
    return;
  const auto i = static_cast<std::size_t>(index);
  Step step;
  step.indices.push_back(static_cast<std::uint32_t>(i));
  step.before.push_back(flag_[i]);
  step.after.push_back(beamflag::isGood(flag_[i]) ? kManualFlag : 0);
  apply(step, true);
  undo_.resize(static_cast<std::size_t>(undoPosition_));
  undo_.push_back(std::move(step));
  undoPosition_ = static_cast<int>(undo_.size());
  emit historyChanged();
  emit pingChanged();
}

std::map<std::string, std::vector<BeamEdit>> SoundingEditorModel::pendingEdits() const {
  std::map<std::string, std::vector<BeamEdit>> out;
  for (std::size_t i = 0; i < flag_.size(); i++) {
    if (flag_[i] == baseline_[i])
      continue;
    const Ping &p = pings_[ping_[i]];
    BeamEdit e;
    e.time = p.time;
    e.multiplicity = p.multiplicity;
    e.beam = beam_[i];
    e.action = beamflag::isGood(flag_[i]) ? BeamAction::Unflag : BeamAction::Flag;
    out[lines_[p.line].id.toStdString()].push_back(e);
  }
  return out;
}

qint64 SoundingEditorModel::save() {
  if (!project_ || saveJob_ != 0)
    return 0;
  const auto edits = pendingEdits();
  if (edits.empty()) {
    emit notify(tr("There are no edits to save"));
    return 0;
  }
  std::string error;
  const std::int64_t id = project_->saveEdits(edits, error);
  if (id == 0) {
    emit notify(tr("The edits could not be saved: %1").arg(QString::fromStdString(error)));
    return 0;
  }
  saveJob_ = id;
  savingFlags_ = flag_;
  emit stateChanged();
  return id;
}

void SoundingEditorModel::jobFinished(qint64 id, const QString &state, const QString &error) {
  if (id == 0 || id != saveJob_)
    return;
  saveJob_ = 0;
  if (state == QLatin1String("completed")) {
    baseline_ = savingFlags_;
    emit saved();
  } else {
    emit notify(tr("The edits could not be saved: %1").arg(error.isEmpty() ? state : error));
  }
  savingFlags_.clear();
  emit stateChanged();
  countsUpdated();
}

QVariantMap SoundingEditorModel::soundingInfo(int index) const {
  QVariantMap out;
  if (index < 0 || index >= static_cast<int>(flag_.size()))
    return out;
  const auto i = static_cast<std::size_t>(index);
  const Ping &p = pings_[ping_[i]];
  const Line &l = lines_[p.line];
  out[QStringLiteral("index")] = index;
  out[QStringLiteral("line")] = l.name;
  out[QStringLiteral("lineId")] = l.id;
  out[QStringLiteral("ping")] = static_cast<int>(ping_[i] - l.firstPing) + 1;
  out[QStringLiteral("pingIndex")] = static_cast<int>(ping_[i]);
  out[QStringLiteral("time")] =
      QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(p.time * 1000.0)).toUTC().toString(Qt::ISODateWithMs);
  out[QStringLiteral("multiplicity")] = p.multiplicity;
  out[QStringLiteral("beam")] = static_cast<int>(beam_[i]);
  out[QStringLiteral("depth")] = -z_[i];
  out[QStringLiteral("lon")] = lon0_ + x_[i] / metersX_;
  out[QStringLiteral("lat")] = lat0_ + y_[i] / metersY_;
  out[QStringLiteral("flag")] = flagText(flag_[i]);
  out[QStringLiteral("edited")] = flag_[i] != baseline_[i];
  return out;
}

void SoundingEditorModel::hover(double x, double y) {
  probe_ = soundingInfo(soundingAt(x, y));
  emit probeChanged();
}

void SoundingEditorModel::leave() {
  if (probe_.isEmpty())
    return;
  probe_.clear();
  emit probeChanged();
}

QVariantList SoundingEditorModel::legend() const {
  QVariantList out;
  if (!renderer_)
    return out;
  for (int i = 0; i <= 10; i++) {
    const double t = i / 10.0;
    out.push_back(QVariantMap{{QStringLiteral("position"), t},
                              {QStringLiteral("color"), QColor::fromRgba(renderer_->color(ColorMap::Haxby, 1.0 - t)).name()}});
  }
  return out;
}

QString SoundingEditorModel::viewState() const {
  const double r = sceneRadius();
  const QJsonObject o{{QStringLiteral("yaw"), camera_.yaw},
                      {QStringLiteral("pitch"), camera_.pitch},
                      {QStringLiteral("distance"), camera_.distance / r},
                      {QStringLiteral("tx"), camera_.target.x / r},
                      {QStringLiteral("ty"), camera_.target.y / r},
                      {QStringLiteral("tz"), camera_.target.z / r},
                      {QStringLiteral("perspective"), camera_.perspective},
                      {QStringLiteral("exaggeration"), exaggeration_},
                      {QStringLiteral("colorMode"), colorMode_},
                      {QStringLiteral("showFlagged"), showFlagged_},
                      {QStringLiteral("pointSize"), pointSize_},
                      {QStringLiteral("tool"), tool_}};
  return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

void SoundingEditorModel::setViewState(const QString &json) {
  if (json.isEmpty())
    return;
  if (!loaded_) {
    pendingState_ = json;
    return;
  }
  const QJsonObject o = QJsonDocument::fromJson(json.toUtf8()).object();
  if (o.isEmpty())
    return;
  exaggeration_ = std::clamp(o.value(QStringLiteral("exaggeration")).toDouble(exaggeration_), 0.1, 200.0);
  colorMode_ = std::clamp(o.value(QStringLiteral("colorMode")).toInt(colorMode_), 0, 3);
  showFlagged_ = o.value(QStringLiteral("showFlagged")).toBool(showFlagged_);
  pointSize_ = std::clamp(o.value(QStringLiteral("pointSize")).toInt(pointSize_), 1, 8);
  tool_ = std::clamp(o.value(QStringLiteral("tool")).toInt(tool_), 0, 4);
  const double r = sceneRadius();
  camera_.yaw = o.value(QStringLiteral("yaw")).toDouble(camera_.yaw);
  camera_.pitch = std::clamp(o.value(QStringLiteral("pitch")).toDouble(camera_.pitch), 1.0, 89.9);
  camera_.distance = std::max(1e-6, o.value(QStringLiteral("distance")).toDouble(camera_.distance / r) * r);
  camera_.target = scene::Vec3{o.value(QStringLiteral("tx")).toDouble(camera_.target.x / r) * r,
                               o.value(QStringLiteral("ty")).toDouble(camera_.target.y / r) * r,
                               o.value(QStringLiteral("tz")).toDouble(camera_.target.z / r) * r};
  camera_.perspective = o.value(QStringLiteral("perspective")).toBool(camera_.perspective);
  emit toolChanged();
  changed();
}

QString SoundingEditorModel::statusText() const {
  if (probe_.isEmpty())
    return QString();
  return tr("%1 · ping %2 · beam %3 · depth %4 m · %5")
      .arg(probe_.value(QStringLiteral("line")).toString())
      .arg(probe_.value(QStringLiteral("ping")).toInt())
      .arg(probe_.value(QStringLiteral("beam")).toInt())
      .arg(QString::number(probe_.value(QStringLiteral("depth")).toDouble(), 'f', 2))
      .arg(probe_.value(QStringLiteral("flag")).toString());
}

}  // namespace mbstudio::ui
