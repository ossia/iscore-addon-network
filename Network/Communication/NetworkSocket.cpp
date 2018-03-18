#include <QAbstractSocket>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QIODevice>
#include <QtWebSockets/QWebSocket>

#include "NetworkSocket.hpp"
#include <Network/Communication/NetworkMessage.hpp>
namespace Network
{
NetworkSocket::NetworkSocket(QWebSocket* sock,
                             QObject* parent):
  QObject{parent},
  m_socket{sock}
{
  init();
}

NetworkSocket::NetworkSocket(QString ip,
                             int port,
                             QObject* parent) :
  QObject{parent},
  m_socket{new QWebSocket{QString{}, {}, this}}
{
  init();

  if(ip.startsWith("::ffff:"))
     ip.remove("::ffff:");
  else if(ip == "::1")
      ip = "127.0.0.1";
  m_socket->open("ws://" + ip + ":" + QString::number(port));

}

void NetworkSocket::sendMessage(const NetworkMessage &mess)
{
  QByteArray b;
  QDataStream writer(&b, QIODevice::WriteOnly);
  writer << mess;
  m_socket->sendBinaryMessage(b);
}

void NetworkSocket::init()
{
  connect(m_socket, qOverload<QAbstractSocket::SocketError>(&QWebSocket::error),
          this, [=] () {
    qDebug() << "Error: " << m_socket->errorString();
  });
  connect(m_socket, &QWebSocket::connected,
          this, [=] () {
    qDebug() << "WS Connected";
    connected();
  });
  connect(m_socket, &QWebSocket::disconnected, this, [] () {
    qDebug("Disconnected");
  });

  connect(m_socket, &QWebSocket::binaryMessageReceived, this, [=] (const QByteArray& b)
  {
    QDataStream reader(b);
    NetworkMessage m;
    reader >> m;

    messageReceived(m);
  });

  // m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
}
}
