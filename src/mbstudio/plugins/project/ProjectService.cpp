#include "ProjectService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <thread>
#include <type_traits>

#include "RequestJson.h"
#include "mbstudio/platform/Paths.h"

namespace mbstudio::project {

namespace {

// 1: MB-Studio 0.2; 2: revisions, product inputs, attempts and requests
constexpr int kProjectVersion = 2;
const char *kSubfolders[] = {"datalists", "products/grids", "products/mosaics", "products/figures",
                             "processed", "exports", "jobs"};

double now() {
  return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
}

QString q(const std::string &s) { return QString::fromStdString(s); }
std::string s(const QJsonValue &v) { return v.toString().toStdString(); }

std::string lower(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
  return text;
}

std::string suffixOf(const std::string &path) {
  const std::string name = platform::fileName(path);
  const auto dot = name.find_last_of('.');
  return dot == std::string::npos ? std::string() : lower(name.substr(dot));
}

std::string stemOf(const std::string &path) {
  const std::string name = platform::fileName(path);
  const auto dot = name.find_last_of('.');
  return dot == std::string::npos ? name : name.substr(0, dot);
}

bool endsWith(const std::string &text, const std::string &suffix) {
  return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool contains(const std::vector<std::string> &list, const std::string &item) {
  return std::find(list.begin(), list.end(), item) != list.end();
}

// Ancillary and product files that are never survey lines.
bool ignoredSuffix(const std::string &suffix) {
  static const std::set<std::string> ignored = {
      ".inf", ".fbt", ".fnv", ".par", ".esf", ".svp", ".log", ".grd", ".nc", ".txt", ".ata", ".ats", ".sta",
      ".bsl", ".csv", ".mb-1", ".tde", ".nve", ".resf", ".aga", ".sga", ".png", ".tif", ".las", ".glb", ".gltf",
      ".xyz", ".asc", ".mbsproj", ".json", ".cmd", ".sh", ".pdf", ".md", ".html", ".zip", ".ps", ".lck", ".tmp",
      ".eps", ".cpt", ".stream"};
  return ignored.count(suffix) > 0;
}

QJsonObject toJson(const ValueMap &values) {
  QJsonObject o;
  for (const auto &kv : values) {
    const QString key = q(kv.first);
    std::visit(
        [&o, &key](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, std::monostate>)
            o[key] = QJsonValue();
          else if constexpr (std::is_same_v<T, bool>)
            o[key] = v;
          else if constexpr (std::is_same_v<T, std::int64_t>)
            o[key] = static_cast<double>(v);
          else if constexpr (std::is_same_v<T, double>)
            o[key] = v;
          else
            o[key] = QString::fromStdString(v);
        },
        kv.second);
  }
  return o;
}

ValueMap fromJson(const QJsonObject &o) {
  ValueMap values;
  for (auto it = o.begin(); it != o.end(); ++it) {
    const std::string key = it.key().toStdString();
    const QJsonValue v = it.value();
    if (v.isBool())
      values[key] = v.toBool();
    else if (v.isDouble()) {
      const double d = v.toDouble();
      if (std::floor(d) == d && std::fabs(d) < 9.0e15)
        values[key] = static_cast<std::int64_t>(d);
      else
        values[key] = d;
    } else if (v.isString())
      values[key] = v.toString().toStdString();
  }
  return values;
}

QJsonArray toJson(const std::vector<std::string> &list) {
  QJsonArray a;
  for (const auto &item : list)
    a.append(q(item));
  return a;
}

std::vector<std::string> stringsFrom(const QJsonValue &v) {
  std::vector<std::string> out;
  for (const auto &item : v.toArray())
    out.push_back(item.toString().toStdString());
  return out;
}

std::string compact(const QJsonObject &o) { return QJsonDocument(o).toJson(QJsonDocument::Compact).toStdString(); }

QJsonObject parseObject(const std::string &json) {
  return QJsonDocument::fromJson(QByteArray::fromStdString(json)).object();
}

GeoBounds merged(const std::vector<SurveyLine> &lines) {
  GeoBounds b;
  bool first = true;
  for (const auto &line : lines) {
    if (!line.bounds.valid())
      continue;
    if (first) {
      b = line.bounds;
      first = false;
    } else {
      b.west = std::min(b.west, line.bounds.west);
      b.east = std::max(b.east, line.bounds.east);
      b.south = std::min(b.south, line.bounds.south);
      b.north = std::max(b.north, line.bounds.north);
    }
  }
  return b;
}

bool isProductType(const std::string &type) {
  return type == "grid" || type == "grid_num" || type == "grid_sd" || type == "mosaic" || type == "mosaic_num" ||
         type == "mosaic_sd" || type == "figure" || type == "export";
}

}  // namespace

ProjectService::ProjectService(std::shared_ptr<IFormatService> formats, std::shared_ptr<ISwathService> swath,
                               std::shared_ptr<IProcessingService> processing, std::shared_ptr<IJobService> jobs,
                               EventBus *events, std::string mbsystemVersion, std::string mbstudioVersion,
                               std::shared_ptr<ISoundingService> soundings, std::shared_ptr<IGridService> grids)
    : formats_(std::move(formats)),
      swath_(std::move(swath)),
      processing_(std::move(processing)),
      jobs_(std::move(jobs)),
      events_(events),
      mbsystemVersion_(std::move(mbsystemVersion)),
      mbstudioVersion_(std::move(mbstudioVersion)),
      soundings_(std::move(soundings)),
      grids_(std::move(grids)) {}

ProjectService::~ProjectService() { detach(); }

void ProjectService::attach() {
  if (events_ == nullptr || subscription_ != 0)
    return;
  std::weak_ptr<ProjectService> weak = shared_from_this();
  subscription_ = events_->subscribe("job.finished", [weak](const Event &event) {
    if (getString(event.data, "owner") != "project")
      return;
    if (auto self = weak.lock())
      self->syncJob(getInt(event.data, "job"));
  });
}

void ProjectService::detach() {
  if (events_ != nullptr && subscription_ != 0) {
    events_->unsubscribe(subscription_);
    subscription_ = 0;
  }
  std::vector<std::int64_t> running;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &kv : pending_)
      running.push_back(kv.first);
  }
  if (jobs_)
    for (const auto id : running)
      jobs_->cancel(id);
}

void ProjectService::changed(const std::string &what) const {
  if (events_ != nullptr)
    events_->publish("project.changed", {{"what", what}});
}

std::string ProjectService::safeName(const std::string &name) {
  std::string out;
  for (const unsigned char c : name) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c >= 0x80)
      out += static_cast<char>(c);
    else
      out += '_';
  }
  while (!out.empty() && (out.back() == '.' || out.back() == '_'))
    out.pop_back();
  return out.empty() ? std::string("product") : out;
}

std::string ProjectService::newId(char prefix) {
  if (prefix == 'L')
    return "L" + std::to_string(nextLine_++);
  return "A" + std::to_string(nextAsset_++);
}

void ProjectService::resources(const std::string &kind, const std::vector<std::string> &lineIds,
                               const std::string &readFile, const std::vector<std::string> &outputs,
                               bool angleCorrection, std::vector<std::string> &reads,
                               std::vector<std::string> &writes) {
  // the programs of these steps write next to the survey line files (edit
  // save files, parameter files, processed files, caches)
  const bool writesLines = kind == "import" || kind == "preprocess" || kind == "clean" || kind == "corrections" ||
                           kind == "process" || kind == "edit" || (kind == "mosaic" && angleCorrection);
  for (const auto &id : lineIds)
    (writesLines ? writes : reads).push_back("line:" + id);
  if (!readFile.empty())
    reads.push_back("file:" + platform::absolutePath(readFile));
  for (const auto &o : outputs)
    writes.push_back("file:" + platform::absolutePath(o));
}

// ---------------------------------------------------------------------------
// project files

bool ProjectService::create(const std::string &directory, const std::string &name, std::string &error) {
  if (directory.empty()) {
    error = "no project folder given";
    return false;
  }
  const std::string root = platform::absolutePath(directory);
  if (!platform::makeDirectories(root)) {
    error = "cannot create the folder " + root;
    return false;
  }
  const std::string projectName = name.empty() ? platform::fileName(root) : name;
  const std::string file = platform::joinPath(root, safeName(projectName) + kSuffix);
  for (const auto &entry : platform::listDirectory(root))
    if (suffixOf(entry) == kSuffix) {
      error = "a project already exists in " + root;
      return false;
    }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (open_) {
      std::string ignored;
      saveLocked(ignored);
    }
    open_ = true;
    name_ = projectName;
    root_ = root;
    file_ = file;
    created_ = now();
    nextLine_ = nextAsset_ = nextJob_ = 1;
    lines_.clear();
    assets_.clear();
    layers_.clear();
    history_.clear();
    pending_.clear();
    reserved_.clear();
    workspace_ = "{}";
    for (const char *sub : kSubfolders)
      platform::makeDirectories(platform::joinPath(root_, sub));
    if (!writeProjectDatalistsLocked(error) || !saveLocked(error)) {
      open_ = false;
      return false;
    }
  }
  changed("opened");
  return true;
}

bool ProjectService::open(const std::string &path, std::string &error) {
  std::string file = platform::absolutePath(path);
  if (platform::isDirectory(file)) {
    std::string found;
    for (const auto &entry : platform::listDirectory(file))
      if (suffixOf(entry) == kSuffix) {
        found = platform::joinPath(file, entry);
        break;
      }
    if (found.empty()) {
      error = "no MB-Studio project in " + file;
      return false;
    }
    file = found;
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (open_) {
      std::string ignored;
      saveLocked(ignored);
    }
    if (!loadLocked(file, error))
      return false;
  }
  changed("opened");
  return true;
}

