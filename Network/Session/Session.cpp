
#include "Session.hpp"

#include <score/tools/std/Optional.hpp>

#include <ossia/detail/algorithms.hpp>

#include <QWebSocket>

#include <Network/Client/LocalClient.hpp>
#include <Network/Communication/NetworkMessage.hpp>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::Session)
W_OBJECT_IMPL(Network::LocalClient)
W_OBJECT_IMPL(Network::RemoteClient)
namespace Network
{
Session::Session(LocalClient* client, Id<Session> id, QObject* parent)
    : IdentifiedObject<Session>{id, "Session", parent}
    , m_client{client}
    , m_mapper{}
    , m_validator{id, m_mapper}
{
  m_client->setParent(this);
  connect(
      this, &Session::emitMessage, this, &Session::sendMessage, Qt::QueuedConnection);
}

Session::~Session() { }

MessageValidator& Session::validator() const
{
  return m_validator;
}

MessageMapper& Session::mapper() const
{
  return m_mapper;
}

Client& Session::master() const
{
  throw;
}

LocalClient& Session::localClient() const
{
  return *m_client;
}

const QList<RemoteClient*>& Session::remoteClients() const
{
  return m_remoteClients;
}

RemoteClient* Session::findClient(Id<Client> target)
{
  const auto& c = remoteClients();
  auto it = ossia::find(c, target);
  if(it != c.end())
    return *it;
  return nullptr;
}

void Session::addClient(RemoteClient* clt)
{
  clt->setParent(this);
  m_remoteClients.append(clt);
  clientAdded(clt);
  clientsChanged();
}

void Session::removeClient(RemoteClient* clt)
{
  this->clientRemoving(clt);
  m_remoteClients.removeAll(clt);
  this->clientRemoved(clt);
  clientsChanged();
  clt->deleteLater();
}

NetworkMessage Session::makeMessage(const QByteArray& address)
{
  NetworkMessage m;
  m.address = address;
  m.clientId = localClient().id();
  m.sessionId = id();

  return m;
}

void Session::broadcastToClients(
    const std::vector<Id<Client>>& clts, const NetworkMessage& m)
{
  for(auto& id : clts)
  {
    sendMessage(id, m);
  }
}

void Session::clearClients()
{
  for(RemoteClient* client : remoteClients())
  {
    QObject::disconnect(client, nullptr, this, nullptr);
    delete client;
  }
}
void Session::broadcastToAllClients(const NetworkMessage& m)
{
  for(RemoteClient* client : remoteClients())
    client->sendMessage(m);
}

void Session::broadcastToAll(const NetworkMessage& m)
{
  broadcastToAllClients(m);
  mapper().map(m);
}

void Session::broadcastToOthers(const Id<Client>& sender, const NetworkMessage& m)
{
  for(const auto& client : remoteClients())
  {
    if(client->id() != sender)
      client->sendMessage(m);
  }
}

void Session::sendMessage(const Id<Client>& target, const NetworkMessage& m)
{
  const auto& c = remoteClients();
  auto it = ossia::find(c, target);
  if(it != c.end())
    (*it)->sendMessage(m);
  else if(target == localClient().id())
  {
    mapper().map(m);
  }
}

void Session::validateMessage(const NetworkMessage& m)
{
  if(validator().validate(m))
    mapper().map(m);
}
}
