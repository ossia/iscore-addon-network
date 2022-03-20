#include "ClientSession.hpp"

#include <score/model/Identifier.hpp>
#include <score/tools/Bind.hpp>

#include <qnamespace.h>

#include <Network/Client/RemoteClient.hpp>
#include <Network/Session/Session.hpp>

class QObject;

namespace Network
{
class LocalClient;
ClientSession::ClientSession(
    RemoteClient& master,
    LocalClient* client,
    Id<Session> id,
    QObject* parent)
    : Session{client, id, parent}, m_master{master}
{
  addClient(&master);

  con(localClient(),
      &LocalClient::createNewClient,
      this,
      &ClientSession::on_createNewClient);

  con(master,
      &RemoteClient::messageReceived,
      this,
      &Session::validateMessage,
      Qt::QueuedConnection);
}

void ClientSession::on_createNewClient(QWebSocket*)
{
  qDebug() << "createNewClient";
}
}
