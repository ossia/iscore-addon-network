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


class SetOrderedMode : public UpdateObjectMetadata
{
    SCORE_COMMAND_DECL(
        DistributedScenarioCommandFactoryName(),
        SetOrderedMode,
        "Set Ordered mode")

    public:
      SetOrderedMode(
        NetworkDocumentPlugin& plug, const Selection& s,
        bool newMode);

    void redo(const score::DocumentContext& ctx) const override;

  private:
    void serializeImpl(DataStreamInput& s) const override;
    void deserializeImpl(DataStreamOutput& s) override;
    bool m_after{};
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
/*
template <typename T>
struct MetadataUndoRedo
{
  Path<T> path;
  QVariantMap before;
  QVariantMap after;
};

class AddCustomMetadata : public score::Command
{
  SCORE_COMMAND_ DECL(
      DistributedScenarioCommandFactoryName(),
      AddCustomMetadata,
      "Change metadata")
public:
  AddCustomMetadata(
      const std::vector<const Scenario::IntervalModel*>& c,
      const std::vector<const Scenario::EventModel*>& e,
      const std::vector<const Scenario::TimeSyncModel*>& n,
      const std::vector<std::pair<QString, QString>>& meta);

  void undo(const score::DocumentContext& ctx) const override;

  void redo(const score::DocumentContext& ctx) const override;

protected:
  void serializeImpl(DataStreamInput& s) const override;
  void deserializeImpl(DataStreamOutput& s) override;

private:
  std::vector<MetadataUndoRedo<Scenario::IntervalModel>> m_intervals;
  std::vector<MetadataUndoRedo<Scenario::EventModel>> m_events;
  std::vector<MetadataUndoRedo<Scenario::TimeSyncModel>> m_nodes;
};
}

void SetCustomMetadata(
    const score::DocumentContext& ctx,
    std::vector<std::pair<QString, QString>> md);
}*/
