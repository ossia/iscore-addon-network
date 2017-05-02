#include <Network/PlayerPlugin.hpp>
#include <Network/Session/PlayerSessionBuilder.hpp>
#include <Network/Settings/NetworkSettingsModel.hpp>
#include <QTcpSocket>

#if defined(OSSIA_DNSSD)
#include <servus/servus.h>
#endif

namespace Network
{
PlayerPlugin::PlayerPlugin(const iscore::ApplicationContext& ctx):
  iscore::ApplicationPlugin{ctx}
{
  m_listenServer.listen(QHostAddress::Any, 0);
  qDebug() << "Listening on " << m_listenServer.serverPort();
  connect(&m_listenServer, &QTcpServer::newConnection,
          this, [&] {
    QTcpSocket* connection = m_listenServer.nextPendingConnection();
    qDebug() << "New connection";
    connect(connection, &QTcpSocket::readyRead,
            this, [=] {
      auto dat = connection->readAll();
      qDebug() << dat;

      m_sessionBuilder = std::make_unique<PlayerSessionBuilder>(
            context,
            connection->localAddress().toString(), dat.toInt());
      m_sessionBuilder->documentLoader = documentLoader;

      connect(m_sessionBuilder.get(), &PlayerSessionBuilder::sessionReady,
              this, [&] () {

        qDebug() << "Session ready";
        m_sessionBuilder.reset();
        if(onDocumentLoaded)
          onDocumentLoaded();
      });

      connect(m_sessionBuilder.get(), &PlayerSessionBuilder::sessionFailed,
              this, [&] () {
        qDebug() << "Session failed";
        m_sessionBuilder.reset();
      });

      m_sessionBuilder->initiateConnection();
    });
  });
}

PlayerPlugin::~PlayerPlugin()
{

}

void PlayerPlugin::initialize()
{
  auto& s = context.settings<Network::Settings::Model>();
  auto name = s.getClientName();
  if(name.isEmpty())
    name = "i-score player";

#if defined(OSSIA_DNSSD)
  m_zeroconf = std::make_unique<servus::Servus>("_iscore_player._tcp");
  m_zeroconf->announce(m_listenServer.serverPort(), name.toStdString());
#endif
}

}
