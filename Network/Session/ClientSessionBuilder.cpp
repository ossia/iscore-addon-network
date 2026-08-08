#include "ClientSessionBuilder.hpp"

#include "ClientSession.hpp"

#include <score/model/Identifier.hpp>
#include <score/plugins/StringFactoryKey.hpp>
#include <score/plugins/documentdelegate/DocumentDelegateFactory.hpp>
#include <score/serialization/DataStreamVisitor.hpp>

#include <core/command/CommandStackSerialization.hpp>
#include <core/document/Document.hpp>

#include <memory>
#include <core/presenter/DocumentManager.hpp>
#include <core/application/ApplicationSettings.hpp>
#include <core/presenter/Presenter.hpp>

#include <QDataStream>
#include <QIODevice>
#include <QJsonDocument>

#include <Network/Client/LocalClient.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Communication/NetworkMessage.hpp>
#include <Network/Communication/NetworkSocket.hpp>
#include <Network/Document/ClientPolicy.hpp>
#include <Network/Document/DocumentPlugin.hpp>
#include <Network/Document/RemoteEnvironment.hpp>
#include <Network/Document/LibraryQueries.hpp>
#include <Network/Document/DeviceStatus.hpp>
#include <Network/Document/RemoteDeviceCatalog.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Network/Document/Execution/SlavePolicy.hpp>
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Communication/Capabilities.hpp>
#include <Network/Settings/NetworkSettingsModel.hpp>
#include <sys/types.h>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Network::ClientSessionBuilder)
namespace Network
{
ClientSessionBuilder::ClientSessionBuilder(
    const score::GUIApplicationContext& ctx, QString ip, int port, PeerRole role)
    : m_context{ctx}
    , m_role{role}
{
  m_mastersocket = new NetworkSocket(ip, port, nullptr);
  connect(
      m_mastersocket, &NetworkSocket::messageReceived, this,
      &ClientSessionBuilder::on_messageReceived);
  // The only thing anyone ever did on connection.
  connect(
      m_mastersocket, &NetworkSocket::connectionFailed, this,
      [this](const QUrl& url, const QString& reason) { connectionFailed(url, reason); });
  connect(m_mastersocket, &NetworkSocket::connected, this, [this] {
    initiateConnection();
    connected();
  });
}

void ClientSessionBuilder::initiateConnection()
{
  // Todo only call this if the socket is ready.
  NetworkMessage askId;
  askId.address = MessagesAPI::instance().session_askNewId;
  {
    QDataStream s{&askId.data, QIODevice::WriteOnly};
    s << m_clientName;
    s << (qint32)m_context.applicationSettings.saveFormatVersion.value();
    s << (qint32)QDataStream::Qt_DefaultCompiledVersion;
    s << Capabilities::local(m_context);
    s << int32_t(m_role);
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

const std::vector<score::CommandData>& ClientSessionBuilder::commandStackData() const
{
  return m_commandStack;
}

void ClientSessionBuilder::on_messageReceived(const NetworkMessage& m)
{
  auto& mapi = MessagesAPI::instance();
  if(m.address == mapi.session_rejected)
  {
    QDataStream s{m.data};
    QString reason;
    s >> reason;
    qWarning() << "The session refused us:" << reason;
    sessionFailed();
    return;
  }
  else if(m.address == mapi.session_idOffer)
  {
    m_sessionId = m.sessionId; // The session offered
    m_masterId = m.clientId;   // Message is from the master
    QDataStream s(m.data);
    int32_t id;
    s >> id; // The offered client id
    m_clientId = Id<Client>(id);

    // What the host can make that we cannot: why their processes show as
    // stand-ins here, and which protocols to offer.
    if(!s.atEnd())
    {
      s >> m_masterCapabilities;
      if(auto missing = Capabilities::local(m_context).lacking(m_masterCapabilities);
         !missing.isEmpty())
      {
        qDebug() << "The host can do things this build cannot:" << missing.summary();
      }
    }

    // The host decides; one too old to answer runs a session of performers.
    if(!s.atEnd())
    {
      int32_t confirmed{};
      s >> confirmed;
      m_role = (confirmed == int32_t(PeerRole::Terminal)) ? PeerRole::Terminal
                                                          : PeerRole::Performer;
    }
    else
    {
      m_role = PeerRole::Performer;
    }

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
    m_session = new ClientSession(
        *remoteClient,
        new LocalClient(
            m_context.settings<Network::Settings::Model>().getClientPort(), m_clientId),
        m_sessionId, nullptr);
    m_session->localClient().setName(m_clientName);

    m_sessionMessage = m.data;

    // Off the socket callback: loading spins the event loop, which a browser
    // cannot do from a WebSocket handler.
    QMetaObject::invokeMethod(this, [this] { buildDocument(); }, Qt::QueuedConnection);
  }
}

void ClientSessionBuilder::buildDocument()
{
  auto& mapi = MessagesAPI::instance();

  // The command stack follows the document in the same message, so both are
  // read from the one stream.
  DataStreamWriter writer{m_sessionMessage};
  writer.m_stream >> m_documentData;

  // The SessionBuilder should have a saved document and saved command list.
  // However there is a difference with what happens when there is a crash :
  // Here the document is sent as it is in its current state. The CommandList
  // only serves in case somebody does undo, so that the computer who joined
  // later can still undo, too.

  const auto docRole = (m_role == PeerRole::Terminal) ? score::DocumentRole::Terminal
                                                      : score::DocumentRole::Local;

  score::Document* doc = m_context.docManager.loadDocument(
      m_context, "Untitled", m_documentData, JSONObject::type(),
      *m_context.interfaces<score::DocumentDelegateList>().begin(),
      docRole); // TODO id instead

  if(!doc)
  {
    qDebug() << "Invalid document received";
    delete m_session;
    m_session = nullptr;

    sessionFailed();
    return;
  }

  score::loadCommandStack(m_context.components, writer, doc->commandStack(), [](auto) {
    return true;
  }); // No redo.

  auto& ctx = doc->context();
  NetworkDocumentPlugin& np = ctx.plugin<NetworkDocumentPlugin>();
  np.setRemoteCapabilities(m_masterCapabilities);

  qDebug().noquote() << "Joined the session as"
                     << (m_role == PeerRole::Terminal
                             ? "a terminal: the score runs on the host, nothing "
                               "is opened or played here"
                             : "a performer: the score runs here too");

  if(m_role == PeerRole::Terminal)
  {
    // No execution policy: it carries netpit traffic for processes running
    // here, and none do.
    m_session->localClient().setRole(PeerRole::Terminal);
    np.setEditPolicy(new TerminalEditionPolicy{m_session, ctx});

    // What may be added is what the machine running the score has.
    if(auto* rpc = np.rpc())
    {
      // Parented to the plug-in: this builder does not outlive the session.
      auto* catalog = new RemoteDeviceCatalog{*rpc, m_masterId, &np};
      ctx.plugin<Explorer::DeviceDocumentPlugin>().setCatalog(catalog);
    }
  }
  else
  {
    np.setEditPolicy(new GUIClientEditionPolicy{m_session, ctx});
    np.setExecPolicy(new SlaveExecutionPolicy(*m_session, np, doc->context()));
  }

  // Here as well as on_documentChanged, which fires before setEditPolicy has
  // given the plug-in a channel to ask over.
  if(auto* rpc = np.rpc())
  {
    importRemoteLibrary(
        *rpc, m_context, m_masterId, m_role == PeerRole::Terminal);

    // Asked, not waited for: a push would arrive before we could receive it.
    requestDeviceStatus(*rpc, ctx, m_masterId);
  }

  // After setEditPolicy: the files belong to the machine that sent the score.
  if(auto* rpc = np.rpc())
    doc->setEnvironment(std::make_unique<RemoteEnvironment>(*rpc, m_masterId));

  // Send a message to the server with the ports that we opened :
  if(auto local_server = m_session->localClient().server())
  {
    m_session->master().sendMessage(m_session->makeMessage(
        mapi.session_portinfo, local_server->m_localAddress,
        local_server->m_localPort));
  }
  else
  {
    m_session->master().sendMessage(m_session->makeMessage(
        mapi.session_portinfo, QString("__web_client__"), 554433));
  }
  sessionReady();
}
}
