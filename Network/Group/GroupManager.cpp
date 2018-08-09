#include <score/tools/std/Optional.hpp>
#include <algorithm>
#include <iterator>
#include <Network/Client/RemoteClient.hpp>
#include <QSignalBlocker>
#include "Group.hpp"
#include "GroupManager.hpp"
#include <score/model/IdentifiedObject.hpp>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::GroupManager)
namespace Network
{
GroupManager::GroupManager(QObject* parent):
    IdentifiedObject<GroupManager>{Id<GroupManager>{0}, "GroupManager", parent}
{

}

void GroupManager::addGroup(Group* group)
{
    m_groups.push_back(group);
    groupAdded(group->id());
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

    groupRemoved(group);

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

void GroupManager::cleanup(QList<RemoteClient*> c)
{
  for(Group* group : m_groups)
  {
    QSignalBlocker b(group);
    auto clients = group->clients();
    for(const auto& clt : clients)
    {
      if(ossia::none_of(c, [&] (auto rc) { return rc->id() == clt; }))
      {
        group->removeClient(clt);
      }
    }
  }
}

std::size_t GroupManager::clientsCount(const std::vector<Id<Group> >& grps)
{
  return std::accumulate(
        grps.begin(),
        grps.end(), 0, [=] (const auto& lhs, const auto& rhs) {
    return lhs + this->group(rhs)->clients().size();
  });
}

std::vector<Id<Client> > GroupManager::clients(const std::vector<Id<Group> >& grps)
{
  //! TODO cache this and update it each time the clients change instead.
  std::vector<Id<Client>> theClients;

  for(auto& id : grps)
  {
    if(auto grp = this->group(id))
    {
      for(auto& clt : grp->clients())
      {
        auto it = ossia::find(theClients, clt);
        if(it == theClients.end())
          theClients.push_back(clt);
      }
    }
  }

  return theClients;
}

const std::vector<Group*>& GroupManager::groups() const
{
  return m_groups;
}
}
