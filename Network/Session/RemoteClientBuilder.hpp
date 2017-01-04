#pragma once
#include <iscore/tools/std/Optional.hpp>
#include <QObject>
#include <QString>

#include <iscore/model/Identifier.hpp>

class QTcpSocket;

namespace Network
{
class RemoteClient;
struct NetworkMessage;
class Client;
class MasterSession;
class NetworkSocket;

//! Used by the master to create a RemoteClient instance from a connecting client.
class RemoteClientBuilder final : public QObject
{
        Q_OBJECT
    public:
        RemoteClientBuilder(MasterSession& session, QTcpSocket* sock);

    signals:
        void clientReady(RemoteClientBuilder* builder, RemoteClient*);

    public slots:
        void on_messageReceived(const NetworkMessage& m);


    private:
        MasterSession& m_session;
        NetworkSocket* m_socket;
        RemoteClient* m_remoteClient{};

        Id<Client> m_clientId;
        QString m_clientName;
};
}
