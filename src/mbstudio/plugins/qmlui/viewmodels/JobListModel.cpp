#include "JobListModel.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QPointer>
#include <algorithm>

#include "mbstudio/kernel/Kernel.h"

namespace mbstudio::ui {

namespace {

bool isFinished(JobState s) { return s == JobState::Completed || s == JobState::Failed || s == JobState::Cancelled; }

QStringList toList(const std::vector<std::string> &items) {
  QStringList out;
  for (const auto &i : items)
    out << QString::fromStdString(i);
  return out;
}

}  // namespace

JobListModel::JobListModel(Kernel *kernel, std::shared_ptr<IJobService> jobs, QObject *parent)
    : QAbstractListModel(parent), kernel_(kernel), service_(std::move(jobs)) {
  refreshTimer_.setSingleShot(true);
  refreshTimer_.setInterval(80);  // coalesce the output events of busy programs
  connect(&refreshTimer_, &QTimer::timeout, this, &JobListModel::refresh);
  clockTimer_.setInterval(1000);  // elapsed time of running jobs
  connect(&clockTimer_, &QTimer::timeout, this, [this]() {
    if (activeCount() == 0) {
      clockTimer_.stop();
      return;
    }
    if (!jobs_.isEmpty())
      emit dataChanged(index(0), index(static_cast<int>(jobs_.size()) - 1), {ElapsedRole});
  });
  if (kernel_ != nullptr && service_) {
    // job events come from worker threads: refresh on the GUI thread
    QPointer<JobListModel> self(this);
    subscription_ = kernel_->events().subscribe("job.*", [self](const Event &) {
      if (self)
        QMetaObject::invokeMethod(
            self.data(),
            [self]() {
              if (self)
                self->scheduleRefresh();
            },
            Qt::QueuedConnection);
    });
  }
  refresh();
}

JobListModel::~JobListModel() {
  if (kernel_ != nullptr && subscription_ != 0)
    kernel_->events().unsubscribe(subscription_);
}

void JobListModel::scheduleRefresh() {
  if (!refreshTimer_.isActive())
    refreshTimer_.start();
}

void JobListModel::refresh() {
  if (!service_)
    return;
  std::vector<JobInfo> list = service_->jobs();
  std::sort(list.begin(), list.end(), [](const JobInfo &a, const JobInfo &b) { return a.id > b.id; });
  // report jobs that have just finished
  QVector<JobInfo> finishedNow;
  for (const auto &j : list) {
    if (!isFinished(j.state))
      continue;
    const auto old = std::find_if(jobs_.begin(), jobs_.end(), [&j](const JobInfo &o) { return o.id == j.id; });
    if (old == jobs_.end() || !isFinished(old->state))
      finishedNow.push_back(j);
  }
  const bool sameRows = static_cast<int>(list.size()) == jobs_.size() &&
                        std::equal(list.begin(), list.end(), jobs_.begin(),
                                   [](const JobInfo &a, const JobInfo &b) { return a.id == b.id; });
  if (sameRows) {
    for (int i = 0; i < jobs_.size(); i++)
      jobs_[i] = list[static_cast<std::size_t>(i)];
    if (!jobs_.isEmpty())
      emit dataChanged(index(0), index(static_cast<int>(jobs_.size()) - 1));
  } else {
    beginResetModel();
    jobs_ = QVector<JobInfo>(list.begin(), list.end());
    endResetModel();
  }
  if (activeCount() > 0 && !clockTimer_.isActive())
    clockTimer_.start();
  emit countChanged();
  for (const auto &j : finishedNow)
    emit jobFinished(j.id, QString::fromStdString(j.kind), QString::fromUtf8(jobStateName(j.state)),
                     QString::fromStdString(j.title), QString::fromStdString(j.error));
}

int JobListModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : count(); }

QVariant JobListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= jobs_.size())
    return {};
  const JobInfo &j = jobs_[index.row()];
  switch (role) {
    case IdRole: return static_cast<qint64>(j.id);
    case TitleRole: return QString::fromStdString(j.title);
    case KindRole: return QString::fromStdString(j.kind);
    case StateRole: return QString::fromUtf8(jobStateName(j.state));
    case StepRole: return j.currentStep + 1;
    case StepCountRole: return j.stepCount;
    case StepTitleRole: return QString::fromStdString(j.stepTitle);
    case LastLineRole: return QString::fromStdString(j.lastLine).trimmed();
    case ElapsedRole: {
      if (j.startedAt <= 0.0)
        return QString();
      const double end = j.finishedAt > 0.0 ? j.finishedAt : QDateTime::currentMSecsSinceEpoch() / 1000.0;
      return formatElapsed(end - j.startedAt);
    }
    case ErrorRole: return QString::fromStdString(j.error);
    case CommandsRole: return toList(j.commandLines).join(QLatin1Char('\n'));
    case LogPathRole: return QString::fromStdString(j.logPath);
    case OutputsRole: return toList(j.outputs);
    case WorkingDirectoryRole: return QString::fromStdString(j.workingDirectory);
    case RetryOfRole: return static_cast<qint64>(j.retryOf);
    default: return {};
  }
}

