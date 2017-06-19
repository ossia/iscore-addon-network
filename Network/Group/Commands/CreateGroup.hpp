#pragma once
#include <Network/Group/Commands/DistributedScenarioCommandFactory.hpp>
#include <iscore/tools/std/Optional.hpp>
#include <iscore/command/Command.hpp>
#include <iscore/model/path/ObjectPath.hpp>
#include <QString>

#include <iscore/model/Identifier.hpp>

struct DataStreamInput;
struct DataStreamOutput;

namespace Network
{
class Group;
class GroupManager;

namespace Command
{
class CreateGroup : public iscore::Command
{
        ISCORE_COMMAND_DECL(DistributedScenarioCommandFactoryName(), CreateGroup, "CreateGroup")
        public:
        CreateGroup(const GroupManager& groupMgrPath, QString groupName);

        void undo(const iscore::DocumentContext& ctx) const override;
        void redo(const iscore::DocumentContext& ctx) const override;

        void serializeImpl(DataStreamInput & s) const override;
        void deserializeImpl(DataStreamOutput & s) override;

    private:
        Path<GroupManager> m_path;
        QString m_name;
        Id<Group> m_newGroupId;
};
}
}
