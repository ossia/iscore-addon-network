#include <Network/Communication/NetworkSocket.hpp>
#include <core/document/Document.hpp>
#include <score/tools/IdentifierGeneration.hpp>
#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QList>
#include <QPair>
#include <QJsonDocument>
#include <sys/types.h>

#include "MasterSession.hpp"
#include "RemoteClientBuilder.hpp"
#include <Network/Communication/NetworkMessage.hpp>
#include <core/command/CommandStack.hpp>
#include <score/command/Command.hpp>
#include <score/command/CommandData.hpp>
#include <score/plugins/customfactory/StringFactoryKey.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Client/LocalClient.hpp>
#include <Network/Client/RemoteClient.hpp>


namespace Network
{
class Client;

RemoteClientBuilder::RemoteClientBuilder(MasterSession& session, QWebSocket* sock):
  m_session{session}
{
  m_socket = new NetworkSocket(sock, nullptr);
  connect(m_socket, &NetworkSocket::messageReceived,
          this, &RemoteClientBuilder::on_messageReceived);
}

void RemoteClientBuilder::on_messageReceived(const NetworkMessage& m)
{
  auto& mapi = MessagesAPI::instance();
  if(m.address == mapi.session_askNewId)
  {
    QDataStream s{m.data};
    s >> m_clientName;

    // TODO validation
    NetworkMessage idOffer;
    idOffer.address = mapi.session_idOffer;
    idOffer.sessionId = m_session.id();
    idOffer.clientId = m_session.localClient().id();
    {
      QDataStream stream(&idOffer.data, QIODevice::WriteOnly);

      // TODO make a strong id with the client array!!!!!!
      int32_t id = score::random_id_generator::getRandomId();
      m_clientId = Id<Client>(id);
      stream << id;
    }

    m_socket->sendMessage(idOffer);

  }
  else if(m.address == mapi.session_join)
  {
    // TODO validation
    NetworkMessage doc;
    doc.address = mapi.session_document;

    // Data is the serialized command stack, and the document models.
    {
    DataStreamReader vr{&doc.data};
    vr.m_stream << QJsonDocument(m_session.document().document.saveAsJson()).toBinaryData();
    vr.readFrom(m_session.document().document.commandStack());
    }

    m_socket->sendMessage(doc);

    m_remoteClient = new RemoteClient(m_socket, m_clientId);
    m_remoteClient->setName(m_clientName);
    emit clientReady(this, m_remoteClient);
  }
}
}
