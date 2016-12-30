#include <iscore/tools/std/Optional.hpp>
#include <algorithm>
#include <iterator>

#include "Group.hpp"
#include "GroupManager.hpp"
#include <iscore/model/IdentifiedObject.hpp>

class QObject;

namespace Network
{
GroupManager::GroupManager(QObject* parent):
    IdentifiedObject<GroupManager>{Id<GroupManager>{0}, "GroupManager", parent}
{

}

void GroupManager::addGroup(Group* group)
{
    m_groups.push_back(group);
    emit groupAdded(group->id());
}

Group*GroupManager::findGroup(const QString& str) const
{
  auto it = ossia::find_if(m_groups, [&] (auto ptr) { return ptr->name() == str; });
  if(it != m_groups.end())
    return *it;
  else
    return nullptr;
}

void GroupManager::removeGroup(Id<Group> group)
{
    using namespace std;

    auto it = find(begin(m_groups), end(m_groups), group);
    m_groups.erase(it);

    emit groupRemoved(group);

    (*it)->deleteLater();
}

Group* GroupManager::group(const Id<Group>& id) const
{
    return *std::find(std::begin(m_groups), std::end(m_groups), id);
}

Id<Group> GroupManager::defaultGroup() const
{
    return m_groups[0]->id();
}

const std::vector<Group*>& GroupManager::groups() const
{
    return m_groups;
}
}