bool ProjectService::loadLocked(const std::string &file, std::string &error) {
  std::string text;
  if (!platform::readTextFile(file, text)) {
    error = "cannot read " + file;
    return false;
  }
  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(text), &parseError);
  if (doc.isNull() || !doc.isObject()) {
    error = "not a valid project file: " + parseError.errorString().toStdString();
    return false;
  }
  const QJsonObject o = doc.object();
  if (o.value(QStringLiteral("format")).toString() != QLatin1String("mbstudio-project")) {
    error = "not an MB-Studio project file: " + file;
    return false;
  }
  const int version = o.value(QStringLiteral("version")).toInt();
  if (version > kProjectVersion) {
    error = "the project was written by a newer MB-Studio (file version " + std::to_string(version) + ")";
    return false;
  }
  const std::string root = platform::parentPath(file);
  auto resolve = [&root](const QJsonValue &v) {
    const std::string p = v.toString().toStdString();
    if (p.empty())
      return p;
    return platform::absolutePath(platform::isAbsolutePath(p) ? p : platform::joinPath(root, p));
  };

  open_ = true;
  file_ = file;
  root_ = root;
  name_ = s(o.value(QStringLiteral("name")));
  created_ = o.value(QStringLiteral("created")).toDouble();
  const QJsonObject counters = o.value(QStringLiteral("counters")).toObject();
  nextLine_ = static_cast<std::int64_t>(counters.value(QStringLiteral("line")).toDouble(1));
  nextAsset_ = static_cast<std::int64_t>(counters.value(QStringLiteral("asset")).toDouble(1));
  nextJob_ = static_cast<std::int64_t>(counters.value(QStringLiteral("job")).toDouble(1));
  pending_.clear();
  reserved_.clear();

  lines_.clear();
  for (const auto &item : o.value(QStringLiteral("lines")).toArray()) {
    const QJsonObject l = item.toObject();
    SurveyLine line;
    line.id = s(l.value(QStringLiteral("id")));
    line.path = resolve(l.value(QStringLiteral("path")));
    line.format = l.value(QStringLiteral("format")).toInt();
    line.formatName = s(l.value(QStringLiteral("formatName")));
    line.size = static_cast<long long>(l.value(QStringLiteral("size")).toDouble());
    line.processingPath = resolve(l.value(QStringLiteral("processingPath")));
    line.processingFormat = l.value(QStringLiteral("processingFormat")).toInt();
    line.state = s(l.value(QStringLiteral("state")));
    line.inspected = l.value(QStringLiteral("inspected")).toBool();
    line.records = static_cast<long>(l.value(QStringLiteral("records")).toDouble());
    line.maxBeams = l.value(QStringLiteral("maxBeams")).toInt();
    line.beamsTotal = static_cast<long>(l.value(QStringLiteral("beamsTotal")).toDouble());
    line.beamsGood = static_cast<long>(l.value(QStringLiteral("beamsGood")).toDouble());
    line.timeStart = s(l.value(QStringLiteral("timeStart")));
    line.timeEnd = s(l.value(QStringLiteral("timeEnd")));
    const QJsonArray b = l.value(QStringLiteral("bounds")).toArray();
    if (b.size() == 4)
      line.bounds = GeoBounds{b[0].toDouble(), b[1].toDouble(), b[2].toDouble(), b[3].toDouble()};
    line.depthMin = l.value(QStringLiteral("depthMin")).toDouble();
    line.depthMax = l.value(QStringLiteral("depthMax")).toDouble();
    line.trackLengthKm = l.value(QStringLiteral("trackLengthKm")).toDouble();
    const QJsonObject r = l.value(QStringLiteral("revisions")).toObject();
    if (!r.isEmpty()) {
      line.editRevision = r.value(QStringLiteral("edit")).toInt();
      line.correctionRevision = r.value(QStringLiteral("correction")).toInt();
      line.sourceRevision = r.value(QStringLiteral("source")).toInt();
      line.processedEdit = r.value(QStringLiteral("processedEdit")).toInt(-1);
      line.processedCorrection = r.value(QStringLiteral("processedCorrection")).toInt(-1);
      line.processedSource = r.value(QStringLiteral("processedSource")).toInt(-1);
    } else if (line.state == "processed") {
      // a project of MB-Studio 0.2: the processed file is taken as current
      line.processedEdit = line.processedCorrection = line.processedSource = 0;
    }
    line.missing = !platform::fileExists(line.path);
    if (!line.processingPath.empty() && !platform::fileExists(line.processingPath)) {
      line.processingPath.clear();
      line.processingFormat = 0;
      line.state = "raw";
      line.sourceRevision++;
    }
    // the processed file may have been deleted outside MB-Studio
    if (line.processedEdit >= 0 && !platform::fileExists(processing_->processedName(line.workPath()))) {
      line.processedEdit = line.processedCorrection = line.processedSource = -1;
      if (line.state == "processed")
        line.state = line.processingPath.empty() ? "raw" : "preprocessed";
    }
    if (line.state.empty())
      line.state = "raw";
    lines_.push_back(line);
  }

  assets_.clear();
  for (const auto &item : o.value(QStringLiteral("assets")).toArray()) {
    const QJsonObject a = item.toObject();
    ProjectAsset asset;
    asset.id = s(a.value(QStringLiteral("id")));
    asset.path = resolve(a.value(QStringLiteral("path")));
    asset.type = s(a.value(QStringLiteral("type")));
    asset.label = s(a.value(QStringLiteral("label")));
    asset.parent = s(a.value(QStringLiteral("parent")));
    asset.job = static_cast<std::int64_t>(a.value(QStringLiteral("job")).toDouble());
    asset.regenerable = a.value(QStringLiteral("regenerable")).toBool(true);
    asset.created = a.value(QStringLiteral("created")).toDouble();
    asset.parameters = fromJson(a.value(QStringLiteral("parameters")).toObject());
    const QJsonObject inputs = a.value(QStringLiteral("inputs")).toObject();
    for (auto it = inputs.begin(); it != inputs.end(); ++it)
      asset.inputs[it.key().toStdString()] = it.value().toString().toStdString();
    assets_.push_back(asset);
  }

  layers_.clear();
  for (const auto &item : o.value(QStringLiteral("layers")).toArray()) {
    const QJsonObject l = item.toObject();
    ProjectLayer layer;
    layer.asset = s(l.value(QStringLiteral("asset")));
    layer.visible = l.value(QStringLiteral("visible")).toBool(true);
    layer.opacity = l.value(QStringLiteral("opacity")).toDouble(1.0);
    layer.colorMap = l.value(QStringLiteral("colorMap")).toInt();
    layer.hillshade = l.value(QStringLiteral("hillshade")).toBool(true);
    layers_.push_back(layer);
  }

  history_.clear();
  for (const auto &item : o.value(QStringLiteral("history")).toArray()) {
    const QJsonObject h = item.toObject();
    JobRecord r;
    r.sequence = static_cast<std::int64_t>(h.value(QStringLiteral("sequence")).toDouble());
    r.kind = s(h.value(QStringLiteral("kind")));
    r.title = s(h.value(QStringLiteral("title")));
    r.state = s(h.value(QStringLiteral("state")));
    // a job that was still running when the project was saved did not finish
    if (r.state == "queued" || r.state == "running")
      r.state = "cancelled";
    r.started = h.value(QStringLiteral("started")).toDouble();
    r.finished = h.value(QStringLiteral("finished")).toDouble();
    r.exitCode = h.value(QStringLiteral("exitCode")).toInt();
    r.error = s(h.value(QStringLiteral("error")));
    r.commands = stringsFrom(h.value(QStringLiteral("commands")));
    r.inputs = stringsFrom(h.value(QStringLiteral("inputs")));
    r.outputs = stringsFrom(h.value(QStringLiteral("outputs")));
    r.parameters = fromJson(h.value(QStringLiteral("parameters")).toObject());
    r.logPath = resolve(h.value(QStringLiteral("log")));
    r.workingDirectory = resolve(h.value(QStringLiteral("workingDirectory")));
    r.mbsystemVersion = s(h.value(QStringLiteral("mbsystem")));
    r.attempt = std::max(1, h.value(QStringLiteral("attempt")).toInt(1));
    r.retryOf = static_cast<std::int64_t>(h.value(QStringLiteral("retryOf")).toDouble());
    r.lines = stringsFrom(h.value(QStringLiteral("lines")));
    if (h.value(QStringLiteral("request")).isObject())
      r.request = compact(h.value(QStringLiteral("request")).toObject());
    history_.push_back(r);
  }
  const QJsonValue workspace = o.value(QStringLiteral("workspace"));
  workspace_ = workspace.isObject() ? compact(workspace.toObject()) : std::string("{}");
  for (const char *sub : kSubfolders)
    platform::makeDirectories(platform::joinPath(root_, sub));
  std::string ignored;
  writeProjectDatalistsLocked(ignored);
  return true;
}

bool ProjectService::save(std::string &error) {
  std::lock_guard<std::mutex> lock(mutex_);
  return saveLocked(error);
}

