#include "ProjectViewModel.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QPointer>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <cmath>
#include <cstring>

#include "JobListModel.h"
#include "VariantListModel.h"
#include "mbstudio/kernel/Kernel.h"

namespace mbstudio::ui {

namespace {

constexpr int kMaxRecent = 10;
constexpr int kMaxLayerPixels = 2048;  // longest side of a map layer image
constexpr std::size_t kTrackPoints = 1500;

QString q(const std::string &s) { return QString::fromStdString(s); }

QString localPath(const QVariant &item) {
  const QUrl url = item.toUrl();
  if (url.isValid() && url.isLocalFile())
    return url.toLocalFile();
  const QString text = item.toString();
  if (text.startsWith(QLatin1String("file:")))
    return QUrl(text).toLocalFile();
  return text;
}

bool isMapType(const std::string &type) {
  return type == "grid" || type == "grid_num" || type == "grid_sd" || type == "mosaic" || type == "mosaic_num" ||
         type == "mosaic_sd";
}

QString styleOf(const ProjectLayer &l) { return QStringLiteral("%1/%2").arg(l.colorMap).arg(l.hillshade ? 1 : 0); }

struct LayerResult {
  bool ok = false;
  QString error;
  QImage image;
  GeoBounds bounds;
  std::shared_ptr<const Grid> grid;
  platform::MapProjection projection;
};

// A projected grid image on the geographic map: the geographic extent of
// the grid outline, and every map pixel sampled from the projected image.
bool warpToGeographic(const Grid &g, const QImage &projected, const platform::MapProjection &projection,
                      QImage &out, GeoBounds &bounds) {
  const double x0 = g.xmin - 0.5 * g.dx(), x1 = g.xmax + 0.5 * g.dx();
  const double y0 = g.ymin - 0.5 * g.dy(), y1 = g.ymax + 0.5 * g.dy();
  bool first = true;
  constexpr int kEdge = 64;
  for (int k = 0; k <= kEdge; k++) {
    const double t = static_cast<double>(k) / kEdge;
    const double xs[4] = {x0 + t * (x1 - x0), x0 + t * (x1 - x0), x0, x1};
    const double ys[4] = {y0, y1, y0 + t * (y1 - y0), y0 + t * (y1 - y0)};
    for (int e = 0; e < 4; e++) {
      double lon = 0.0, lat = 0.0;
      if (!projection.inverse(xs[e], ys[e], lon, lat))
        return false;
      if (first) {
        bounds = GeoBounds{lon, lon, lat, lat};
        first = false;
      }
      bounds.west = std::min(bounds.west, lon);
      bounds.east = std::max(bounds.east, lon);
      bounds.south = std::min(bounds.south, lat);
      bounds.north = std::max(bounds.north, lat);
    }
  }
  if (!bounds.valid())
    return false;
  const double aspect = (bounds.north - bounds.south) /
                        ((bounds.east - bounds.west) * std::max(0.05, std::cos(0.5 * (bounds.south + bounds.north) * 3.14159265358979323846 / 180.0)));
  const int width = std::clamp(static_cast<int>(projected.width() * 1.2), 16, kMaxLayerPixels);
  const int height = std::clamp(static_cast<int>(width * aspect), 16, kMaxLayerPixels);
  out = QImage(width, height, QImage::Format_ARGB32);
  out.fill(Qt::transparent);
  for (int py = 0; py < height; py++) {
    auto *line = reinterpret_cast<QRgb *>(out.scanLine(py));
    const double lat = bounds.north - (py + 0.5) / height * (bounds.north - bounds.south);
    for (int px = 0; px < width; px++) {
      const double lon = bounds.west + (px + 0.5) / width * (bounds.east - bounds.west);
      double x = 0.0, y = 0.0;
      if (!projection.forward(lon, lat, x, y))
        continue;
      const int ix = static_cast<int>(std::floor((x - x0) / (x1 - x0) * projected.width()));
      const int iy = static_cast<int>(std::floor((y1 - y) / (y1 - y0) * projected.height()));
      if (ix < 0 || iy < 0 || ix >= projected.width() || iy >= projected.height())
        continue;
      line[px] = projected.pixel(ix, iy);
    }
  }
  return true;
}

QDateTime fromEpoch(double t) { return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(t * 1000.0)); }

}  // namespace

