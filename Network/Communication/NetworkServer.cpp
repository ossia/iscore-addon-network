#include "NetworkServer.hpp"

#include <QDebug>
#include <QHostAddress>
#include <QList>
#include <QNetworkInterface>
#include <QString>
#include <QWebSocket>
#include <QtWebSockets/QWebSocketServer>

#include <QDir>
#include <QFile>

#if !defined(__EMSCRIPTEN__)
// A browser has no TLS of its own to offer: it is the side that connects.
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>

#include <optional>
#endif

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::NetworkServer)
namespace Network
{
#if !defined(__EMSCRIPTEN__)
namespace
{
//! Where this machine keeps the certificate it serves wss with, if it has one.
//! Deliberately the same pair the wasm build is served from: a peer that
//! accepted the page's certificate has already accepted this one.
std::optional<QSslConfiguration> secureConfiguration()
{
  const auto dir = qEnvironmentVariable(
      "SCORE_NETWORK_CERT_DIR",
      QDir::homePath() + QStringLiteral("/ossia/wasm/certs"));

  QFile cert{dir + "/cert.pem"};
  QFile key{dir + "/key.pem"};
  if(!cert.exists() || !key.exists())
    return std::nullopt;

  if(!cert.open(QIODevice::ReadOnly) || !key.open(QIODevice::ReadOnly))
    return std::nullopt;

  QSslCertificate certificate{&cert, QSsl::Pem};
  QSslKey privateKey{&key, QSsl::Rsa, QSsl::Pem};
  if(certificate.isNull() || privateKey.isNull())
  {
    qWarning() << "Could not read the certificate in" << dir;
    return std::nullopt;
  }

  QSslConfiguration conf;
  conf.setLocalCertificate(certificate);
  conf.setPrivateKey(privateKey);
  conf.setPeerVerifyMode(QSslSocket::VerifyNone);
  return conf;
}
}
#endif

NetworkServer::NetworkServer(int port, QObject* parent)
    : QObject{parent}
{
  m_server = new QWebSocketServer(
      "i-score-network", QWebSocketServer::SslMode::NonSecureMode, this);

  while(!m_server->listen(QHostAddress::Any, port))
  {
    port++;
  }

#if !defined(__EMSCRIPTEN__)
  // use the first non-localhost IPv4 address
  for(const auto& ip : QNetworkInterface::allAddresses())
  {
    if(ip != QHostAddress::LocalHost && ip.toIPv4Address())
    {
      m_localAddress = ip.toString();
      break;
    }
  }
#endif

  // if we did not find one, use IPv4 localhost
  if(m_localAddress.isEmpty())
  {
    m_localAddress = QHostAddress(QHostAddress::LocalHost).toString();
  }

  m_localPort = m_server->serverPort();
  qDebug() << "Server: " << m_localAddress << ":" << m_localPort;

  auto accept = [this](QWebSocketServer* from) {
    auto sock = from->nextPendingConnection();
    sock->setOutgoingFrameSize(2000);
    newSocket(sock);
  };

  connect(m_server, &QWebSocketServer::newConnection, this, [this, accept]() {
    accept(m_server);
  });

#if !defined(__EMSCRIPTEN__)
  // A second listener, when this machine has a certificate to offer.
  //
  // A browser served over https cannot open a plain socket to anything but
  // itself, so a peer reaching this host over a network -- a VPN, another
  // machine on the LAN -- has no way in through the port above. Both are
  // served: an ordinary peer keeps using the plain one, and nothing has to
  // decide which the session "really" runs on.
  if(auto ssl = secureConfiguration(); ssl)
  {
    m_secureServer = new QWebSocketServer(
        "i-score-network", QWebSocketServer::SslMode::SecureMode, this);
    m_secureServer->setSslConfiguration(*ssl);

    int securePort = m_localPort + 1;
    while(!m_secureServer->listen(QHostAddress::Any, securePort))
    {
      if(++securePort > m_localPort + 64)
      {
        qWarning() << "No port for the secure listener; wss is not available";
        delete m_secureServer;
        m_secureServer = nullptr;
        break;
      }
    }

    if(m_secureServer)
    {
      qDebug() << "Server (wss): " << m_localAddress << ":"
               << m_secureServer->serverPort();
      connect(
          m_secureServer, &QWebSocketServer::newConnection, this,
          [this, accept]() { accept(m_secureServer); });
    }
  }
#endif
}

int NetworkServer::securePort() const
{
  return m_secureServer ? m_secureServer->serverPort() : 0;
}

int NetworkServer::port() const
{
  return m_server->serverPort();
}
}
