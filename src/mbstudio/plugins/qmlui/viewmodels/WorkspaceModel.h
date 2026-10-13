// MB-Studio QML UI - the documents open in the central tab area. The kinds
// of documents ("workspaces") are contributions in the kernel registry, so a
// plugin adds a workspace without changing the main window; this model
// holds the open tabs, the active one and their persisted state.
#pragma once

#include <QAbstractListModel>
#include <QVariantList>
#include <QVector>

namespace mbstudio {
class Kernel;
}

namespace mbstudio::ui {

class WorkspaceModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)
  Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
  Q_PROPERTY(QString currentWorkspace READ currentWorkspace NOTIFY currentChanged)
  // every registered workspace: [{id, title, icon, category, url, singleton}]
  Q_PROPERTY(QVariantList available READ available NOTIFY availableChanged)

 public:
  enum Roles { UidRole = Qt::UserRole + 1, WorkspaceRole, TitleRole, IconRole, UrlRole, ArgumentRole, ClosableRole };

  explicit WorkspaceModel(Kernel *kernel, QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  int count() const { return static_cast<int>(tabs_.size()); }
  int currentIndex() const { return current_; }
  void setCurrentIndex(int index);
  QString currentWorkspace() const;
  QVariantList available() const;

  // Open (or activate) a workspace; argument is its document, e.g. a grid
  // path. Returns the tab index or -1 for an unknown workspace.
  Q_INVOKABLE int open(const QString &workspace, const QString &argument = QString(),
                       const QString &title = QString());
  Q_INVOKABLE void close(int index);
  Q_INVOKABLE int indexOf(const QString &workspace) const;
  Q_INVOKABLE QString argument(int index) const;
  Q_INVOKABLE QVariantMap workspaceInfo(const QString &workspace) const;
  // Re-read the registry (after plugins changed).
  Q_INVOKABLE void reload();
  // Open tabs as JSON and back (kept in the project).
  Q_INVOKABLE QString saveState() const;
  Q_INVOKABLE void restoreState(const QString &json);

 signals:
  void countChanged();
  void currentChanged();
  void availableChanged();

 private:
  struct Tab {
    int uid = 0;
    QString workspace;
    QString title;
    QString argument;
  };
  struct Info {
    QString id, title, icon, category, url;
    bool singleton = true;
  };
  const Info *find(const QString &id) const;
  Kernel *kernel_;
  QVector<Info> registry_;
  QVector<Tab> tabs_;
  int current_ = -1;
  int nextUid_ = 1;
};

}  // namespace mbstudio::ui
