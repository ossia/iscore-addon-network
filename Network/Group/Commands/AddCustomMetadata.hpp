#pragma once
#include <iscore/command/Command.hpp>
#include <iscore/selection/Selection.hpp>
#include <Network/Group/Commands/DistributedScenarioCommandFactory.hpp>

namespace Scenario { class ConstraintModel; }
namespace Scenario { class EventModel; }
namespace Scenario { class TimeNodeModel; }

namespace Network
{
namespace Command
{
template<typename T>
struct MetadataUndoRedo
{
    Path<T> path;
    QVariantMap before;
    QVariantMap after;
};

class AddCustomMetadata : public iscore::Command
{
    ISCORE_COMMAND_DECL(
        DistributedScenarioCommandFactoryName(), AddCustomMetadata,
        "Change metadata")
  public:
    AddCustomMetadata(
        const QList<const Scenario::ConstraintModel*>& c,
        const QList<const Scenario::EventModel*>& e,
        const QList<const Scenario::TimeNodeModel*>& n,
        const std::vector<std::pair<QString, QString>>& meta
        );

    void undo(const iscore::DocumentContext& ctx) const override;

    void redo(const iscore::DocumentContext& ctx) const override;

  protected:
    void serializeImpl(DataStreamInput& s) const override;
    void deserializeImpl(DataStreamOutput& s) override;

  private:
    std::vector<MetadataUndoRedo<Scenario::ConstraintModel>> m_constraints;
    std::vector<MetadataUndoRedo<Scenario::EventModel>> m_events;
    std::vector<MetadataUndoRedo<Scenario::TimeNodeModel>> m_nodes;
};

}

void SetCustomMetadata(
    const iscore::DocumentContext& ctx,
    std::vector<std::pair<QString, QString>> md);
}
