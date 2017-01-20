#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/MessageValidator.hpp>
#include <iscore/tools/std/Optional.hpp>

#include <Network/Communication/NetworkMessage.hpp>
#include "Session.hpp"
#include <Network/Client/LocalClient.hpp>

class QObject;

namespace Network
{
Session::Session(LocalClient* client, Id<Session> id, QObject* parent):
    IdentifiedObject<Session>{id, "Session", parent},
    m_client{client},
    m_mapper{new MessageMapper},
    m_validator{new MessageValidator(*this, mapper())}
{
    m_client->setParent(this);
    connect(this, &Session::emitMessage,
            this, &Session::sendMessage,
            Qt::QueuedConnection);
}

Session::~Session()
{
    delete m_mapper;
    delete m_validator;
}

MessageValidator&Session::validator() const
{
  return *m_validator;
}

MessageMapper&Session::mapper() const
{
  return *m_mapper;
}

Client&Session::master() const { throw; }

LocalClient&Session::localClient() const
{
  return *m_client;
}

const QList<RemoteClient*>&Session::remoteClients() const
{
  return m_remoteClients;
}

RemoteClient*Session::findClient(Id<Client> target)
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
  emit clientAdded(clt);
  emit clientsChanged();
}

NetworkMessage Session::makeMessage(const QByteArray& address)
{
  NetworkMessage m;
  m.address = address;
  m.clientId = localClient().id();
  m.sessionId = id();

  return m;
}

void Session::broadcastToAllClients(NetworkMessage m)
{
  for(RemoteClient* client : remoteClients())
    client->sendMessage(m);
}

void Session::broadcastToOthers(Id<Client> sender, NetworkMessage m)
{
  for(const auto& client : remoteClients())
  {
    if(client->id() != sender)
      client->sendMessage(m);
  }
}

void Session::sendMessage(Id<Client> target, NetworkMessage m)
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


void Session::validateMessage(NetworkMessage m)
{
  if(validator().validate(m))
    mapper().map(m);
}
}
