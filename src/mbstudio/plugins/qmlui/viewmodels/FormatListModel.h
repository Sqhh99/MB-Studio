// MB-Studio QML UI - MB-System data formats with text filtering.
#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <memory>

#include "mbstudio/api/FormatService.h"

namespace mbstudio::ui {

class FormatListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
  Q_PROPERTY(int count READ count NOTIFY countChanged)
  Q_PROPERTY(int totalCount READ totalCount NOTIFY countChanged)

 public:
  enum Roles { IdRole = Qt::UserRole + 1, NameRole, DescriptionRole, AttributesRole };

  explicit FormatListModel(std::shared_ptr<IFormatService> service, QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  QString filter() const { return filter_; }
  void setFilter(const QString &filter);
  int count() const { return static_cast<int>(visible_.size()); }
  int totalCount() const { return static_cast<int>(all_.size()); }

  Q_INVOKABLE QVariantMap get(int row) const;
  // Format id implied by a file name, 0 if unknown.
  Q_INVOKABLE int identify(const QString &path) const;

 signals:
  void filterChanged();
  void countChanged();

 private:
  void rebuild();
  std::shared_ptr<IFormatService> service_;
  QVector<FormatInfo> all_;
  QVector<int> visible_;
  QString filter_;
};

}  // namespace mbstudio::ui
