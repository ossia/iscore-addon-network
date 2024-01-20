
#include "AddClientToGroup.hpp"

#include <score/model/path/ObjectPath.hpp>
#include <score/model/path/PathSerialization.hpp>
#include <score/serialization/DataStreamVisitor.hpp>

#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>

#include <algorithm>

namespace Network
{
namespace Command
{
AddClientToGroup::AddClientToGroup(Id<Client> client, Id<Group> group)
    : m_client{client}
    , m_group{group}
{
}

void AddClientToGroup::undo(const score::DocumentContext& ctx) const
{
  ctx.plugin<Network::NetworkDocumentPlugin>()
      .groupManager()
      .group(m_group)
      ->removeClient(m_client);
}

void AddClientToGroup::redo(const score::DocumentContext& ctx) const
{
  ctx.plugin<Network::NetworkDocumentPlugin>().groupManager().group(m_group)->addClient(
      m_client);
}

void AddClientToGroup::serializeImpl(DataStreamInput& s) const
{
  s << m_client << m_group;
}

void AddClientToGroup::deserializeImpl(DataStreamOutput& s)
{
  s >> m_client >> m_group;
}
}
}