bool ProjectService::saveLocked(std::string &error) {
  if (!open_) {
    error = "no project is open";
    return false;
  }
  auto rel = [this](const std::string &path) { return q(path.empty() ? path : platform::relativePath(path, root_)); };
  QJsonObject o;
  o[QStringLiteral("format")] = QStringLiteral("mbstudio-project");
  o[QStringLiteral("version")] = kProjectVersion;
  o[QStringLiteral("name")] = q(name_);
  o[QStringLiteral("created")] = created_;
  o[QStringLiteral("modified")] = now();
  o[QStringLiteral("mbstudio")] = q(mbstudioVersion_);
  o[QStringLiteral("mbsystem")] = q(mbsystemVersion_);
  QJsonObject counters;
  counters[QStringLiteral("line")] = static_cast<double>(nextLine_);
  counters[QStringLiteral("asset")] = static_cast<double>(nextAsset_);
  counters[QStringLiteral("job")] = static_cast<double>(nextJob_);
  o[QStringLiteral("counters")] = counters;

  QJsonArray lines;
  for (const auto &line : lines_) {
    QJsonObject l;
    l[QStringLiteral("id")] = q(line.id);
    l[QStringLiteral("path")] = rel(line.path);
    l[QStringLiteral("format")] = line.format;
    l[QStringLiteral("formatName")] = q(line.formatName);
    l[QStringLiteral("size")] = static_cast<double>(line.size);
    l[QStringLiteral("processingPath")] = rel(line.processingPath);
    l[QStringLiteral("processingFormat")] = line.processingFormat;
    l[QStringLiteral("state")] = q(line.state);
    l[QStringLiteral("revisions")] = QJsonObject{{QStringLiteral("edit"), line.editRevision},
                                                 {QStringLiteral("correction"), line.correctionRevision},
                                                 {QStringLiteral("source"), line.sourceRevision},
                                                 {QStringLiteral("processedEdit"), line.processedEdit},
                                                 {QStringLiteral("processedCorrection"), line.processedCorrection},
                                                 {QStringLiteral("processedSource"), line.processedSource}};
    l[QStringLiteral("inspected")] = line.inspected;
    l[QStringLiteral("records")] = static_cast<double>(line.records);
    l[QStringLiteral("maxBeams")] = line.maxBeams;
    l[QStringLiteral("beamsTotal")] = static_cast<double>(line.beamsTotal);
    l[QStringLiteral("beamsGood")] = static_cast<double>(line.beamsGood);
    l[QStringLiteral("timeStart")] = q(line.timeStart);
    l[QStringLiteral("timeEnd")] = q(line.timeEnd);
    l[QStringLiteral("bounds")] = QJsonArray{line.bounds.west, line.bounds.east, line.bounds.south, line.bounds.north};
    l[QStringLiteral("depthMin")] = line.depthMin;
    l[QStringLiteral("depthMax")] = line.depthMax;
    l[QStringLiteral("trackLengthKm")] = line.trackLengthKm;
    lines.append(l);
  }
  o[QStringLiteral("lines")] = lines;

  QJsonArray assets;
  for (const auto &asset : assets_) {
    QJsonObject a;
    a[QStringLiteral("id")] = q(asset.id);
    a[QStringLiteral("path")] = rel(asset.path);
    a[QStringLiteral("type")] = q(asset.type);
    a[QStringLiteral("label")] = q(asset.label);
    a[QStringLiteral("parent")] = q(asset.parent);
    a[QStringLiteral("job")] = static_cast<double>(asset.job);
    a[QStringLiteral("regenerable")] = asset.regenerable;
    a[QStringLiteral("created")] = asset.created;
    a[QStringLiteral("parameters")] = toJson(asset.parameters);
    QJsonObject inputs;
    for (const auto &kv : asset.inputs)
      inputs[q(kv.first)] = q(kv.second);
    a[QStringLiteral("inputs")] = inputs;
    assets.append(a);
  }
  o[QStringLiteral("assets")] = assets;

  QJsonArray layers;
  for (const auto &layer : layers_) {
    QJsonObject l;
    l[QStringLiteral("asset")] = q(layer.asset);
    l[QStringLiteral("visible")] = layer.visible;
    l[QStringLiteral("opacity")] = layer.opacity;
    l[QStringLiteral("colorMap")] = layer.colorMap;
    l[QStringLiteral("hillshade")] = layer.hillshade;
    layers.append(l);
  }
  o[QStringLiteral("layers")] = layers;

  QJsonArray history;
  for (const auto &r : history_) {
    QJsonObject h;
    h[QStringLiteral("sequence")] = static_cast<double>(r.sequence);
    h[QStringLiteral("kind")] = q(r.kind);
    h[QStringLiteral("title")] = q(r.title);
    h[QStringLiteral("state")] = q(r.state);
    h[QStringLiteral("started")] = r.started;
    h[QStringLiteral("finished")] = r.finished;
    h[QStringLiteral("exitCode")] = r.exitCode;
    h[QStringLiteral("error")] = q(r.error);
    h[QStringLiteral("commands")] = toJson(r.commands);
    h[QStringLiteral("inputs")] = toJson(r.inputs);
    h[QStringLiteral("outputs")] = toJson(r.outputs);
    h[QStringLiteral("parameters")] = toJson(r.parameters);
    h[QStringLiteral("log")] = rel(r.logPath);
    h[QStringLiteral("workingDirectory")] = rel(r.workingDirectory);
    h[QStringLiteral("mbsystem")] = q(r.mbsystemVersion);
    h[QStringLiteral("attempt")] = r.attempt;
    h[QStringLiteral("retryOf")] = static_cast<double>(r.retryOf);
    h[QStringLiteral("lines")] = toJson(r.lines);
    if (!r.request.empty())
      h[QStringLiteral("request")] = parseObject(r.request);
    history.append(h);
  }
  o[QStringLiteral("history")] = history;
  o[QStringLiteral("workspace")] = parseObject(workspace_);

  const std::string text = QJsonDocument(o).toJson(QJsonDocument::Indented).toStdString();
  // write a temporary file first so that a crash never leaves half a project
  const std::string temporary = file_ + ".tmp";
  if (!platform::writeTextFile(temporary, text)) {
    error = "cannot write " + temporary;
    return false;
  }
  platform::removeFile(file_);
  if (!platform::writeTextFile(file_, text)) {
    error = "cannot write " + file_;
    return false;
  }
  platform::removeFile(temporary);
  return true;
}

void ProjectService::close() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!open_)
      return;
    std::string ignored;
    saveLocked(ignored);
    open_ = false;
    name_.clear();
    root_.clear();
    file_.clear();
    lines_.clear();
    assets_.clear();
    layers_.clear();
    history_.clear();
    pending_.clear();
    reserved_.clear();
    workspace_ = "{}";
  }
  changed("closed");
}

bool ProjectService::isOpen() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return open_;
}

std::string ProjectService::name() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return name_;
}

std::string ProjectService::root() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return root_;
}

std::string ProjectService::file() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return file_;
}

std::vector<SurveyLine> ProjectService::lines() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return lines_;
}

std::vector<ProjectAsset> ProjectService::assets() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<ProjectAsset> out = assets_;
  for (auto &a : out)
    deriveStatusLocked(a);
  return out;
}

void ProjectService::deriveStatusLocked(ProjectAsset &asset) const {
  asset.staleInputs = 0;
  if (!isProductType(asset.type)) {
    asset.status.clear();
    return;
  }
  if (asset.inputs.empty()) {
    // added from a file: what it was made from is not known
    asset.status = asset.regenerable ? "current" : "unknown";
    return;
  }
  int outdated = 0, pending = 0;
  for (const auto &kv : asset.inputs) {
    const auto line =
        std::find_if(lines_.begin(), lines_.end(), [&kv](const SurveyLine &l) { return l.id == kv.first; });
    if (line == lines_.end() || line->missing || line->dataStamp() != kv.second) {
      outdated++;
      continue;
    }
    const std::string st = line->processingStatus();
    if (st == "edits-pending" || st == "corrections-pending" || st == "processing-required")
      pending++;
  }
  asset.status = outdated > 0 ? "outdated" : pending > 0 ? "pending" : "current";
  asset.staleInputs = outdated > 0 ? outdated : pending;
}

std::vector<ProjectLayer> ProjectService::layers() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return layers_;
}

std::vector<JobRecord> ProjectService::history() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return history_;
}

const JobRecord *ProjectService::recordLocked(std::int64_t sequence) const {
  for (const auto &r : history_)
    if (r.sequence == sequence)
      return &r;
  return nullptr;
}

std::int64_t ProjectService::sequenceOfJob(std::int64_t jobId) const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (jobId == 0)
    return 0;
  for (const auto &r : history_)
    if (r.jobId == jobId)
      return r.sequence;
  return 0;
}

std::string ProjectService::workspaceState() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return workspace_;
}

void ProjectService::setWorkspaceState(const std::string &json) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!open_ || json == workspace_)
      return;
    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!doc.isObject())
      return;
    workspace_ = json;
    std::string ignored;
    saveLocked(ignored);
  }
  changed("workspace");
}

GeoBounds ProjectService::bounds(const std::vector<std::string> &lineIds) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<SurveyLine> inspected;
  for (const auto &line : selectLocked(lineIds))
    if (line.inspected)
      inspected.push_back(line);
  return merged(inspected);
}

std::vector<SurveyLine> ProjectService::selectLocked(const std::vector<std::string> &ids) const {
  std::vector<SurveyLine> out;
  for (const auto &line : lines_) {
    if (line.missing)
      continue;
    if (ids.empty() || contains(ids, line.id))
      out.push_back(line);
  }
  return out;
}

