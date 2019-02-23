#pragma once
#include <QObject>

#include <wobjectdefs.h>
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

  QWebSocketServer& server() const { return *m_server; }

  QString m_localAddress;
  int m_localPort;

  void newSocket(QWebSocket* sock) W_SIGNAL(newSocket, sock);

private:
  QWebSocketServer* m_server{};
};
}

W_REGISTER_ARGTYPE(QWebSocket*)
