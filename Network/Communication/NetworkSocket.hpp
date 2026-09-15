#pragma once
#include <QObject>
#include <QUrl>
#include <QString>

#include <Network/Client/LocalClient.hpp>
#include <Network/Communication/NetworkMessage.hpp>

#include <verdigris>
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

  //! Never reached at all. Carries the address it tried, because the usual
  //! reason a secure one fails is a certificate the browser will not take on
  //! trust -- and there is no way to ask it to, from a socket.
  void connectionFailed(QUrl url, QString reason)
      W_SIGNAL(connectionFailed, url, reason)

private:
  void init();
  QWebSocket* m_socket{};
  QUrl m_url;
  bool m_everConnected{};
};
}
