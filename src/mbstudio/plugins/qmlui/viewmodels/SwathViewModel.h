// MB-Studio QML UI - inspection of a swath sonar file (runs off the GUI
// thread). As a line document it knows the file versions of its survey
// line (original, preprocessed, processed) and which one it shows.
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
  // the survey line (empty for a file opened on its own) and its versions:
  // [{version, path, format, exists}]
  Q_PROPERTY(QString lineId READ lineId NOTIFY versionChanged)
  Q_PROPERTY(QVariantList versions READ versions NOTIFY versionChanged)
  Q_PROPERTY(QString version READ version WRITE setVersion NOTIFY versionChanged)
  Q_PROPERTY(QString viewState READ viewState WRITE setViewState NOTIFY versionChanged)

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
  QString lineId() const { return lineId_; }
  QVariantList versions() const { return versions_; }
  QString version() const { return version_; }
  // Show another version of the line (inspects its file).
  void setVersion(const QString &version);
  QString viewState() const;
  void setViewState(const QString &json);
  // A survey line document: its id, versions and the version to show
  // ("" picks the most processed one).
  void setLine(const QString &lineId, const QVariantList &versions, const QString &version);

  Q_INVOKABLE void inspect(const QString &path, int format = 0);
  Q_INVOKABLE void cancel();
  Q_INVOKABLE void clear();
  // Wait for a running inspection (tests).
  Q_INVOKABLE bool waitForFinished(int timeoutMs = 30000);

  static QVariantMap toVariant(const SwathSummary &s);

 signals:
  void stateChanged();
  void finished(bool ok);
  void versionChanged();

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
  QString lineId_;
  QVariantList versions_;
  QString version_;
};

}  // namespace mbstudio::ui