ProjectViewModel::ProjectViewModel(Kernel *kernel, std::shared_ptr<IProjectService> project,
                                   std::shared_ptr<IGridService> grids, std::shared_ptr<IGridRenderService> renderer,
                                   QObject *parent)
    : QObject(parent),
      kernel_(kernel),
      project_(std::move(project)),
      grids_(std::move(grids)),
      renderer_(std::move(renderer)),
      settings_(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("MB-Studio"), QStringLiteral("mbstudio")) {
  lines_ = new VariantListModel({QStringLiteral("lineId"), QStringLiteral("name"), QStringLiteral("path"),
                                 QStringLiteral("format"), QStringLiteral("formatName"), QStringLiteral("state"),
                                 QStringLiteral("missing"), QStringLiteral("inspected"), QStringLiteral("records"),
                                 QStringLiteral("timeStart"), QStringLiteral("timeEnd"), QStringLiteral("trackKm"),
                                 QStringLiteral("depthMin"), QStringLiteral("depthMax"), QStringLiteral("sizeMB"),
                                 QStringLiteral("selected"), QStringLiteral("status"), QStringLiteral("versions")},
                                QStringLiteral("lineId"), this);
  products_ = new VariantListModel({QStringLiteral("assetId"), QStringLiteral("label"), QStringLiteral("type"),
                                    QStringLiteral("path"), QStringLiteral("fileName"), QStringLiteral("parent"),
                                    QStringLiteral("quality"), QStringLiteral("mappable"), QStringLiteral("layer"),
                                    QStringLiteral("visible"), QStringLiteral("opacity"), QStringLiteral("colorMap"),
                                    QStringLiteral("hillshade"), QStringLiteral("exists"), QStringLiteral("job"),
                                    QStringLiteral("status"), QStringLiteral("staleInputs"),
                                    QStringLiteral("regenerable")},
                                   QStringLiteral("assetId"), this);
  history_ = new VariantListModel({QStringLiteral("sequence"), QStringLiteral("kind"), QStringLiteral("title"),
                                   QStringLiteral("state"), QStringLiteral("started"), QStringLiteral("duration"),
                                   QStringLiteral("error"), QStringLiteral("commands"), QStringLiteral("logPath"),
                                   QStringLiteral("outputs"), QStringLiteral("jobId"), QStringLiteral("attempt"),
                                   QStringLiteral("retryOf"), QStringLiteral("canRetry"), QStringLiteral("lineCount")},
                                  QStringLiteral("sequence"), this);
  recent_ = settings_.value(QStringLiteral("projects/recent")).toStringList();
  if (kernel_ != nullptr && project_) {
    QPointer<ProjectViewModel> self(this);
    subscription_ = kernel_->events().subscribe("project.changed", [self](const Event &event) {
      const QString what = q(getString(event.data, "what"));
      if (self)
        QMetaObject::invokeMethod(
            self.data(),
            [self, what]() {
              if (self)
                self->onChanged(what);
            },
            Qt::QueuedConnection);
    });
  }
  refresh();
}

ProjectViewModel::~ProjectViewModel() {
  if (kernel_ != nullptr && subscription_ != 0)
    kernel_->events().unsubscribe(subscription_);
}

QObject *ProjectViewModel::lines() const { return lines_; }
QObject *ProjectViewModel::products() const { return products_; }
QObject *ProjectViewModel::history() const { return history_; }
int ProjectViewModel::lineCount() const { return static_cast<int>(lineData_.size()); }

int ProjectViewModel::inspectedCount() const {
  return static_cast<int>(std::count_if(lineData_.begin(), lineData_.end(), [](const SurveyLine &l) { return l.inspected; }));
}

QStringList ProjectViewModel::selectedIds() const {
  QStringList out;
  for (const auto &l : lineData_)  // in project order
    if (selected_.contains(q(l.id)))
      out << q(l.id);
  return out;
}

QString ProjectViewModel::selectionText() const {
  const int n = lineCount();
  if (n == 0)
    return tr("No survey lines");
  if (selected_.isEmpty())
    return n == 1 ? tr("The survey line") : tr("All %1 survey lines").arg(n);
  return tr("%1 of %2 survey lines").arg(selected_.size()).arg(n);
}

std::vector<std::string> ProjectViewModel::selection() const {
  std::vector<std::string> out;
  for (const auto &id : selectedIds())
    out.push_back(id.toStdString());
  return out;
}

GeoBounds ProjectViewModel::selectionBounds() const { return project_ ? project_->bounds(selection()) : GeoBounds(); }

void ProjectViewModel::onChanged(const QString &what) {
  if (what == QLatin1String("opened") || what == QLatin1String("closed")) {
    refresh();
    return;
  }
  if (what == QLatin1String("lines")) {
    refreshLines();
    refreshProducts();  // product states follow the lines
    loadTracks();
  } else if (what == QLatin1String("assets") || what == QLatin1String("layers")) {
    refreshProducts();
    updateLayers();
  } else if (what == QLatin1String("history")) {
    refreshHistory();
  }
  emit progressChanged();
}

void ProjectViewModel::refresh() {
  const bool open = project_ && project_->isOpen();
  const QString file = open ? q(project_->file()) : QString();
  if (file != file_) {
    selected_.clear();
    anchor_.clear();
    tracks_.clear();
    layers_.clear();
    tracksGeneration_++;
    layerGeneration_++;
    pendingLayers_ = 0;
  }
  open_ = open;
  name_ = open ? q(project_->name()) : QString();
  root_ = open ? q(project_->root()) : QString();
  file_ = file;
  emit projectChanged();
  refreshLines();
  refreshProducts();
  refreshHistory();
  loadTracks();
  updateLayers();
  emit selectionChanged();
  emit progressChanged();
  mapRevision_++;
  emit mapChanged();
}

