// MB-Studio QML UI - kernel log and job events as a list model.
#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QVector>

#include "mbstudio/kernel/EventBus.h"

namespace mbstudio {
class Kernel;
}

namespace mbstudio::ui {

class LogModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)
  Q_PROPERTY(int warningCount READ warningCount NOTIFY countChanged)

 public:
  enum Roles { LevelRole = Qt::UserRole + 1, SourceRole, MessageRole, TimeRole };

  explicit LogModel(Kernel *kernel, QObject *parent = nullptr);
  ~LogModel() override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  int count() const { return static_cast<int>(entries_.size()); }
  int warningCount() const { return warnings_; }

  Q_INVOKABLE void append(const QString &level, const QString &source, const QString &message);
  Q_INVOKABLE void clear();
  Q_INVOKABLE QString text() const;  // whole log as plain text (copy to clipboard)

 signals:
  void countChanged();

 private:
  struct Entry {
    QString level, source, message;
    QDateTime time;
  };
  Kernel *kernel_;
  EventBus::SubscriptionId subscription_ = 0;
  QVector<Entry> entries_;
  int warnings_ = 0;
};

}  // namespace mbstudio::ui
