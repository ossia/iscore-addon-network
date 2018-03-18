#pragma once
#include <QObject>
#include <QString>
#include <Network/Communication/NetworkMessage.hpp>
class QWebSocket;

namespace Network
{
// Utilisé par le serveur lorsque le client se connecte :
// le client a un NetworkSerializationServer qui tourne
// et le serveur écrit dedans avec le NetworkSerializationSocket
class NetworkSocket : public QObject
{
        Q_OBJECT
    public:
        NetworkSocket(QWebSocket* sock, QObject* parent);
        NetworkSocket(QString ip, int port, QObject* parent);

        void sendMessage(const NetworkMessage&);

        QWebSocket& socket() const { return *m_socket; }

    Q_SIGNALS:
        void connected();
        void messageReceived(NetworkMessage);

    private:
        void init();
        QWebSocket* m_socket{};
};
}