void ProjectViewModel::refreshLines() {
  lineData_ = open_ ? project_->lines() : std::vector<SurveyLine>();
  // drop selected lines that no longer exist
  QSet<QString> ids;
  for (const auto &l : lineData_)
    ids.insert(q(l.id));
  const int before = selected_.size();
  selected_.intersect(ids);
  QVector<QVariantMap> items;
  for (const auto &l : lineData_) {
    QVariantMap m;
    m[QStringLiteral("lineId")] = q(l.id);
    m[QStringLiteral("name")] = QFileInfo(q(l.path)).fileName();
    m[QStringLiteral("path")] = QDir::toNativeSeparators(q(l.path));
    m[QStringLiteral("format")] = l.format;
    m[QStringLiteral("formatName")] = q(l.formatName);
    m[QStringLiteral("state")] = q(l.state);
    m[QStringLiteral("missing")] = l.missing;
    m[QStringLiteral("inspected")] = l.inspected;
    m[QStringLiteral("records")] = static_cast<double>(l.records);
    m[QStringLiteral("timeStart")] = q(l.timeStart).left(19).replace(QLatin1Char('T'), QLatin1Char(' '));
    m[QStringLiteral("timeEnd")] = q(l.timeEnd).left(19).replace(QLatin1Char('T'), QLatin1Char(' '));
    m[QStringLiteral("trackKm")] = l.trackLengthKm;
    m[QStringLiteral("depthMin")] = l.depthMin;
    m[QStringLiteral("depthMax")] = l.depthMax;
    m[QStringLiteral("sizeMB")] = static_cast<double>(l.size) / (1024.0 * 1024.0);
    m[QStringLiteral("selected")] = selected_.contains(q(l.id));
    m[QStringLiteral("status")] = q(l.processingStatus());
    m[QStringLiteral("versions")] = l.processedEdit >= 0 ? 3 : l.processingPath.empty() ? 1 : 2;
    items.push_back(m);
  }
  lines_->setItems(items);
  emit linesChanged();
  if (before != selected_.size())
    emit selectionChanged();
}

void ProjectViewModel::refreshProducts() {
  assetData_ = open_ ? project_->assets() : std::vector<ProjectAsset>();
  layerData_ = open_ ? project_->layers() : std::vector<ProjectLayer>();
  // main products first, each followed by its quality grids
  std::vector<const ProjectAsset *> ordered;
  for (const auto &a : assetData_)
    if (a.parent.empty()) {
      ordered.push_back(&a);
      for (const auto &c : assetData_)
        if (c.parent == a.id)
          ordered.push_back(&c);
    }
  for (const auto &a : assetData_)  // orphans
    if (!a.parent.empty() && std::find(ordered.begin(), ordered.end(), &a) == ordered.end())
      ordered.push_back(&a);
  QVector<QVariantMap> items;
  for (const auto *a : ordered) {
    QVariantMap m;
    m[QStringLiteral("assetId")] = q(a->id);
    m[QStringLiteral("label")] = q(a->label.empty() ? a->path : a->label);
    m[QStringLiteral("type")] = q(a->type);
    m[QStringLiteral("path")] = QDir::toNativeSeparators(q(a->path));
    m[QStringLiteral("fileName")] = QFileInfo(q(a->path)).fileName();
    m[QStringLiteral("parent")] = q(a->parent);
    m[QStringLiteral("quality")] = !a->parent.empty();
    m[QStringLiteral("mappable")] = isMapType(a->type);
    const auto layer =
        std::find_if(layerData_.begin(), layerData_.end(), [a](const ProjectLayer &l) { return l.asset == a->id; });
    m[QStringLiteral("layer")] = layer != layerData_.end();
    m[QStringLiteral("visible")] = layer != layerData_.end() && layer->visible;
    m[QStringLiteral("opacity")] = layer != layerData_.end() ? layer->opacity : 1.0;
    m[QStringLiteral("colorMap")] = layer != layerData_.end() ? layer->colorMap : 0;
    m[QStringLiteral("hillshade")] = layer != layerData_.end() ? layer->hillshade : true;
    m[QStringLiteral("exists")] = QFileInfo::exists(q(a->path));
    m[QStringLiteral("job")] = static_cast<double>(a->job);
    m[QStringLiteral("status")] = q(a->status);
    m[QStringLiteral("staleInputs")] = a->staleInputs;
    m[QStringLiteral("regenerable")] = a->regenerable && a->job > 0;
    items.push_back(m);
  }
  products_->setItems(items);
}