std::vector<LineVersion> ProjectService::lineVersions(const std::string &lineId) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<LineVersion> out;
  const auto it = std::find_if(lines_.begin(), lines_.end(), [&lineId](const SurveyLine &l) { return l.id == lineId; });
  if (it == lines_.end())
    return out;
  out.push_back(LineVersion{"original", it->path, it->format, platform::fileExists(it->path)});
  if (!it->processingPath.empty())
    out.push_back(LineVersion{"preprocessed", it->processingPath, it->processingFormat,
                              platform::fileExists(it->processingPath)});
  const std::string processed = processing_->processedName(it->workPath());
  if (platform::fileExists(processed))
    out.push_back(LineVersion{"processed", processed, it->workFormat(), true});
  return out;
}

// ---------------------------------------------------------------------------
// datalists

std::string ProjectService::datalist(const std::string &which) const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!open_)
    return std::string();
  const std::string dir = platform::joinPath(root_, "datalists");
  if (which == "raw")
    return platform::joinPath(dir, "raw.mb-1");
  if (which == "processed")
    return platform::joinPath(dir, "workp.mb-1");
  return platform::joinPath(dir, "work.mb-1");
}

bool ProjectService::writeSelectionLocked(const std::vector<SurveyLine> &lines, const std::string &directory,
                                          Selection &selection, std::string &error) const {
  std::vector<DatalistEntry> raw, work;
  for (const auto &line : lines) {
    raw.push_back({line.path, line.format, 1.0});
    work.push_back({line.workPath(), line.workFormat(), 1.0});
  }
  selection.lines = lines;
  selection.raw = platform::joinPath(directory, "raw.mb-1");
  selection.work = platform::joinPath(directory, "lines.mb-1");
  selection.workProcessed = platform::joinPath(directory, "linesp.mb-1");
  std::string rawText, workText;
  if (!processing_->datalistText(raw, directory, false, rawText, error) ||
      !processing_->datalistText(work, directory, false, workText, error))
    return false;
  platform::makeDirectories(directory);
  if (!platform::writeTextFile(selection.raw, rawText) || !platform::writeTextFile(selection.work, workText) ||
      !platform::writeTextFile(selection.workProcessed, processing_->processedDatalistText("lines.mb-1"))) {
    error = "cannot write the datalists in " + directory;
    return false;
  }
  return true;
}

bool ProjectService::writeProjectDatalistsLocked(std::string &error) {
  const std::string dir = platform::joinPath(root_, "datalists");
  std::vector<DatalistEntry> raw, work;
  for (const auto &line : lines_) {
    if (line.missing)
      continue;
    raw.push_back({line.path, line.format, 1.0});
    work.push_back({line.workPath(), line.workFormat(), 1.0});
  }
  std::string rawText, workText;
  if (!processing_->datalistText(raw, dir, false, rawText, error) ||
      !processing_->datalistText(work, dir, false, workText, error))
    return false;
  platform::makeDirectories(dir);
  if (!platform::writeTextFile(platform::joinPath(dir, "raw.mb-1"), rawText) ||
      !platform::writeTextFile(platform::joinPath(dir, "work.mb-1"), workText) ||
      !platform::writeTextFile(platform::joinPath(dir, "workp.mb-1"), processing_->processedDatalistText("work.mb-1"))) {
    error = "cannot write the project datalists";
    return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// survey lines

void ProjectService::collect(const std::string &path, bool recursive, int depth, std::vector<std::string> &files,
                             std::vector<std::string> &skipped, bool explicitFile) const {
  if (platform::isDirectory(path)) {
    if (depth > 8)
      return;
    // never import the project's own derived files
    for (const char *sub : {"processed", "products", "jobs", "datalists", "exports"})
      if (platform::absolutePath(path) == platform::absolutePath(platform::joinPath(root_, sub)))
        return;
    std::vector<std::string> entries = platform::listDirectory(path);
    std::sort(entries.begin(), entries.end());
    for (const auto &entry : entries) {
      const std::string child = platform::joinPath(path, entry);
      if (platform::isDirectory(child)) {
        if (recursive)
          collect(child, recursive, depth + 1, files, skipped, false);
      } else {
        collect(child, recursive, depth + 1, files, skipped, false);
      }
    }
    return;
  }
  if (!platform::fileExists(path)) {
    skipped.push_back(path + ": file not found");
    return;
  }
  const std::string suffix = suffixOf(path);
  if (ignoredSuffix(suffix)) {
    if (explicitFile)
      skipped.push_back(platform::fileName(path) + ": not a swath data file");
    return;
  }
  // "linep.mb59" next to "line.mb59" is an mbprocess output, not a new line
  const std::string stem = stemOf(path);
  if (stem.size() > 1 && stem.back() == 'p' &&
      platform::fileExists(platform::joinPath(platform::parentPath(path), stem.substr(0, stem.size() - 1) + suffix)))
    return;
  files.push_back(path);
}

ImportResult ProjectService::addFiles(const std::vector<std::string> &paths, bool recursive) {
  ImportResult result;
  std::vector<std::string> files;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!open_) {
      result.skipped.push_back("no project is open");
      return result;
    }
  }
  for (const auto &p : paths)
    collect(platform::absolutePath(p), recursive, 0, files, result.skipped, !platform::isDirectory(p));

  std::vector<SurveyLine> added;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string datalistDir = platform::joinPath(root_, "datalists");
    for (const auto &file : files) {
      const std::string path = platform::absolutePath(file);
      if (std::any_of(lines_.begin(), lines_.end(), [&path](const SurveyLine &l) { return l.path == path; }) ||
          std::any_of(added.begin(), added.end(), [&path](const SurveyLine &l) { return l.path == path; })) {
        result.skipped.push_back(platform::fileName(path) + ": already in the project");
        continue;
      }
      const int format = formats_ ? formats_->identify(path) : 0;
      if (format <= 0) {
        result.skipped.push_back(platform::fileName(path) +
                                 (format < 0 ? ": datalists are not imported, add their files"
                                             : ": format not recognised by MB-System"));
        continue;
      }
      const std::string relative = platform::relativePath(path, datalistDir);
      if (relative.find(' ') != std::string::npos) {
        result.skipped.push_back(platform::fileName(path) +
                                 ": the path contains spaces, which MB-System datalists cannot hold; "
                                 "move the data to a folder without spaces");
        continue;
      }
      SurveyLine line;
      line.id = newId('L');
      line.path = path;
      line.format = format;
      FormatInfo info;
      if (formats_->format(format, info))
        line.formatName = info.name;
      line.size = platform::fileSize(path);
      line.state = "raw";
      added.push_back(line);
    }
    if (added.empty())
      return result;
    lines_.insert(lines_.end(), added.begin(), added.end());
    std::string error;
    if (!writeProjectDatalistsLocked(error))
      result.skipped.push_back(error);
    if (jobs_) {
      std::vector<std::string> ids;
      for (const auto &line : added)
        ids.push_back(line.id);
      result.job = submitImportLocked(ids, 0, 1, error);
      if (result.job == 0)
        result.skipped.push_back(error);
    }
    saveLocked(error);
  }
  result.added = static_cast<int>(added.size());
  changed("lines");
  changed("history");
  return result;
}

