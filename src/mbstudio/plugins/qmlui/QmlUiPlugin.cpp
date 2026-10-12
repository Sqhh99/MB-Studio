// MB-Studio QML UI plugin: provides the graphical front end (IFrontend).
//
// The user interface is a plugin like any other: it depends on the services
// of the "mbsystem" plugin only through their interfaces, and the kernel and
// the MB-System adapter run without it (mbstudio --headless / --command).
#include <memory>

#include "QmlFrontend.h"
#include "mbstudio/kernel/Kernel.h"
#include "mbstudio/kernel/Plugin.h"

namespace mbstudio::ui {

class QmlUiPlugin : public IPlugin {
 public:
  PluginInfo info() const override {
    PluginInfo info;
    info.id = "qmlui";
    info.name = "Qt Quick user interface";
    info.version = MBSTUDIO_VERSION;
    info.description = "Qt6/QML desktop user interface";
    info.dependencies = {"mbsystem"};
    return info;
  }
  bool initialize(Kernel &kernel, std::string &) override {
    kernel.services().provide<IFrontend>(std::make_shared<QmlFrontend>(), "qmlui");
    return true;
  }
};

}  // namespace mbstudio::ui

MBSTUDIO_DECLARE_PLUGIN(mbstudio::ui::QmlUiPlugin)