void ProjectViewModel::refreshHistory() {
  historyData_ = open_ ? project_->history() : std::vector<JobRecord>();
  QVector<QVariantMap> items;
  for (auto it = historyData_.rbegin(); it != historyData_.rend(); ++it) {
    const JobRecord &r = *it;
    QVariantMap m;
    m[QStringLiteral("sequence")] = static_cast<double>(r.sequence);
    m[QStringLiteral("kind")] = q(r.kind);
    m[QStringLiteral("title")] = q(r.title);
    m[QStringLiteral("state")] = q(r.state);
    m[QStringLiteral("started")] =
        r.started > 0 ? fromEpoch(r.started).toString(QStringLiteral("yyyy-MM-dd HH:mm")) : QString();
    m[QStringLiteral("duration")] =
        r.finished > r.started && r.started > 0 ? JobListModel::formatElapsed(r.finished - r.started) : QString();
    m[QStringLiteral("error")] = q(r.error);
    QStringList commands;
    for (const auto &c : r.commands)
      commands << q(c);
    m[QStringLiteral("commands")] = commands.join(QLatin1Char('\n'));
    m[QStringLiteral("logPath")] = QDir::toNativeSeparators(q(r.logPath));
    QStringList outputs;
    for (const auto &o : r.outputs)
      outputs << QDir::toNativeSeparators(q(o));
    m[QStringLiteral("outputs")] = outputs;
    m[QStringLiteral("jobId")] = static_cast<double>(r.jobId);
    m[QStringLiteral("attempt")] = r.attempt;
    m[QStringLiteral("retryOf")] = static_cast<double>(r.retryOf);
    m[QStringLiteral("canRetry")] = !r.request.empty() && r.kind != "edit" && r.state != "queued" && r.state != "running";
    m[QStringLiteral("lineCount")] = static_cast<int>(r.lines.size());
    items.push_back(m);
  }
  history_->setItems(items);
}

QVariantMap ProjectViewModel::summary() const {
  QVariantMap m;
  double size = 0.0, km = 0.0, dmin = 0.0, dmax = 0.0;
  qint64 records = 0;
  QString start, end;
  QStringList formats;
  bool depth = false;
  for (const auto &l : lineData_) {
    size += static_cast<double>(l.size);
    km += l.trackLengthKm;
    records += l.records;
    if (l.inspected) {
      if (!depth || l.depthMin < dmin)
        dmin = l.depthMin;
      if (!depth || l.depthMax > dmax)
        dmax = l.depthMax;
      depth = true;
    }
    const QString s = q(l.timeStart), e = q(l.timeEnd);
    if (!s.isEmpty() && (start.isEmpty() || s < start))
      start = s;
    if (!e.isEmpty() && (end.isEmpty() || e > end))
      end = e;
    const QString f = QStringLiteral("%1 (%2)").arg(q(l.formatName)).arg(l.format);
    if (!formats.contains(f))
      formats << f;
  }
  m[QStringLiteral("lines")] = lineCount();
  m[QStringLiteral("sizeMB")] = size / (1024.0 * 1024.0);
  m[QStringLiteral("trackKm")] = km;
  m[QStringLiteral("records")] = static_cast<double>(records);
  m[QStringLiteral("timeStart")] = start.left(19).replace(QLatin1Char('T'), QLatin1Char(' '));
  m[QStringLiteral("timeEnd")] = end.left(19).replace(QLatin1Char('T'), QLatin1Char(' '));
  m[QStringLiteral("depthMin")] = dmin;
  m[QStringLiteral("depthMax")] = dmax;
  m[QStringLiteral("hasDepth")] = depth;
  m[QStringLiteral("formats")] = formats.join(QStringLiteral(", "));
  const GeoBounds b = project_ && open_ ? project_->bounds() : GeoBounds();
  m[QStringLiteral("hasBounds")] = b.valid();
  m[QStringLiteral("west")] = b.west;
  m[QStringLiteral("east")] = b.east;
  m[QStringLiteral("south")] = b.south;
  m[QStringLiteral("north")] = b.north;
  return m;
}

QVariantMap ProjectViewModel::nextAction() const {
  QVariantMap m;
  if (!project_ || !open_)
    return m;
  const NextAction next = project_->nextAction();
  m[QStringLiteral("kind")] = q(next.kind);
  m[QStringLiteral("count")] = next.count;
  // the reason in the interface language
  QString reason;
  if (next.kind == "import")
    reason = tr("Add the survey lines of the project.");
  else if (next.kind == "preprocess")
    reason = tr("%n survey line(s) are in a raw vendor format; preprocess them first.", nullptr, next.count);
  else if (next.kind == "process")
    reason = tr("%n survey line(s) have edits or corrections that are not applied yet.", nullptr, next.count);
  else if (next.kind == "regenerate")
    reason = tr("%n product(s) were made from data that has changed since.", nullptr, next.count);
  else if (next.kind == "grid")
    reason = tr("Make a terrain model of the survey.");
  m[QStringLiteral("reason")] = reason;
  return m;
}

