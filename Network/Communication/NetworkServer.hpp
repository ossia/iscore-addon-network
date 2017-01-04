#pragma once
#include <QObject>

class QTcpServer;
class QTcpSocket;

namespace Network
{
class NetworkServer : public QObject
{
        Q_OBJECT
    public:
        NetworkServer(int port, QObject* parent);
        int port() const;

        QTcpServer& server() const { return *m_tcpServer; }

        QString m_localAddress;
        int m_localPort;
    signals:
        void newSocket(QTcpSocket* sock);

    private:
        QTcpServer* m_tcpServer{};
};
}

