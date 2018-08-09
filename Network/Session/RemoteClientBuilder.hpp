#pragma once
#include <score/tools/std/Optional.hpp>
#include <QObject>
#include <QString>

#include <score/model/Identifier.hpp>
#include <wobjectdefs.h>

class QWebSocket;

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
        W_OBJECT(RemoteClientBuilder)
    public:
        RemoteClientBuilder(MasterSession& session, QWebSocket* sock);

        void clientReady(RemoteClientBuilder* builder, RemoteClient* c) W_SIGNAL(clientReady, builder, c);

        void on_messageReceived(const NetworkMessage& m); W_SLOT(on_messageReceived)


    private:
        MasterSession& m_session;
        NetworkSocket* m_socket;
        RemoteClient* m_remoteClient{};

        Id<Client> m_clientId;
        QString m_clientName;
};
}
