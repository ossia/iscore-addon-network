#include <iscore/tools/std/Optional.hpp>

#if defined(USE_ZEROCONF)
#include <KF5/KDNSSD/DNSSD/PublicService>
#endif

#include <qnamespace.h>

#include "MasterSession.hpp"
#include <Network/Communication/NetworkMessage.hpp>
#include <iscore/model/Identifier.hpp>
#include <iscore/tools/Todo.hpp>
#include <Network/Client/LocalClient.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Session/RemoteClientBuilder.hpp>
#include <Network/Session/Session.hpp>
#include <QHostAddress>
#include <servus/servus.h>
namespace Network
{
class Client;
MasterSession::MasterSession(
    const iscore::DocumentContext& doc,
    LocalClient* theclient,
    Id<Session> id, QObject* parent):
  Session{theclient, id, parent},
  m_ctx{doc}
{
  con(localClient(), &LocalClient::createNewClient,
      this, &MasterSession::on_createNewClient);

#if defined(OSSIA_DNSSD)
    m_dnssd = std::make_unique<servus::Servus>("_iscore._tcp");
    m_dnssd->announce(localClient().localPort(), "i-score master");
#endif
}

MasterSession::~MasterSession()
{

}

void MasterSession::on_createNewClient(QWebSocket* sock)
{
  RemoteClientBuilder* builder = new RemoteClientBuilder(*this, sock);
  connect(builder, &RemoteClientBuilder::clientReady,
          this, &MasterSession::on_clientReady);
  m_clientBuilders.append(builder);
}

void MasterSession::on_clientReady(RemoteClientBuilder* bldr, RemoteClient* clt)
{
  m_clientBuilders.removeOne(bldr);
  delete bldr;

  connect(clt, &RemoteClient::messageReceived,
          this, &Session::validateMessage, Qt::QueuedConnection);
  con(clt->socket().socket(), &QWebSocket::disconnected,
      this, [=] {
    removeClient(clt);
  });

  addClient(clt);
}
}
