
#include <iscore/tools/IdentifierGeneration.hpp>
#include <vector>

#include "CreateGroup.hpp"
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>
#include <iscore/serialization/DataStreamVisitor.hpp>
#include <iscore/model/path/ObjectPath.hpp>
#include <iscore/model/path/PathSerialization.hpp>


namespace Network
{
namespace Command
{
CreateGroup::CreateGroup(const GroupManager& mgr, QString groupName):
    m_path{mgr},
    m_name{groupName}
{
    m_newGroupId = getStrongId(mgr.groups());
}

void CreateGroup::undo(const iscore::DocumentContext& ctx) const
{
    m_path.find(ctx).removeGroup(m_newGroupId);
}

void CreateGroup::redo(const iscore::DocumentContext& ctx) const
{
    auto& mgr = m_path.find(ctx);
    mgr.addGroup(new Group{m_name, m_newGroupId, &mgr});
}

void CreateGroup::serializeImpl(DataStreamInput& s) const
{
    s << m_path << m_name << m_newGroupId;
}

void CreateGroup::deserializeImpl(DataStreamOutput& s)
{
    s >> m_path >> m_name >> m_newGroupId;
}
}
}
