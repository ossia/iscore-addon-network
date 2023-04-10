#pragma once

#include <score/command/Command.hpp>
#include <score/command/CommandGeneratorMap.hpp>
#include <score/plugins/application/GUIApplicationPlugin.hpp>
#include <score/plugins/qt_interfaces/CommandFactory_QtInterface.hpp>
#include <score/plugins/qt_interfaces/FactoryInterface_QtInterface.hpp>
#include <score/plugins/qt_interfaces/GUIApplicationPlugin_QtInterface.hpp>
#include <score/plugins/qt_interfaces/PluginRequirements_QtInterface.hpp>

#include <QObject>

#include <utility>
#include <vector>

namespace score
{

class PanelFactory;
} // namespace score

class score_addon_network
    : public score::Plugin_QtInterface
    , public score::ApplicationPlugin_QtInterface
    , public score::CommandFactory_QtInterface
    , public score::FactoryInterface_QtInterface
{
  SCORE_PLUGIN_METADATA(1, "33508c6d-46a1-4449-bfff-57246d579621")

public:
  score_addon_network();
  virtual ~score_addon_network();

private:
  std::vector<score::InterfaceBase*> factories(
      const score::ApplicationContext& ctx,
      const score::InterfaceKey& factoryName) const override;

  score::ApplicationPlugin*
  make_applicationPlugin(const score::ApplicationContext& app) override;
  score::GUIApplicationPlugin*
  make_guiApplicationPlugin(const score::GUIApplicationContext& app) override;

  std::pair<const CommandGroupKey, CommandGeneratorMap> make_commands() override;
};
