#pragma once

#include <iscore/plugins/qt_interfaces/PluginRequirements_QtInterface.hpp>
#include <iscore/plugins/qt_interfaces/GUIApplicationContextPlugin_QtInterface.hpp>
#include <iscore/plugins/qt_interfaces/FactoryInterface_QtInterface.hpp>
#include <QObject>
#include <utility>
#include <vector>

#include <iscore/command/CommandGeneratorMap.hpp>
#include <iscore/command/SerializableCommand.hpp>
#include <iscore/plugins/application/GUIApplicationContextPlugin.hpp>

namespace iscore {

class PanelFactory;
}  // namespace iscore

class iscore_addon_network :
        public QObject,
        public iscore::Plugin_QtInterface,
        public iscore::GUIApplicationContextPlugin_QtInterface,
        public iscore::CommandFactory_QtInterface,
        public iscore::FactoryInterface_QtInterface
{
        Q_OBJECT
        Q_PLUGIN_METADATA(IID GUIApplicationContextPlugin_QtInterface_iid)
        Q_INTERFACES(
                iscore::Plugin_QtInterface
                iscore::GUIApplicationContextPlugin_QtInterface
                iscore::CommandFactory_QtInterface
                iscore::FactoryInterface_QtInterface)

    public:
        iscore_addon_network();
        virtual ~iscore_addon_network();

    private:
        std::vector<std::unique_ptr<iscore::FactoryInterfaceBase>> factories(
                const iscore::ApplicationContext& ctx,
                const iscore::AbstractFactoryKey& factoryName) const override;

        iscore::GUIApplicationContextPlugin* make_applicationPlugin(
                const iscore::ApplicationContext& app) override;

        std::pair<const CommandParentFactoryKey, CommandGeneratorMap> make_commands() override;

        iscore::Version version() const override;
        UuidKey<iscore::Plugin> key() const override;
};