QVariantMap ProjectViewModel::progress() const {
  QVariantMap m;
  int preprocessed = 0, processed = 0;
  bool needsPreprocess = false;
  for (const auto &l : lineData_) {
    if (l.state == "preprocessed" || l.state == "processed")
      preprocessed++;
    if (l.state == "processed")
      processed++;
  }
  (void)needsPreprocess;
  auto done = [this](const char *kind) {
    return std::any_of(historyData_.begin(), historyData_.end(),
                       [kind](const JobRecord &r) { return r.kind == kind && r.state == "completed"; });
  };
  int grids = 0, mosaics = 0, exports = 0;
  for (const auto &a : assetData_) {
    grids += a.type == "grid" ? 1 : 0;
    mosaics += a.type == "mosaic" ? 1 : 0;
    exports += a.type == "export" ? 1 : 0;
  }
  m[QStringLiteral("lines")] = lineCount();
  m[QStringLiteral("inspected")] = inspectedCount();
  m[QStringLiteral("preprocessed")] = preprocessed;
  m[QStringLiteral("cleaned")] = done("clean");
  m[QStringLiteral("corrected")] = done("corrections");
  m[QStringLiteral("processed")] = processed;
  m[QStringLiteral("grids")] = grids;
  m[QStringLiteral("mosaics")] = mosaics;
  m[QStringLiteral("exports")] = exports;
  return m;
}

// ---------------------------------------------------------------------------
// project life cycle

void ProjectViewModel::addRecent(const QString &file) {
  recent_.removeAll(file);
  recent_.prepend(file);
  while (recent_.size() > kMaxRecent)
    recent_.removeLast();
  settings_.setValue(QStringLiteral("projects/recent"), recent_);
  emit recentProjectsChanged();
}

void ProjectViewModel::clearRecentProjects() {
  recent_.clear();
  settings_.remove(QStringLiteral("projects/recent"));
  emit recentProjectsChanged();
}

bool ProjectViewModel::createProject(const QString &folder, const QString &name) {
  if (!project_) {
    emit notify(tr("Projects are not available (the project plugin is not loaded)"));
    return false;
  }
  std::string error;
  if (!project_->create(localPath(folder).toStdString(), name.toStdString(), error)) {
    emit notify(tr("Could not create the project: %1").arg(q(error)));
    return false;
  }
  refresh();
  addRecent(file_);
  return true;
}

bool ProjectViewModel::openProject(const QString &path) {
  if (!project_) {
    emit notify(tr("Projects are not available (the project plugin is not loaded)"));
    return false;
  }
  std::string error;
  if (!project_->open(localPath(path).toStdString(), error)) {
    emit notify(tr("Could not open the project: %1").arg(q(error)));
    return false;
  }
  refresh();
  addRecent(file_);
  return true;
}

void ProjectViewModel::closeProject() {
  if (project_)
    project_->close();
  refresh();
}

int ProjectViewModel::addData(const QVariantList &items, bool recursive) {
  if (!project_ || !open_) {
    emit notify(tr("Create or open a project first"));
    return 0;
  }
  std::vector<std::string> paths;
  for (const auto &item : items) {
    const QString p = localPath(item);
    if (!p.isEmpty())
      paths.push_back(p.toStdString());
  }
  const ImportResult r = project_->addFiles(paths, recursive);
  refreshLines();
  loadTracks();
  if (r.added > 0 && r.skipped.empty())
    emit notify(tr("Added %n survey line(s); reading statistics and navigation in the background", nullptr, r.added));
  else if (r.added > 0)
    emit notify(tr("Added %1 survey line(s), skipped %2: %3")
                    .arg(r.added)
                    .arg(static_cast<int>(r.skipped.size()))
                    .arg(q(r.skipped.front())));
  else if (!r.skipped.empty())
    emit notify(tr("No survey lines added: %1").arg(q(r.skipped.front())));
  else
    emit notify(tr("No swath data files found"));
  if (kernel_ != nullptr)
    for (const auto &s : r.skipped)
      kernel_->log(LogLevel::Warning, "project", "skipped " + s);
  return r.added;
}

int ProjectViewModel::removeSelectedLines() {
  if (!project_ || selected_.isEmpty())
    return 0;
  const int n = project_->removeLines(selection());
  selected_.clear();
  refreshLines();
  loadTracks();
  emit selectionChanged();
  return n;
}

// ---------------------------------------------------------------------------
// selection

void ProjectViewModel::select(const QString &id, int mode) {
  if (mode == 1) {
    if (selected_.contains(id))
      selected_.remove(id);
    else
      selected_.insert(id);
    anchor_ = id;
  } else if (mode == 2 && !anchor_.isEmpty()) {
    int a = -1, b = -1;
    for (int i = 0; i < static_cast<int>(lineData_.size()); i++) {
      if (q(lineData_[static_cast<std::size_t>(i)].id) == anchor_)
        a = i;
      if (q(lineData_[static_cast<std::size_t>(i)].id) == id)
        b = i;
    }
    if (a >= 0 && b >= 0) {
      for (int i = std::min(a, b); i <= std::max(a, b); i++)
        selected_.insert(q(lineData_[static_cast<std::size_t>(i)].id));
    }
  } else {
    selected_.clear();
    if (!id.isEmpty())
      selected_.insert(id);
    anchor_ = id;
  }
  refreshLines();
  emit selectionChanged();
  mapRevision_++;
  emit mapChanged();
}

