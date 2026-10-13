#include "mbstudio/kernel/Contributions.h"

#include <algorithm>
#include <set>

namespace mbstudio {

namespace {

template <class T>
void sortByOrder(std::vector<T> &items) {
  std::stable_sort(items.begin(), items.end(), [](const T &a, const T &b) {
    return a.order != b.order ? a.order < b.order : a.id < b.id;
  });
}

template <class T>
bool containsId(const std::vector<T> &items, const std::string &id) {
  return std::any_of(items.begin(), items.end(), [&id](const T &t) { return t.id == id; });
}

template <class T>
std::size_t eraseOwnedBy(std::vector<T> &items, const std::string &owner) {
  const auto before = items.size();
  items.erase(std::remove_if(items.begin(), items.end(), [&owner](const T &t) { return t.owner == owner; }),
              items.end());
  return before - items.size();
}

bool fail(std::string *error, const std::string &message) {
  if (error != nullptr)
    *error = message;
  return false;
}

}  // namespace

bool ContributionRegistry::validate(const std::vector<WorkspaceContribution> &workspaces,
                                    const std::vector<ActionContribution> &actions,
                                    const std::vector<PanelContribution> &panels, std::string *error) const {
  std::set<std::string> seen;
  for (const auto &w : workspaces) {
    if (w.id.empty() || w.title.empty() || w.qmlUrl.empty())
      return fail(error, "workspace '" + w.id + "' needs an id, a title and a component");
    if (containsId(workspaces_, w.id) || !seen.insert("w:" + w.id).second)
      return fail(error, "workspace '" + w.id + "' is already registered");
  }
  for (const auto &a : actions) {
    if (a.id.empty() || a.title.empty() || a.commandId.empty())
      return fail(error, "action '" + a.id + "' needs an id, a title and a command");
    if (containsId(actions_, a.id) || !seen.insert("a:" + a.id).second)
      return fail(error, "action '" + a.id + "' is already registered");
  }
  for (const auto &p : panels) {
    if (p.id.empty() || p.title.empty() || p.qmlUrl.empty())
      return fail(error, "panel '" + p.id + "' needs an id, a title and a component");
    if (p.placement != "left" && p.placement != "right" && p.placement != "bottom")
      return fail(error, "panel '" + p.id + "' has an unknown placement '" + p.placement + "'");
    if (containsId(panels_, p.id) || !seen.insert("p:" + p.id).second)
      return fail(error, "panel '" + p.id + "' is already registered");
  }
  return true;
}

bool ContributionRegistry::addAll(const std::vector<WorkspaceContribution> &workspaces,
                                  const std::vector<ActionContribution> &actions,
                                  const std::vector<PanelContribution> &panels, std::string *error) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!validate(workspaces, actions, panels, error))
    return false;
  workspaces_.insert(workspaces_.end(), workspaces.begin(), workspaces.end());
  actions_.insert(actions_.end(), actions.begin(), actions.end());
  panels_.insert(panels_.end(), panels.begin(), panels.end());
  sortByOrder(workspaces_);
  sortByOrder(actions_);
  sortByOrder(panels_);
  return true;
}

bool ContributionRegistry::addWorkspace(const WorkspaceContribution &workspace, std::string *error) {
  return addAll({workspace}, {}, {}, error);
}

bool ContributionRegistry::addAction(const ActionContribution &action, std::string *error) {
  return addAll({}, {action}, {}, error);
}

bool ContributionRegistry::addPanel(const PanelContribution &panel, std::string *error) {
  return addAll({}, {}, {panel}, error);
}

std::size_t ContributionRegistry::removeOwnedBy(const std::string &owner) {
  std::lock_guard<std::mutex> lock(mutex_);
  return eraseOwnedBy(workspaces_, owner) + eraseOwnedBy(actions_, owner) + eraseOwnedBy(panels_, owner);
}

std::vector<WorkspaceContribution> ContributionRegistry::workspaces() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return workspaces_;
}

std::vector<ActionContribution> ContributionRegistry::actions() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return actions_;
}

std::vector<PanelContribution> ContributionRegistry::panels() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return panels_;
}

bool ContributionRegistry::hasWorkspace(const std::string &id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return containsId(workspaces_, id);
}

}  // namespace mbstudio
