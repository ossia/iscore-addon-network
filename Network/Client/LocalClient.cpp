#include "LocalClient.hpp"

#include <QWebSocket>

namespace Network
{
LocalClient::LocalClient(Id<Client> id, QObject* parent)
  : Client{id, parent}, m_server{new NetworkServer{9090, this}}
{
  connect(
        m_server,
        &NetworkServer::newSocket,
        this,
        &LocalClient::createNewClient);
}


int LocalClient::localPort() { return m_server->port(); }

NetworkServer& LocalClient::server() const { return *m_server; }

}
