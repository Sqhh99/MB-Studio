#include "FormatListModel.h"

namespace mbstudio::ui {

FormatListModel::FormatListModel(std::shared_ptr<IFormatService> service, QObject *parent)
    : QAbstractListModel(parent), service_(std::move(service)) {
  if (service_)
    for (const auto &f : service_->formats())
      all_.push_back(f);
  rebuild();
}

int FormatListModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : count(); }

QVariant FormatListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= visible_.size())
    return {};
  const FormatInfo &f = all_[visible_[index.row()]];
  switch (role) {
    case IdRole: return f.id;
    case NameRole: return QString::fromStdString(f.name);
    case DescriptionRole: return QString::fromStdString(f.description);
    case AttributesRole: return QString::fromStdString(f.attributes);
    default: return {};
  }
}

QHash<int, QByteArray> FormatListModel::roleNames() const {
  return {{IdRole, "formatId"}, {NameRole, "name"}, {DescriptionRole, "description"}, {AttributesRole, "attributes"}};
}

void FormatListModel::setFilter(const QString &filter) {
  if (filter == filter_)
    return;
  filter_ = filter;
  rebuild();
  emit filterChanged();
}

void FormatListModel::rebuild() {
  beginResetModel();
  visible_.clear();
  const QString needle = filter_.trimmed();
  for (int i = 0; i < all_.size(); i++) {
    const FormatInfo &f = all_[i];
    if (needle.isEmpty() || QString::number(f.id) == needle ||
        QString::fromStdString(f.name).contains(needle, Qt::CaseInsensitive) ||
        QString::fromStdString(f.description).contains(needle, Qt::CaseInsensitive) ||
        QString::fromStdString(f.attributes).contains(needle, Qt::CaseInsensitive))
      visible_.push_back(i);
  }
  endResetModel();
  emit countChanged();
}

QVariantMap FormatListModel::get(int row) const {
  QVariantMap out;
  if (row < 0 || row >= visible_.size())
    return out;
  const FormatInfo &f = all_[visible_[row]];
  out[QStringLiteral("formatId")] = f.id;
  out[QStringLiteral("name")] = QString::fromStdString(f.name);
  out[QStringLiteral("description")] = QString::fromStdString(f.description);
  out[QStringLiteral("attributes")] = QString::fromStdString(f.attributes);
  return out;
}

int FormatListModel::identify(const QString &path) const {
  return service_ ? service_->identify(path.toStdString()) : 0;
}

}  // namespace mbstudio::ui
