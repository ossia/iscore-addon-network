#pragma once
#include <score/command/Command.hpp>
#include <score/selection/Selection.hpp>

#include <Network/Group/Commands/DistributedScenarioCommandFactory.hpp>
/*
namespace Scenario
{
class IntervalModel;
}
namespace Scenario
{
class EventModel;
}
namespace Scenario
{
class TimeSyncModel;
}

namespace Network
{
namespace Command
{
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