std::int64_t ProjectService::submitImportLocked(const std::vector<std::string> &ids, std::int64_t retryOf, int attempt,
                                                std::string &error) {
  const std::vector<SurveyLine> lines = selectLocked(ids);
  if (lines.empty() || ids.empty()) {
    error = "the survey lines are no longer in the project";
    return 0;
  }
  const std::int64_t sequence = nextJob_;
  const std::string jobDirectory = platform::joinPath(root_, "jobs/" + std::to_string(sequence));
  Selection selection;
  if (!writeSelectionLocked(lines, jobDirectory, selection, error))
    return 0;

  // statistics, caches (.inf/.fbt/.fnv) and the metadata of the lines
  CommandPlan plan = processing_->ancillary(platform::relativePath(selection.work, jobDirectory));
  for (auto &step : plan.steps)
    step.allowFailure = true;  // a single bad file must not hide the others
  std::vector<std::string> lineIds;
  for (const auto &line : lines)
    lineIds.push_back(line.id);
  std::weak_ptr<ProjectService> weak = shared_from_this();
  const std::string projectFile = file_;
  JobStep metadata;
  metadata.title = "Read survey line metadata";
  metadata.action = [weak, lineIds, projectFile](std::string &log) {
    auto self = weak.lock();
    if (!self || self->file() != projectFile)
      return true;
    int read = 0;
    for (const auto &id : lineIds) {
      SurveyLine line;
      {
        std::lock_guard<std::mutex> lock(self->mutex_);
        const auto it = std::find_if(self->lines_.begin(), self->lines_.end(),
                                     [&id](const SurveyLine &l) { return l.id == id; });
        if (it == self->lines_.end())
          continue;
        line = *it;
      }
      const SwathSummary summary = self->swath_->inspect(line.workPath(), line.workFormat(), 2, nullptr);
      if (!summary.ok) {
        log += platform::fileName(line.path) + ": " + summary.error + "\n";
        continue;
      }
      read++;
      std::lock_guard<std::mutex> lock(self->mutex_);
      const auto it = std::find_if(self->lines_.begin(), self->lines_.end(),
                                   [&id](const SurveyLine &l) { return l.id == id; });
      if (it == self->lines_.end())
        continue;
      it->inspected = true;
      it->records = summary.records;
      it->maxBeams = summary.maxBeams;
      it->beamsTotal = summary.beamsTotal;
      it->beamsGood = summary.beamsGood;
      it->timeStart = summary.timeStartIso;
      it->timeEnd = summary.timeEndIso;
      it->bounds = GeoBounds{summary.lonMin, summary.lonMax, summary.latMin, summary.latMax};
      it->depthMin = summary.depthMin;
      it->depthMax = summary.depthMax;
      it->trackLengthKm = summary.trackLengthKm;
      if (it->formatName.empty())
        it->formatName = summary.formatName;
    }
    log += "metadata of " + std::to_string(read) + " of " + std::to_string(lineIds.size()) + " survey lines\n";
    {
      std::lock_guard<std::mutex> lock(self->mutex_);
      std::string ignored;
      self->saveLocked(ignored);
    }
    self->changed("lines");
    return read > 0 || lineIds.empty();
  };
  plan.steps.push_back(metadata);

  JobSpec spec;
  spec.title = "Import " + std::to_string(lines.size()) + " survey line" + (lines.size() == 1 ? "" : "s");
  if (attempt > 1)
    spec.title += " (attempt " + std::to_string(attempt) + ")";
  spec.kind = "import";
  spec.owner = "project";
  spec.workingDirectory = jobDirectory;
  spec.logPath = platform::joinPath(jobDirectory, "job.log");
  spec.steps = plan.steps;
  spec.retryable = false;
  for (const auto &line : lines)
    spec.inputs.push_back(line.path);
  spec.parameters["lines"] = static_cast<std::int64_t>(lines.size());
  resources("import", lineIds, std::string(), {}, false, spec.reads, spec.writes);

  JobRecord record;
  record.sequence = sequence;
  record.kind = spec.kind;
  record.title = spec.title;
  record.state = "queued";
  record.commands = plan.commandLines();
  record.inputs = spec.inputs;
  record.parameters = spec.parameters;
  record.logPath = spec.logPath;
  record.workingDirectory = jobDirectory;
  record.mbsystemVersion = mbsystemVersion_;
  record.attempt = attempt;
  record.retryOf = retryOf;
  record.lines = lineIds;
  ProcessingRequest request;
  request.kind = "import";
  request.lines = lineIds;
  record.request = compact(requestToJson(request));
  // submit under the lock so that the job cannot finish before it is pending
  const std::int64_t id = jobs_->submit(spec, &error);
  if (id == 0)
    return 0;
  nextJob_++;
  record.jobId = id;
  history_.push_back(record);
  pending_[id] = sequence;
  return id;
}

int ProjectService::removeLines(const std::vector<std::string> &ids) {
  int removed = 0;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto before = lines_.size();
    lines_.erase(std::remove_if(lines_.begin(), lines_.end(),
                                [&ids](const SurveyLine &l) { return contains(ids, l.id); }),
                 lines_.end());
    removed = static_cast<int>(before - lines_.size());
    if (removed == 0)
      return 0;
    std::string ignored;
    writeProjectDatalistsLocked(ignored);
    saveLocked(ignored);
  }
  changed("lines");
  return removed;
}

std::string ProjectService::fnvOf(const SurveyLine &line) const {
  for (const std::string &candidate :
       {processing_->processedName(line.workPath()) + ".fnv", line.workPath() + ".fnv", line.path + ".fnv"})
    if (platform::fileExists(candidate))
      return candidate;
  return std::string();
}

std::vector<TrackPoint> ProjectService::navigation(const std::string &lineId, std::size_t maxPoints) const {
  SurveyLine line;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = std::find_if(lines_.begin(), lines_.end(), [&lineId](const SurveyLine &l) { return l.id == lineId; });
    if (it == lines_.end())
      return {};
    line = *it;
  }
  const std::string fnv = fnvOf(line);
  return fnv.empty() ? std::vector<TrackPoint>() : swath_->readNavigation(fnv, maxPoints);
}

// ---------------------------------------------------------------------------
// products and layers

ProjectAsset *ProjectService::assetLocked(const std::string &id) {
  for (auto &a : assets_)
    if (a.id == id)
      return &a;
  return nullptr;
}

std::string ProjectService::addGrid(const std::string &path) {
  const std::string absolute = platform::absolutePath(path);
  // the coordinate system of the grid (read before taking the lock)
  ValueMap described;
  if (grids_ && platform::fileExists(absolute)) {
    const GridLoadResult r = grids_->load(absolute);
    if (r.ok) {
      described["geographic"] = r.grid.geographic;
      described["gridProjection"] = r.grid.projection;
      described["west"] = r.grid.xmin;
      described["east"] = r.grid.xmax;
      described["south"] = r.grid.ymin;
      described["north"] = r.grid.ymax;
    }
  }
  std::string id;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!open_ || !platform::fileExists(absolute))
      return std::string();
    for (const auto &a : assets_)
      if (a.path == absolute)
        return a.id;
    ProjectAsset asset;
    asset.id = newId('A');
    asset.path = absolute;
    asset.type = "grid";
    asset.label = stemOf(absolute);
    asset.regenerable = false;
    asset.created = now();
    asset.parameters = described;
    assets_.push_back(asset);
    id = asset.id;
    // quality grids written by mbgrid -M next to it
    const std::string root = absolute.substr(0, absolute.size() - suffixOf(absolute).size());
    for (const char *kind : {"num", "sd"}) {
      const std::string extra = root + "_" + kind + ".grd";
      if (platform::fileExists(extra)) {
        ProjectAsset quality;
        quality.id = newId('A');
        quality.path = extra;
        quality.type = std::string("grid_") + kind;
        quality.label = stemOf(extra);
        quality.parent = id;
        quality.regenerable = false;
        quality.created = now();
        quality.parameters = described;
        assets_.push_back(quality);
      }
    }
    layers_.push_back(ProjectLayer{id, true, 1.0, 0, true});
    std::string ignored;
    saveLocked(ignored);
  }
  changed("assets");
  return id;
}

bool ProjectService::removeAsset(const std::string &id, bool deleteFiles) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> remove;
    for (const auto &a : assets_)
      if (a.id == id || a.parent == id)
        remove.push_back(a.id);
    if (remove.empty())
      return false;
    for (const auto &a : assets_)
      if (deleteFiles && contains(remove, a.id))
        platform::removeFile(a.path);
    assets_.erase(std::remove_if(assets_.begin(), assets_.end(),
                                 [&remove](const ProjectAsset &a) { return contains(remove, a.id); }),
                  assets_.end());
    layers_.erase(std::remove_if(layers_.begin(), layers_.end(),
                                 [&remove](const ProjectLayer &l) { return contains(remove, l.asset); }),
                  layers_.end());
    std::string ignored;
    saveLocked(ignored);
  }
  changed("assets");
  return true;
}

bool ProjectService::setLayer(const ProjectLayer &layer) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!assetLocked(layer.asset))
      return false;
    auto it = std::find_if(layers_.begin(), layers_.end(), [&layer](const ProjectLayer &l) { return l.asset == layer.asset; });
    ProjectLayer clamped = layer;
    clamped.opacity = std::clamp(layer.opacity, 0.0, 1.0);
    if (it == layers_.end())
      layers_.push_back(clamped);
    else
      *it = clamped;
    std::string ignored;
    saveLocked(ignored);
  }
  changed("layers");
  return true;
}

bool ProjectService::moveLayer(const std::string &asset, int position) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(layers_.begin(), layers_.end(), [&asset](const ProjectLayer &l) { return l.asset == asset; });
    if (it == layers_.end())
      return false;
    const ProjectLayer layer = *it;
    layers_.erase(it);
    position = std::clamp(position, 0, static_cast<int>(layers_.size()));
    layers_.insert(layers_.begin() + position, layer);
    std::string ignored;
    saveLocked(ignored);
  }
  changed("layers");
  return true;
}

std::string ProjectService::uniqueRootLocked(const std::string &directory, const std::string &name,
                                             const std::string &ext) const {
  // neither an existing file nor one that a queued or running job will write
  auto taken = [this, &ext](const std::string &root) {
    if (platform::fileExists(root + ext))
      return true;
    const std::string candidate = platform::absolutePath(root + ext);
    for (const auto &kv : reserved_)
      for (const auto &output : kv.second)
        if (output == candidate)
          return true;
    return false;
  };
  const std::string base = safeName(name);
  std::string candidate = platform::joinPath(directory, base);
  for (int n = 2; taken(candidate) && n < 1000; n++)
    candidate = platform::joinPath(directory, base + "-" + std::to_string(n));
  return candidate;
}

