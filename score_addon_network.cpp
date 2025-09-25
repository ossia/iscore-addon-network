#include <score/command/CommandGeneratorMap.hpp>
#include <score/plugins/FactorySetup.hpp>
#include <score/plugins/documentdelegate/plugin/DocumentPluginCreator.hpp>
#include <score/plugins/qt_interfaces/GUIApplicationPlugin_QtInterface.hpp>

#include <Avnd/Factories.hpp>
#include <Netpit/NetpitAudio.hpp>
#include <Netpit/NetpitMessage.hpp>
#include <Netpit/NetpitVideo.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Group/Commands/DistributedScenarioCommandFactory.hpp>
#include <Network/Group/Panel/GroupPanelFactory.hpp>
#include <Network/NetworkApplicationPlugin.hpp>
#include <Network/PlayerPlugin.hpp>
#include <Network/Settings/NetworkSettings.hpp>

#include <score_addon_network.hpp>
#include <score_addon_network_commands_files.hpp>

#include <unordered_map>
namespace Network
{
class DocumentPluginFactory : public score::DocumentPluginFactory
{
  SCORE_CONCRETE("58c9e19a-fde3-47d0-a121-35853fec667d")

public:
  score::DocumentPlugin*
  load(const VisitorVariant& var, score::DocumentContext& doc, QObject* parent) override
  {
    return score::deserialize_dyn(var, [&](auto&& deserializer) {
      return new NetworkDocumentPlugin{doc, deserializer, parent};
    });
  }
};
}
namespace score
{

class PanelFactory;
} // namespace score
score_addon_network::score_addon_network() { }

score_addon_network::~score_addon_network() { }

// Interfaces implementations :
score::ApplicationPlugin*
score_addon_network::make_applicationPlugin(const score::ApplicationContext& app)
{
  return new Network::PlayerPlugin{app};
}

score::GUIApplicationPlugin*
score_addon_network::make_guiApplicationPlugin(const score::GUIApplicationContext& app)
{
  return new Network::NetworkApplicationPlugin{app};
}

std::vector<score::InterfaceBase*> score_addon_network::factories(
    const score::ApplicationContext& ctx, const score::InterfaceKey& key) const
{
  {
    std::vector<score::InterfaceBase*> fx;
    oscr::instantiate_fx<Netpit::MessagePit>(fx, ctx, key);
    oscr::instantiate_fx<Netpit::AudioPit>(fx, ctx, key);
    oscr::instantiate_fx<Netpit::VideoPit>(fx, ctx, key);
    
    if(!fx.empty())
      return fx;
  }
  return instantiate_factories<
      score::ApplicationContext,
      FW<score::DocumentPluginFactory, Network::DocumentPluginFactory>,
      FW<score::PanelDelegateFactory, Network::PanelDelegateFactory>,
      FW<score::SettingsDelegateFactory, Network::Settings::Factory>>(ctx, key);
}

std::pair<const CommandGroupKey, CommandGeneratorMap>
score_addon_network::make_commands()
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
