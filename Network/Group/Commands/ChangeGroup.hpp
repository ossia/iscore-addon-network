/*
#pragma once

#include <score/command/Command.hpp>
#include <score/model/Identifier.hpp>
#include <score/model/path/ObjectPath.hpp>
#include <score/tools/std/Optional.hpp>

#include <Network/Group/Commands/DistributedScenarioCommandFactory.hpp>

struct DataStreamInput;
struct DataStreamOutput;

namespace Network
{
class Group;
namespace Command
{
class ChangeGroup : public score::Command
{
        SCORE_ COMMAND_DECL(DistributedScenarioCommandFactoryName(), ChangeGroup, "Change the group of an element")

    public:
        ChangeGroup(QObject& object, Id<Group> newGroup);

        void undo(const score::DocumentContext& ctx) const override;
        void redo(const score::DocumentContext& ctx) const override;

        void serializeImpl(DataStreamInput & s) const override;
        void deserializeImpl(DataStreamOutput & s) override;

    private:
        ObjectPath m_path;
        Id<Group> m_newGroup, m_oldGroup;
};
}
}

*/
