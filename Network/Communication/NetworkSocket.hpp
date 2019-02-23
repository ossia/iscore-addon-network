#pragma once
#include <QObject>
#include <QString>

#include <Network/Client/LocalClient.hpp>
#include <Network/Communication/NetworkMessage.hpp>
#include <wobjectdefs.h>
class QWebSocket;

namespace Network
{
// Utilisé par le serveur lorsque le client se connecte :
// le client a un NetworkSerializationServer qui tourne
// et le serveur écrit dedans avec le NetworkSerializationSocket
class NetworkSocket : public QObject
{
  W_OBJECT(NetworkSocket)
public:
  NetworkSocket(QWebSocket* sock, QObject* parent);
  NetworkSocket(QString ip, int port, QObject* parent);

  void sendMessage(const NetworkMessage&);

  QWebSocket& socket() const { return *m_socket; }

  void connected() W_SIGNAL(connected);
  void messageReceived(NetworkMessage m) W_SIGNAL(messageReceived, m);

private:
  void init();
  QWebSocket* m_socket{};
};
}
