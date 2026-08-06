# Session tests. Replication is per-document rather than per-application, so a
# master and a client can be two documents in one process talking over a real
# socket -- see SessionReplicationTest.cpp.
score_add_test(test_network_session_replication
  SOURCES SessionReplicationTest.cpp
  APP
  PLUGINS score_addon_network score_plugin_scenario score_lib_process)
target_include_directories(test_network_session_replication PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/..")
