#include <Scenario/Application/ScenarioActions.hpp>

#include <Engine/ApplicationPlugin.hpp>

#include <score/actions/ActionManager.hpp>
#include <score/tools/Bind.hpp>

#include <core/command/CommandStack.hpp>
#include <core/document/Document.hpp>

#include <Netpit/Netpit.hpp>
#include <Network/Communication/MessageMapper.hpp>
#include <Network/Document/Execution/BasicPruner.hpp>
#include <Network/Document/MasterPolicy.hpp>
#include <Network/Document/RemoteCommand.hpp>
#include <Network/Document/Transport.hpp>
#include <Network/Document/DeviceStatus.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Group/NetworkActions.hpp>

namespace Network
{

class Client;

MasterEditionPolicy::MasterEditionPolicy(
    MasterSession* s, const score::DocumentContext& c)
    : m_session{s}
    , m_ctx{c}
    , m_keep{*s}
{
  auto& stack = c.document.commandStack();
  auto& mapi = MessagesAPI::instance();

  // Peers that do not execute have no other way to know where the score is,
  // nor whether the devices they can see are connected.
  bindTransportBroadcast(*this, *m_session, m_ctx);
  bindDeviceStatusBroadcast(*this, *m_session, m_ctx);

  // A peer joining mid-session would otherwise see nothing until something
  // happened to change.
  con(*s, &Session::clientAdded, this,
      [this](RemoteClient*) { broadcastAllDeviceStatus(*m_session, m_ctx); });

  /////////////////////////////////////////////////////////////////////////////
  /// From the master to the clients
  /////////////////////////////////////////////////////////////////////////////
  con(stack, &score::CommandStack::localCommand, this, [&](score::Command* cmd) {
    using namespace std::literals;
    if(!this->sendCommands())
      return;
    if(this->sendControls()
       || (!this->sendControls() && cmd->key().toString() != "SetControlValue"sv))
      m_session->broadcastToAllClients(
          m_session->makeMessage(mapi.command_new, score::CommandData{*cmd}));
  });

  // Undo-redo
  con(stack, &score::CommandStack::localUndo, this, [&]() {
    if(!this->sendCommands())
      return;
    // FIXME if it's a control which wasn't sent, we mustn't send its undo either
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.command_undo));
  });
  con(stack, &score::CommandStack::localRedo, this, [&]() {
    if(!this->sendCommands())
      return;
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.command_redo));
  });
  con(stack, &score::CommandStack::localIndexChanged, this, [&](int32_t idx) {
    if(!this->sendCommands())
      return;
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.command_index, idx));
  });

  // Lock - unlock
  con(c.objectLocker, &score::ObjectLocker::lock, this, [&](QByteArray arr) {
    if(!this->sendCommands())
      return;
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.lock, arr));
  });
  con(c.objectLocker, &score::ObjectLocker::unlock, this, [&](QByteArray arr) {
    if(!this->sendCommands())
      return;
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.unlock, arr));
  });

  // Play
  if(c.app.mainWindow)
  {
    auto& play_act = c.app.actions.action<Actions::NetworkPlay>();
    connect(play_act.action(), &QAction::triggered, this, [&] {
      m_session->broadcastToAllClients(m_session->makeMessage(mapi.play));
      play();
    });
    auto& stop_act = c.app.actions.action<Actions::NetworkStop>();
    connect(stop_act.action(), &QAction::triggered, this, [&] {
      m_session->broadcastToAllClients(m_session->makeMessage(mapi.stop));
      stop();
    });
  }

  /////////////////////////////////////////////////////////////////////////////
  /// From a client to the master and the other clients
  /////////////////////////////////////////////////////////////////////////////
  s->mapper().addHandler(this, mapi.command_new, [&](const NetworkMessage& m) {
    if(applyRemoteCommand(m_ctx, m.data, OnCommandFailure::Decline))
    {
      m_session->broadcastToOthers(m.clientId, m);
      return;
    }

    // Not broadcast: the master and the other clients all stay on the state
    // before this command, so the only peer out of sync is the one that sent
    // it -- it applied the command locally before telling us about it. Say so,
    // so that it stops sending edits built on a model we do not share.
    m_session->sendMessage(
        m.clientId, m_session->makeMessage(mapi.command_rejected));
  });

  // Undo-redo
  // Movements of the stack are as much a message from the wire as a command
  // is. undoQuiet pops whether or not there is anything to pop, and
  // setIndexQuiet walks toward whatever number it is handed.
  s->mapper().addHandler(this, mapi.command_undo, [&](const NetworkMessage& m) {
    if(stack.canUndo())
    {
      stack.undoQuiet();
      m_session->broadcastToOthers(m.clientId, m);
    }
  });
  s->mapper().addHandler(this, mapi.command_redo, [&](const NetworkMessage& m) {
    if(stack.canRedo())
    {
      stack.redoQuiet();
      m_session->broadcastToOthers(m.clientId, m);
    }
  });

  s->mapper().addHandler(this, mapi.command_index, [&](const NetworkMessage& m) {
    QDataStream stream{m.data};
    int32_t idx{};
    stream >> idx;
    if(idx >= 0 && idx <= stack.size())
    {
      stack.setIndexQuiet(idx);
      m_session->broadcastToOthers(m.clientId, m);
    }
  });

  // Lock-unlock
  s->mapper().addHandler(this, mapi.lock, [&](const NetworkMessage& m) {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.objectLocker.on_lock(data);
    m_session->broadcastToOthers(m.clientId, m);
  });

  s->mapper().addHandler(this, mapi.unlock, [&](const NetworkMessage& m) {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.objectLocker.on_unlock(data);
    m_session->broadcastToOthers(m.clientId, m);
  });

  s->mapper().addHandler(this, mapi.play, [&](const NetworkMessage& m) {
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.play));
    play();
  });
  s->mapper().addHandler(this, mapi.stop, [&](const NetworkMessage& m) {
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.stop));
    stop();
  });

  s->mapper().addHandler(this, mapi.ping, [&](const NetworkMessage& m) {
    qint64 t = std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::high_resolution_clock::now().time_since_epoch())
                   .count();
    m_session->sendMessage(m.clientId, m_session->makeMessage(mapi.pong, t));
  });

  s->mapper().addHandler(this, mapi.pong, [&](const NetworkMessage& m) { m_keep.on_pong(m); });

  s->mapper().addHandler(this, mapi.session_portinfo, [&](const NetworkMessage& m) {
    QString s;
    int p;
    QDataStream stream{m.data};
    stream >> s >> p;

    auto clt = m_session->findClient(m.clientId);
    if(clt)
    {
      clt->m_clientServerAddress = s;
      clt->m_clientServerPort = p;
    }
    qDebug() << "REMOTE CLIENT IP" << s << p;
    m_session->broadcastToOthers(m.clientId, m);
  });
}

void MasterEditionPolicy::play()
{
  auto sm = score::IDocument::try_get<Scenario::ScenarioDocumentModel>(m_ctx.document);
  if(sm)
  {
    Netpit::setCurrentDocument(m_ctx);
    auto& plug = m_ctx.app.guiApplicationPlugin<Engine::ApplicationPlugin>();
    plug.execution().request_play_interval(
        sm->baseInterval(), BasicPruner{m_ctx.plugin<NetworkDocumentPlugin>()},
        TimeVal{});
  }
}

void MasterEditionPolicy::stop()
{
  auto sm = score::IDocument::try_get<Scenario::ScenarioDocumentModel>(m_ctx.document);
  if(sm)
  {
    auto stop_action = m_ctx.app.actions.action<Actions::Stop>().action();
    stop_action->trigger();
  }
}
}
