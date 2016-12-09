#pragma once
#include <DistributedScenario/Commands/DistributedScenarioCommandFactory.hpp>
#include <iscore/tools/std/Optional.hpp>
#include <iscore/command/Command.hpp>
#include <iscore/tools/ObjectPath.hpp>

#include <iscore/tools/SettableIdentifier.hpp>

struct DataStreamInput;
struct DataStreamOutput;

namespace Network
{
class Client;
class Group;
namespace Command
{
class AddClientToGroup : public iscore::Command
{
        ISCORE_COMMAND_DECL(DistributedScenarioCommandFactoryName(), AddClientToGroup, "AddClientToGroup")
    public:
        AddClientToGroup(ObjectPath&& groupMgrPath,
                         Id<Client> client,
                         Id<Group> group);

        void undo() const override;
        void redo() const override;

        void serializeImpl(DataStreamInput & s) const override;
        void deserializeImpl(DataStreamOutput & s) override;

    private:
        ObjectPath m_path;
        Id<Client> m_client;
        Id<Group> m_group;
};
}
}
