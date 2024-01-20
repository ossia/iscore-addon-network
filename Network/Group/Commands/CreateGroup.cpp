
#include "CreateGroup.hpp"

#include <score/model/path/ObjectPath.hpp>
#include <score/model/path/PathSerialization.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/tools/IdentifierGeneration.hpp>

#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>

#include <vector>

namespace Network
{
namespace Command
{
CreateGroup::CreateGroup(const GroupManager& mgr, QString groupName)
    : m_name{groupName}
{
  m_newGroupId = getStrongId(mgr.groups());
}

void CreateGroup::undo(const score::DocumentContext& ctx) const
{
  ctx.plugin<Network::NetworkDocumentPlugin>().groupManager().removeGroup(m_newGroupId);
}

void CreateGroup::redo(const score::DocumentContext& ctx) const
{
  auto& mgr = ctx.plugin<Network::NetworkDocumentPlugin>().groupManager();
  mgr.addGroup(new Group{m_name, m_newGroupId, &mgr});
}

void CreateGroup::serializeImpl(DataStreamInput& s) const
{
  s << m_name << m_newGroupId;
}

void CreateGroup::deserializeImpl(DataStreamOutput& s)
{
  s >> m_name >> m_newGroupId;
}
}
}
