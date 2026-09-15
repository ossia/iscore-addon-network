#pragma once
#include <score_addon_network_export.h>
#include <Network/Client/Client.hpp>
#include <Network/Communication/NetworkServer.hpp>
class QWebSocket;
// Has a TCP server to receive incoming connections from other clients.
namespace Network
{
class SCORE_ADDON_NETWORK_EXPORT LocalClient : public Client
{
  W_OBJECT(LocalClient)
public:
  LocalClient(int local_port, Id<Client> id, QObject* parent = nullptr);

  template <typename Deserializer>
  LocalClient(Deserializer&& vis, QObject* parent)
      : Client{vis, parent}
  {
  }

  int localPort();

  NetworkServer* server() const noexcept;

  void createNewClient(QWebSocket* w) W_SIGNAL(createNewClient, w);

private:
  NetworkServer* m_server{};
};

}
