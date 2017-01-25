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
#include "NetworkApplicationPlugin.hpp"
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
#if defined(USE_ZEROCONF)
#include <Explorer/Widgets/ZeroConf/ZeroconfBrowser.hpp>
#endif

#include "IpDialog.hpp"


struct VisitorVariant;

namespace Network
{
class Client;
class Session;

NetworkApplicationPlugin::NetworkApplicationPlugin(const iscore::GUIApplicationContext& app) :
  GUIApplicationPlugin {app}
{
}

void NetworkApplicationPlugin::setupClientConnection(QString ip, int port)
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

iscore::GUIElements NetworkApplicationPlugin::makeGUIElements()
{
#ifdef USE_ZEROCONF
  m_zeroconfBrowser = new ZeroconfBrowser{"_iscore._tcp", qApp->activeWindow()};
  connect(m_zeroconfBrowser, &ZeroconfBrowser::sessionSelected,
          this, &NetworkApplicationPlugin::setupClientConnection);
#endif

  using namespace iscore;
  QMenu* fileMenu = context.menus.get().at(iscore::Menus::File()).menu();

  // TODO do this better.
#ifdef USE_ZEROCONF
  fileMenu->addAction(m_zeroconfBrowser->makeAction());
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
      setupClientConnection(dial.ip(), dial.port());
    }
  });

  fileMenu->addAction(connectLocal);


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

  iscore::GUIElements g;
  g.actions.add<Actions::NetworkPlay>(playAction);

  playMenu->addAction(playAction);

  return g;
}

}
