// Unit tests of the MB-Studio kernel: values, events, commands, services,
// plugin loading and life cycle.
#include <atomic>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "FixtureService.h"
#include "mbstudio/kernel/Kernel.h"
#include "mbstudio/platform/Paths.h"

using namespace mbstudio;

namespace {

std::string fixtureDir() { return MBSTUDIO_FIXTURE_DIR; }

std::string fixturePath(const char *name) {
  return platform::joinPath(fixtureDir(), std::string(PluginManager::filePrefix()) + name + PluginManager::fileSuffix());
}

class CountingPlugin : public IPlugin {
 public:
  CountingPlugin(std::string id, std::vector<std::string> deps, std::vector<std::string> *order, bool ok = true)
      : id_(std::move(id)), deps_(std::move(deps)), order_(order), ok_(ok) {}
  PluginInfo info() const override { return {id_, id_, "1", "", deps_}; }
  bool initialize(Kernel &kernel, std::string &error) override {
    order_->push_back("start:" + id_);
    kernel.commands().add({id_ + ".cmd", "", "", id_}, [](const ValueMap &) { return CommandResult::success(); });
    if (!ok_)
      error = "refused";
    return ok_;
  }
  void shutdown(Kernel &) override { order_->push_back("stop:" + id_); }

 private:
  std::string id_;
  std::vector<std::string> deps_;
  std::vector<std::string> *order_;
  bool ok_;
};

}  // namespace

TEST(Value, TypedAccessAndJson) {
  ValueMap m{{"s", std::string("a \"quoted\"\npath")}, {"i", std::int64_t(42)}, {"d", 2.5}, {"b", true}};
  EXPECT_EQ(42, getInt(m, "i"));
  EXPECT_DOUBLE_EQ(42.0, getDouble(m, "i"));
  EXPECT_EQ(2, getInt(m, "d"));
  EXPECT_TRUE(getBool(m, "b"));
  EXPECT_EQ("fallback", getString(m, "missing", "fallback"));
  EXPECT_EQ(R"({"b": true, "d": 2.5, "i": 42, "s": "a \"quoted\"\npath"})", valueMapToJson(m));
}

TEST(EventBus, WildcardsAndUnsubscribe) {
  EventBus bus;
  int all = 0, jobs = 0, exact = 0;
  bus.subscribe("*", [&](const Event &) { all++; });
  const auto id = bus.subscribe("job.*", [&](const Event &) { jobs++; });
  bus.subscribe("job.finished", [&](const Event &e) {
    exact++;
    EXPECT_EQ(3, getInt(e.data, "code"));
  });
  EXPECT_EQ(3u, bus.publish("job.finished", {{"code", std::int64_t(3)}}));
  bus.publish("log.info");
  bus.publish("jobs");  // "job.*" must not match "jobs"
  EXPECT_EQ(3, all);
  EXPECT_EQ(1, jobs);
  EXPECT_EQ(1, exact);
  EXPECT_TRUE(bus.unsubscribe(id));
  bus.publish("job.started");
  EXPECT_EQ(1, jobs);
}

TEST(EventBus, HandlersMayPublish) {
  EventBus bus;
  int second = 0;
  bus.subscribe("first", [&](const Event &) { bus.publish("second"); });
  bus.subscribe("second", [&](const Event &) { second++; });
  bus.publish("first");
  EXPECT_EQ(1, second);
}

TEST(CommandRegistry, ExecuteAndErrors) {
  CommandRegistry commands;
  EXPECT_TRUE(commands.add({"math.add", "Add", "", "test"}, [](const ValueMap &a) {
    return CommandResult::success({{"sum", getInt(a, "a") + getInt(a, "b")}});
  }));
  EXPECT_FALSE(commands.add({"math.add", "", "", "other"}, [](const ValueMap &) { return CommandResult(); }));
  commands.add({"math.throw", "", "", "test"}, [](const ValueMap &) -> CommandResult {
    throw std::runtime_error("boom");
  });
  auto r = commands.execute("math.add", {{"a", std::int64_t(2)}, {"b", std::int64_t(3)}});
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(5, getInt(r.data, "sum"));
  r = commands.execute("math.throw");
  EXPECT_FALSE(r.ok);
  EXPECT_NE(std::string::npos, r.message.find("boom"));
  EXPECT_FALSE(commands.execute("nope").ok);
  EXPECT_EQ(2u, commands.removeOwnedBy("test"));
  EXPECT_TRUE(commands.list().empty());
}

