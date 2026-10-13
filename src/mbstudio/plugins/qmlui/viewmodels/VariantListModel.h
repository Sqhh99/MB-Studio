// MB-Studio QML UI - a list model of QVariantMaps whose keys are the roles.
// Used for the project lists (survey lines, products, history).
#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

namespace mbstudio::ui {

class VariantListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)

 public:
  // keys: the role names; idKey identifies an item across updates
  VariantListModel(QStringList keys, QString idKey, QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  int count() const { return static_cast<int>(items_.size()); }
  // Replace the items; rows whose ids are unchanged are updated in place.
  void setItems(const QVector<QVariantMap> &items);
  const QVector<QVariantMap> &items() const { return items_; }
  Q_INVOKABLE QVariantMap get(int row) const;
  Q_INVOKABLE int indexOf(const QString &id) const;

 signals:
  void countChanged();

 private:
  QStringList keys_;
  QString idKey_;
  QVector<QVariantMap> items_;
};

}  // namespace mbstudio::ui