void ProjectViewModel::selectAll() {
  for (const auto &l : lineData_)
    selected_.insert(q(l.id));
  refreshLines();
  emit selectionChanged();
  mapRevision_++;
  emit mapChanged();
}

void ProjectViewModel::selectIds(const QStringList &ids) {
  selected_.clear();
  for (const auto &l : lineData_)
    if (ids.contains(q(l.id)))
      selected_.insert(q(l.id));
  anchor_ = ids.isEmpty() ? QString() : ids.last();
  refreshLines();
  emit selectionChanged();
  mapRevision_++;
  emit mapChanged();
}

void ProjectViewModel::clearSelection() {
  if (selected_.isEmpty())
    return;
  selected_.clear();
  refreshLines();
  emit selectionChanged();
  mapRevision_++;
  emit mapChanged();
}

QVariantMap ProjectViewModel::lineInfo(const QString &id) const {
  const int row = lines_->indexOf(id);
  return row >= 0 ? lines_->get(row) : QVariantMap();
}

QVariantMap ProjectViewModel::productInfo(const QString &id) const {
  const int row = products_->indexOf(id);
  QVariantMap m = row >= 0 ? products_->get(row) : QVariantMap();
  for (const auto &a : assetData_) {
    if (q(a.id) != id)
      continue;
    QVariantMap params;
    for (const auto &kv : a.parameters)
      params[q(kv.first)] = q(valueToString(kv.second));
    m[QStringLiteral("parameters")] = params;
    m[QStringLiteral("created")] =
        a.created > 0 ? fromEpoch(a.created).toString(QStringLiteral("yyyy-MM-dd HH:mm")) : QString();
  }
  return m;
}

QString ProjectViewModel::productPath(const QString &id) const {
  for (const auto &a : assetData_)
    if (q(a.id) == id)
      return q(a.path);
  return QString();
}

// ---------------------------------------------------------------------------
// layers

void ProjectViewModel::setLayerVisible(const QString &asset, bool visible) {
  if (!project_)
    return;
  ProjectLayer layer{asset.toStdString(), visible, 1.0, 0, true};
  bool found = false;
  for (const auto &l : layerData_)
    if (q(l.asset) == asset) {
      layer = l;
      layer.visible = visible;
      found = true;
    }
  if (!found) {
    // a product shown for the first time: quality grids in a rainbow, no shading
    for (const auto &a : assetData_)
      if (q(a.id) == asset && (a.type.find("_num") != std::string::npos || a.type.find("_sd") != std::string::npos)) {
        layer.colorMap = 2;
        layer.hillshade = false;
      } else if (q(a.id) == asset && a.type == "mosaic") {
        layer.colorMap = 3;
        layer.hillshade = false;
      }
  }
  project_->setLayer(layer);
}

void ProjectViewModel::setLayerOpacity(const QString &asset, double opacity) {
  for (auto l : layerData_)
    if (q(l.asset) == asset) {
      l.opacity = opacity;
      // update the drawing at once, the project file follows
      for (auto &m : layers_)
        if (m.asset == asset)
          m.opacity = opacity;
      mapRevision_++;
      emit mapChanged();
      project_->setLayer(l);
    }
}

void ProjectViewModel::setLayerColorMap(const QString &asset, int colorMap) {
  for (auto l : layerData_)
    if (q(l.asset) == asset) {
      l.colorMap = colorMap;
      project_->setLayer(l);
    }
}

void ProjectViewModel::setLayerHillshade(const QString &asset, bool hillshade) {
  for (auto l : layerData_)
    if (q(l.asset) == asset) {
      l.hillshade = hillshade;
      project_->setLayer(l);
    }
}

bool ProjectViewModel::removeProduct(const QString &asset, bool deleteFiles) {
  return project_ && project_->removeAsset(asset.toStdString(), deleteFiles);
}

QString ProjectViewModel::addGrid(const QString &path) {
  if (!project_ || !open_)
    return QString();
  return q(project_->addGrid(localPath(path).toStdString()));
}

QString ProjectViewModel::newestProductOfJob(qint64 jobId) const {
  std::int64_t sequence = -1;
  for (const auto &r : (project_ && open_ ? project_->history() : std::vector<JobRecord>()))
    if (r.jobId == jobId)
      sequence = r.sequence;
  if (sequence < 0 || !project_)
    return QString();
  for (const auto &a : project_->assets())
    if (a.job == sequence && a.parent.empty())
      return q(a.path);
  return QString();
}

QString ProjectViewModel::newestAssetOfJob(qint64 jobId) const {
  std::int64_t sequence = -1;
  for (const auto &r : historyData_)
    if (r.jobId == jobId)
      sequence = r.sequence;
  if (sequence < 0 && project_ && open_)
    for (const auto &r : project_->history())
      if (r.jobId == jobId)
        sequence = r.sequence;
  if (sequence < 0 || !project_)
    return QString();
  for (const auto &a : project_->assets())
    if (a.job == sequence && a.parent.empty())
      return q(a.id);
  return QString();
}

