#include "VariantListModel.h"

namespace mbstudio::ui {

VariantListModel::VariantListModel(QStringList keys, QString idKey, QObject *parent)
    : QAbstractListModel(parent), keys_(std::move(keys)), idKey_(std::move(idKey)) {}

int VariantListModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : count(); }

QVariant VariantListModel::data(const QModelIndex &index, int role) const {
  const int key = role - Qt::UserRole - 1;
  if (!index.isValid() || index.row() >= items_.size() || key < 0 || key >= keys_.size())
    return {};
  return items_[index.row()].value(keys_[key]);
}

QHash<int, QByteArray> VariantListModel::roleNames() const {
  QHash<int, QByteArray> roles;
  for (int i = 0; i < keys_.size(); i++)
    roles[Qt::UserRole + 1 + i] = keys_[i].toUtf8();
  return roles;
}

void VariantListModel::setItems(const QVector<QVariantMap> &items) {
  bool same = items.size() == items_.size();
  for (int i = 0; same && i < items.size(); i++)
    same = items[i].value(idKey_) == items_[i].value(idKey_);
  if (same) {
    for (int i = 0; i < items.size(); i++) {
      if (items[i] != items_[i]) {
        items_[i] = items[i];
        emit dataChanged(index(i), index(i));
      }
    }
    return;
  }
  beginResetModel();
  items_ = items;
  endResetModel();
  emit countChanged();
}

QVariantMap VariantListModel::get(int row) const {
  return row >= 0 && row < items_.size() ? items_[row] : QVariantMap();
}

int VariantListModel::indexOf(const QString &id) const {
  for (int i = 0; i < items_.size(); i++)
    if (items_[i].value(idKey_).toString() == id)
      return i;
  return -1;
}

}  // namespace mbstudio::ui
