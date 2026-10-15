// MB-Studio QML UI - the documents open in the central area. The kinds of
// documents ("workspaces") are contributions in the kernel registry, so a
// plugin adds a workspace without changing the main window; this model
// holds the open tabs, the active one, the split of the area into two
// groups side by side, and each tab's document object.
//
// A document workspace (grid, terrain, line inspector, editor, figure) is
// opened once per argument and gets its own document object from the
// document factory, so two grids or two lines can be open and compared at
// the same time. A document object may have a "viewState" property (JSON);
// it is saved with the tabs and given back when the tabs are restored.
#pragma once

#include <QAbstractListModel>
#include <QPointer>
#include <QVariantList>
#include <QVector>
#include <functional>

namespace mbstudio {
class Kernel;
}

namespace mbstudio::ui {

class WorkspaceModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)
  // the active tab (in either group)
  Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
  Q_PROPERTY(QString currentWorkspace READ currentWorkspace NOTIFY currentChanged)
  Q_PROPERTY(QObject *currentDocument READ currentDocument NOTIFY currentChanged)
  Q_PROPERTY(QString currentArgument READ currentArgument NOTIFY currentChanged)
  // the area is split: group 1 (right) has tabs
  Q_PROPERTY(bool split READ split NOTIFY layoutChanged)
  // the tab shown in each group (-1: none)
  Q_PROPERTY(int primaryIndex READ primaryIndex NOTIFY layoutChanged)
  Q_PROPERTY(int secondaryIndex READ secondaryIndex NOTIFY layoutChanged)
  // every registered workspace: [{id, title, icon, category, url, singleton}]
  Q_PROPERTY(QVariantList available READ available NOTIFY availableChanged)

 public:
  enum Roles {
    UidRole = Qt::UserRole + 1,
    WorkspaceRole,
    TitleRole,
    IconRole,
    UrlRole,
    ArgumentRole,
    ClosableRole,
    DocumentRole,
    GroupRole,
    ShownRole  // the tab shown in its group
  };
  using DocumentFactory = std::function<QObject *(const QString &workspace, const QString &argument, QObject *parent)>;

  explicit WorkspaceModel(Kernel *kernel, QObject *parent = nullptr);
  ~WorkspaceModel() override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  int count() const { return static_cast<int>(tabs_.size()); }
  int currentIndex() const;
  void setCurrentIndex(int index);
  QString currentWorkspace() const;
  QObject *currentDocument() const;
  QString currentArgument() const;
  bool split() const;
  int primaryIndex() const { return indexOfUid(shown_[0]); }
  int secondaryIndex() const { return indexOfUid(shown_[1]); }
  QVariantList available() const;
  void setDocumentFactory(DocumentFactory factory) { factory_ = std::move(factory); }

  // Open (or activate) a workspace; argument is its document, e.g. a grid
  // path. group -1 opens it in the active group. Returns the tab index or
  // -1 for an unknown workspace.
  Q_INVOKABLE int open(const QString &workspace, const QString &argument = QString(),
                       const QString &title = QString(), int group = -1);
  Q_INVOKABLE void close(int index);
  // Move a tab to the other group (0 left, 1 right): split or join the area.
  Q_INVOKABLE void moveToGroup(int index, int group);
  Q_INVOKABLE void unsplit();
  Q_INVOKABLE int indexOf(const QString &workspace, const QString &argument = QString()) const;
  Q_INVOKABLE QString argument(int index) const;
  Q_INVOKABLE QObject *document(int index) const;
  Q_INVOKABLE int group(int index) const;
  Q_INVOKABLE QVariantMap workspaceInfo(const QString &workspace) const;
  Q_INVOKABLE void setTitle(int index, const QString &title);
  // Re-read the registry (after plugins changed).
  Q_INVOKABLE void reload();
  // Open tabs with their groups and view states as JSON and back (kept in
  // the project).
  Q_INVOKABLE QString saveState() const;
  Q_INVOKABLE void restoreState(const QString &json);
  // Every open document (for broadcasts such as finished jobs).
  QList<QObject *> documents() const;

 signals:
  void countChanged();
  void currentChanged();
  void layoutChanged();
  void availableChanged();
  // a document's view state changed (to save the workspace)
  void stateChanged();

 private:
  struct Tab {
    int uid = 0;
    QString workspace;
    QString title;
    QString argument;
    int group = 0;
    QPointer<QObject> document;
  };
  struct Info {
    QString id, title, icon, category, url;
    bool singleton = true;
  };
  const Info *find(const QString &id) const;
  int indexOfUid(int uid) const;
  void activate(int index);
  void normaliseGroups();
  Kernel *kernel_;
  QVector<Info> registry_;
  QVector<Tab> tabs_;
  int currentUid_ = 0;  // the active tab
  int shown_[2] = {0, 0};  // the tab shown in each group
  int nextUid_ = 1;
  DocumentFactory factory_;
};

}  // namespace mbstudio::ui