TEST(ServiceRegistry, ProvideGetWithdraw) {
  struct Impl : test::IGreeter {
    std::string greet(const std::string &n) const override { return "hi " + n; }
  };
  ServiceRegistry services;
  EXPECT_FALSE(services.has<test::IGreeter>());
  EXPECT_TRUE(services.provide<test::IGreeter>(std::make_shared<Impl>(), "p"));
  EXPECT_FALSE(services.provide<test::IGreeter>(std::make_shared<Impl>(), "q"));  // first provider wins
  ASSERT_TRUE(services.get<test::IGreeter>());
  EXPECT_EQ("hi x", services.get<test::IGreeter>()->greet("x"));
  EXPECT_EQ(1u, services.withdrawProvidedBy("p"));
  EXPECT_FALSE(services.get<test::IGreeter>());
}

TEST(PluginManager, StaticPluginsStartInDependencyOrder) {
  std::vector<std::string> order;
  {
    Kernel kernel;
    kernel.plugins().addStatic([&] { return std::make_unique<CountingPlugin>("ui", std::vector<std::string>{"core"}, &order); });
    kernel.plugins().addStatic([&] { return std::make_unique<CountingPlugin>("core", std::vector<std::string>{}, &order); });
    kernel.plugins().addStatic([&] { return std::make_unique<CountingPlugin>("orphan", std::vector<std::string>{"missing"}, &order); });
    kernel.plugins().addStatic([&] { return std::make_unique<CountingPlugin>("broken", std::vector<std::string>{}, &order, false); });
    EXPECT_EQ(2u, kernel.start());
    EXPECT_TRUE(kernel.plugins().isActive("core"));
    EXPECT_TRUE(kernel.plugins().isActive("ui"));
    EXPECT_FALSE(kernel.plugins().isActive("orphan"));
    EXPECT_FALSE(kernel.plugins().isActive("broken"));
    EXPECT_FALSE(kernel.commands().has("broken.cmd"));  // removed after failed start
    EXPECT_TRUE(kernel.commands().has("ui.cmd"));
    for (const auto &p : kernel.plugins().plugins()) {
      if (p.info.id == "orphan") {
        EXPECT_EQ(PluginState::Failed, p.state);
        EXPECT_NE(std::string::npos, p.error.find("missing"));
      }
    }
    kernel.stop();
    EXPECT_FALSE(kernel.commands().has("ui.cmd"));
  }
  const std::vector<std::string> expected = {"start:broken", "start:core", "start:ui", "stop:ui", "stop:core"};
  EXPECT_EQ(expected, order);
}

TEST(PluginManager, CircularDependenciesFail) {
  std::vector<std::string> order;
  Kernel kernel;
  kernel.plugins().addStatic([&] { return std::make_unique<CountingPlugin>("a", std::vector<std::string>{"b"}, &order); });
  kernel.plugins().addStatic([&] { return std::make_unique<CountingPlugin>("b", std::vector<std::string>{"a"}, &order); });
  EXPECT_EQ(0u, kernel.start());
  EXPECT_TRUE(order.empty());
}

TEST(PluginManager, DisabledPluginsDoNotStart) {
  std::vector<std::string> order;
  KernelConfig config;
  config.disabledPlugins = {"core"};
  Kernel kernel(config);
  kernel.plugins().addStatic([&] { return std::make_unique<CountingPlugin>("core", std::vector<std::string>{}, &order); });
  EXPECT_EQ(0u, kernel.start());
  EXPECT_EQ(PluginState::Disabled, kernel.plugins().plugins().at(0).state);
}

