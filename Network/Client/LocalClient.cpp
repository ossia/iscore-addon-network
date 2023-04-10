#include "LocalClient.hpp"

#include <QWebSocket>

namespace Network
{
LocalClient::LocalClient(Id<Client> id, QObject* parent)
    : Client{id, parent}
#if !defined(__EMSCRIPTEN__)
    , m_server{new NetworkServer{9090, this}}
#endif
{
#if !defined(__EMSCRIPTEN__)
  connect(m_server, &NetworkServer::newSocket, this, &LocalClient::createNewClient);
#endif
}

int LocalClient::localPort()
{
  return m_server->port();
}

NetworkServer* LocalClient::server() const noexcept
{
  return m_server;
}

}