void ProjectService::registerOutputs(const std::string &kind, const std::vector<std::string> &outputs,
                                     const std::string &label, std::int64_t sequence, const ValueMap &parameters,
                                     const std::map<std::string, std::string> &inputs) {
  // describe the grids (coordinate system and extent) before taking the lock
  std::map<std::string, ValueMap> described;
  if (grids_ && (kind == "grid" || kind == "mosaic"))
    for (const auto &output : outputs) {
      if (!platform::fileExists(output) || suffixOf(output) != ".grd")
        continue;
      const GridLoadResult r = grids_->load(output);
      if (!r.ok)
        continue;
      ValueMap &d = described[platform::absolutePath(output)];
      d["geographic"] = r.grid.geographic;
      d["gridProjection"] = r.grid.projection;
      d["west"] = r.grid.xmin;
      d["east"] = r.grid.xmax;
      d["south"] = r.grid.ymin;
      d["north"] = r.grid.ymax;
    }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string mainId;
    for (const auto &output : outputs) {
      if (!platform::fileExists(output))
        continue;
      const std::string path = platform::absolutePath(output);
      ProjectAsset asset;
      const std::string stem = stemOf(path);
      std::string type = kind;
      if (kind == "grid" || kind == "mosaic") {
        if (endsWith(stem, "_num"))
          type = kind + "_num";
        else if (endsWith(stem, "_sd"))
          type = kind + "_sd";
      }
      // replace an older record of the same file
      for (auto &a : assets_)
        if (a.path == path)
          asset.id = a.id;
      assets_.erase(std::remove_if(assets_.begin(), assets_.end(), [&path](const ProjectAsset &a) { return a.path == path; }),
                    assets_.end());
      if (asset.id.empty())
        asset.id = newId('A');
      asset.path = path;
      asset.type = type;
      if (endsWith(type, "_num"))
        asset.label = label + " (count)";
      else if (endsWith(type, "_sd"))
        asset.label = label + " (std. dev.)";
      else if (kind == "figure")
        asset.label = label + " (" + suffixOf(path).substr(1) + ")";
      else
        asset.label = label;
      asset.job = sequence;
      asset.parameters = parameters;
      const auto d = described.find(path);
      if (d != described.end())
        for (const auto &kv : d->second)
          asset.parameters[kv.first] = kv.second;
      asset.inputs = inputs;
      asset.created = now();
      // the first file is the main product (the grid, the PNG of a figure)
      const bool main = kind == "figure" ? mainId.empty() : type == kind && mainId.empty();
      if (main)
        mainId = asset.id;
      else
        asset.parent = mainId;
      assets_.push_back(asset);
      if (type == "grid" || type == "mosaic") {
        if (std::none_of(layers_.begin(), layers_.end(), [&asset](const ProjectLayer &l) { return l.asset == asset.id; }))
          layers_.push_back(ProjectLayer{asset.id, true, 1.0, type == "mosaic" ? 3 : 0, type == "grid"});
      }
    }
    std::string ignored;
    saveLocked(ignored);
  }
  changed("assets");
}

// ---------------------------------------------------------------------------
// processing

CommandPlan ProjectService::planLocked(const ProcessingRequest &request, const std::string &jobDirectory,
                                       const Selection &selection, std::int64_t sequence) const {
  CommandPlan plan;
  if (!open_) {
    plan.error = "no project is open";
    return plan;
  }
  // Programs get paths relative to the job folder (their working directory):
  // MBIO builds shell commands from datalist entries without quoting, so
  // relative paths keep spaces in the project folder name harmless.
  auto rel = [&jobDirectory](const std::string &path) { return platform::relativePath(path, jobDirectory); };
  auto findAsset = [this](const std::string &id) -> const ProjectAsset * {
    for (const auto &a : assets_)
      if (a.id == id)
        return &a;
    return nullptr;
  };
  const bool needsLines = request.kind != "export-grid" && request.kind != "figure";
  if (needsLines && selection.lines.empty()) {
    plan.error = "no survey lines selected";
    return plan;
  }
  if (request.kind == "preprocess") {
    PreprocessParameters p = request.preprocess;
    p.outputDirectory = "processed";  // relative to the project folder, no spaces
    plan = processing_->preprocess(platform::relativePath(selection.raw, root_), p);
    for (auto &step : plan.steps)
      step.workingDirectory = root_;
    for (const auto &line : selection.lines)
      plan.outputs.push_back(
          platform::joinPath(platform::joinPath(root_, "processed"), processing_->preprocessOutputName(line.path, line.format)));
    if (plan.ok && std::none_of(selection.lines.begin(), selection.lines.end(),
                                [this](const SurveyLine &l) { return processing_->recommendPreprocess(l.format); }))
      plan.warnings.push_back("These survey lines are already in a processing format; preprocessing is optional.");
  } else if (request.kind == "clean") {
    plan = processing_->clean(rel(selection.work), request.clean);
  } else if (request.kind == "corrections") {
    CorrectionParameters p = request.corrections;
    p.workingDirectory = jobDirectory;
    if (p.svpMode == 2 && p.longitude == 0.0 && p.latitude == 0.0) {
      // centre of the selected lines
      const GeoBounds b = merged(selection.lines);
      p.longitude = 0.5 * (b.west + b.east);
      p.latitude = 0.5 * (b.south + b.north);
    }
    plan = processing_->corrections(rel(selection.work), p);
  } else if (request.kind == "process") {
    ProcessParameters p = request.process;
    p.processedDatalist = rel(selection.workProcessed);
    plan = processing_->process(rel(selection.work), p);
  } else if (request.kind == "grid") {
    GridParameters p = request.grid;
    const std::string name = request.name.empty() ? std::string("bathymetry") : request.name;
    p.outputRoot = rel(uniqueRootLocked(platform::joinPath(root_, "products/grids"), name, ".grd"));
    plan = processing_->grid(rel(selection.workProcessed), p);
  } else if (request.kind == "mosaic") {
    MosaicParameters p = request.mosaic;
    const std::string name = request.name.empty() ? std::string("mosaic") : request.name;
    p.outputRoot = rel(uniqueRootLocked(platform::joinPath(root_, "products/mosaics"), name, ".grd"));
    p.processedDatalist = rel(selection.workProcessed);
    plan = processing_->mosaic(rel(p.angleCorrection ? selection.work : selection.workProcessed), p);
  } else if (request.kind == "export-grid") {
    const ProjectAsset *asset = findAsset(request.asset);
    if (asset == nullptr) {
      plan.error = "choose a grid to export";
      return plan;
    }
    ExportParameters p = request.exporting;
    const std::string ext = p.kind == "geotiff" ? ".tif" : p.kind == "esri" ? ".asc" : p.kind == "gltf" ? ".glb" : ".xyz";
    const std::string root = uniqueRootLocked(platform::joinPath(root_, "exports"),
                                              request.name.empty() ? stemOf(asset->path) : request.name, ext);
    p.output = rel(p.kind == "gltf" ? root : root + ext);
    plan = processing_->exportGrid(rel(asset->path), p);
  } else if (request.kind == "export-soundings") {
    ExportParameters p = request.exporting;
    const std::string ext = p.kind == "las" ? ".las" : ".xyz";
    p.output = rel(uniqueRootLocked(platform::joinPath(root_, "exports"),
                                    request.name.empty() ? std::string("soundings") : request.name, ext) +
                   ext);
    plan = processing_->exportSoundings(rel(selection.workProcessed), p);
  } else if (request.kind == "figure") {
    FigureParameters p = request.figure;
    p.grid.clear();
    p.gridBounds = GeoBounds();
    if (!request.asset.empty()) {
      const ProjectAsset *asset = findAsset(request.asset);
      if (asset == nullptr) {
        plan.error = "the grid of the figure is no longer in the project";
        return plan;
      }
      p.grid = rel(asset->path);
      // grids registered by MB-Studio 0.3 describe their coordinate system;
      // older ones are geographic unless they were gridded in UTM
      const ValueMap &d = asset->parameters;
      if (d.count("geographic"))
        p.geographic = getBool(d, "geographic", true);
      else
        p.geographic = getString(d, "projection").empty();
      if (d.count("west"))
        p.gridBounds = GeoBounds{getDouble(d, "west"), getDouble(d, "east"), getDouble(d, "south"), getDouble(d, "north")};
    } else {
      p.geographic = true;
    }
    p.tracks.clear();
    if (p.kind == "overview" || p.showTracks)
      for (const auto &line : selection.lines) {
        const std::string fnv = fnvOf(line);
        if (!fnv.empty())
          p.tracks.push_back(rel(fnv));
      }
    if (!p.region.valid() && p.grid.empty()) {
      GeoBounds b = merged(selection.lines);
      if (b.valid()) {
        const double mx = 0.05 * (b.east - b.west), my = 0.05 * (b.north - b.south);
        b = GeoBounds{b.west - mx, b.east + mx, b.south - my, b.north + my};
      }
      p.region = b;
    }
    if (p.kind == "quality" && p.palette == "haxby")
      p.palette = "hot";
    const std::string ext = p.formats.empty() ? std::string(".png") : "." + p.formats.front();
    const std::string name = request.name.empty() ? p.kind : request.name;
    p.outputRoot = rel(uniqueRootLocked(platform::joinPath(root_, "products/figures"), name, ext));
    p.session = "mbstudio-job-" + std::to_string(sequence);
    p.temporaryDirectory = jobDirectory;
    p.ghostscript = processing_->figureDependencies().ghostscriptPath;
    plan = processing_->figure(p);
  } else {
    plan.error = "unknown processing step: " + request.kind;
    return plan;
  }
  if (plan.ok) {
    for (auto &step : plan.steps)
      if (step.workingDirectory.empty())
        step.workingDirectory = jobDirectory;
    for (auto &output : plan.outputs)
      if (!platform::isAbsolutePath(output))
        output = platform::absolutePath(platform::joinPath(jobDirectory, output));
    plan.parameters["lines"] = static_cast<std::int64_t>(selection.lines.size());
  }
  return plan;
}