QVariantList ProjectViewModel::lineVersions(const QString &lineId) const {
  QVariantList out;
  if (!project_ || !open_)
    return out;
  for (const auto &v : project_->lineVersions(lineId.toStdString()))
    out.push_back(QVariantMap{{QStringLiteral("version"), q(v.version)},
                              {QStringLiteral("path"), q(v.path)},
                              {QStringLiteral("format"), v.format},
                              {QStringLiteral("exists"), v.exists}});
  return out;
}

qint64 ProjectViewModel::retryJob(qint64 sequence) {
  if (!project_ || !open_)
    return 0;
  std::string error;
  const qint64 id = project_->retry(sequence, error);
  if (id == 0)
    emit notify(tr("Could not run the job again: %1").arg(q(error)));
  else
    emit notify(tr("Running job #%1 again as a new attempt").arg(sequence));
  return id;
}

qint64 ProjectViewModel::regenerateProduct(const QString &assetId) {
  if (!project_ || !open_)
    return 0;
  std::string error;
  const qint64 id = project_->regenerate(assetId.toStdString(), error);
  if (id == 0)
    emit notify(tr("Could not make the product again: %1").arg(q(error)));
  else
    emit notify(tr("Making the product again with the current data"));
  return id;
}

QString ProjectViewModel::layerProblem(const QString &assetId) const {
  for (const auto &l : layers_)
    if (l.asset == assetId)
      return l.error;
  return QString();
}

bool ProjectViewModel::hasMapLayer(const QString &assetId) const {
  for (const auto &l : layers_)
    if (l.asset == assetId)
      return l.visible;
  return false;
}

bool ProjectViewModel::layerReady(const QString &assetId) const {
  for (const auto &l : layers_)
    if (l.asset == assetId)
      return !l.image.isNull() || !l.error.isEmpty();
  return true;
}

QVariantMap ProjectViewModel::valueAt(double lon, double lat) const {
  QVariantMap out;
  for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
    if (!it->visible || !it->grid)
      continue;
    const Grid &g = *it->grid;
    // the position in the grid's coordinates
    double gx = lon, gy = lat;
    if (!g.geographic && !(it->projection.valid() && it->projection.forward(lon, lat, gx, gy)))
      continue;
    if (gx < g.xmin - 0.5 * g.dx() || gx > g.xmax + 0.5 * g.dx() || gy < g.ymin - 0.5 * g.dy() ||
        gy > g.ymax + 0.5 * g.dy())
      continue;
    const int ix = std::clamp(static_cast<int>(std::lround((gx - g.xmin) / g.dx())), 0, g.nx - 1);
    const int iy = std::clamp(static_cast<int>(std::lround((g.ymax - gy) / g.dy())), 0, g.ny - 1);
    const float z = g.at(ix, iy);
    if (std::isnan(z))
      continue;
    out[QStringLiteral("z")] = static_cast<double>(z);
    out[QStringLiteral("layer")] = it->label;
    out[QStringLiteral("units")] = q(g.zUnits);
    return out;
  }
  return out;
}

void ProjectViewModel::loadTracks() {
  const int generation = ++tracksGeneration_;
  if (!project_ || !open_ || lineData_.empty()) {
    tracks_.clear();
    tracksLoading_ = false;
    mapRevision_++;
    emit mapChanged();
    return;
  }
  tracksLoading_ = true;
  emit mapChanged();
  std::vector<std::pair<std::string, QString>> lines;
  for (const auto &l : lineData_)
    lines.emplace_back(l.id, QFileInfo(q(l.path)).fileName());
  auto project = project_;
  auto *watcher = new QFutureWatcher<QVector<MapTrack>>(this);
  connect(watcher, &QFutureWatcher<QVector<MapTrack>>::finished, this, [this, watcher, generation]() {
    watcher->deleteLater();
    if (generation != tracksGeneration_)
      return;
    tracks_ = watcher->result();
    tracksLoading_ = false;
    mapRevision_++;
    emit mapChanged();
  });
  watcher->setFuture(QtConcurrent::run([project, lines]() {
    QVector<MapTrack> out;
    for (const auto &l : lines) {
      MapTrack t;
      t.id = q(l.first);
      t.name = l.second;
      for (const auto &p : project->navigation(l.first, kTrackPoints))
        t.points.push_back(QPointF(p.lon, p.lat));
      if (!t.points.isEmpty())
        out.push_back(t);
    }
    return out;
  }));
}

