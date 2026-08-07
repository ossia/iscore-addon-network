
#include "NetworkApplicationPlugin.hpp"

#include "IpDialog.hpp"

#include <score/actions/ActionManager.hpp>
#include <score/actions/Menu.hpp>
#include <score/actions/MenuManager.hpp>
#include <score/application/ApplicationContext.hpp>
#include <score/model/Identifier.hpp>
#include <score/plugins/application/GUIApplicationPlugin.hpp>
#include <score/tools/IdentifierGeneration.hpp>
#include <score/tools/std/Optional.hpp>

#include <core/application/ApplicationSettings.hpp>
#include <core/command/CommandStack.hpp>
#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>
#include <core/presenter/DocumentManager.hpp>

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QPair>
#include <QTcpSocket>
#include <qnamespace.h>

#include <Network/Client/LocalClient.hpp>
#include <Network/Document/ClientPolicy.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/Execution/MasterPolicy.hpp>
#include <Network/Document/MasterPolicy.hpp>
#include <Network/Group/NetworkActions.hpp>
#include <Network/Group/Panel/GroupPanelDelegate.hpp>
#include <Network/Session/ClientSessionBuilder.hpp>
#include <Network/Session/MasterSession.hpp>
#include <Network/Document/LibraryQueries.hpp>

#include <algorithm>
#include <vector>

#if defined(OSSIA_DNSSD)
#include <Explorer/Widgets/ZeroConf/ZeroconfBrowser.hpp>
#endif

#include <QCommandLineParser>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::NetworkApplicationPlugin)
struct VisitorVariant;

namespace Network
{
class Client;
class Session;

NetworkApplicationPlugin::NetworkApplicationPlugin(
    const score::GUIApplicationContext& app)
    : GUIApplicationPlugin{app}
{
  // Command-line option parsing
  QCommandLineParser parser;

  QCommandLineOption net_join_opt(
      "network-join", QCoreApplication::translate("net", "ip:port"), "Name", "");
  parser.addOption(net_join_opt);
  QCommandLineOption net_host_opt(
      "network-host", QCoreApplication::translate("net", "port"), "Name", "");
  parser.addOption(net_host_opt);
  QCommandLineOption net_terminal_opt(
      "network-terminal",
      QCoreApplication::translate(
          "net", "Join as a terminal: edit and watch the score, but run "
                 "nothing on this machine."));
  parser.addOption(net_terminal_opt);

  parser.parse(app.applicationSettings.arguments);
  if(parser.isSet(net_terminal_opt))
    this->m_arg_role = PeerRole::Terminal;
  this->m_arg_net_join = parser.value(net_join_opt);
  {
    bool ok = false;
    if(int n = this->m_arg_net_join.toInt(&ok); ok && (n < 1 || n > 65535))
      this->m_arg_net_join = QString::number(9090);
  }
  this->m_arg_net_host = parser.value(net_host_opt);
  // FIXME
  // if(!m_arg_net_host.isEmpty() || !m_arg_net_join.isEmpty())
  // {
  //   ((score::ApplicationSettings&)app.applicationSettings).tryToRestore = false;
  // }
}

NetworkApplicationPlugin::~NetworkApplicationPlugin() { }

void NetworkApplicationPlugin::on_createdDocument(score::Document& doc)
{
  if(!m_arg_net_host.isEmpty())
  {
    do_makeServer(doc);
    m_arg_net_host = {};
    m_arg_net_join = {};
    return;
  }
}

void NetworkApplicationPlugin::on_documentChanged(
    score::Document* olddoc, score::Document* newdoc)
{
  if(!newdoc || newdoc->role() != score::DocumentRole::Terminal)
    return;

  auto* plug = newdoc->context().findPlugin<NetworkDocumentPlugin>();
  if(!plug)
    return;

  auto* rpc = plug->rpc();
  auto* session = plug->policy().session();
  if(!rpc || !session)
    return;

  importRemoteLibrary(*rpc, context, session->master().id(), true);
}

bool NetworkApplicationPlugin::handleLoading()
{
  if(!m_arg_net_join.isEmpty())
  {
    QString name, host, ip;
    int port{};

    if(m_arg_net_join.contains("@"))
    {
      auto v = m_arg_net_join.split("@");
      name = v[0];
      host = v[1];
    }
    else
    {
      name = "cmd";
      host = m_arg_net_join;
    }

    auto v = host.split(":");

    if(v.size() >= 1)
      ip = v[0];
    else
      ip = "127.0.0.1";

    if(v.size() >= 2)
      port = v[1].toInt();
    else
      port = 9090;

    m_arg_net_host = {};
    m_arg_net_join = {};
    joinSession(ip, port, m_arg_role);
    return true;
  }
  return false;
}

void NetworkApplicationPlugin::setupClientConnection(
    QString name, QString ip, int port, QMap<QString, QByteArray>)
{
  joinSession(ip, port, PeerRole::Performer);
}

void NetworkApplicationPlugin::joinSession(QString ip, int port, PeerRole role)
{
  m_sessionBuilder = std::make_unique<ClientSessionBuilder>(context, ip, port, role);

  connect(m_sessionBuilder.get(), &ClientSessionBuilder::sessionReady, this, [&]() {
    if(auto panel = context.findPanel<Network::PanelDelegate>())
      panel->networkPluginReady();

    m_sessionBuilder.reset();
  });
  connect(m_sessionBuilder.get(), &ClientSessionBuilder::sessionFailed, this, [&]() {
    m_sessionBuilder.reset();
  });
}

void NetworkApplicationPlugin::setupPlayerConnection(
    QString name, QString ip, int port, QMap<QString, QByteArray>)
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
  connect(s, &QTcpSocket::connected, this, [=] {
    s->write(QString::number(ms->localClient().localPort()).toUtf8());
    // s->deleteLater();
  });
  connect(
      s, qOverload<QAbstractSocket::SocketError>(&QTcpSocket::errorOccurred), this,
      [=](auto) {
    qDebug("Socket error");
    s->deleteLater();
      });
}

