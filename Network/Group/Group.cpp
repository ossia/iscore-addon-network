#include "Group.hpp"

#include <score/model/IdentifiedObject.hpp>

#include <algorithm>
#include <iterator>

class Client;
class QObject;

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::Group)
namespace Network
{
Group::Group(QString name, Id<Group> id, QObject* parent)
    : IdentifiedObject<Group>{id, "Group", parent}, m_name{name}
{
}

QString Group::name() const
{
  return m_name;
}

void Group::setName(QString arg)
{
  if (m_name == arg)
    return;

  m_name = arg;
  nameChanged(arg);
}

void Group::addClient(Id<Client> clt)
{
  m_executingClients.push_back(clt);
  clientAdded(clt);
}

void Group::removeClient(Id<Client> clt)
{
  auto it = std::find(
      std::begin(m_executingClients), std::end(m_executingClients), clt);
  SCORE_ASSERT(it != std::end(m_executingClients));

  m_executingClients.erase(it);
  clientRemoved(clt);
}

bool Group::hasClient(const Id<Client>& clt) const
{
  return ossia::find(m_executingClients, clt) != m_executingClients.end();
}
}
