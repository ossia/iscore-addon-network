#pragma once
#include <QObject>

#include <verdigris>
class QWebSocketServer;
class QWebSocket;

namespace Network
{
class NetworkServer : public QObject
{
  W_OBJECT(NetworkServer)
public:
  NetworkServer(int port, QObject* parent);
  int port() const;

  //! The port serving wss, or 0 when this machine has no certificate to offer.
  int securePort() const;

  QWebSocketServer& server() const { return *m_server; }

  QString m_localAddress;
  int m_localPort;

  void newSocket(QWebSocket* sock) W_SIGNAL(newSocket, sock);

private:
  QWebSocketServer* m_server{};
  QWebSocketServer* m_secureServer{};
};
}

W_REGISTER_ARGTYPE(QWebSocket*)
