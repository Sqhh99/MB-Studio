#include "WorkspaceModel.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

#include "mbstudio/kernel/Kernel.h"

namespace mbstudio::ui {

WorkspaceModel::WorkspaceModel(Kernel *kernel, QObject *parent) : QAbstractListModel(parent), kernel_(kernel) {
  reload();
}

void WorkspaceModel::reload() {
  registry_.clear();
  if (kernel_ != nullptr) {
    for (const auto &w : kernel_->contributions().workspaces()) {
      Info info;
      info.id = QString::fromStdString(w.id);
      // titles of contributions are translated in the "Contribution" context
      info.title = QCoreApplication::translate("Contribution", w.title.c_str());
      info.icon = QString::fromStdString(w.icon);
      info.category = QString::fromStdString(w.category);
      info.url = QString::fromStdString(w.qmlUrl);
      info.singleton = w.singleton;
      registry_.push_back(info);
    }
  }
  // titles of open tabs follow the language
  for (int i = 0; i < tabs_.size(); i++)
    if (const Info *info = find(tabs_[i].workspace); info != nullptr && tabs_[i].argument.isEmpty())
      tabs_[i].title = info->title;
  if (!tabs_.isEmpty())
    emit dataChanged(index(0), index(static_cast<int>(tabs_.size()) - 1), {TitleRole});
  emit availableChanged();
}

const WorkspaceModel::Info *WorkspaceModel::find(const QString &id) const {
  for (const auto &i : registry_)
    if (i.id == id)
      return &i;
  return nullptr;
}

int WorkspaceModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : count(); }

QVariant WorkspaceModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= tabs_.size())
    return {};
  const Tab &t = tabs_[index.row()];
  const Info *info = find(t.workspace);
  switch (role) {
    case UidRole: return t.uid;
    case WorkspaceRole: return t.workspace;
    case TitleRole: return t.title;
    case IconRole: return info != nullptr ? info->icon : QString();
    case UrlRole: return info != nullptr ? info->url : QString();
    case ArgumentRole: return t.argument;
    case ClosableRole: return t.workspace != QLatin1String("map") && t.workspace != QLatin1String("start");
    default: return {};
  }
}

QHash<int, QByteArray> WorkspaceModel::roleNames() const {
  return {{UidRole, "uid"},   {WorkspaceRole, "workspace"}, {TitleRole, "title"},      {IconRole, "icon"},
          {UrlRole, "url"},   {ArgumentRole, "argument"},   {ClosableRole, "closable"}};
}

void WorkspaceModel::setCurrentIndex(int index) {
  if (index < 0 || index >= tabs_.size() || index == current_)
    return;
  current_ = index;
  emit currentChanged();
}

QString WorkspaceModel::currentWorkspace() const {
  return current_ >= 0 && current_ < tabs_.size() ? tabs_[current_].workspace : QString();
}

QVariantList WorkspaceModel::available() const {
  QVariantList out;
  for (const auto &i : registry_)
    out.push_back(QVariantMap{{QStringLiteral("id"), i.id},
                              {QStringLiteral("title"), i.title},
                              {QStringLiteral("icon"), i.icon},
                              {QStringLiteral("category"), i.category},
                              {QStringLiteral("url"), i.url},
                              {QStringLiteral("singleton"), i.singleton}});
  return out;
}

QVariantMap WorkspaceModel::workspaceInfo(const QString &workspace) const {
  const Info *i = find(workspace);
  if (i == nullptr)
    return {};
  return {{QStringLiteral("id"), i->id},   {QStringLiteral("title"), i->title}, {QStringLiteral("icon"), i->icon},
          {QStringLiteral("url"), i->url}, {QStringLiteral("singleton"), i->singleton}};
}

int WorkspaceModel::indexOf(const QString &workspace) const {
  for (int i = 0; i < tabs_.size(); i++)
    if (tabs_[i].workspace == workspace)
      return i;
  return -1;
}

QString WorkspaceModel::argument(int index) const {
  return index >= 0 && index < tabs_.size() ? tabs_[index].argument : QString();
}

int WorkspaceModel::open(const QString &workspace, const QString &argument, const QString &title) {
  const Info *info = find(workspace);
  if (info == nullptr)
    return -1;
  const QString tabTitle = title.isEmpty() ? info->title : title;
  int index = -1;
  if (info->singleton) {
    index = indexOf(workspace);
  } else {
    for (int i = 0; i < tabs_.size(); i++)
      if (tabs_[i].workspace == workspace && tabs_[i].argument == argument)
        index = i;
  }
  if (index >= 0) {
    Tab &t = tabs_[index];
    if (t.argument != argument || t.title != tabTitle) {
      t.argument = argument;
      t.title = tabTitle;
      emit dataChanged(this->index(index), this->index(index));
    }
  } else {
    index = static_cast<int>(tabs_.size());
    beginInsertRows(QModelIndex(), index, index);
    tabs_.push_back(Tab{nextUid_++, workspace, tabTitle, argument});
    endInsertRows();
    emit countChanged();
  }
  if (current_ != index) {
    current_ = index;
    emit currentChanged();
  }
  return index;
}

void WorkspaceModel::close(int index) {
  if (index < 0 || index >= tabs_.size() || !data(this->index(index), ClosableRole).toBool())
    return;
  beginRemoveRows(QModelIndex(), index, index);
  tabs_.removeAt(index);
  endRemoveRows();
  emit countChanged();
  if (current_ > index) {
    current_--;
    emit currentChanged();
  } else if (current_ == index) {
    // activate the tab before the closed one
    current_ = tabs_.isEmpty() ? -1 : std::clamp(index - 1, 0, static_cast<int>(tabs_.size()) - 1);
    emit currentChanged();
  }
}

QString WorkspaceModel::saveState() const {
  QJsonArray tabs;
  for (const auto &t : tabs_)
    tabs.append(QJsonObject{{QStringLiteral("workspace"), t.workspace},
                            {QStringLiteral("argument"), t.argument},
                            {QStringLiteral("title"), t.title}});
  return QString::fromUtf8(
      QJsonDocument(QJsonObject{{QStringLiteral("tabs"), tabs}, {QStringLiteral("current"), current_}})
          .toJson(QJsonDocument::Compact));
}

void WorkspaceModel::restoreState(const QString &json) {
  const QJsonObject o = QJsonDocument::fromJson(json.toUtf8()).object();
  const QJsonArray tabs = o.value(QStringLiteral("tabs")).toArray();
  if (tabs.isEmpty())
    return;
  for (const auto &item : tabs) {
    const QJsonObject t = item.toObject();
    const QString workspace = t.value(QStringLiteral("workspace")).toString();
    const Info *info = find(workspace);
    if (info == nullptr)
      continue;
    const QString argument = t.value(QStringLiteral("argument")).toString();
    open(workspace, argument, argument.isEmpty() ? QString() : t.value(QStringLiteral("title")).toString());
  }
  const int current = o.value(QStringLiteral("current")).toInt(-1);
  if (current >= 0 && current < tabs_.size())
    setCurrentIndex(current);
}

}  // namespace mbstudio::ui
