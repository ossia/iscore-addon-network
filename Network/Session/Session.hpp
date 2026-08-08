#pragma once
#include <score_addon_network_export.h>
#include <score/model/IdentifiedObject.hpp>
#include <score/model/Identifier.hpp>

#include <QDataStream>
#include <QIODevice>
#include <QList>
#include <QString>

#include <Network/Client/LocalClient.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Communication/MessageMapper.hpp>
#include <Network/Communication/MessageValidator.hpp>
#include <Network/Communication/NetworkMessage.hpp>

#include <algorithm>

namespace Network
{
class MessageMapper;
class MessageValidator;
class SCORE_ADDON_NETWORK_EXPORT Session : public IdentifiedObject<Session>
{
  W_OBJECT(Session)
public:
  Session(LocalClient* client, Id<Session> id, QObject* parent = nullptr);
  ~Session();

  MessageValidator& validator() const;
  MessageMapper& mapper() const;

  virtual Client& master() const;
  LocalClient& localClient() const;

  const QList<RemoteClient*>& remoteClients() const;
  RemoteClient* findClient(Id<Client> target);
  void addClient(RemoteClient* clt);
  void removeClient(RemoteClient* clt);

  NetworkMessage makeMessage(const QByteArray& address);

  template <typename... Args>
  NetworkMessage makeMessage(const QByteArray& address, Args&&... args)
  {
    NetworkMessage m;
    m.address = address;
    m.clientId = localClient().id();
    m.sessionId = id();

    impl_makeMessage(
        QDataStream{&m.data, QIODevice::WriteOnly}, std::forward<Args&&>(args)...);

    return m;
  }

  //! Does not include self
  void broadcastToAllClients(const NetworkMessage& m);

  //! To the peers that asked to watch, and nobody else.
  //!
  //! What the host observes -- where the playhead is, what its devices report,
  //! what it printed -- is only of use to a peer that does not run the score
  //! itself. A performer binds none of these, so sending them costs it the
  //! bandwidth and a "not handled" line per message, and costs the host the
  //! serialisation per client.
  void broadcastToTerminals(const NetworkMessage& m);

  //! Whether anybody is watching: the observation machinery can be skipped
  //! entirely when nothing would be sent.
  bool hasTerminals() const noexcept;

  //! Includes self
  void broadcastToAll(const NetworkMessage& m);
  void broadcastToOthers(const Id<Client>& sender, const NetworkMessage& m);

  void sendMessage(const Id<Client>& target, const NetworkMessage& m);
  void broadcastToClients(const std::vector<Id<Client>>& clts, const NetworkMessage& m);

  void clearClients();

  void clientAdded(RemoteClient* c) W_SIGNAL(clientAdded, c);
  void clientRemoving(RemoteClient* c) W_SIGNAL(clientRemoving, c);
  void clientRemoved(RemoteClient* c) W_SIGNAL(clientRemoved, c);
  void clientsChanged() W_SIGNAL(clientsChanged);

  void emitMessage(Id<Client> target, const NetworkMessage& m)
      W_SIGNAL(emitMessage, target, m);

  void validateMessage(const NetworkMessage& m);
  W_SLOT(validateMessage);

private:
  template <typename... Args>
  void impl_makeMessage(QDataStream&& s, Args&&... args)
  {
    DataStreamInput ss{s};
    ((ss << args), ...);
  }

  LocalClient* m_client{};
  mutable MessageMapper m_mapper;
  mutable MessageValidator m_validator;
  QList<RemoteClient*> m_remoteClients;
};
}
W_REGISTER_ARGTYPE(Network::RemoteClient*)
