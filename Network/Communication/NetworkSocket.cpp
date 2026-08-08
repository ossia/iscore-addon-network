#include "NetworkSocket.hpp"

#include <QAbstractSocket>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QIODevice>
#include <QtWebSockets/QWebSocket>

#include <Network/Communication/NetworkMessage.hpp>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::NetworkSocket)
namespace Network
{
NetworkSocket::NetworkSocket(QWebSocket* sock, QObject* parent)
    : QObject{parent}
    , m_socket{sock}
{
  init();
}

NetworkSocket::NetworkSocket(QString ip, int port, QObject* parent)
    : QObject{parent}
    , m_socket{new QWebSocket{QString{}, {}, this}}
{
  init();

  // The address may carry its own scheme. A page served over https cannot open
  // a plain socket -- the browser refuses it outright, and Qt says so as
  // "Unsupported WebSocket scheme: ws" -- so reaching a host from anywhere but
  // this machine means wss, and only the caller knows how it was reached.
  QString scheme = "ws";
  for(const auto& known : {"wss://", "ws://"})
  {
    if(ip.startsWith(known))
    {
      scheme = QString::fromUtf8(known).chopped(3);
      ip.remove(0, qstrlen(known));
      break;
    }
  }

  if(ip.startsWith("::ffff:"))
    ip.remove("::ffff:");
  else if(ip == "::1")
    ip = "127.0.0.1";

  m_socket->open(QUrl(scheme + "://" + ip + ":" + QString::number(port)));
}

void NetworkSocket::sendMessage(const NetworkMessage& mess)
{
  QByteArray b;
  QDataStream writer(&b, QIODevice::WriteOnly);
  writer << mess;
  m_socket->sendBinaryMessage(b);
}

void NetworkSocket::init()
{
  connect(
      m_socket,
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
      QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
#else
      &QWebSocket::errorOccurred,
#endif
      this, [this]() { qDebug() << "Error: " << m_socket->errorString(); });
  connect(m_socket, &QWebSocket::connected, this, [this]() {
    qDebug() << "WS Connected";
    connected();
  });
  connect(m_socket, &QWebSocket::disconnected, this, []() { qDebug("Disconnected"); });

  connect(
      m_socket, &QWebSocket::binaryMessageReceived, this, [this](const QByteArray& b) {
        QDataStream reader(b);
        NetworkMessage m;
        reader >> m;

        messageReceived(m);
      });

  // m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
}
}
