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
class QObject;
class QTcpSocket;


namespace Network
{
class Client;
MasterSession::MasterSession(iscore::Document* doc, LocalClient* theclient, Id<Session> id, QObject* parent):
    Session{theclient, id, parent},
    m_document{doc}
{
    con(localClient(), SIGNAL(createNewClient(QTcpSocket*)),
            this, SLOT(on_createNewClient(QTcpSocket*)));

/* TODO Servus
#ifdef USE_ZEROCONF
    auto service = new KDNSSD::PublicService("Session i-score",
                                             "_iscore._tcp",
                                             localClient().localPort());
    service->publishAsync();
#endif
*/
}

void MasterSession::on_createNewClient(QTcpSocket* sock)
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
    addClient(clt);
}
}
