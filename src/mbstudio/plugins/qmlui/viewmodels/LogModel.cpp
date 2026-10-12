#include "LogModel.h"

#include <QPointer>

#include "mbstudio/kernel/Kernel.h"

namespace mbstudio::ui {

namespace {
constexpr int kMaxEntries = 5000;
}

LogModel::LogModel(Kernel *kernel, QObject *parent) : QAbstractListModel(parent), kernel_(kernel) {
  if (kernel_ == nullptr)
    return;
  for (const auto &e : kernel_->logHistory())
    append(QString::fromUtf8(logLevelName(e.level)), QString::fromStdString(e.source),
           QString::fromStdString(e.message));
  // Kernel events may be published from worker threads: queue them to ours.
  QPointer<LogModel> self(this);
  subscription_ = kernel_->events().subscribe("log.*", [self](const Event &event) {
    const QString level = QString::fromStdString(event.topic.substr(4));
    const QString source = QString::fromStdString(getString(event.data, "source"));
    const QString message = QString::fromStdString(getString(event.data, "message"));
    if (self)
      QMetaObject::invokeMethod(
          self.data(),
          [self, level, source, message]() {
            if (self)
              self->append(level, source, message);
          },
          Qt::QueuedConnection);
  });
}

LogModel::~LogModel() {
  if (kernel_ != nullptr && subscription_ != 0)
    kernel_->events().unsubscribe(subscription_);
}

int LogModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : count(); }

QVariant LogModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= entries_.size())
    return {};
  const Entry &e = entries_[index.row()];
  switch (role) {
    case LevelRole: return e.level;
    case SourceRole: return e.source;
    case MessageRole: return e.message;
    case TimeRole: return e.time.toString(QStringLiteral("HH:mm:ss"));
    default: return {};
  }
}

QHash<int, QByteArray> LogModel::roleNames() const {
  return {{LevelRole, "level"}, {SourceRole, "source"}, {MessageRole, "message"}, {TimeRole, "time"}};
}

void LogModel::append(const QString &level, const QString &source, const QString &message) {
  if (entries_.size() >= kMaxEntries) {
    beginRemoveRows(QModelIndex(), 0, 0);
    if (entries_.front().level == QLatin1String("warning") || entries_.front().level == QLatin1String("error"))
      warnings_--;
    entries_.removeFirst();
    endRemoveRows();
  }
  beginInsertRows(QModelIndex(), count(), count());
  entries_.push_back(Entry{level, source, message, QDateTime::currentDateTime()});
  if (level == QLatin1String("warning") || level == QLatin1String("error"))
    warnings_++;
  endInsertRows();
  emit countChanged();
}

void LogModel::clear() {
  beginResetModel();
  entries_.clear();
  warnings_ = 0;
  endResetModel();
  emit countChanged();
}

QString LogModel::text() const {
  QString out;
  for (const auto &e : entries_)
    out += QStringLiteral("%1 [%2] %3: %4\n")
               .arg(e.time.toString(QStringLiteral("HH:mm:ss")), e.level, e.source, e.message);
  return out;
}

}  // namespace mbstudio::ui