CommandPlan ProjectService::plan(const ProcessingRequest &request) const {
  std::lock_guard<std::mutex> lock(mutex_);
  Selection selection;
  const std::string jobDirectory = platform::joinPath(root_, "jobs/" + std::to_string(nextJob_));
  selection.lines = selectLocked(request.lines);
  selection.raw = platform::joinPath(jobDirectory, "raw.mb-1");
  selection.work = platform::joinPath(jobDirectory, "lines.mb-1");
  selection.workProcessed = platform::joinPath(jobDirectory, "linesp.mb-1");
  return planLocked(request, jobDirectory, selection, nextJob_);
}

std::int64_t ProjectService::run(const ProcessingRequest &request, std::string &error) {
  if (!jobs_) {
    error = "the job service is not available";
    return 0;
  }
  std::int64_t jobId = 0;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!open_) {
      error = "no project is open";
      return 0;
    }
    jobId = submitLocked(request, 0, 1, error);
    if (jobId == 0)
      return 0;
    std::string ignored;
    saveLocked(ignored);
  }
  changed("history");
  return jobId;
}

std::int64_t ProjectService::submitLocked(const ProcessingRequest &request, std::int64_t retryOf, int attempt,
                                          std::string &error) {
  if (request.kind == "import")
    return submitImportLocked(request.lines, retryOf, attempt, error);
  if (request.kind == "edit") {
    error = "sounding edits are saved from the sounding editor";
    return 0;
  }
  const std::int64_t sequence = nextJob_;
  const std::string jobDirectory = platform::joinPath(root_, "jobs/" + std::to_string(sequence));
  Selection selection;
  if (request.kind != "export-grid") {
    if (!writeSelectionLocked(selectLocked(request.lines), jobDirectory, selection, error))
      return 0;
  } else {
    platform::makeDirectories(jobDirectory);
  }
  if (request.kind == "preprocess")
    platform::makeDirectories(platform::joinPath(root_, "processed"));
  CommandPlan plan = planLocked(request, jobDirectory, selection, sequence);
  if (!plan.ok) {
    error = plan.error;
    return 0;
  }
  for (const auto &output : plan.outputs)
    platform::makeDirectories(platform::parentPath(output));

  std::vector<std::string> ids;
  std::map<std::string, std::string> inputs;     // what the products are made from
  std::map<std::string, std::array<int, 3>> revisions;  // what mbprocess applies
  for (const auto &line : selection.lines) {
    ids.push_back(line.id);
    inputs[line.id] = line.dataStamp();
    revisions[line.id] = {line.editRevision, line.correctionRevision, line.sourceRevision};
  }
  std::string readFile;
  if (request.kind == "export-grid" || request.kind == "figure")
    for (const auto &a : assets_)
      if (a.id == request.asset)
        readFile = a.path;
  if (request.kind == "export-grid" || (request.kind == "figure" && request.figure.kind != "overview" &&
                                         !request.figure.showTracks))
    inputs.clear();
  if (request.kind == "export-grid" || request.kind == "figure") {
    // a product of a product: made from the same data as its grid
    for (const auto &a : assets_)
      if (a.id == request.asset)
        for (const auto &kv : a.inputs)
          inputs.emplace(kv.first, kv.second);
  }

  // a last step that records what the programs produced in the project
  std::weak_ptr<ProjectService> weak = shared_from_this();
  const std::string projectFile = file_;
  const std::string kind = request.kind;
  const std::vector<std::string> outputs = plan.outputs;
  const std::string label = request.name.empty() ? plan.title : request.name;
  const ValueMap parameters = plan.parameters;
  const bool angleCorrection = request.kind == "mosaic" && request.mosaic.angleCorrection;
  JobStep record;
  record.title = "Register the results in the project";
  record.action = [weak, projectFile, kind, outputs, ids, label, sequence, parameters, inputs, revisions,
                   angleCorrection](std::string &log) {
    auto self = weak.lock();
    if (!self || self->file() != projectFile) {
      log += "the project was closed; the results are not registered\n";
      return true;
    }
    if (kind == "grid" || kind == "mosaic" || kind == "export-grid" || kind == "export-soundings" ||
        kind == "corrections" || kind == "figure") {
      const std::string type = kind == "export-grid" || kind == "export-soundings" ? std::string("export")
                               : kind == "corrections"                             ? std::string("svp")
                                                                                   : kind;
      self->registerOutputs(type, outputs, label, sequence, parameters,
                            kind == "corrections" ? std::map<std::string, std::string>() : inputs);
      int found = 0;
      for (const auto &o : outputs)
        found += platform::fileExists(o) ? 1 : 0;
      log += std::to_string(found) + " product(s) added to the project\n";
      if (kind == "corrections" || angleCorrection) {
        // new parameters for mbprocess: the processed files are behind
        {
          std::lock_guard<std::mutex> lock(self->mutex_);
          for (auto &line : self->lines_) {
            if (!contains(ids, line.id))
              continue;
            line.correctionRevision++;
            // the mosaic job ran mbprocess after mbbackangle
            if (angleCorrection && platform::fileExists(self->processing_->processedName(line.workPath()))) {
              line.processedEdit = line.editRevision;
              line.processedCorrection = line.correctionRevision;
              line.processedSource = line.sourceRevision;
              line.state = "processed";
            }
          }
          std::string ignored;
          self->saveLocked(ignored);
        }
        self->changed("lines");
      }
      if (found == 0 && kind != "corrections") {
        log += "the program finished without writing its products\n";
        return false;
      }
      return true;
    }
    // preprocess / clean / process: follow the files and revisions of each line
    int updated = 0;
    {
      std::lock_guard<std::mutex> lock(self->mutex_);
      for (auto &line : self->lines_) {
        if (!contains(ids, line.id))
          continue;
        if (kind == "preprocess") {
          const std::string output = platform::absolutePath(platform::joinPath(
              platform::joinPath(self->root_, "processed"), self->processing_->preprocessOutputName(line.path, line.format)));
          if (platform::fileExists(output)) {
            if (line.processingPath != output) {
              // a new working file: its edits and corrections start afresh
              line.sourceRevision++;
              line.editRevision = line.correctionRevision = 0;
              line.processedEdit = line.processedCorrection = line.processedSource = -1;
            }
            line.processingPath = output;
            line.processingFormat = self->processing_->preprocessOutputFormat(line.format);
            line.state = "preprocessed";
            updated++;
          }
        } else if (kind == "clean") {
          line.editRevision++;
          updated++;
        } else if (kind == "process") {
          const auto r = revisions.find(line.id);
          if (platform::fileExists(self->processing_->processedName(line.workPath())) && r != revisions.end() &&
              r->second[2] == line.sourceRevision) {
            // the processed file has what was set when the job was started
            line.processedEdit = r->second[0];
            line.processedCorrection = r->second[1];
            line.processedSource = r->second[2];
            line.state = "processed";
            updated++;
          }
        }
      }
      std::string error;
      if (kind == "preprocess" && !self->writeProjectDatalistsLocked(error))
        log += error + "\n";
      self->saveLocked(error);
    }
    self->changed("lines");
    self->changed("assets");  // product states depend on the lines
    log += std::to_string(updated) + " of " + std::to_string(ids.size()) + " survey lines updated\n";
    return kind == "clean" || updated > 0;
  };
  plan.steps.push_back(record);

  JobSpec spec;
  spec.title = attempt > 1 ? plan.title + " (attempt " + std::to_string(attempt) + ")" : plan.title;
  spec.kind = request.kind;
  spec.owner = "project";
  spec.workingDirectory = jobDirectory;
  spec.logPath = platform::joinPath(jobDirectory, "job.log");
  spec.steps = plan.steps;
  spec.outputs = plan.outputs;
  spec.parameters = plan.parameters;
  spec.retryable = false;  // repeated by retry() with new outputs and a new record
  for (const auto &line : selection.lines)
    spec.inputs.push_back(line.workPath());
  if (!readFile.empty())
    spec.inputs.push_back(readFile);
  resources(request.kind, ids, readFile, plan.outputs, angleCorrection, spec.reads, spec.writes);

  JobRecord entry;
  entry.sequence = sequence;
  entry.kind = request.kind;
  entry.title = spec.title;
  entry.state = "queued";
  entry.commands = plan.commandLines();
  entry.inputs = spec.inputs;
  entry.outputs = plan.outputs;
  entry.parameters = plan.parameters;
  entry.logPath = spec.logPath;
  entry.workingDirectory = jobDirectory;
  entry.mbsystemVersion = mbsystemVersion_;
  entry.attempt = attempt;
  entry.retryOf = retryOf;
  entry.lines = ids;
  ProcessingRequest recorded = request;
  recorded.lines = ids;  // the lines it actually worked on
  entry.request = compact(requestToJson(recorded));
  const std::int64_t jobId = jobs_->submit(spec, &error);
  if (jobId == 0)
    return 0;
  nextJob_++;
  entry.jobId = jobId;
  history_.push_back(entry);
  pending_[jobId] = sequence;
  reserved_[sequence] = plan.outputs;
  return jobId;
}

bool ProjectService::request(std::int64_t sequence, ProcessingRequest &request) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const JobRecord *r = recordLocked(sequence);
  if (r == nullptr || r->request.empty())
    return false;
  return requestFromJson(parseObject(r->request), request);
}

