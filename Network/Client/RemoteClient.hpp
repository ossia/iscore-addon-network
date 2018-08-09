#pragma once
#include <Network/Client/Client.hpp>
#include <Network/Communication/NetworkSocket.hpp>
namespace Network
{
// Has a TCP socket for exchange with this client.
class RemoteClient : public Client
{
        W_OBJECT(RemoteClient)
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

        void messageReceived(NetworkMessage m) W_SIGNAL(messageReceived, m);

    private:
        NetworkSocket* m_socket{};
};

}
