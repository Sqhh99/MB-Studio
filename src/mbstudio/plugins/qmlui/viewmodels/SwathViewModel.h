// MB-Studio QML UI - inspection of a swath sonar file (runs off the GUI thread).
#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <atomic>
#include <memory>

#include "mbstudio/api/SwathService.h"

namespace mbstudio::ui {

class SwathViewModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
  Q_PROPERTY(bool loaded READ loaded NOTIFY stateChanged)
  Q_PROPERTY(QString error READ error NOTIFY stateChanged)
  Q_PROPERTY(QString path READ path NOTIFY stateChanged)
  Q_PROPERTY(QString fileName READ fileName NOTIFY stateChanged)
  Q_PROPERTY(QVariantMap summary READ summary NOTIFY stateChanged)
  Q_PROPERTY(QVariantList track READ track NOTIFY stateChanged)
  Q_PROPERTY(QVariantList profile READ profile NOTIFY stateChanged)

 public:
  explicit SwathViewModel(std::shared_ptr<ISwathService> service, QObject *parent = nullptr);
  ~SwathViewModel() override;

  bool busy() const { return busy_; }
  bool loaded() const { return loaded_; }
  QString error() const { return error_; }
  QString path() const { return path_; }
  QString fileName() const;
  QVariantMap summary() const { return summary_; }
  QVariantList track() const { return track_; }      // [{x: lon, y: lat}, ...]
  QVariantList profile() const { return profile_; }  // [{x: seconds, y: depth}, ...]

  Q_INVOKABLE void inspect(const QString &path, int format = 0);
  Q_INVOKABLE void cancel();
  Q_INVOKABLE void clear();
  // Wait for a running inspection (tests).
  Q_INVOKABLE bool waitForFinished(int timeoutMs = 30000);

  static QVariantMap toVariant(const SwathSummary &s);

 signals:
  void stateChanged();
  void finished(bool ok);

 private:
  void onFinished();
  std::shared_ptr<ISwathService> service_;
  QFutureWatcher<SwathSummary> watcher_;
  std::shared_ptr<std::atomic<bool>> cancel_;
  bool busy_ = false;
  bool loaded_ = false;
  QString error_;
  QString path_;
  QVariantMap summary_;
  QVariantList track_;
  QVariantList profile_;
};

}  // namespace mbstudio::ui