std::int64_t ProjectService::retry(std::int64_t sequence, std::string &error) {
  std::int64_t jobId = 0;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!open_) {
      error = "no project is open";
      return 0;
    }
    const JobRecord *r = recordLocked(sequence);
    if (r == nullptr) {
      error = "no such job in the project history";
      return 0;
    }
    if (r->state == "queued" || r->state == "running") {
      error = "the job is still running";
      return 0;
    }
    if (r->request.empty()) {
      error = "this job was recorded by an earlier version of MB-Studio; run it again from its processing panel";
      return 0;
    }
    ProcessingRequest request;
    if (!requestFromJson(parseObject(r->request), request)) {
      error = "the recorded request of the job cannot be read";
      return 0;
    }
    if (request.kind == "edit") {
      error = "sounding edits are saved from the sounding editor; open the lines there and save again";
      return 0;
    }
    if (!r->lines.empty()) {
      request.lines.clear();
      for (const auto &id : r->lines)
        if (std::any_of(lines_.begin(), lines_.end(), [&id](const SurveyLine &l) { return l.id == id && !l.missing; }))
          request.lines.push_back(id);
      if (request.lines.empty() && request.kind != "export-grid" && request.kind != "figure") {
        error = "the survey lines of the job are no longer in the project";
        return 0;
      }
    }
    const int attempt = r->attempt + 1;
    jobId = submitLocked(request, sequence, attempt, error);
    if (jobId == 0)
      return 0;
    std::string ignored;
    saveLocked(ignored);
  }
  changed("history");
  return jobId;
}

std::int64_t ProjectService::regenerate(const std::string &assetId, std::string &error) {
  std::int64_t jobId = 0;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const ProjectAsset *asset = nullptr;
    for (const auto &a : assets_)
      if (a.id == assetId)
        asset = &a;
    if (asset != nullptr && !asset->parent.empty())
      for (const auto &a : assets_)
        if (a.id == asset->parent)
          asset = &a;
    if (asset == nullptr) {
      error = "no such product";
      return 0;
    }
    const JobRecord *r = recordLocked(asset->job);
    ProcessingRequest request;
    if (r == nullptr || r->request.empty() || !requestFromJson(parseObject(r->request), request)) {
      error = "the recipe of this product is not recorded (it was added from a file or by an earlier version)";
      return 0;
    }
    if (!r->lines.empty())
      request.lines = r->lines;
    request.lines.erase(std::remove_if(request.lines.begin(), request.lines.end(),
                                       [this](const std::string &id) {
                                         return std::none_of(lines_.begin(), lines_.end(), [&id](const SurveyLine &l) {
                                           return l.id == id && !l.missing;
                                         });
                                       }),
                        request.lines.end());
    if (request.lines.empty() && request.kind != "export-grid" && request.kind != "figure") {
      error = "the survey lines of this product are no longer in the project";
      return 0;
    }
    if (request.name.empty())
      request.name = asset->label;
    jobId = submitLocked(request, 0, 1, error);
    if (jobId == 0)
      return 0;
    std::string ignored;
    saveLocked(ignored);
  }
  changed("history");
  return jobId;
}

std::int64_t ProjectService::saveEdits(const std::map<std::string, std::vector<BeamEdit>> &edits, std::string &error) {
  if (!jobs_ || !soundings_) {
    error = "sounding editing is not available";
    return 0;
  }
  std::int64_t jobId = 0;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!open_) {
      error = "no project is open";
      return 0;
    }
    std::vector<std::string> ids;
    std::size_t count = 0;
    std::weak_ptr<ProjectService> weak = shared_from_this();
    const std::string projectFile = file_;
    JobSpec spec;
    for (const auto &kv : edits) {
      if (kv.second.empty())
        continue;
      const auto line =
          std::find_if(lines_.begin(), lines_.end(), [&kv](const SurveyLine &l) { return l.id == kv.first; });
      if (line == lines_.end() || line->missing) {
        error = "the survey line " + kv.first + " is no longer in the project";
        return 0;
      }
      ids.push_back(line->id);
      count += kv.second.size();
      const std::string id = line->id;
      const std::string path = line->workPath();
      const int format = line->workFormat();
      const std::vector<BeamEdit> list = kv.second;
      JobStep step;
      step.title = "Save " + std::to_string(list.size()) + " edits of " + platform::fileName(path);
      step.action = [weak, projectFile, id, path, format, list](std::string &log) {
        auto self = weak.lock();
        if (!self)
          return false;
        std::string failure, esf;
        if (!self->soundings_->saveEdits(path, format, list, failure, &esf)) {
          log += failure + "\n";
          return false;
        }
        log += std::to_string(list.size()) + " edit events appended to " + esf + "\n";
        if (self->file() != projectFile)
          return true;
        {
          std::lock_guard<std::mutex> lock(self->mutex_);
          for (auto &l : self->lines_)
            if (l.id == id && l.workPath() == path)
              l.editRevision++;
          std::string ignored;
          self->saveLocked(ignored);
        }
        self->changed("lines");
        self->changed("assets");
        return true;
      };
      spec.steps.push_back(step);
      spec.inputs.push_back(path);
      spec.outputs.push_back(path + ".esf");
    }
    if (spec.steps.empty()) {
      error = "there are no edits to save";
      return 0;
    }
    const std::int64_t sequence = nextJob_;
    const std::string jobDirectory = platform::joinPath(root_, "jobs/" + std::to_string(sequence));
    platform::makeDirectories(jobDirectory);
    spec.title = "Save " + std::to_string(count) + " sounding edits (" + std::to_string(ids.size()) + " line" +
                 (ids.size() == 1 ? "" : "s") + ")";
    spec.kind = "edit";
    spec.owner = "project";
    spec.workingDirectory = jobDirectory;
    spec.logPath = platform::joinPath(jobDirectory, "job.log");
    spec.retryable = false;
    spec.parameters["edits"] = static_cast<std::int64_t>(count);
    spec.parameters["lines"] = static_cast<std::int64_t>(ids.size());
    resources("edit", ids, std::string(), {}, false, spec.reads, spec.writes);

    JobRecord entry;
    entry.sequence = sequence;
    entry.kind = spec.kind;
    entry.title = spec.title;
    entry.state = "queued";
    entry.inputs = spec.inputs;
    entry.outputs = spec.outputs;
    entry.parameters = spec.parameters;
    entry.logPath = spec.logPath;
    entry.workingDirectory = jobDirectory;
    entry.mbsystemVersion = mbsystemVersion_;
    entry.lines = ids;
    ProcessingRequest request;
    request.kind = "edit";
    request.lines = ids;
    entry.request = compact(requestToJson(request));
    jobId = jobs_->submit(spec, &error);
    if (jobId == 0)
      return 0;
    nextJob_++;
    entry.jobId = jobId;
    history_.push_back(entry);
    pending_[jobId] = sequence;
    std::string ignored;
    saveLocked(ignored);
  }
  changed("history");
  return jobId;
}

NextAction ProjectService::nextAction() const {
  std::lock_guard<std::mutex> lock(mutex_);
  NextAction next;
  if (!open_)
    return next;
  if (lines_.empty()) {
    next.kind = "import";
    next.reason = "Add the survey lines of the project.";
    return next;
  }
  int raw = 0, unprocessedChanges = 0;
  for (const auto &line : lines_) {
    if (line.missing)
      continue;
    if (line.processingPath.empty() && processing_->recommendPreprocess(line.format))
      raw++;
    const std::string st = line.processingStatus();
    if (st == "edits-pending" || st == "corrections-pending" || st == "processing-required")
      unprocessedChanges++;
  }
  if (raw > 0) {
    next.kind = "preprocess";
    next.count = raw;
    next.reason = std::to_string(raw) + " survey line(s) are in a raw vendor format; preprocess them first.";
    return next;
  }
  if (unprocessedChanges > 0) {
    next.kind = "process";
    next.count = unprocessedChanges;
    next.reason = std::to_string(unprocessedChanges) + " survey line(s) have edits or corrections that are not applied yet.";
    return next;
  }
  int outdated = 0, grids = 0;
  for (auto a : assets_) {
    if (a.parent.empty() && (a.type == "grid" || a.type == "mosaic"))
      grids++;
    deriveStatusLocked(a);
    if (a.parent.empty() && a.status == "outdated")
      outdated++;
  }
  if (outdated > 0) {
    next.kind = "regenerate";
    next.count = outdated;
    next.reason = std::to_string(outdated) + " product(s) were made from data that has changed since.";
    return next;
  }
  if (grids == 0) {
    next.kind = "grid";
    next.reason = "Make a terrain model of the survey.";
    return next;
  }
  return next;
}

void ProjectService::release(std::int64_t sequence) { reserved_.erase(sequence); }

void ProjectService::syncJob(std::int64_t jobId) {
  JobInfo info;
  if (!jobs_ || !jobs_->job(jobId, info))
    return;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto p = pending_.find(jobId);
    if (p == pending_.end())
      return;
    for (auto &r : history_) {
      if (r.sequence != p->second)
        continue;
      r.state = jobStateName(info.state);
      r.started = info.startedAt;
      r.finished = info.finishedAt;
      r.exitCode = info.exitCode;
      r.error = info.error;
    }
    if (info.state == JobState::Completed || info.state == JobState::Failed || info.state == JobState::Cancelled) {
      release(p->second);
      pending_.erase(p);
    }
    std::string ignored;
    saveLocked(ignored);
  }
  changed("history");
}

bool ProjectService::waitForJobs(int timeoutMs) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
  for (;;) {
    std::vector<std::int64_t> ids;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (const auto &kv : pending_)
        ids.push_back(kv.first);
    }
    if (ids.empty())
      return true;
    for (const auto id : ids) {
      const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
      if (left.count() <= 0 || !jobs_->wait(id, static_cast<int>(left.count())))
        return false;
      syncJob(id);
    }
  }
}

}  // namespace mbstudio::project
