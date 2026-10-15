#include "WorkspaceModel.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaProperty>
#include <algorithm>

#include "mbstudio/kernel/Kernel.h"

namespace mbstudio::ui {

WorkspaceModel::WorkspaceModel(Kernel *kernel, QObject *parent) : QAbstractListModel(parent), kernel_(kernel) {
  reload();
}

WorkspaceModel::~WorkspaceModel() {
  for (auto &t : tabs_)
    delete t.document.data();
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

int WorkspaceModel::indexOfUid(int uid) const {
  if (uid == 0)
    return -1;
  for (int i = 0; i < tabs_.size(); i++)
    if (tabs_[i].uid == uid)
      return i;
  return -1;
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
    case DocumentRole: return QVariant::fromValue(t.document.data());
    case GroupRole: return t.group;
    case ShownRole: return shown_[t.group == 1 ? 1 : 0] == t.uid;
    default: return {};
  }
}

QHash<int, QByteArray> WorkspaceModel::roleNames() const {
  return {{UidRole, "uid"},           {WorkspaceRole, "workspace"}, {TitleRole, "title"},
          {IconRole, "icon"},         {UrlRole, "url"},             {ArgumentRole, "argument"},
          {ClosableRole, "closable"}, {DocumentRole, "document"},   {GroupRole, "group"},
          {ShownRole, "shown"}};
}

int WorkspaceModel::currentIndex() const { return indexOfUid(currentUid_); }

void WorkspaceModel::activate(int index) {
  if (index < 0 || index >= tabs_.size())
    return;
  const Tab &t = tabs_[index];
  const bool changedCurrent = currentUid_ != t.uid;
  const bool changedShown = shown_[t.group] != t.uid;
  currentUid_ = t.uid;
  shown_[t.group] = t.uid;
  if (changedShown) {
    emit dataChanged(this->index(0), this->index(static_cast<int>(tabs_.size()) - 1), {ShownRole});
    emit layoutChanged();
  }
  if (changedCurrent)
    emit currentChanged();
}

void WorkspaceModel::setCurrentIndex(int index) { activate(index); }

QString WorkspaceModel::currentWorkspace() const {
  const int i = currentIndex();
  return i >= 0 ? tabs_[i].workspace : QString();
}

QObject *WorkspaceModel::currentDocument() const {
  const int i = currentIndex();
  return i >= 0 ? tabs_[i].document.data() : nullptr;
}

QString WorkspaceModel::currentArgument() const {
  const int i = currentIndex();
  return i >= 0 ? tabs_[i].argument : QString();
}

bool WorkspaceModel::split() const {
  return std::any_of(tabs_.begin(), tabs_.end(), [](const Tab &t) { return t.group == 1; });
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

int WorkspaceModel::indexOf(const QString &workspace, const QString &argument) const {
  for (int i = 0; i < tabs_.size(); i++)
    if (tabs_[i].workspace == workspace && (argument.isEmpty() || tabs_[i].argument == argument))
      return i;
  return -1;
}

QString WorkspaceModel::argument(int index) const {
  return index >= 0 && index < tabs_.size() ? tabs_[index].argument : QString();
}

QObject *WorkspaceModel::document(int index) const {
  return index >= 0 && index < tabs_.size() ? tabs_[index].document.data() : nullptr;
}

int WorkspaceModel::group(int index) const { return index >= 0 && index < tabs_.size() ? tabs_[index].group : 0; }

void WorkspaceModel::setTitle(int index, const QString &title) {
  if (index < 0 || index >= tabs_.size() || tabs_[index].title == title)
    return;
  tabs_[index].title = title;
  emit dataChanged(this->index(index), this->index(index), {TitleRole});
}

QList<QObject *> WorkspaceModel::documents() const {
  QList<QObject *> out;
  for (const auto &t : tabs_)
    if (t.document)
      out << t.document.data();
  return out;
}

int WorkspaceModel::open(const QString &workspace, const QString &argument, const QString &title, int group) {
  const Info *info = find(workspace);
  if (info == nullptr)
    return -1;
  const QString tabTitle = title.isEmpty() ? info->title : title;
  int index = -1;
  for (int i = 0; i < tabs_.size(); i++)
    if (tabs_[i].workspace == workspace && (info->singleton || tabs_[i].argument == argument))
      index = i;
  if (index >= 0) {
    Tab &t = tabs_[index];
    if (info->singleton && t.argument != argument) {
      // a singleton shows another document: a fresh document object
      t.argument = argument;
      delete t.document.data();
      t.document = factory_ ? factory_(workspace, argument, this) : nullptr;
    }
    if (t.title != tabTitle) {
      t.title = tabTitle;
    }
    emit dataChanged(this->index(index), this->index(index));
  } else {
    const int current = currentIndex();
    int g = group >= 0 ? std::min(group, 1) : current >= 0 ? tabs_[current].group : 0;
    index = static_cast<int>(tabs_.size());
    Tab t{nextUid_++, workspace, tabTitle, argument, g, nullptr};
    if (factory_)
      t.document = factory_(workspace, argument, this);
    if (t.document) {
      // a changed camera, colour table or version is saved with the project
      const QMetaObject *meta = t.document->metaObject();
      const int property = meta->indexOfProperty("viewState");
      if (property >= 0 && meta->property(property).hasNotifySignal())
        connect(t.document.data(), meta->property(property).notifySignal(), this,
                metaObject()->method(metaObject()->indexOfSignal("stateChanged()")));
    }
    beginInsertRows(QModelIndex(), index, index);
    tabs_.push_back(t);
    endInsertRows();
    emit countChanged();
    if (g == 1)
      emit layoutChanged();
  }
  activate(index);
  return index;
}

void WorkspaceModel::normaliseGroups() {
  // the left group is never empty while the right one has tabs
  const bool left = std::any_of(tabs_.begin(), tabs_.end(), [](const Tab &t) { return t.group == 0; });
  if (!left)
    for (auto &t : tabs_)
      t.group = 0;
  for (int g = 0; g < 2; g++) {
    if (indexOfUid(shown_[g]) >= 0 && tabs_[indexOfUid(shown_[g])].group == g)
      continue;
    shown_[g] = 0;
    for (const auto &t : tabs_)
      if (t.group == g)
        shown_[g] = t.uid;
  }
}

void WorkspaceModel::close(int index) {
  if (index < 0 || index >= tabs_.size() || !data(this->index(index), ClosableRole).toBool())
    return;
  const int uid = tabs_[index].uid;
  const int g = tabs_[index].group;
  QObject *doc = tabs_[index].document.data();
  beginRemoveRows(QModelIndex(), index, index);
  tabs_.removeAt(index);
  endRemoveRows();
  if (doc != nullptr)
    doc->deleteLater();
  emit countChanged();
  if (shown_[g] == uid) {
    // show the tab before the closed one in the same group
    shown_[g] = 0;
    for (int i = std::min(index, static_cast<int>(tabs_.size())) - 1; i >= 0 && shown_[g] == 0; i--)
      if (tabs_[i].group == g)
        shown_[g] = tabs_[i].uid;
    for (int i = 0; i < tabs_.size() && shown_[g] == 0; i++)
      if (tabs_[i].group == g)
        shown_[g] = tabs_[i].uid;
  }
  normaliseGroups();
  if (currentUid_ == uid || indexOfUid(currentUid_) < 0)
    currentUid_ = shown_[g] != 0 ? shown_[g] : shown_[0];
  emit dataChanged(this->index(0), this->index(std::max(0, static_cast<int>(tabs_.size()) - 1)), {ShownRole, GroupRole});
  emit layoutChanged();
  emit currentChanged();
}

void WorkspaceModel::moveToGroup(int index, int group) {
  if (index < 0 || index >= tabs_.size())
    return;
  group = group == 1 ? 1 : 0;
  Tab &t = tabs_[index];
  if (t.group == group)
    return;
  // the left group keeps at least one tab
  if (group == 1 && std::count_if(tabs_.begin(), tabs_.end(), [](const Tab &x) { return x.group == 0; }) <= 1)
    return;
  t.group = group;
  shown_[group] = t.uid;
  normaliseGroups();
  emit dataChanged(this->index(0), this->index(static_cast<int>(tabs_.size()) - 1), {GroupRole, ShownRole});
  emit layoutChanged();
  activate(index);
}

void WorkspaceModel::unsplit() {
  if (!split())
    return;
  for (auto &t : tabs_)
    t.group = 0;
  normaliseGroups();
  shown_[0] = currentUid_ != 0 ? currentUid_ : shown_[0];
  emit dataChanged(this->index(0), this->index(static_cast<int>(tabs_.size()) - 1), {GroupRole, ShownRole});
  emit layoutChanged();
}

QString WorkspaceModel::saveState() const {
  QJsonArray tabs;
  for (const auto &t : tabs_) {
    QJsonObject o{{QStringLiteral("workspace"), t.workspace},
                  {QStringLiteral("argument"), t.argument},
                  {QStringLiteral("title"), t.title},
                  {QStringLiteral("group"), t.group}};
    if (t.document) {
      const QVariant state = t.document->property("viewState");
      if (state.isValid() && !state.toString().isEmpty())
        o[QStringLiteral("state")] = state.toString();
    }
    tabs.append(o);
  }
  return QString::fromUtf8(QJsonDocument(QJsonObject{{QStringLiteral("tabs"), tabs},
                                                     {QStringLiteral("current"), currentIndex()},
                                                     {QStringLiteral("primary"), primaryIndex()},
                                                     {QStringLiteral("secondary"), secondaryIndex()}})
                               .toJson(QJsonDocument::Compact));
}

void WorkspaceModel::restoreState(const QString &json) {
  const QJsonObject o = QJsonDocument::fromJson(json.toUtf8()).object();
  const QJsonArray tabs = o.value(QStringLiteral("tabs")).toArray();
  if (tabs.isEmpty())
    return;
  QVector<int> restored;  // tab index of each saved tab (-1: not restored)
  for (const auto &item : tabs) {
    const QJsonObject t = item.toObject();
    const QString workspace = t.value(QStringLiteral("workspace")).toString();
    if (find(workspace) == nullptr) {
      restored << -1;
      continue;
    }
    const QString argument = t.value(QStringLiteral("argument")).toString();
    const int index = open(workspace, argument,
                           argument.isEmpty() ? QString() : t.value(QStringLiteral("title")).toString(),
                           t.value(QStringLiteral("group")).toInt(0));
    restored << index;
    if (index >= 0 && tabs_[index].document && t.contains(QStringLiteral("state")))
      tabs_[index].document->setProperty("viewState", t.value(QStringLiteral("state")).toString());
  }
  normaliseGroups();
  auto restoredIndex = [&restored](int saved) {
    return saved >= 0 && saved < restored.size() ? restored[saved] : -1;
  };
  const int primary = restoredIndex(o.value(QStringLiteral("primary")).toInt(-1));
  const int secondary = restoredIndex(o.value(QStringLiteral("secondary")).toInt(-1));
  if (primary >= 0 && tabs_[primary].group == 0)
    shown_[0] = tabs_[primary].uid;
  if (secondary >= 0 && tabs_[secondary].group == 1)
    shown_[1] = tabs_[secondary].uid;
  emit dataChanged(this->index(0), this->index(static_cast<int>(tabs_.size()) - 1), {GroupRole, ShownRole});
  emit layoutChanged();
  const int current = restoredIndex(o.value(QStringLiteral("current")).toInt(-1));
  if (current >= 0)
    activate(current);
}

}  // namespace mbstudio::ui
