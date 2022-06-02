#pragma once
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <score/command/Command.hpp>
#include <score/selection/Selection.hpp>

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
template <typename T>
struct MetadataUndoRedo
{
  Path<T> path;
  std::optional<ObjectMetadata> before;
};

class UpdateObjectMetadata : public score::Command
{
  public:
    using score::Command::Command;
    UpdateObjectMetadata(
      NetworkDocumentPlugin& plug, const Selection& s);

  protected:
    void undo(const score::DocumentContext& ctx) const override;

    std::vector<MetadataUndoRedo<Scenario::IntervalModel>> m_intervals;
    std::vector<MetadataUndoRedo<Scenario::EventModel>> m_events;
    std::vector<MetadataUndoRedo<Scenario::TimeSyncModel>> m_nodes;
    std::vector<MetadataUndoRedo<Process::ProcessModel>> m_processes;
};

class SetSyncMode : public UpdateObjectMetadata
{
  SCORE_COMMAND_DECL(
      DistributedScenarioCommandFactoryName(),
      SetSyncMode,
      "Set Sync mode")

  public:
    SetSyncMode(
      NetworkDocumentPlugin& plug, const Selection& s,
        SyncMode newMode);

    void redo(const score::DocumentContext& ctx) const override;

  private:
    void serializeImpl(DataStreamInput& s) const override;
    void deserializeImpl(DataStreamOutput& s) override;
    SyncMode m_after{};
};

class SetShareMode : public UpdateObjectMetadata
{
    SCORE_COMMAND_DECL(
        DistributedScenarioCommandFactoryName(),
        SetShareMode,
        "Set Share mode")

    public:
      SetShareMode(
        NetworkDocumentPlugin& plug, const Selection& s,
        ShareMode newMode);

    void redo(const score::DocumentContext& ctx) const override;

  private:
    void serializeImpl(DataStreamInput& s) const override;
    void deserializeImpl(DataStreamOutput& s) override;
    ShareMode m_after{};
};


class SetGroup : public UpdateObjectMetadata
{
    SCORE_COMMAND_DECL(
        DistributedScenarioCommandFactoryName(),
        SetGroup,
        "Set group")

    public:
      SetGroup(
        NetworkDocumentPlugin& plug, const Selection& s,
        QString newMode);

    void redo(const score::DocumentContext& ctx) const override;

  private:
    void serializeImpl(DataStreamInput& s) const override;
    void deserializeImpl(DataStreamOutput& s) override;
    QString m_after{};
};

}
