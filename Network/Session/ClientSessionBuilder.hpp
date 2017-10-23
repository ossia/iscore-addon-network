#pragma once
#include <score/tools/std/Optional.hpp>
#include <score/command/Command.hpp>
#include <score/command/CommandData.hpp>
#include <score/model/Identifier.hpp>
#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
namespace score
{
struct GUIApplicationContext;
}

namespace Network
{
class Client;
class ClientSession;
class NetworkSocket;
class Session;
struct NetworkMessage;

//! Used by a client to join a Session.
class ClientSessionBuilder final : public QObject
{
        Q_OBJECT
    public:
        ClientSessionBuilder(
                const score::GUIApplicationContext&,
                QString ip,
                int port);

        void initiateConnection();
        ClientSession* builtSession() const;
        QByteArray documentData() const;
        const std::vector<score::CommandData>& commandStackData() const;

    public slots:
        void on_messageReceived(const NetworkMessage& m);

    signals:
        void connected();
        void sessionReady();
        void sessionFailed();

    private:
        const score::GUIApplicationContext& m_context;
        QString m_clientName{"A Client"};
        Id<Client> m_masterId, m_clientId;
        Id<Session> m_sessionId;
        NetworkSocket* m_mastersocket{};


        std::vector<score::CommandData> m_commandStack;
        QByteArray m_documentData;

        ClientSession* m_session{};
};
}
