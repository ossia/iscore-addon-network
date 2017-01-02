#pragma once
/*
#include <Network/Group/Commands/DistributedScenarioCommandFactory.hpp>
#include <iscore/tools/std/Optional.hpp>
#include <iscore/command/Command.hpp>
#include <iscore/model/path/ObjectPath.hpp>

#include <iscore/model/Identifier.hpp>

struct DataStreamInput;
struct DataStreamOutput;

namespace Network
{
class Group;
namespace Command
{
class ChangeGroup : public iscore::Command
{
        ISCORE_ COMMAND_DECL(DistributedScenarioCommandFactoryName(), ChangeGroup, "Change the group of an element")

    public:
        ChangeGroup(ObjectPath&& path, Id<Group> newGroup);

        void undo() const override;
        void redo() const override;

        void serializeImpl(DataStreamInput & s) const override;
        void deserializeImpl(DataStreamOutput & s) override;

    private:
        ObjectPath m_path;
        Id<Group> m_newGroup, m_oldGroup;
};
}
}
*/
