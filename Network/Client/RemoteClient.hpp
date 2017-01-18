#pragma once
#include "Client.hpp"
#include <Network/Communication/NetworkSocket.hpp>
namespace Network
{
// Has a TCP socket for exchange with this client.
class RemoteClient : public Client
{
        Q_OBJECT
    public:
        RemoteClient(NetworkSocket* socket,
                     Id<Client> id,
                     QObject* parent = nullptr):
            Client(id, parent),
            m_socket{socket}
        {
            connect(m_socket, &NetworkSocket::messageReceived,
                    this,     &RemoteClient::messageReceived);
        }

        template<typename Deserializer>
        RemoteClient(Deserializer&& vis, QObject* parent) :
            Client {vis, parent}
        {
        }

        void sendMessage(const NetworkMessage& m)
        {
            m_socket->sendMessage(m);
        }

        NetworkSocket& socket() const { return *m_socket; }

        QString m_clientServerAddress;
        int m_clientServerPort{};
    signals:
        void messageReceived(NetworkMessage);

    private:
        NetworkSocket* m_socket{};
};

}