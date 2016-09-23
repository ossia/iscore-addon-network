cmake_minimum_required(VERSION 3.0)
project(iscore_addon_network LANGUAGES CXX)

if(ANDROID)
    message("TODO FIXME ON ANDROID")
    return()
endif()

iscore_common_setup()

# Packages
find_package(Qt5 5.3 REQUIRED COMPONENTS Core Widgets Network Xml Test)

# Files & main target
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Repartition")

set(SRCS
"${CMAKE_CURRENT_SOURCE_DIR}/Repartition/client/ClientSerialization.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/GroupSerialization.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/GroupManagerSerialization.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/GroupMetadataSerialization.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/DocumentPlugins/PlaceholderNetworkPolicy.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DocumentPlugins/NetworkDocumentPluginSerialization.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Commands/AddClientToGroup.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Commands/RemoveClientFromGroup.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Commands/CreateGroup.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Commands/ChangeGroup.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Commands/DistributedScenarioCommandFactory.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Group.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/GroupManager.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/GroupMetadata.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Panel/GroupPanelFactory.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Panel/GroupPanelDelegate.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Panel/Widgets/GroupHeaderItem.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Panel/Widgets/GroupListWidget.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Panel/Widgets/GroupTableCheckbox.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Panel/Widgets/GroupWidget.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Panel/Widgets/SessionHeaderItem.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/Panel/Widgets/GroupTableWidget.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/DistributedScenario/GroupMetadataWidget.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DocumentPlugins/NetworkClientDocumentPlugin.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DocumentPlugins/NetworkDocumentPlugin.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/DocumentPlugins/NetworkMasterDocumentPlugin.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/IpDialog.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/IpWidget.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/iscore_plugin_network.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/NetworkApplicationPlugin.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Repartition/session/ClientSession.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Repartition/session/ClientSessionBuilder.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Repartition/session/MasterSession.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Repartition/session/Session.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Repartition/session/RemoteClientBuilder.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Serialization/MessageMapper.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Serialization/MessageValidator.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Serialization/NetworkMessage.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Serialization/NetworkServer.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Serialization/NetworkSocket.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/settings_impl/NetworkSettingsModel.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/settings_impl/NetworkSettingsPresenter.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/settings_impl/NetworkSettingsView.cpp"
)

file(GLOB_RECURSE HDRS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

#Remove zeroconf header from the glob
foreach(item ${HDRS})
  if(${item} MATCHES "Zeroconf")
    list(REMOVE_ITEM HDRS ${item})
  endif()
endforeach()

if(ISCORE_ZEROCONF)
    message("Will use ZeroConf")
    if(WIN32)
        include_directories(/f/kdnssd/usr/include)
        include_directories("f:/kdnssd/usr/include")
    endif()
endif()

add_library(${PROJECT_NAME} ${SRCS} ${HDRS})
iscore_generate_command_list_file(${PROJECT_NAME} "${HDRS}")

target_link_libraries(${PROJECT_NAME} PUBLIC
                                    iscore_lib_base
                                    iscore_plugin_scenario
                                    iscore_lib_process
                                    Qt5::Core
                                    Qt5::Widgets
                                    Qt5::Network)

if(ISCORE_ZEROCONF)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_ZEROCONF)
    target_link_libraries(${PROJECT_NAME} PUBLIC KF5::DNSSD)

    if(NOT APPLE AND NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC dns_sd)
    endif()
endif()

setup_iscore_plugin(${PROJECT_NAME})