TEST(PluginManager, LoadsSharedLibraryPlugins) {
  KernelConfig config;
  config.pluginDirectories = {fixtureDir()};
  Kernel kernel(config);
  EXPECT_EQ(2u, kernel.start());  // fixture + dependent; the bad ABI one fails
  ASSERT_TRUE(kernel.plugins().isActive("fixture"));
  ASSERT_TRUE(kernel.plugins().isActive("dependent"));

  auto greeter = kernel.services().get<test::IGreeter>();
  ASSERT_TRUE(greeter);
  EXPECT_EQ("Hello, kernel", greeter->greet("kernel"));
  greeter.reset();  // release plugin objects before the library unloads

  auto r = kernel.commands().execute("fixture.greet", {{"name", std::string("Windows")}});
  EXPECT_TRUE(r.ok);
  EXPECT_EQ("Hello, Windows", getString(r.data, "text"));
  EXPECT_EQ("Hello, dependent", getString(kernel.commands().execute("dependent.check").data, "text"));

  bool sawBadAbi = false;
  for (const auto &p : kernel.plugins().plugins()) {
    if (p.path.find("badabi") != std::string::npos) {
      sawBadAbi = true;
      EXPECT_EQ(PluginState::Failed, p.state);
      EXPECT_NE(std::string::npos, p.error.find("ABI"));
    }
  }
  EXPECT_TRUE(sawBadAbi);
  EXPECT_TRUE(kernel.contributions().hasWorkspace("fixture.workspace"));
  ASSERT_EQ(1u, kernel.contributions().actions().size());
  EXPECT_EQ("fixture.greet", kernel.contributions().actions()[0].commandId);
  kernel.stop();
  EXPECT_FALSE(kernel.services().has<test::IGreeter>());
  EXPECT_FALSE(kernel.contributions().hasWorkspace("fixture.workspace"));
  EXPECT_TRUE(kernel.contributions().actions().empty());
}

TEST(Contributions, ValidatedAndAtomic) {
  ContributionRegistry registry;
  WorkspaceContribution map{"map", "Map", "map", "view", "qrc:/Map.qml", "ui", 10, true};
  WorkspaceContribution grid{"grid", "Grid", "terrain", "view", "qrc:/Grid.qml", "ui", 5, false};
  std::string error;
  EXPECT_TRUE(registry.addWorkspace(map, &error)) << error;
  EXPECT_TRUE(registry.addWorkspace(grid, &error)) << error;
  ASSERT_EQ(2u, registry.workspaces().size());
  EXPECT_EQ("grid", registry.workspaces()[0].id);  // sorted by order
  EXPECT_FALSE(registry.addWorkspace(map, &error));
  EXPECT_NE(std::string::npos, error.find("already"));

  // one invalid entry rejects the whole set
  ActionContribution good{"a.good", "Good", "tools", "cmd.good", "", "", "other", 1};
  ActionContribution bad{"a.bad", "", "tools", "cmd.bad", "", "", "other", 2};
  WorkspaceContribution extra{"extra", "Extra", "", "", "qrc:/Extra.qml", "other", 1, true};
  EXPECT_FALSE(registry.addAll({extra}, {good, bad}, {}, &error));
  EXPECT_TRUE(registry.actions().empty());
  EXPECT_FALSE(registry.hasWorkspace("extra"));

  PanelContribution panel{"p", "Panel", "middle", "qrc:/P.qml", "other", 1};
  EXPECT_FALSE(registry.addPanel(panel, &error));
  panel.placement = "right";
  EXPECT_TRUE(registry.addPanel(panel, &error)) << error;
  EXPECT_TRUE(registry.addAll({extra}, {good}, {}, &error)) << error;
  EXPECT_EQ(3u, registry.removeOwnedBy("other"));
  EXPECT_EQ(2u, registry.workspaces().size());
}

TEST(PluginManager, ReportsUnloadableLibraries) {
  PluginManager manager;
  std::string error;
  EXPECT_FALSE(manager.load(fixturePath("does-not-exist"), &error));
  EXPECT_FALSE(error.empty());
  ASSERT_EQ(1u, manager.plugins().size());
  EXPECT_EQ(PluginState::Failed, manager.plugins()[0].state);
}

TEST(Kernel, LogHistoryAndEvents) {
  Kernel kernel;
  std::vector<std::string> seen;
  kernel.events().subscribe("log.*", [&](const Event &e) { seen.push_back(e.topic + ":" + getString(e.data, "message")); });
  kernel.log(LogLevel::Warning, "test", "careful");
  ASSERT_FALSE(kernel.logHistory().empty());
  EXPECT_EQ("careful", kernel.logHistory().back().message);
  ASSERT_EQ(1u, seen.size());
  EXPECT_EQ("log.warning:careful", seen[0]);
}
