// MB-Studio QML UI - browsing and running the MB-System command line programs.
#pragma once

#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QVector>
#include <atomic>
#include <memory>

#include "mbstudio/api/ToolService.h"

namespace mbstudio::ui {

class ToolsViewModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
  Q_PROPERTY(int count READ count NOTIFY countChanged)
  Q_PROPERTY(int availableCount READ availableCount NOTIFY countChanged)
  Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
  Q_PROPERTY(QVariantMap current READ current NOTIFY currentChanged)
  Q_PROPERTY(QString arguments READ arguments WRITE setArguments NOTIFY argumentsChanged)
  Q_PROPERTY(QString workingDirectory READ workingDirectory WRITE setWorkingDirectory NOTIFY argumentsChanged)
  Q_PROPERTY(QString commandPreview READ commandPreview NOTIFY argumentsChanged)
  Q_PROPERTY(bool running READ running NOTIFY runningChanged)
  Q_PROPERTY(QString output READ output NOTIFY outputChanged)
  Q_PROPERTY(int exitCode READ exitCode NOTIFY runningChanged)
  Q_PROPERTY(QString lastError READ lastError NOTIFY runningChanged)

 public:
  enum Roles { NameRole = Qt::UserRole + 1, CategoryRole, SummaryRole, ExampleRole, AvailableRole, PathRole };

  explicit ToolsViewModel(std::shared_ptr<IToolService> service, QObject *parent = nullptr);
  ~ToolsViewModel() override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  QString filter() const { return filter_; }
  void setFilter(const QString &filter);
  int count() const { return static_cast<int>(visible_.size()); }
  int availableCount() const;
  int currentIndex() const { return current_; }
  void setCurrentIndex(int index);
  QVariantMap current() const;
  QString arguments() const { return arguments_; }
  void setArguments(const QString &arguments);
  QString workingDirectory() const { return workingDirectory_; }
  void setWorkingDirectory(const QString &directory);
  QString commandPreview() const;
  bool running() const { return running_; }
  QString output() const { return output_; }
  int exitCode() const { return exitCode_; }
  QString lastError() const { return lastError_; }

  Q_INVOKABLE bool selectTool(const QString &name);
  Q_INVOKABLE void useExample();
  Q_INVOKABLE void run();
  Q_INVOKABLE void showHelp();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void clearOutput();
  Q_INVOKABLE bool waitForFinished(int timeoutMs = 60000);

 signals:
  void filterChanged();
  void countChanged();
  void currentChanged();
  void argumentsChanged();
  void runningChanged();
  void outputChanged();
  void finished(int exitCode);

 private:
  void start(const QString &tool, const QStringList &arguments);
  void appendOutput(const QString &text);
  void rebuild();
  std::shared_ptr<IToolService> service_;
  QVector<ToolInfo> tools_;
  QVector<int> visible_;
  QString filter_;
  int current_ = -1;  // index into tools_
  QString arguments_;
  QString workingDirectory_;
  bool running_ = false;
  QString output_;
  int exitCode_ = 0;
  QString lastError_;
  QFutureWatcher<platform::ProcessResult> watcher_;
  std::shared_ptr<std::atomic<bool>> cancel_;
};

}  // namespace mbstudio::ui
