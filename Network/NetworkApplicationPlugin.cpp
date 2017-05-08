#include "NetworkApplicationPlugin.hpp"
#include <iscore/plugins/application/GUIApplicationPlugin.hpp>
#include <iscore/tools/std/Optional.hpp>
#include <core/document/Document.hpp>
#include <QAction>
#include <QApplication>
#include <QDebug>
#include <qnamespace.h>

#include <QPair>
#include <algorithm>
#include <vector>

#include <Network/Document/ClientPolicy.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/MasterPolicy.hpp>
#include <Network/Group/NetworkActions.hpp>
#include <Network/Session/ClientSessionBuilder.hpp>
#include <iscore/tools/IdentifierGeneration.hpp>

#include <iscore/application/ApplicationContext.hpp>
#include <core/command/CommandStack.hpp>
#include <core/document/DocumentModel.hpp>
#include <core/presenter/DocumentManager.hpp>
#include <iscore/actions/MenuManager.hpp>
#include <iscore/actions/Menu.hpp>
#include <iscore/plugins/application/GUIApplicationPlugin.hpp>
#include <iscore/model/Identifier.hpp>
#include <iscore/actions/Menu.hpp>
#include <Network/Client/LocalClient.hpp>
#include <Network/Session/MasterSession.hpp>
#include <iscore/actions/ActionManager.hpp>
#include <Network/Group/Panel/GroupPanelDelegate.hpp>
#include <QMessageBox>
#include "IpDialog.hpp"

#if defined(OSSIA_DNSSD)
#include <Explorer/Widgets/ZeroConf/ZeroconfBrowser.hpp>
#endif

struct VisitorVariant;

namespace Network
{
class Client;
class Session;

NetworkApplicationPlugin::NetworkApplicationPlugin(const iscore::GUIApplicationContext& app) :
  GUIApplicationPlugin {app}
{
}

void NetworkApplicationPlugin::setupClientConnection(QString name, QString ip, int port, QMap<QString, QByteArray>)
{
  m_sessionBuilder = std::make_unique<ClientSessionBuilder>(
        context,
        ip,
        port);

  connect(m_sessionBuilder.get(), &ClientSessionBuilder::sessionReady,
          this, [&] () {
    auto& panel = context.panel<Network::PanelDelegate>();
    panel.networkPluginReady();

    m_sessionBuilder.reset();
  });
  connect(m_sessionBuilder.get(), &ClientSessionBuilder::sessionFailed,
          this, [&] () {
    m_sessionBuilder.reset();
  });

  m_sessionBuilder->initiateConnection();
}

void NetworkApplicationPlugin::setupPlayerConnection(
    QString name,
    QString ip,
    int port,
    QMap<QString, QByteArray>)
{
  auto cur = currentDocument();
  if(!cur)
    return;

  NetworkDocumentPlugin* plug = cur->context().findPlugin<NetworkDocumentPlugin>();
  if(!plug)
    return;

  auto session = plug->policy().session();
  auto ms = dynamic_cast<MasterSession*>(session);
  if(!ms)
    return;

  auto s = new QTcpSocket;

  s->connectToHost(ip, port);
  connect(s, &QTcpSocket::connected,
          this, [=] {
    s->write(QString::number(ms->localClient().localPort()).toUtf8());
    //s->deleteLater();
  });
  connect(s, qOverload<QAbstractSocket::SocketError>(&QTcpSocket::error),
          this, [=] (auto) {
    qDebug("Socket error");
    s->deleteLater();
  });
}

iscore::GUIElements NetworkApplicationPlugin::makeGUIElements()
{
  using namespace iscore;
  QMenu* fileMenu = context.menus.get().at(iscore::Menus::File()).menu();

#ifdef OSSIA_DNSSD
  m_serverBrowser = new ZeroconfBrowser{"_iscore._tcp", qApp->activeWindow()};
  connect(m_serverBrowser, &ZeroconfBrowser::sessionSelected,
          this, &NetworkApplicationPlugin::setupClientConnection);
  auto serveract = m_serverBrowser->makeAction();
  serveract->setText("Browse for server");

  m_playerBrowser = new ZeroconfBrowser{"_iscore_player._tcp", qApp->activeWindow()};
  connect(m_playerBrowser, &ZeroconfBrowser::sessionSelected,
          this, &NetworkApplicationPlugin::setupPlayerConnection);
  auto playeract = m_playerBrowser->makeAction();
  playeract->setText("Browse for players");

  fileMenu->addAction(serveract);
  fileMenu->addAction(playeract);
#endif

  QAction* makeServer = new QAction {tr("Make Server"), this};
  connect(makeServer, &QAction::triggered, this,
          [&] ()
  {
    if(auto doc = currentDocument())
    {
      auto clt = new LocalClient(Id<Client>(0));
      clt->setName(tr("Master"));
      auto serv = new MasterSession(currentDocument(), clt, Id<Session>(1234));
      auto policy = new MasterEditionPolicy{serv, currentDocument()->context()};

      auto realplug = new NetworkDocumentPlugin{doc->context(), policy, getStrongId(doc->model().pluginModels()), doc};
      auto execpol = new MasterExecutionPolicy(*serv, *realplug, doc->context());

      qApp->setStyleSheet("");
      realplug->setExecPolicy(execpol);
      doc->model().addPluginModel(realplug);

      auto& panel = context.panel<Network::PanelDelegate>();
      panel.networkPluginReady();
    }
  });

  fileMenu->addAction(makeServer);

  QAction* connectLocal = new QAction {tr("Join local"), this};
  connect(connectLocal, &QAction::triggered, this,
          [&] () {
    IpDialog dial{QApplication::activeWindow()};

    if(dial.exec())
    {
      // Default is 127.0.0.1 : 9090
      setupClientConnection(QString{}, dial.ip(), dial.port(), {});
    }
  });

  fileMenu->addAction(connectLocal);

  // Execution
  QMenu* playMenu = context.menus.get().at(iscore::Menus::Play()).menu();

  QAction* playAction = new QAction{tr("Play (network)"), this};
  connect(playAction, &QAction::triggered, this, [&] {
    if(auto doc = currentDocument())
    {
      auto np = doc->context().findPlugin<NetworkDocumentPlugin>();
      if(np)
        np->policy().play();
    }
  });

  QAction* stopAction = new QAction{tr("Stop (network)"), this};
  connect(stopAction, &QAction::triggered, this, [&] {
    if(auto doc = currentDocument())
    {
      auto np = doc->context().findPlugin<NetworkDocumentPlugin>();
      if(np)
        np->policy().stop();
    }
  });

  iscore::GUIElements g;
  g.actions.add<Actions::NetworkPlay>(playAction);
  g.actions.add<Actions::NetworkStop>(stopAction);

  playMenu->addAction(playAction);
  playMenu->addAction(stopAction);

  return g;
}

}
