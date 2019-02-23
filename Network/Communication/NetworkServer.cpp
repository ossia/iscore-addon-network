#include "NetworkServer.hpp"

#include <QDebug>
#include <QHostAddress>
#include <QList>
#include <QNetworkInterface>
#include <QString>
#include <QWebSocket>
#include <QtWebSockets/QWebSocketServer>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::NetworkServer)
namespace Network
{
NetworkServer::NetworkServer(int port, QObject* parent) : QObject{parent}
{
  m_server = new QWebSocketServer(
      "i-score-network", QWebSocketServer::SslMode::NonSecureMode, this);

  while (!m_server->listen(QHostAddress::Any, port))
  {
    port++;
  }

  QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

  // use the first non-localhost IPv4 address
  for (int i = 0; i < ipAddressesList.size(); ++i)
  {
    if (ipAddressesList.at(i) != QHostAddress::LocalHost
        && ipAddressesList.at(i).toIPv4Address())
    {
      m_localAddress = ipAddressesList.at(i).toString();
      break;
    }
  }

  // if we did not find one, use IPv4 localhost
  if (m_localAddress.isEmpty())
  {
    m_localAddress = QHostAddress(QHostAddress::LocalHost).toString();
  }

  m_localPort = m_server->serverPort();
  qDebug() << "Server: " << m_localAddress << ":" << m_localPort;

  connect(m_server, &QWebSocketServer::newConnection, this, [=]() {
    newSocket(m_server->nextPendingConnection());
  });
}

int NetworkServer::port() const
{
  return m_server->serverPort();
}
}
