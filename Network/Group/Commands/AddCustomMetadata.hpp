#pragma once
#include <Process/ProcessFlags.hpp>

#include <score/command/Command.hpp>
#include <score/selection/Selection.hpp>

#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Group/Commands/DistributedScenarioCommandFactory.hpp>
namespace Process
{
class ProcessModel;
}
namespace Scenario
{
class IntervalModel;
class EventModel;
class TimeSyncModel;
}

namespace Network
{
class NetworkDocumentPlugin;
}
namespace Network::Command
{
template <typename T, typename V>
struct MetadataUndoRedo
{
  Path<T> path;
  V before;
};
template <typename V>
struct MetadataUndoRedos
{
  std::vector<MetadataUndoRedo<Scenario::IntervalModel, V>> intervals;
  std::vector<MetadataUndoRedo<Scenario::EventModel, V>> events;
  std::vector<MetadataUndoRedo<Scenario::TimeSyncModel, V>> nodes;
  std::vector<MetadataUndoRedo<Process::ProcessModel, V>> processes;
};

class SetSyncMode : public score::Command
{
  SCORE_COMMAND_DECL(
      DistributedScenarioCommandFactoryName(), SetSyncMode, "Set Sync mode")

public:
  SetSyncMode(NetworkDocumentPlugin& plug, const Selection& s, SyncMode newMode);

  void undo(const score::DocumentContext& ctx) const override;
  void redo(const score::DocumentContext& ctx) const override;

  MetadataUndoRedos<Process::NetworkFlags> metadatas;

private:
  void serializeImpl(DataStreamInput& s) const override;
  void deserializeImpl(DataStreamOutput& s) override;
  SyncMode m_after{};
};

class SetShareMode : public score::Command
{
  SCORE_COMMAND_DECL(
      DistributedScenarioCommandFactoryName(), SetShareMode, "Set Share mode")

public:
  SetShareMode(NetworkDocumentPlugin& plug, const Selection& s, ShareMode newMode);

  void undo(const score::DocumentContext& ctx) const override;
  void redo(const score::DocumentContext& ctx) const override;

  MetadataUndoRedos<Process::NetworkFlags> metadatas;

private:
  void serializeImpl(DataStreamInput& s) const override;
  void deserializeImpl(DataStreamOutput& s) override;
  ShareMode m_after{};
};

class SetGroup : public score::Command
{
  SCORE_COMMAND_DECL(DistributedScenarioCommandFactoryName(), SetGroup, "Set group")

public:
  SetGroup(NetworkDocumentPlugin& plug, const Selection& s, QString newMode);

  void undo(const score::DocumentContext& ctx) const override;
  void redo(const score::DocumentContext& ctx) const override;

  MetadataUndoRedos<QString> metadatas;

private:
  void serializeImpl(DataStreamInput& s) const override;
  void deserializeImpl(DataStreamOutput& s) override;
  QString m_after{};
};

}