void NetworkApplicationPlugin::do_makeServer(score::Document& doc)
{
  const auto& ctx = doc.context();
  NetworkDocumentPlugin* plug = ctx.findPlugin<NetworkDocumentPlugin>();
  qDebug() << Q_FUNC_INFO << (QObject*)plug;
  if(m_arg_net_host.isEmpty())
    m_arg_net_host = "9090";

  if(plug)
  {
    auto clt = new LocalClient(m_arg_net_host.toInt(), Id<Client>(0));
    clt->setName(tr("Master"));
    auto serv = new MasterSession(
        ctx, clt, Id<Session>{score::random_id_generator::getRandomId()});
    auto editpol = new MasterEditionPolicy{serv, ctx};
    plug->setEditPolicy(editpol);
    auto execpol = new MasterExecutionPolicy{*serv, *plug, ctx};
    plug->setExecPolicy(execpol);
  }
  else
  {
    auto clt = new LocalClient(m_arg_net_host.toInt(), Id<Client>(0));
    clt->setName(tr("Master"));
    auto serv = new MasterSession(
        ctx, clt, Id<Session>{score::random_id_generator::getRandomId()});
    auto policy = new MasterEditionPolicy{serv, ctx};
    auto plug = new NetworkDocumentPlugin{ctx, policy, &doc};
    auto execpol = new MasterExecutionPolicy{*serv, *plug, ctx};

    plug->setExecPolicy(execpol);
    doc.model().addPluginModel(plug);
  }

  if(auto panel = context.findPanel<Network::PanelDelegate>())
    panel->networkPluginReady();
}

score::GUIElements NetworkApplicationPlugin::makeGUIElements()
{
  using namespace score;
  QMenu* fileMenu = context.menus.get().at(score::Menus::File()).menu();
  fileMenu->addSeparator();

#ifdef OSSIA_DNSSD
  m_serverBrowser = new ZeroconfBrowser{"_score._tcp", qApp->activeWindow()};
  connect(
      m_serverBrowser, &ZeroconfBrowser::sessionSelected, this,
      &NetworkApplicationPlugin::setupClientConnection);
  auto serveract = m_serverBrowser->makeAction();
  serveract->setText("Browse for server");

  m_playerBrowser = new ZeroconfBrowser{"_score_player._tcp", qApp->activeWindow()};
  connect(
      m_playerBrowser, &ZeroconfBrowser::sessionSelected, this,
      &NetworkApplicationPlugin::setupPlayerConnection);
  auto playeract = m_playerBrowser->makeAction();
  playeract->setText("Browse for players");

  fileMenu->addAction(serveract);
  fileMenu->addAction(playeract);
#endif

  QAction* makeServer = new QAction{tr("Make Server"), this};
#if defined(__EMSCRIPTEN__)
  // A page cannot listen for connections: LocalClient has no server there, and
  // QWebSocketServer does not exist. The web build can join a session, not host
  // one, and saying so beats a menu entry that half-works.
  makeServer->setEnabled(false);
  makeServer->setToolTip(
      tr("A score running in a browser can join a session but cannot host one."));
#else
  connect(makeServer, &QAction::triggered, this, [this] {
    if(auto doc = currentDocument())
      do_makeServer(*doc);
  });
#endif

  fileMenu->addAction(makeServer);

  QAction* connectLocal = new QAction{tr("Join Server"), this};
  connect(connectLocal, &QAction::triggered, this, [this]() {
    // Shown rather than exec()'d: exec() runs a nested event loop, which the
    // browser's main thread cannot provide. There it returned immediately with
    // a rejection, so joining a session from the web build did nothing at all.
    auto* dial = new IpDialog{QApplication::activeWindow()};
    dial->setAttribute(Qt::WA_DeleteOnClose);
    connect(dial, &QDialog::accepted, this, [this, dial] {
      // Default is 127.0.0.1 : 9090
      joinSession(dial->ip(), dial->port(), dial->role());
    });
    dial->open();
  });

  fileMenu->addAction(connectLocal);

  // Execution
  QMenu* playMenu = context.menus.get().at(score::Menus::Play()).menu();

  QAction* playAction = new QAction{tr("Play (network)"), this};
  QAction* stopAction = new QAction{tr("Stop (network)"), this};

  score::GUIElements g;
  g.actions.add<Actions::NetworkPlay>(playAction);
  g.actions.add<Actions::NetworkStop>(stopAction);

  playMenu->addSeparator();
  playMenu->addAction(playAction);
  playMenu->addAction(stopAction);

  return g;
}
}
