#pragma once
#include <score/model/Identifier.hpp>

#include <Network/Session/Session.hpp>

namespace Network
{
class LocalClient;
class RemoteClient;
class ClientSession final : public Session
{
public:
  ClientSession(
      RemoteClient& master,
      LocalClient* client,
      Id<Session> id,
      QObject* parent = nullptr);

  ~ClientSession();
  RemoteClient& master() const override { return m_master; }

private:
  void on_createNewClient(QWebSocket*);
  RemoteClient& m_master;
};
}
