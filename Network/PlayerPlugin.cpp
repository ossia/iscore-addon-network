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
}

PlayerPlugin::~PlayerPlugin()
{

}

void PlayerPlugin::initialize()
{
  auto& s = context.settings<Network::Settings::Model>();
  con(s, &Network::Settings::Model::PlayerPortChanged,
      this, [this] { setupServer(); });

  setupServer();
}

void PlayerPlugin::setupServer()
{
  auto& s = context.settings<Network::Settings::Model>();
  m_listenServer.close();
  m_listenServer.listen(QHostAddress::Any, s.getPlayerPort());
  qDebug() << "Player: listening on " << m_listenServer.serverPort();

  connect(&m_listenServer, &QTcpServer::newConnection,
          this, [&] {
    QTcpSocket* connection = m_listenServer.nextPendingConnection();
    connect(connection, &QTcpSocket::readyRead,
            this, [=] {
      auto dat = connection->readAll();

      m_sessionBuilder = std::make_unique<PlayerSessionBuilder>(
            context,
            connection->peerAddress().toString(), dat.toInt());
      m_sessionBuilder->documentLoader = documentLoader;
      connection->close();
      connection->deleteLater();

      connect(m_sessionBuilder.get(), &PlayerSessionBuilder::sessionReady,
              this, [&] () {
        m_sessionBuilder.reset();
        if(onDocumentLoaded)
          onDocumentLoaded();
      });

      connect(m_sessionBuilder.get(), &PlayerSessionBuilder::sessionFailed,
              this, [&] () {
        m_sessionBuilder.reset();
      });

      connect(m_sessionBuilder.get(), &PlayerSessionBuilder::connected,
              this, [&] {
        m_sessionBuilder->initiateConnection();
      });
    });
  });


  auto name = s.getClientName();
  if(name.isEmpty())
    name = "i-score player";

#if defined(OSSIA_DNSSD)
  m_zeroconf = std::make_unique<servus::Servus>("_iscore_player._tcp");
  m_zeroconf->announce(m_listenServer.serverPort(), name.toStdString());
#endif
}

}
