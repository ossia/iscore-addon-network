# Session tests. Replication is per-document rather than per-application, so a
# master and a client can be two documents in one process talking over a real
# socket -- see SessionReplicationTest.cpp.
score_add_test(test_network_session_replication
  SOURCES SessionReplicationTest.cpp
  APP
  PLUGINS score_addon_network score_plugin_scenario score_lib_process)
target_include_directories(test_network_session_replication PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/..")

# What a session looks like once there are layers. Needs the GUI application
# fixture, which cannot share a binary with the headless one.
score_add_test(test_network_session_layers
  SOURCES SessionLayerTest.cpp
  GUI
  PLUGINS score_addon_network score_plugin_scenario score_lib_process
          score_plugin_automation score_plugin_curve)
target_include_directories(test_network_session_layers PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/..")

# What is plugged into the other machine: a terminal has none of the hardware,
# and often not the protocol either, so the list has to come across.
score_add_test(test_network_remote_enumeration
  SOURCES RemoteEnumerationTest.cpp
  APP
  PLUGINS score_addon_network score_plugin_scenario score_lib_process
          score_plugin_protocols score_plugin_deviceexplorer)
target_include_directories(test_network_remote_enumeration PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/..")
