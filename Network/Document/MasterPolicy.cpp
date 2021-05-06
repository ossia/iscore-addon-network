#include <Scenario/Application/ScenarioActions.hpp>

#include <score/actions/ActionManager.hpp>
#include <score/tools/Bind.hpp>

#include <Engine/ApplicationPlugin.hpp>
#include <Network/Communication/MessageMapper.hpp>
#include <Network/Document/Execution/BasicPruner.hpp>
#include <Network/Document/MasterPolicy.hpp>
#include <Network/Group/NetworkActions.hpp>

namespace Network
{

class Client;

MasterEditionPolicy::MasterEditionPolicy(
    MasterSession* s,
    const score::DocumentContext& c)
    : m_session{s}, m_ctx{c}, m_keep{*s}
{
  auto& stack = c.document.commandStack();
  auto& mapi = MessagesAPI::instance();

  /////////////////////////////////////////////////////////////////////////////
  /// From the master to the clients
  /////////////////////////////////////////////////////////////////////////////
  con(stack,
      &score::CommandStack::localCommand,
      this,
      [&](score::Command* cmd) {
        m_session->broadcastToAllClients(m_session->makeMessage(
            mapi.command_new, score::CommandData{*cmd}));
      });

  // Undo-redo
  con(stack, &score::CommandStack::localUndo, this, [&]() {
    m_session->broadcastToAllClients(
        m_session->makeMessage(mapi.command_undo));
  });
  con(stack, &score::CommandStack::localRedo, this, [&]() {
    m_session->broadcastToAllClients(
        m_session->makeMessage(mapi.command_redo));
  });
  con(stack, &score::CommandStack::localIndexChanged, this, [&](int32_t idx) {
    m_session->broadcastToAllClients(
        m_session->makeMessage(mapi.command_index, idx));
  });

  // Lock - unlock
  con(c.objectLocker, &score::ObjectLocker::lock, this, [&](QByteArray arr) {
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.lock, arr));
  });
  con(c.objectLocker, &score::ObjectLocker::unlock, this, [&](QByteArray arr) {
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.unlock, arr));
  });

  // Play
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

  /////////////////////////////////////////////////////////////////////////////
  /// From a client to the master and the other clients
  /////////////////////////////////////////////////////////////////////////////
  s->mapper().addHandler(mapi.command_new, [&](const NetworkMessage& m) {
    score::CommandData cmd;
    DataStreamWriter writer{m.data};
    writer.writeTo(cmd);

    stack.redoAndPushQuiet(m_ctx.app.instantiateUndoCommand(cmd));

    m_session->broadcastToOthers(m.clientId, m);
  });

  // Undo-redo
  s->mapper().addHandler(mapi.command_undo, [&](const NetworkMessage& m) {
    stack.undoQuiet();
    m_session->broadcastToOthers(m.clientId, m);
  });
  s->mapper().addHandler(mapi.command_redo, [&](const NetworkMessage& m) {
    stack.redoQuiet();
    m_session->broadcastToOthers(m.clientId, m);
  });

  s->mapper().addHandler(mapi.command_index, [&](const NetworkMessage& m) {
    QDataStream stream{m.data};
    int32_t idx;
    stream >> idx;
    stack.setIndexQuiet(idx);
    m_session->broadcastToOthers(m.clientId, m);
  });

  // Lock-unlock
  s->mapper().addHandler(mapi.lock, [&](const NetworkMessage& m) {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.objectLocker.on_lock(data);
    m_session->broadcastToOthers(m.clientId, m);
  });

  s->mapper().addHandler(mapi.unlock, [&](const NetworkMessage& m) {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.objectLocker.on_unlock(data);
    m_session->broadcastToOthers(m.clientId, m);
  });

  s->mapper().addHandler(mapi.play, [&](const NetworkMessage& m) {
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.play));
    play();
  });
  s->mapper().addHandler(mapi.stop, [&](const NetworkMessage& m) {
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.stop));
    stop();
  });

  s->mapper().addHandler(mapi.ping, [&](const NetworkMessage& m) {
    qint64 t
        = std::chrono::duration_cast<std::chrono::nanoseconds>(
              std::chrono::high_resolution_clock::now().time_since_epoch())
              .count();
    m_session->sendMessage(m.clientId, m_session->makeMessage(mapi.pong, t));
  });

  s->mapper().addHandler(
      mapi.pong, [&](const NetworkMessage& m) { m_keep.on_pong(m); });

  s->mapper().addHandler(mapi.session_portinfo, [&](const NetworkMessage& m) {
    QString s;
    int p;
    QDataStream stream{m.data};
    stream >> s >> p;

    auto clt = m_session->findClient(m.clientId);
    if (clt)
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
  auto sm = score::IDocument::try_get<Scenario::ScenarioDocumentModel>(
      m_ctx.document);
  if (sm)
  {
    auto& plug = m_ctx.app.guiApplicationPlugin<Engine::ApplicationPlugin>();
    plug.execution().request_play_interval(
        sm->baseInterval(),
        BasicPruner{m_ctx.plugin<NetworkDocumentPlugin>()},
        TimeVal{});
  }
}

void MasterEditionPolicy::stop()
{
  auto sm = score::IDocument::try_get<Scenario::ScenarioDocumentModel>(
      m_ctx.document);
  if (sm)
  {
    auto stop_action = m_ctx.app.actions.action<Actions::Stop>().action();
    stop_action->trigger();
  }
}
}
