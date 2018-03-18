#pragma once
#include <QObject>

class QWebSocketServer;
class QWebSocket;

namespace Network
{
class NetworkServer : public QObject
{
        Q_OBJECT
    public:
        NetworkServer(int port, QObject* parent);
        int port() const;

        QWebSocketServer& server() const { return *m_server; }

        QString m_localAddress;
        int m_localPort;
    Q_SIGNALS:
        void newSocket(QWebSocket* sock);

    private:
        QWebSocketServer* m_server{};
};
}

