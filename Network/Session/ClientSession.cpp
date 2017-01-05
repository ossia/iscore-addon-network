#include <iscore/tools/std/Optional.hpp>
#include <qnamespace.h>

#include "ClientSession.hpp"
#include <iscore/model/Identifier.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Session/Session.hpp>

class QObject;

namespace Network
{
class LocalClient;
ClientSession::ClientSession(RemoteClient* master,
                             LocalClient* client,
                             Id<Session> id,
                             QObject* parent):
    Session{client, id, parent},
    m_master{master}
{
    addClient(master);

    con(localClient(), SIGNAL(createNewClient(QTcpSocket*)),
            this, SLOT(on_createNewClient(QTcpSocket*)));

    connect(master, &RemoteClient::messageReceived,
            this, &Session::validateMessage, Qt::QueuedConnection);
}
}