QHash<int, QByteArray> JobListModel::roleNames() const {
  return {{IdRole, "jobId"},         {TitleRole, "title"},     {KindRole, "kind"},
          {StateRole, "state"},      {StepRole, "step"},       {StepCountRole, "stepCount"},
          {StepTitleRole, "stepTitle"}, {LastLineRole, "lastLine"}, {ElapsedRole, "elapsed"},
          {ErrorRole, "error"},      {CommandsRole, "commands"}, {LogPathRole, "logPath"},
          {OutputsRole, "outputs"},  {WorkingDirectoryRole, "workingDirectory"}, {RetryOfRole, "retryOf"}};
}

int JobListModel::activeCount() const {
  return static_cast<int>(std::count_if(jobs_.begin(), jobs_.end(), [](const JobInfo &j) { return !isFinished(j.state); }));
}

int JobListModel::failedCount() const {
  return static_cast<int>(
      std::count_if(jobs_.begin(), jobs_.end(), [](const JobInfo &j) { return j.state == JobState::Failed; }));
}

QString JobListModel::activity() const {
  for (const auto &j : jobs_)
    if (j.state == JobState::Running)
      return j.stepCount > 1 ? tr("%1 · step %2 of %3").arg(QString::fromStdString(j.title)).arg(j.currentStep + 1).arg(j.stepCount)
                             : QString::fromStdString(j.title);
  return QString();
}

QString JobListModel::formatElapsed(double seconds) {
  const qint64 s = static_cast<qint64>(seconds < 0 ? 0 : seconds);
  if (s < 60)
    return QStringLiteral("%1 s").arg(s);
  if (s < 3600)
    return QStringLiteral("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
  return QStringLiteral("%1:%2:%3").arg(s / 3600).arg((s / 60) % 60, 2, 10, QLatin1Char('0')).arg(s % 60, 2, 10, QLatin1Char('0'));
}

bool JobListModel::cancel(qint64 id) {
  if (!service_)
    return false;
  const bool ok = service_->cancel(id);
  scheduleRefresh();
  return ok;
}

qint64 JobListModel::retry(qint64 id) {
  if (!service_)
    return 0;
  const qint64 newId = service_->retry(id);
  scheduleRefresh();
  return newId;
}

QString JobListModel::log(qint64 id) const {
  return service_ ? QString::fromStdString(service_->log(id)) : QString();
}

QVariantMap JobListModel::job(qint64 id) const {
  QVariantMap out;
  JobInfo j;
  if (!service_ || !service_->job(id, j))
    return out;
  out[QStringLiteral("id")] = static_cast<qint64>(j.id);
  out[QStringLiteral("title")] = QString::fromStdString(j.title);
  out[QStringLiteral("kind")] = QString::fromStdString(j.kind);
  out[QStringLiteral("state")] = QString::fromUtf8(jobStateName(j.state));
  out[QStringLiteral("error")] = QString::fromStdString(j.error);
  out[QStringLiteral("commands")] = toList(j.commandLines).join(QLatin1Char('\n'));
  out[QStringLiteral("outputs")] = toList(j.outputs);
  out[QStringLiteral("logPath")] = QString::fromStdString(j.logPath);
  return out;
}

void JobListModel::clearFinished() {
  if (!service_)
    return;
  service_->clearFinished();
  refresh();
}

bool JobListModel::waitFor(qint64 id, int timeoutMs) {
  if (!service_)
    return false;
  QElapsedTimer timer;
  timer.start();
  for (;;) {
    JobInfo j;
    if (!service_->job(id, j) || isFinished(j.state))
      break;
    if (timer.elapsed() > timeoutMs)
      return false;
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
  }
  // deliver the queued events of the job's last steps
  QElapsedTimer settle;
  settle.start();
  while (settle.elapsed() < 300)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
  refresh();
  return true;
}

}  // namespace mbstudio::ui
