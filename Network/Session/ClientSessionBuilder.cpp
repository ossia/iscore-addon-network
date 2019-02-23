#include "ClientSessionBuilder.hpp"

#include "ClientSession.hpp"

#include <score/application/ApplicationContext.hpp>
#include <score/command/Command.hpp>
#include <score/model/Identifier.hpp>
#include <score/plugins/StringFactoryKey.hpp>
#include <score/plugins/documentdelegate/DocumentDelegateFactory.hpp>
#include <score/serialization/DataStreamVisitor.hpp>

#include <core/command/CommandStackSerialization.hpp>
#include <core/document/Document.hpp>
#include <core/presenter/DocumentManager.hpp>
#include <core/presenter/Presenter.hpp>

#include <QDataStream>
#include <QIODevice>
#include <QJsonDocument>
#include <QTcpServer>

#include <Network/Client/LocalClient.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Communication/NetworkMessage.hpp>
#include <Network/Communication/NetworkSocket.hpp>
#include <Network/Document/ClientPolicy.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/Execution/SlavePolicy.hpp>
#include <Network/Document/MasterPolicy.hpp>
#include <sys/types.h>
#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::ClientSessionBuilder)
namespace Network
{
ClientSessionBuilder::ClientSessionBuilder(
    const score::GUIApplicationContext& ctx,
    QString ip,
    int port)
    : m_context{ctx}
{
  m_mastersocket = new NetworkSocket(ip, port, nullptr);
  connect(
      m_mastersocket,
      &NetworkSocket::messageReceived,
      this,
      &ClientSessionBuilder::on_messageReceived);
  connect(
      m_mastersocket,
      &NetworkSocket::connected,
      this,
      &ClientSessionBuilder::connected);
}

void ClientSessionBuilder::initiateConnection()
{
  // Todo only call this if the socket is ready.
  NetworkMessage askId;
  askId.address = MessagesAPI::instance().session_askNewId;
  {
    QDataStream s{&askId.data, QIODevice::WriteOnly};
    s << m_clientName;
  }

  m_mastersocket->sendMessage(askId);
}

ClientSession* ClientSessionBuilder::builtSession() const
{
  return m_session;
}

QByteArray ClientSessionBuilder::documentData() const
{
  return m_documentData;
}

const std::vector<score::CommandData>&
ClientSessionBuilder::commandStackData() const
{
  return m_commandStack;
}

void ClientSessionBuilder::on_messageReceived(const NetworkMessage& m)
{
  auto& mapi = MessagesAPI::instance();
  if (m.address == mapi.session_idOffer)
  {
    m_sessionId = m.sessionId; // The session offered
    m_masterId = m.clientId;   // Message is from the master
    QDataStream s(m.data);
    int32_t id;
    s >> id; // The offered client id
    m_clientId = Id<Client>(id);

    NetworkMessage join;
    join.address = mapi.session_join;
    join.clientId = m_clientId;
    join.sessionId = m.sessionId;

    m_mastersocket->sendMessage(join);
  }
  else if (m.address == mapi.session_document)
  {
    auto remoteClient = new RemoteClient(m_mastersocket, m_masterId);
    remoteClient->setName("RemoteMaster");
    m_session = new ClientSession(
        *remoteClient, new LocalClient(m_clientId), m_sessionId, nullptr);
    m_session->localClient().setName(m_clientName);

    // We start building our document.
    DataStreamWriter writer{m.data};
    writer.m_stream >> m_documentData;

    // The SessionBuilder should have a saved document and saved command list.
    // However there is a difference with what happens when there is a crash :
    // Here the document is sent as it is in its current state. The CommandList
    // only serves in case somebody does undo, so that the computer who joined
    // later can still undo, too.

    score::Document* doc = m_context.docManager.loadDocument(
        m_context,
        "Untitled",
        QJsonDocument::fromBinaryData(m_documentData).object(),
        *m_context.interfaces<score::DocumentDelegateList>()
             .begin()); // TODO id instead

    if (!doc)
    {
      qDebug() << "Invalid document received";
      delete m_session;
      m_session = nullptr;

      sessionFailed();
      return;
    }

    score::loadCommandStack(
        m_context.components, writer, doc->commandStack(), [](auto) {
        }); // No redo.

    auto& ctx = doc->context();
    NetworkDocumentPlugin& np = ctx.plugin<NetworkDocumentPlugin>();
    for (auto e : np.groupManager().groups())
      qDebug() << e->name();
    np.setEditPolicy(new GUIClientEditionPolicy{m_session, ctx});
    np.setExecPolicy(new SlaveExecutionPolicy(*m_session, np, doc->context()));

    // Send a message to the server with the ports that we opened :
    auto& local_server = m_session->localClient().server();
    m_session->master().sendMessage(m_session->makeMessage(
        mapi.session_portinfo,
        local_server.m_localAddress,
        local_server.m_localPort));

    sessionReady();
  }
}
}
