#include "SwathViewModel.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointF>
#include <QtConcurrent/QtConcurrentRun>

namespace mbstudio::ui {

SwathViewModel::SwathViewModel(std::shared_ptr<ISwathService> service, QObject *parent)
    : QObject(parent), service_(std::move(service)) {
  connect(&watcher_, &QFutureWatcher<SwathSummary>::finished, this, &SwathViewModel::onFinished);
}

SwathViewModel::~SwathViewModel() {
  cancel();
  watcher_.waitForFinished();
}

QString SwathViewModel::fileName() const { return QFileInfo(path_).fileName(); }

QVariantMap SwathViewModel::toVariant(const SwathSummary &s) {
  QVariantMap m;
  m[QStringLiteral("format")] = s.format;
  m[QStringLiteral("formatName")] = QString::fromStdString(s.formatName);
  m[QStringLiteral("records")] = static_cast<qlonglong>(s.records);
  m[QStringLiteral("maxBeams")] = s.maxBeams;
  m[QStringLiteral("beamsTotal")] = static_cast<qlonglong>(s.beamsTotal);
  m[QStringLiteral("beamsGood")] = static_cast<qlonglong>(s.beamsGood);
  m[QStringLiteral("beamsFlagged")] = static_cast<qlonglong>(s.beamsFlagged);
  m[QStringLiteral("goodPercent")] = s.beamsTotal > 0 ? 100.0 * s.beamsGood / s.beamsTotal : 0.0;
  m[QStringLiteral("timeStart")] = QString::fromStdString(s.timeStartIso);
  m[QStringLiteral("timeEnd")] = QString::fromStdString(s.timeEndIso);
  m[QStringLiteral("durationHours")] = (s.timeEnd - s.timeStart) / 3600.0;
  m[QStringLiteral("lonMin")] = s.lonMin;
  m[QStringLiteral("lonMax")] = s.lonMax;
  m[QStringLiteral("latMin")] = s.latMin;
  m[QStringLiteral("latMax")] = s.latMax;
  m[QStringLiteral("depthMin")] = s.depthMin;
  m[QStringLiteral("depthMax")] = s.depthMax;
  m[QStringLiteral("trackLengthKm")] = s.trackLengthKm;
  return m;
}

void SwathViewModel::inspect(const QString &path, int format) {
  if (!service_ || path.isEmpty())
    return;
  cancel();
  watcher_.waitForFinished();
  path_ = path;
  busy_ = true;
  error_.clear();
  emit stateChanged();
  cancel_ = std::make_shared<std::atomic<bool>>(false);
  auto service = service_;
  auto cancelFlag = cancel_;
  const std::string file = path.toStdString();
  watcher_.setFuture(QtConcurrent::run([service, cancelFlag, file, format]() {
    return service->inspect(file, format, 4000, cancelFlag.get());
  }));
}

void SwathViewModel::onFinished() {
  const SwathSummary s = watcher_.result();
  busy_ = false;
  loaded_ = s.ok;
  error_ = QString::fromStdString(s.error);
  summary_ = s.ok ? toVariant(s) : QVariantMap();
  track_.clear();
  profile_.clear();
  if (s.ok) {
    const double t0 = s.track.empty() ? 0.0 : s.track.front().time;
    for (const auto &p : s.track) {
      track_.push_back(QPointF(p.lon, p.lat));
      if (p.depth != 0.0)
        profile_.push_back(QPointF(p.time - t0, p.depth));
    }
  }
  emit stateChanged();
  emit finished(s.ok);
}

void SwathViewModel::cancel() {
  if (cancel_)
    cancel_->store(true);
}

void SwathViewModel::clear() {
  cancel();
  watcher_.waitForFinished();
  loaded_ = false;
  busy_ = false;
  error_.clear();
  path_.clear();
  summary_.clear();
  track_.clear();
  profile_.clear();
  emit stateChanged();
}

void SwathViewModel::setLine(const QString &lineId, const QVariantList &versions, const QString &version) {
  lineId_ = lineId;
  versions_ = versions;
  version_.clear();
  QString wanted = version;
  if (wanted.isEmpty())
    for (const auto &v : versions)  // the most processed file
      if (v.toMap().value(QStringLiteral("exists")).toBool())
        wanted = v.toMap().value(QStringLiteral("version")).toString();
  setVersion(wanted);
  emit versionChanged();
}

void SwathViewModel::setVersion(const QString &version) {
  for (const auto &v : versions_) {
    const QVariantMap m = v.toMap();
    if (m.value(QStringLiteral("version")).toString() != version)
      continue;
    if (version == version_ && loaded_)
      return;
    version_ = version;
    inspect(m.value(QStringLiteral("path")).toString(), m.value(QStringLiteral("format")).toInt());
    emit versionChanged();
    return;
  }
}

QString SwathViewModel::viewState() const {
  return version_.isEmpty() ? QString()
                            : QString::fromUtf8(QJsonDocument(QJsonObject{{QStringLiteral("version"), version_}})
                                                    .toJson(QJsonDocument::Compact));
}

void SwathViewModel::setViewState(const QString &json) {
  const QString version = QJsonDocument::fromJson(json.toUtf8()).object().value(QStringLiteral("version")).toString();
  if (!version.isEmpty())
    setVersion(version);
}

bool SwathViewModel::waitForFinished(int timeoutMs) {
  QElapsedTimer timer;
  timer.start();
  while (busy_ && timer.elapsed() < timeoutMs)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
  return !busy_;
}

}  // namespace mbstudio::ui
