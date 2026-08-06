#include "RemoteClientBuilder.hpp"

#include "MasterSession.hpp"

#include <score/command/Command.hpp>
#include <score/command/CommandData.hpp>
#include <score/plugins/StringFactoryKey.hpp>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>
#include <score/application/ApplicationContext.hpp>
#include <score/tools/IdentifierGeneration.hpp>

#include <core/application/ApplicationSettings.hpp>

#include <core/command/CommandStack.hpp>
#include <core/document/Document.hpp>

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QJsonDocument>
#include <QList>
#include <QPair>

#include <Network/Client/LocalClient.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Communication/Capabilities.hpp>
#include <Network/Communication/NetworkMessage.hpp>
#include <Network/Communication/NetworkSocket.hpp>
#include <Network/Document/Execution/SyncMode.hpp>
#include <sys/types.h>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::RemoteClientBuilder)

namespace Network
{
class Client;

namespace
{
//! Empty when the joining peer is compatible with us, otherwise why not.
QString incompatibility(QDataStream& s)
{
  if(s.atEnd())
    return QObject::tr(
        "it runs a version of score too old to say which formats it speaks");

  qint32 saveFormat{}, streamVersion{};
  s >> saveFormat >> streamVersion;

  const auto ourSaveFormat
      = score::AppContext().applicationSettings.saveFormatVersion.value();
  if(saveFormat != ourSaveFormat)
    return QObject::tr("it uses document format %1, we use %2")
        .arg(saveFormat)
        .arg(ourSaveFormat);

  if(streamVersion != QDataStream::Qt_DefaultCompiledVersion)
    return QObject::tr("it encodes commands with Qt stream version %1, we use %2")
        .arg(streamVersion)
        .arg((int)QDataStream::Qt_DefaultCompiledVersion);

  return {};
}
}

RemoteClientBuilder::RemoteClientBuilder(MasterSession& session, QWebSocket* sock)
    : m_session{session}
{
  m_socket = new NetworkSocket(sock, nullptr);
  connect(
      m_socket, &NetworkSocket::messageReceived, this,
      &RemoteClientBuilder::on_messageReceived);
}

void RemoteClientBuilder::on_messageReceived(const NetworkMessage& m)
{
  auto& mapi = MessagesAPI::instance();
  if(m.address == mapi.session_askNewId)
  {
    QDataStream s{m.data};
    s >> m_clientName;

    // Peers exchange commands as raw QDataStream payloads and mirror each
    // other's document, so both ends have to agree on how those are encoded
    // and on what the model means. Neither is negotiable after the fact: a
    // mismatch does not fail loudly, it reads the wrong bytes into the right
    // fields. Refuse the join instead.
    if(auto reason = incompatibility(s); !reason.isEmpty())
    {
      NetworkMessage rejected;
      rejected.address = mapi.session_rejected;
      rejected.sessionId = m_session.id();
      rejected.clientId = m_session.localClient().id();
      {
        QDataStream stream(&rejected.data, QIODevice::WriteOnly);
        stream << reason;
      }
      qWarning() << "Refused a client:" << reason;
      m_socket->sendMessage(rejected);
      m_refused = true;
      return;
    }

    NetworkMessage idOffer;
    idOffer.address = mapi.session_idOffer;
    idOffer.sessionId = m_session.id();
    idOffer.clientId = m_session.localClient().id();
    {
      QDataStream stream(&idOffer.data, QIODevice::WriteOnly);

      // TODO make a strong id with the client array!!!!!!
      int32_t id = score::random_id_generator::getRandomId();
      m_clientId = Id<Client>(id);
      m_offered = true;
      stream << id;
      stream << Capabilities::local(score::AppContext());
    }

    if(!s.atEnd())
    {
      Capabilities theirs;
      s >> theirs;
      if(auto missing = theirs.lacking(Capabilities::local(score::AppContext()));
         !missing.isEmpty())
      {
        qDebug() << "Client" << m_clientName
                 << "cannot construct everything this session uses:"
                 << missing.summary();
      }
    }

    m_socket->sendMessage(idOffer);
  }
  else if(m.address == mapi.session_join)
  {
    // Refusing a client only means anything if it cannot then help itself to
    // the document: nothing obliged it to ask for an id first, or to stop
    // after being told no, and joining twice made two clients on one socket.
    if(m_refused || !m_offered || m_joined)
    {
      qWarning() << "Ignoring a join from a client that was not admitted";
      return;
    }
    m_joined = true;

    NetworkMessage doc;
    doc.address = mapi.session_document;

    // Data is the serialized command stack, and the document models.
    {
      DataStreamReader vr{&doc.data};
      JSONObject::Serializer wr{};
      m_session.document().document.saveAsJson(wr);
      vr.m_stream << wr.toByteArray();
      vr.readFrom(m_session.document().document.commandStack());
    }

    m_socket->sendMessage(doc);

    m_remoteClient = new RemoteClient(m_socket, m_clientId);
    m_remoteClient->setName(m_clientName);
    clientReady(this, m_remoteClient);
  }
}
}
