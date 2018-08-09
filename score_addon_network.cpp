#include <Network/NetworkApplicationPlugin.hpp>
#include <score_addon_network.hpp>

#include <score/tools/ForEachType.hpp>
#include <Network/Group/Commands/DistributedScenarioCommandFactory.hpp>
#include <Network/Group/Panel/GroupPanelFactory.hpp>
#include <score/plugins/qt_interfaces/GUIApplicationPlugin_QtInterface.hpp>
#include <score_addon_network_commands_files.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Settings/NetworkSettings.hpp>

#include <score/plugins/customfactory/FactorySetup.hpp>

#include <score/command/CommandGeneratorMap.hpp>
#include <Network/PlayerPlugin.hpp>
#include <unordered_map>
namespace score {

class PanelFactory;
}  // namespace score
score_addon_network::score_addon_network()
{
}

score_addon_network::~score_addon_network()
{

}

// Interfaces implementations :
score::ApplicationPlugin*
score_addon_network::make_applicationPlugin(
        const score::ApplicationContext& app)
{
    return new Network::PlayerPlugin{app};
}

score::GUIApplicationPlugin*
score_addon_network::make_guiApplicationPlugin(
    const score::GUIApplicationContext& app)
{
  return new Network::NetworkApplicationPlugin{app};
}

std::vector<std::unique_ptr<score::InterfaceBase>>
score_addon_network::factories(
        const score::ApplicationContext& ctx,
        const score::InterfaceKey& key) const
{
    return instantiate_factories<
            score::ApplicationContext,
        FW<score::DocumentPluginFactory,
            Network::DocumentPluginFactory>,
        FW<score::PanelDelegateFactory,
            Network::PanelDelegateFactory>,
        FW<score::SettingsDelegateFactory,
            Network::Settings::Factory>
    >(ctx, key);
}

std::pair<const CommandGroupKey, CommandGeneratorMap> score_addon_network::make_commands()
{
    using namespace Network;
    using namespace Network::Command;
    std::pair<const CommandGroupKey, CommandGeneratorMap> cmds{
        DistributedScenarioCommandFactoryName(), CommandGeneratorMap{}};

    ossia::for_each_type<
    #include <score_addon_network_commands.hpp>
        >(score::commands::FactoryInserter{cmds.second});

    return cmds;
}

#include <score/plugins/PluginInstances.hpp>
SCORE_EXPORT_PLUGIN(score_addon_network)
