#pragma once
#include <Network/Group/Commands/DistributedScenarioCommandFactory.hpp>
#include <score/tools/std/Optional.hpp>
#include <score/command/Command.hpp>
#include <score/model/path/ObjectPath.hpp>
#include <QString>

#include <score/model/Identifier.hpp>

struct DataStreamInput;
struct DataStreamOutput;

namespace Network
{
class Group;
class GroupManager;

namespace Command
{
class CreateGroup : public score::Command
{
        SCORE_COMMAND_DECL(DistributedScenarioCommandFactoryName(), CreateGroup, "CreateGroup")
        public:
        CreateGroup(const GroupManager& groupMgrPath, QString groupName);

        void undo(const score::DocumentContext& ctx) const override;
        void redo(const score::DocumentContext& ctx) const override;

        void serializeImpl(DataStreamInput & s) const override;
        void deserializeImpl(DataStreamOutput & s) override;

    private:
        Path<GroupManager> m_path;
        QString m_name;
        Id<Group> m_newGroupId;
};
}
}
