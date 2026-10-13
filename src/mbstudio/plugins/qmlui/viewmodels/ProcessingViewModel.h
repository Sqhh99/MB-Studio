// MB-Studio QML UI - the processing panels (preprocess, clean, corrections,
// process, grid, mosaic, export). Each kind has a declarative parameter
// schema (fields with type, unit, range, options and visibility rules) that
// the QML panel renders generically; the mapping of the values to the typed
// recipe parameters, and from there to program options, is explicit C++.
// The panel shows the exact commands the job will run before it is started.
#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

#include "mbstudio/api/ProjectService.h"

namespace mbstudio::ui {

class ProjectViewModel;

class ProcessingViewModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString kind READ kind WRITE setKind NOTIFY kindChanged)
  Q_PROPERTY(QString title READ title NOTIFY kindChanged)
  Q_PROPERTY(QString description READ description NOTIFY kindChanged)
  Q_PROPERTY(QVariantList fields READ fields NOTIFY kindChanged)
  Q_PROPERTY(QStringList kinds READ kinds CONSTANT)
  // incremented on every parameter change (re-evaluates value bindings)
  Q_PROPERTY(int revision READ revision NOTIFY parametersChanged)
  Q_PROPERTY(QString preview READ preview NOTIFY previewChanged)
  Q_PROPERTY(QStringList outputs READ outputs NOTIFY previewChanged)
  Q_PROPERTY(QStringList warnings READ warnings NOTIFY previewChanged)
  Q_PROPERTY(QString error READ error NOTIFY previewChanged)
  Q_PROPERTY(QString estimate READ estimate NOTIFY previewChanged)
  Q_PROPERTY(bool ready READ ready NOTIFY previewChanged)

 public:
  ProcessingViewModel(std::shared_ptr<IProjectService> project, std::shared_ptr<IProcessingService> processing,
                      ProjectViewModel *projectModel, QObject *parent = nullptr);

  QString kind() const { return kind_; }
  void setKind(const QString &kind);
  QString title() const;
  QString description() const;
  QVariantList fields() const;
  static QStringList kinds();
  int revision() const { return revision_; }
  QString preview() const { return preview_; }
  QStringList outputs() const { return outputs_; }
  QStringList warnings() const { return warnings_; }
  QString error() const { return error_; }
  QString estimate() const { return estimate_; }
  bool ready() const { return error_.isEmpty() && !preview_.isEmpty(); }

  Q_INVOKABLE QVariant value(const QString &key) const;
  Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
  Q_INVOKABLE bool fieldVisible(const QString &key) const;
  // options of an enum field whose choices depend on the project (export source)
  Q_INVOKABLE QVariantList options(const QString &key) const;
  Q_INVOKABLE void resetDefaults();
  // Start the job; returns its id (0 on error, see error / notify).
  Q_INVOKABLE qint64 run();
  Q_INVOKABLE void updatePreview();

  // The typed request for the current values (exposed for tests).
  ProcessingRequest request() const;

 signals:
  void kindChanged();
  void parametersChanged();
  void previewChanged();
  void started(qint64 jobId, const QString &title);
  void notify(const QString &message);

 private:
  QVariantMap defaults(const QString &kind) const;
  QVariantList schema(const QString &kind) const;
  std::shared_ptr<IProjectService> project_;
  std::shared_ptr<IProcessingService> processing_;
  ProjectViewModel *projectModel_;
  QString kind_ = QStringLiteral("grid");
  QMap<QString, QVariantMap> values_;  // per kind, kept while the application runs
  int revision_ = 0;
  QString preview_;
  QStringList outputs_;
  QStringList warnings_;
  QString error_;
  QString estimate_;
  QTimer previewTimer_;
};

}  // namespace mbstudio::ui
