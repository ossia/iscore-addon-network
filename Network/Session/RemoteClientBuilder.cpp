#include <Network/Communication/NetworkSocket.hpp>
#include <core/document/Document.hpp>
#include <iscore/tools/IdentifierGeneration.hpp>
#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QList>
#include <QPair>
#include <sys/types.h>

#include "MasterSession.hpp"
#include "RemoteClientBuilder.hpp"
#include <Network/Communication/NetworkMessage.hpp>
#include <core/command/CommandStack.hpp>
#include <iscore/command/Command.hpp>
#include <iscore/command/CommandData.hpp>
#include <iscore/plugins/customfactory/StringFactoryKey.hpp>
#include <iscore/serialization/DataStreamVisitor.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <Network/Client/LocalClient.hpp>
#include <Network/Client/RemoteClient.hpp>


namespace Network
{
class Client;

RemoteClientBuilder::RemoteClientBuilder(MasterSession& session, QTcpSocket* sock):
  m_session{session}
{
  m_socket = new NetworkSocket(sock, nullptr);
  connect(m_socket, SIGNAL(messageReceived(NetworkMessage)),
          this, SLOT(on_messageReceived(NetworkMessage)));
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
      int32_t id = iscore::random_id_generator::getRandomId();
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
    DataStreamReader vr{&doc.data};
    vr.m_stream << m_session.document()->saveAsByteArray();
    vr.readFrom(m_session.document()->commandStack());

    m_socket->sendMessage(doc);

    m_remoteClient = new RemoteClient(m_socket, m_clientId);
    m_remoteClient->setName(m_clientName);
    emit clientReady(this, m_remoteClient);
  }
}
}
