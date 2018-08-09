#include <score/tools/std/Optional.hpp>

#include <qnamespace.h>

#include "MasterSession.hpp"
#include <Network/Communication/NetworkMessage.hpp>
#include <score/model/Identifier.hpp>
#include <score/tools/Todo.hpp>
#include <Network/Client/LocalClient.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Session/RemoteClientBuilder.hpp>
#include <Network/Session/Session.hpp>
#include <QHostAddress>
#undef OSSIA_DNSSD
#if defined(OSSIA_DNSSD)
#include <servus/servus.h>
#endif

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::MasterSession)
namespace Network
{
class Client;
MasterSession::MasterSession(
    const score::DocumentContext& doc,
    LocalClient* theclient,
    Id<Session> id, QObject* parent):
  Session{theclient, id, parent},
  m_ctx{doc}
{
  con(localClient(), &LocalClient::createNewClient,
      this, &MasterSession::on_createNewClient);

#if defined(OSSIA_DNSSD)
    m_dnssd = std::make_unique<servus::Servus>("_score._tcp");
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
