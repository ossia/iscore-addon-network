
#include "RemoveClientFromGroup.hpp"

#include <score/document/DocumentContext.hpp>
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
RemoveClientFromGroup::RemoveClientFromGroup(Id<Client> client, Id<Group> group)
    : m_client{client}
    , m_group{group}
{
}

void RemoveClientFromGroup::undo(const score::DocumentContext& ctx) const
{
  ctx.plugin<Network::NetworkDocumentPlugin>().groupManager().group(m_group)->addClient(
      m_client);
}

void RemoveClientFromGroup::redo(const score::DocumentContext& ctx) const
{
  ctx.plugin<Network::NetworkDocumentPlugin>()
      .groupManager()
      .group(m_group)
      ->removeClient(m_client);
}

void RemoveClientFromGroup::serializeImpl(DataStreamInput& s) const
{
  s << m_client << m_group;
}

void RemoveClientFromGroup::deserializeImpl(DataStreamOutput& s)
{
  s >> m_client >> m_group;
}
}
}
