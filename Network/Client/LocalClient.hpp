#pragma once
#include <Network/Client/Client.hpp>
#include <Network/Communication/NetworkServer.hpp>
#include <QWebSocket>
// Has a TCP server to receive incoming connections from other clients.
namespace Network
{
class LocalClient : public Client
{
        Q_OBJECT
    public:
        LocalClient(Id<Client> id, QObject* parent = nullptr):
            Client{id, parent},
            m_server{new NetworkServer{9090, this}}
        {
            // todo : envoyer id et name du client.
            connect(m_server, &NetworkServer::newSocket,
                    this, &LocalClient::createNewClient);
        }

        template<typename Deserializer>
        LocalClient(Deserializer&& vis, QObject* parent) :
            Client {vis, parent}
        {
        }


        int localPort()
        {
            return m_server->port();
        }

        NetworkServer& server() const { return *m_server; }

    signals:
        void createNewClient(QWebSocket*);

    private:
        NetworkServer* m_server{};
};
}
