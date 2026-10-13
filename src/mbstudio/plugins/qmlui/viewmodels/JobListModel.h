// MB-Studio QML UI - the background jobs of the job service (queue, state,
// current step, last output line, errors) with cancel, retry and the log.
#pragma once

#include <QAbstractListModel>
#include <QTimer>
#include <QVector>
#include <memory>

#include "mbstudio/api/JobService.h"
#include "mbstudio/kernel/EventBus.h"

namespace mbstudio {
class Kernel;
}

namespace mbstudio::ui {

class JobListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)
  Q_PROPERTY(int activeCount READ activeCount NOTIFY countChanged)
  Q_PROPERTY(int failedCount READ failedCount NOTIFY countChanged)
  Q_PROPERTY(bool available READ available CONSTANT)
  // short description of what runs ("Grid bathymetry (25 m) · step 1/2")
  Q_PROPERTY(QString activity READ activity NOTIFY countChanged)

 public:
  enum Roles {
    IdRole = Qt::UserRole + 1,
    TitleRole,
    KindRole,
    StateRole,
    StepRole,
    StepCountRole,
    StepTitleRole,
    LastLineRole,
    ElapsedRole,
    ErrorRole,
    CommandsRole,
    LogPathRole,
    OutputsRole,
    WorkingDirectoryRole,
    RetryOfRole
  };

  JobListModel(Kernel *kernel, std::shared_ptr<IJobService> jobs, QObject *parent = nullptr);
  ~JobListModel() override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  int count() const { return static_cast<int>(jobs_.size()); }
  int activeCount() const;
  int failedCount() const;
  bool available() const { return service_ != nullptr; }
  QString activity() const;

  Q_INVOKABLE bool cancel(qint64 id);
  Q_INVOKABLE qint64 retry(qint64 id);
  Q_INVOKABLE QString log(qint64 id) const;
  Q_INVOKABLE QVariantMap job(qint64 id) const;
  Q_INVOKABLE void clearFinished();
  Q_INVOKABLE void refresh();
  // Process events until the job has finished (tests).
  Q_INVOKABLE bool waitFor(qint64 id, int timeoutMs = 120000);

  static QString formatElapsed(double seconds);

 signals:
  void countChanged();
  // a job reached completed, failed or cancelled
  void jobFinished(qint64 id, const QString &kind, const QString &state, const QString &title, const QString &error);

 private:
  void scheduleRefresh();
  Kernel *kernel_;
  std::shared_ptr<IJobService> service_;
  QVector<JobInfo> jobs_;  // newest first
  QTimer refreshTimer_;
  QTimer clockTimer_;
  EventBus::SubscriptionId subscription_ = 0;
};

}  // namespace mbstudio::ui
