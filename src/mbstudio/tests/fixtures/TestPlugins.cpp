// Test fixture plugins, compiled into separate shared libraries:
//   MBSTUDIO_FIXTURE == 1 : "fixture" plugin providing a service, a command and
//                            workbench contributions
//   MBSTUDIO_FIXTURE == 2 : "dependent" plugin requiring "fixture"
//   MBSTUDIO_FIXTURE == 3 : library with a wrong ABI version
#include <memory>
#include <string>

#include "mbstudio/kernel/Kernel.h"
#include "mbstudio/kernel/Plugin.h"
#include "FixtureService.h"

namespace {

class GreeterImpl : public mbstudio::test::IGreeter {
 public:
  std::string greet(const std::string &name) const override { return "Hello, " + name; }
};

#if MBSTUDIO_FIXTURE == 1
class FixturePlugin : public mbstudio::IPlugin {
 public:
  mbstudio::PluginInfo info() const override { return {"fixture", "Fixture", "1.0", "test plugin", {}}; }
  bool initialize(mbstudio::Kernel &kernel, std::string &error) override {
    kernel.services().provide<mbstudio::test::IGreeter>(std::make_shared<GreeterImpl>(), "fixture");
    kernel.commands().add({"fixture.greet", "Greet", "Say hello", "fixture"}, [](const mbstudio::ValueMap &args) {
      return mbstudio::CommandResult::success({{"text", "Hello, " + mbstudio::getString(args, "name")}});
    });
    // a workspace and a menu action, without touching the front end
    mbstudio::WorkspaceContribution workspace;
    workspace.id = "fixture.workspace";
    workspace.title = "Fixture";
    workspace.qmlUrl = "qrc:/fixture/FixtureWorkspace.qml";
    workspace.owner = "fixture";
    mbstudio::ActionContribution action;
    action.id = "fixture.greet";
    action.title = "Greet";
    action.menu = "tools";
    action.commandId = "fixture.greet";
    action.owner = "fixture";
    return kernel.contributions().addAll({workspace}, {action}, {}, &error);
  }
};
#elif MBSTUDIO_FIXTURE == 2
class FixturePlugin : public mbstudio::IPlugin {
 public:
  mbstudio::PluginInfo info() const override { return {"dependent", "Dependent", "1.0", "", {"fixture"}}; }
  bool initialize(mbstudio::Kernel &kernel, std::string &error) override {
    auto greeter = kernel.services().get<mbstudio::test::IGreeter>();
    if (!greeter) {
      error = "greeter service missing";
      return false;
    }
    const std::string text = greeter->greet("dependent");
    kernel.commands().add({"dependent.check", "Check", "", "dependent"}, [text](const mbstudio::ValueMap &) {
      return mbstudio::CommandResult::success({{"text", text}});
    });
    return true;
  }
};
#endif

}  // namespace

#if MBSTUDIO_FIXTURE == 3
extern "C" MBSTUDIO_PLUGIN_EXPORT int mbstudio_plugin_abi_version() { return mbstudio::kPluginAbiVersion + 100; }
extern "C" MBSTUDIO_PLUGIN_EXPORT mbstudio::IPlugin *mbstudio_create_plugin() { return nullptr; }
extern "C" MBSTUDIO_PLUGIN_EXPORT void mbstudio_destroy_plugin(mbstudio::IPlugin *) {}
#else
MBSTUDIO_DECLARE_PLUGIN(FixturePlugin)
#endif
