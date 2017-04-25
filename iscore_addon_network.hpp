#pragma once

#include <iscore/plugins/qt_interfaces/PluginRequirements_QtInterface.hpp>
#include <iscore/plugins/qt_interfaces/GUIApplicationPlugin_QtInterface.hpp>
#include <iscore/plugins/qt_interfaces/FactoryInterface_QtInterface.hpp>
#include <iscore/plugins/qt_interfaces/CommandFactory_QtInterface.hpp>
#include <QObject>
#include <utility>
#include <vector>

#include <iscore/command/CommandGeneratorMap.hpp>
#include <iscore/command/Command.hpp>
#include <iscore/plugins/application/GUIApplicationPlugin.hpp>

namespace iscore {

class PanelFactory;
}  // namespace iscore

class iscore_addon_network :
        public QObject,
        public iscore::Plugin_QtInterface,
        public iscore::ApplicationPlugin_QtInterface,
        public iscore::CommandFactory_QtInterface,
        public iscore::FactoryInterface_QtInterface
{
        Q_OBJECT
        Q_PLUGIN_METADATA(IID ApplicationPlugin_QtInterface_iid)
        Q_INTERFACES(
                iscore::Plugin_QtInterface
                iscore::ApplicationPlugin_QtInterface
                iscore::CommandFactory_QtInterface
                iscore::FactoryInterface_QtInterface)
  ISCORE_PLUGIN_METADATA(1, "33508c6d-46a1-4449-bfff-57246d579621")

    public:
        iscore_addon_network();
        virtual ~iscore_addon_network();

    private:
        std::vector<std::unique_ptr<iscore::InterfaceBase>> factories(
                const iscore::ApplicationContext& ctx,
                const iscore::InterfaceKey& factoryName) const override;

        iscore::GUIApplicationPlugin* make_guiApplicationPlugin(
                const iscore::GUIApplicationContext& app) override;

        std::pair<const CommandGroupKey, CommandGeneratorMap> make_commands() override;
};
