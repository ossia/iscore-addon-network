#pragma once
#include <QWebSocket>

#include <Network/Client/Client.hpp>
#include <Network/Communication/NetworkServer.hpp>
// Has a TCP server to receive incoming connections from other clients.
namespace Network
{
class LocalClient : public Client
{
  W_OBJECT(LocalClient)
public:
  LocalClient(Id<Client> id, QObject* parent = nullptr)
      : Client{id, parent}, m_server{new NetworkServer{9090, this}}
  {
    connect(
        m_server,
        &NetworkServer::newSocket,
        this,
        &LocalClient::createNewClient);
  }

  template <typename Deserializer>
  LocalClient(Deserializer&& vis, QObject* parent) : Client{vis, parent}
  {
  }

  int localPort() { return m_server->port(); }

  NetworkServer& server() const { return *m_server; }

  void createNewClient(QWebSocket* w) W_SIGNAL(createNewClient, w);

private:
  NetworkServer* m_server{};
};
}
