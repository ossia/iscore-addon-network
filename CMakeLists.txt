project(score_addon_network LANGUAGES CXX)

score_common_setup()
# Packages

find_package(${QT_VERSION} REQUIRED COMPONENTS Core Widgets Network WebSockets Xml)

# Files & main target
set(HDRS
    Network/Client/Client.hpp
    Network/Client/LocalClient.hpp
    Network/Client/RemoteClient.hpp
    Network/Communication/MessageMapper.hpp
    Network/Communication/MessageValidator.hpp
    Network/Communication/NetworkMessage.hpp
    Network/Communication/NetworkServer.hpp
    Network/Communication/NetworkSocket.hpp
    Network/Document/ClientPolicy.hpp
    Network/Document/DocumentPlugin.hpp
    Network/Document/MasterPolicy.hpp
    Network/Document/Timekeeper.hpp
    Network/Document/NTP.hpp
    Network/Document/PlaceholderPolicy.hpp
    Network/Document/Execution/BasicPruner.hpp
    Network/Document/Execution/DateExpression.hpp
    Network/Document/Execution/SyncMode.hpp
    Network/Document/Execution/Context.hpp
    Network/Document/Execution/FreeScenarioPolicy.hpp
    Network/Document/Execution/SharedScenarioPolicy.hpp
    Network/Document/Execution/SharedNonCompensatedExpressions.hpp
    Network/Document/Execution/SharedCompensatedExpressions.hpp
    Network/Document/Execution/MixedScenarioPolicy.hpp
    Network/Document/Execution/MasterPolicy.hpp
    Network/Document/Execution/SlavePolicy.hpp
    Network/Group/Commands/AddClientToGroup.hpp
    Network/Group/Commands/ChangeGroup.hpp
    Network/Group/Commands/CreateGroup.hpp
    Network/Group/Commands/DistributedScenarioCommandFactory.hpp
    Network/Group/Commands/RemoveClientFromGroup.hpp
    Network/Group/Commands/RemoveGroup.hpp
    Network/Group/Commands/AddCustomMetadata.hpp
    Network/Group/Group.hpp
    Network/Group/NetworkActions.hpp
    Network/Group/GroupManager.hpp
    Network/Group/GroupMetadata.hpp
    Network/Group/GroupMetadataWidget.hpp
    Network/Group/Panel/GroupPanelDelegate.hpp
    Network/Group/Panel/GroupPanelFactory.hpp
    Network/Group/Panel/Widgets/GroupHeaderItem.hpp
    Network/Group/Panel/Widgets/GroupListWidget.hpp
    Network/Group/Panel/Widgets/GroupTableCheckbox.hpp
    Network/Group/Panel/Widgets/GroupTableWidget.hpp
    Network/Group/Panel/Widgets/GroupWidget.hpp
    Network/Group/Panel/Widgets/SessionHeaderItem.hpp
    Network/IpDialog.hpp
    Network/IpWidget.hpp
    Network/NetworkApplicationPlugin.hpp
    Network/PlayerPlugin.hpp
    Network/Session/ClientSessionBuilder.hpp
    Network/Session/PlayerSessionBuilder.hpp
    Network/Session/ClientSession.hpp
    Network/Session/MasterSession.hpp
    Network/Session/RemoteClientBuilder.hpp
    Network/Session/Session.hpp
    Network/Settings/commands/ClientNameChangedCommand.hpp
    Network/Settings/commands/ClientPortChangedCommand.hpp
    Network/Settings/commands/MasterPortChangedCommand.hpp
    Network/Settings/NetworkSettings.hpp
    Network/Settings/NetworkSettingsModel.hpp
    Network/Settings/NetworkSettingsPresenter.hpp
    Network/Settings/NetworkSettingsView.hpp
    score_addon_network.hpp

    )
set(SRCS
    Network/Client/ClientSerialization.cpp
    Network/Group/GroupManagerSerialization.cpp
    Network/Group/GroupMetadataSerialization.cpp

    Network/Client/LocalClient.cpp
    Network/Communication/MessageMapper.cpp
    Network/Communication/MessageValidator.cpp
    Network/Communication/NetworkMessage.cpp
    Network/Communication/NetworkServer.cpp
    Network/Communication/NetworkSocket.cpp
    Network/Document/ClientPolicy.cpp
    Network/Document/DocumentPlugin.cpp
    Network/Document/DocumentPluginSerialization.cpp
    Network/Document/PlaceholderPolicy.cpp
    Network/Document/MasterPolicy.cpp
    Network/Document/NTP.cpp
    Network/Document/Execution/BasicPruner.cpp
    Network/Document/Execution/DateExpression.cpp
    Network/Document/Execution/FreeScenarioPolicy.cpp
    Network/Document/Execution/SharedScenarioPolicy.cpp
    Network/Document/Execution/MixedScenarioPolicy.cpp
    Network/Document/Execution/MasterPolicy.cpp
    Network/Document/Execution/SlavePolicy.cpp
    Network/Group/Commands/AddClientToGroup.cpp
    Network/Group/Commands/ChangeGroup.cpp
    Network/Group/Commands/CreateGroup.cpp
    Network/Group/Commands/DistributedScenarioCommandFactory.cpp
    Network/Group/Commands/RemoveClientFromGroup.cpp
    Network/Group/Commands/AddCustomMetadata.cpp
    Network/Group/Group.cpp
    Network/Group/GroupManager.cpp
    Network/Group/GroupMetadata.cpp
    Network/Group/GroupMetadataWidget.cpp
    Network/Group/GroupSerialization.cpp
    Network/Group/Panel/GroupPanelDelegate.cpp
    Network/Group/Panel/GroupPanelFactory.cpp
    Network/Group/Panel/Widgets/GroupHeaderItem.cpp
    Network/Group/Panel/Widgets/GroupListWidget.cpp
    Network/Group/Panel/Widgets/GroupTableCheckbox.cpp
    Network/Group/Panel/Widgets/GroupTableWidget.cpp
    Network/Group/Panel/Widgets/GroupWidget.cpp
    Network/Group/Panel/Widgets/SessionHeaderItem.cpp
    Network/IpDialog.cpp
    Network/IpWidget.cpp
    Network/NetworkApplicationPlugin.cpp
    Network/PlayerPlugin.cpp
    Network/Session/ClientSessionBuilder.cpp
    Network/Session/PlayerSessionBuilder.cpp
    Network/Session/ClientSession.cpp
    Network/Session/MasterSession.cpp
    Network/Session/RemoteClientBuilder.cpp
    Network/Session/Session.cpp
    Network/Settings/NetworkSettingsModel.cpp
    Network/Settings/NetworkSettingsPresenter.cpp
    Network/Settings/NetworkSettingsView.cpp

    Netpit/Netpit.hpp
    Netpit/Netpit.cpp
    Netpit/NetpitMessage.hpp

    Netpit/MessageContext.hpp
    Netpit/MessageContext.cpp


    score_addon_network.cpp
)

add_library(${PROJECT_NAME} ${SRCS} ${HDRS})
score_generate_command_list_file(${PROJECT_NAME} "${HDRS}")
# target_compile_options(${PROJECT_NAME} PRIVATE -ftime-trace)
target_link_libraries(${PROJECT_NAME} PUBLIC
                                    score_lib_base
                                    score_plugin_scenario
                                    score_lib_process
                                    score_plugin_engine
                                    score_plugin_avnd
                                    ${QT_PREFIX}::Core
                                    ${QT_PREFIX}::Widgets
                                    ${QT_PREFIX}::Network
                                    ${QT_PREFIX}::WebSockets
                                    )


setup_score_plugin(${PROJECT_NAME})
