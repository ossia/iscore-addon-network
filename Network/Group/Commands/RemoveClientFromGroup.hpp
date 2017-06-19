#pragma once
#include <Network/Group/Commands/DistributedScenarioCommandFactory.hpp>
#include <iscore/tools/std/Optional.hpp>
#include <iscore/command/Command.hpp>
#include <iscore/model/path/ObjectPath.hpp>

#include <iscore/model/Identifier.hpp>

struct DataStreamInput;
struct DataStreamOutput;

namespace Network
{
class Client;
class Group;

namespace Command
{
class RemoveClientFromGroup : public iscore::Command
{
        ISCORE_COMMAND_DECL(DistributedScenarioCommandFactoryName(), RemoveClientFromGroup, "RemoveClientFromGroup")

    public:
        RemoveClientFromGroup(
                ObjectPath&& groupMgrPath,
                Id<Client> client,
                Id<Group> group);

        void undo(const iscore::DocumentContext& ctx) const override;
        void redo(const iscore::DocumentContext& ctx) const override;

        void serializeImpl(DataStreamInput & s) const override;
        void deserializeImpl(DataStreamOutput & s) override;

    private:
        ObjectPath m_path;
        Id<Client> m_client;
        Id<Group> m_group;
};
}
}
