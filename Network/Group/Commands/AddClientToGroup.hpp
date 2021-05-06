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
class GroupManager;
class Client;
class Group;
namespace Command
{
class AddClientToGroup : public score::Command
{
  SCORE_COMMAND_DECL(
      DistributedScenarioCommandFactoryName(),
      AddClientToGroup,
      "AddClientToGroup")
public:
  AddClientToGroup(
      Id<Client> client,
      Id<Group> group);

  void undo(const score::DocumentContext& ctx) const override;
  void redo(const score::DocumentContext& ctx) const override;

  void serializeImpl(DataStreamInput& s) const override;
  void deserializeImpl(DataStreamOutput& s) override;

private:
  Id<Client> m_client;
  Id<Group> m_group;
};
}
}
