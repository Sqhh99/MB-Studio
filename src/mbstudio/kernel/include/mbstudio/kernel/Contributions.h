// MB-Studio kernel - user interface contributions.
//
// Plugins declare what they add to the workbench (workspaces, actions bound
// to commands, side panels) instead of editing the front end. The front end
// reads the registry and decides how to present the contributions. The
// kernel only stores and validates the declarations; it knows nothing about
// QML or any other toolkit (the qmlUrl is an opaque string for the front end).
#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "mbstudio/kernel/Export.h"

namespace mbstudio {

// A kind of document the workbench can open in its central tab area.
struct WorkspaceContribution {
  std::string id;        // unique, e.g. "bathymetry.map"
  std::string title;
  std::string icon;      // icon name known to the front end ("terrain", "map", ...)
  std::string category;  // grouping in menus ("view", "tools", ...)
  std::string qmlUrl;    // component the front end instantiates
  std::string owner;     // plugin id
  int order = 100;       // position in menus (ascending)
  bool singleton = true; // at most one open instance
};

// A menu or toolbar entry that runs a kernel command or a front end action.
struct ActionContribution {
  std::string id;         // unique, e.g. "project.grid"
  std::string title;
  std::string menu;       // "file", "process", "tools", "view", "help"
  std::string commandId;  // kernel command, or "ui:<name>" for a front end action
  std::string shortcut;   // e.g. "Ctrl+G"
  std::string icon;
  std::string owner;
  int order = 100;
};

// A dock panel ("left", "right" or "bottom").
struct PanelContribution {
  std::string id;
  std::string title;
  std::string placement;
  std::string qmlUrl;
  std::string owner;
  int order = 100;
};

class MBSTUDIO_KERNEL_API ContributionRegistry {
 public:
  // Each add fails (and registers nothing) if the id is empty, already used
  // or a required field is missing; error explains why.
  bool addWorkspace(const WorkspaceContribution &workspace, std::string *error = nullptr);
  bool addAction(const ActionContribution &action, std::string *error = nullptr);
  bool addPanel(const PanelContribution &panel, std::string *error = nullptr);

  // Register a set of contributions atomically: either all or none.
  bool addAll(const std::vector<WorkspaceContribution> &workspaces, const std::vector<ActionContribution> &actions,
              const std::vector<PanelContribution> &panels, std::string *error = nullptr);

  std::size_t removeOwnedBy(const std::string &owner);

  // Sorted by order, then id.
  std::vector<WorkspaceContribution> workspaces() const;
  std::vector<ActionContribution> actions() const;
  std::vector<PanelContribution> panels() const;
  bool hasWorkspace(const std::string &id) const;

 private:
  bool validate(const std::vector<WorkspaceContribution> &workspaces, const std::vector<ActionContribution> &actions,
                const std::vector<PanelContribution> &panels, std::string *error) const;
  mutable std::mutex mutex_;
  std::vector<WorkspaceContribution> workspaces_;
  std::vector<ActionContribution> actions_;
  std::vector<PanelContribution> panels_;
};

}  // namespace mbstudio
