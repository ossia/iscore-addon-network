#include <score/serialization/DataStreamVisitor.hpp>
#include <QDataStream>
#include <QIODevice>
#include <sys/types.h>
#include <Network/Document/MasterPolicy.hpp>
#include <Network/Document/Execution/SlavePolicy.hpp>

#include "ClientSession.hpp"
#include "PlayerSessionBuilder.hpp"
#include <Network/Communication/NetworkMessage.hpp>
#include <Network/Communication/NetworkSocket.hpp>
#include <score/command/Command.hpp>
#include <score/plugins/customfactory/StringFactoryKey.hpp>
#include <score/model/Identifier.hpp>
#include <core/document/Document.hpp>
#include <core/command/CommandStackSerialization.hpp>
#include <Network/Client/LocalClient.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/ClientPolicy.hpp>
#include <QTcpServer>
#include <score/plugins/documentdelegate/DocumentDelegateFactory.hpp>
#include <score/application/ApplicationContext.hpp>
#include <Network/Settings/NetworkSettingsModel.hpp>
namespace Network
{
PlayerSessionBuilder::PlayerSessionBuilder(
    const score::ApplicationContext& ctx,
    QString ip,
    int port):
  m_context{ctx}
{
  qDebug() << "PlayerSessionBuilder(): " << ip << port;
  m_mastersocket = new NetworkSocket(ip, port, nullptr);
  connect(m_mastersocket, &NetworkSocket::messageReceived,
          this, &PlayerSessionBuilder::on_messageReceived);
  connect(m_mastersocket, &NetworkSocket::connected,
          this, &PlayerSessionBuilder::connected);
}

void PlayerSessionBuilder::initiateConnection()
{
  // Todo only call this if the socket is ready.
  NetworkMessage askId;
  askId.address = MessagesAPI::instance().session_askNewId;
  {
    QDataStream s{&askId.data, QIODevice::WriteOnly};
    s << m_context.settings<Network::Settings::Model>().getClientName();
  }

  m_mastersocket->sendMessage(askId);
}

ClientSession*PlayerSessionBuilder::builtSession() const
{
  return m_session;
}

QByteArray PlayerSessionBuilder::documentData() const
{
  return m_documentData;
}

const std::vector<score::CommandData>& PlayerSessionBuilder::commandStackData() const
{
  return m_commandStack;
}

void PlayerSessionBuilder::on_messageReceived(const NetworkMessage& m)
{
  auto& mapi = MessagesAPI::instance();
  if(m.address == mapi.session_idOffer)
  {
    m_sessionId = m.sessionId; // The session offered
    m_masterId = m.clientId; // Message is from the master
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
  else if(m.address == mapi.session_document)
  {
    auto remoteClient = new RemoteClient(m_mastersocket, m_masterId);
    remoteClient->setName("RemoteMaster");
    m_session = new ClientSession(*remoteClient,
                                  new LocalClient(m_clientId),
                                  m_sessionId,
                                  nullptr);
    m_session->localClient().setName(m_clientName);

    // We start building our document.
    DataStreamWriter writer{m.data};
    writer.m_stream >> m_documentData;

    if(!documentLoader)
    {
        qDebug() << "Cannot load document";
        delete m_session;
        m_session = nullptr;

        emit sessionFailed();
        return;
    }

    score::Document* doc = documentLoader(m_documentData);

    if(!doc)
    {
      qDebug() << "Invalid document received";
      delete m_session;
      m_session = nullptr;

      emit sessionFailed();
      return;
    }

    score::loadCommandStack(
          m_context.components,
          writer,
          doc->commandStack(),
          [] (auto) { });
// No redo.
    auto& ctx = doc->context();
    NetworkDocumentPlugin& np = ctx.plugin<NetworkDocumentPlugin>();
    np.setEditPolicy(new PlayerClientEditionPolicy{m_session, ctx});
    np.setExecPolicy(new SlaveExecutionPolicy{*m_session, np, doc->context()});

    // Send a message to the server with the ports that we opened :
    auto& local_server = m_session->localClient().server();
    m_session->master().sendMessage(
          m_session->makeMessage(mapi.session_portinfo, local_server.m_localAddress, local_server.m_localPort));

    emit sessionReady();
  }
}
}
