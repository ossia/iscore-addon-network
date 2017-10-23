
#include <algorithm>

#include "AddClientToGroup.hpp"
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/model/path/ObjectPath.hpp>


namespace Network
{
namespace Command
{
AddClientToGroup::AddClientToGroup(ObjectPath&& groupMgrPath,
                                   Id<Client> client,
                                   Id<Group> group):
    m_path{std::move(groupMgrPath)},
    m_client{client},
    m_group{group}
{
}

void AddClientToGroup::undo(const score::DocumentContext& ctx) const
{
    m_path.find<GroupManager>(ctx).group(m_group)->removeClient(m_client);
}

void AddClientToGroup::redo(const score::DocumentContext& ctx) const
{
    m_path.find<GroupManager>(ctx).group(m_group)->addClient(m_client);
}

void AddClientToGroup::serializeImpl(DataStreamInput& s) const
{
    s << m_path << m_client << m_group;
}

void AddClientToGroup::deserializeImpl(DataStreamOutput& s)
{
    s >> m_path >> m_client >> m_group;
}
}
}