void ProjectViewModel::updateLayers() {
  const int generation = layerGeneration_;
  QVector<MapLayer> next;
  for (const auto &l : layerData_) {
    const auto asset =
        std::find_if(assetData_.begin(), assetData_.end(), [&l](const ProjectAsset &a) { return a.id == l.asset; });
    if (asset == assetData_.end() || !isMapType(asset->type))
      continue;
    MapLayer layer;
    layer.asset = q(l.asset);
    layer.label = q(asset->label);
    layer.opacity = l.opacity;
    layer.visible = l.visible;
    layer.style = styleOf(l);
    // keep what was rendered with the same style
    for (const auto &old : layers_)
      if (old.asset == layer.asset && old.style == layer.style) {
        layer.image = old.image;
        layer.bounds = old.bounds;
        layer.grid = old.grid;
        layer.projection = old.projection;
        layer.error = old.error;
      }
    const bool needed = layer.visible && layer.image.isNull() && layer.error.isEmpty() && grids_ && renderer_;
    next.push_back(layer);
    if (!needed)
      continue;
    pendingLayers_++;
    auto grids = grids_;
    auto renderer = renderer_;
    const std::string path = asset->path;
    RenderOptions options;
    options.colorMap = static_cast<ColorMap>(std::clamp(l.colorMap, 0, 3));
    options.hillshade = l.hillshade;
    const QString id = layer.asset;
    const QString style = layer.style;
    auto *watcher = new QFutureWatcher<LayerResult>(this);
    connect(watcher, &QFutureWatcher<LayerResult>::finished, this, [this, watcher, generation, id, style]() {
      watcher->deleteLater();
      if (generation != layerGeneration_)
        return;
      pendingLayers_ = std::max(0, pendingLayers_ - 1);
      const LayerResult r = watcher->result();
      for (auto &m : layers_)
        if (m.asset == id && m.style == style) {
          m.image = r.image;
          m.bounds = r.bounds;
          m.grid = r.grid;
          m.projection = r.projection;
          m.error = r.ok ? QString() : (r.error.isEmpty() ? tr("The grid could not be read") : r.error);
        }
      if (!r.ok && !r.error.isEmpty())
        emit notify(r.error);
      emit layerLoaded(id, r.ok, r.ok ? QString() : r.error);
      mapRevision_++;
      emit mapChanged();
    });
    watcher->setFuture(QtConcurrent::run([grids, renderer, path, options]() {
      LayerResult r;
      GridLoadResult loaded = grids->load(path);
      if (!loaded.ok) {
        r.error = QCoreApplication::translate("ProjectViewModel", "Could not read %1: %2")
                      .arg(QFileInfo(q(path)).fileName(), q(loaded.error));
        return r;
      }
      const Grid &g = loaded.grid;
      const platform::MapProjection projection =
          g.geographic ? platform::MapProjection::parse("") : platform::MapProjection::parse(g.projection);
      if (!g.geographic && (!projection.valid() || projection.geographic())) {
        r.error = QCoreApplication::translate(
                      "ProjectViewModel",
                      "%1 is a projected grid whose projection is not recorded in the file; open it in the grid view")
                      .arg(QFileInfo(q(path)).fileName());
        return r;
      }
      const Image image = renderer->render(g, options);
      QImage out(image.width, image.height, QImage::Format_ARGB32);
      for (int y = 0; y < image.height; y++)
        memcpy(out.scanLine(y), image.pixels.data() + static_cast<size_t>(y) * image.width,
               static_cast<size_t>(image.width) * 4);
      if (out.width() > kMaxLayerPixels || out.height() > kMaxLayerPixels)
        out = out.scaled(kMaxLayerPixels, kMaxLayerPixels, Qt::KeepAspectRatio, Qt::SmoothTransformation);
      if (!g.geographic) {
        QImage warped;
        GeoBounds geo;
        if (!warpToGeographic(g, out, projection, warped, geo)) {
          r.error = QCoreApplication::translate("ProjectViewModel", "%1 could not be placed on the map (%2)")
                        .arg(QFileInfo(q(path)).fileName(), QString::fromStdString(projection.name()));
          return r;
        }
        r.image = warped;
        r.bounds = geo;
        r.projection = projection;
        r.grid = std::make_shared<const Grid>(std::move(loaded.grid));
        r.ok = true;
        return r;
      }
      r.image = out;
      r.bounds = GeoBounds{g.xmin - 0.5 * g.dx(), g.xmax + 0.5 * g.dx(), g.ymin - 0.5 * g.dy(), g.ymax + 0.5 * g.dy()};
      r.projection = projection;
      r.grid = std::make_shared<const Grid>(std::move(loaded.grid));
      r.ok = true;
      return r;
    }));
  }
  layers_ = next;
  mapRevision_++;
  emit mapChanged();
}

bool ProjectViewModel::waitForMap(int timeoutMs) {
  QElapsedTimer timer;
  timer.start();
  while (mapBusy() && timer.elapsed() < timeoutMs)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
  return !mapBusy();
}

QString ProjectViewModel::workspaceState() const {
  return project_ && open_ ? q(project_->workspaceState()) : QString();
}

void ProjectViewModel::setWorkspaceState(const QString &json) {
  if (project_ && open_)
    project_->setWorkspaceState(json.toStdString());
}

QString ProjectViewModel::datalistPath(const QString &which) const {
  return project_ && open_ ? QDir::toNativeSeparators(q(project_->datalist(which.toStdString()))) : QString();
}

}  // namespace mbstudio::ui
