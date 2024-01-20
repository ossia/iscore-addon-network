#include <Scenario/Application/ScenarioActions.hpp>

#include <Engine/ApplicationPlugin.hpp>

#include <score/actions/ActionManager.hpp>
#include <score/tools/Bind.hpp>

#include <core/command/CommandStack.hpp>
#include <core/document/Document.hpp>

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Document/ClientPolicy.hpp>
#include <Network/Document/Execution/BasicPruner.hpp>
#include <Network/Group/NetworkActions.hpp>
namespace Network
{
ClientEditionPolicy::ClientEditionPolicy(
    ClientSession* s, const score::DocumentContext& c)
    : EditionPolicy{c}
    , m_session{s}
    , m_keep{*s}
{
  auto& stack = c.document.commandStack();
  auto& locker = c.document.locker();
  auto& mapi = MessagesAPI::instance();
  /////////////////////////////////////////////////////////////////////////////
  /// To the master
  /////////////////////////////////////////////////////////////////////////////
  con(stack, &score::CommandStack::localCommand, this, [&](score::Command* cmd) {
    if(canSendCommand(cmd))
      m_session->master().sendMessage(
          m_session->makeMessage(mapi.command_new, score::CommandData{*cmd}));
  });

  // Undo-redo
  con(stack, &score::CommandStack::localUndo, this, [&]() {
    if(!this->sendCommands())
      return;
    m_session->master().sendMessage(m_session->makeMessage(mapi.command_undo));
  });
  con(stack, &score::CommandStack::localRedo, this, [&]() {
    if(!this->sendCommands())
      return;
    m_session->master().sendMessage(m_session->makeMessage(mapi.command_redo));
  });
  con(stack, &score::CommandStack::localIndexChanged, this, [&](int32_t idx) {
    if(!this->sendCommands())
      return;
    m_session->master().sendMessage(m_session->makeMessage(mapi.command_index, idx));
  });

  // Lock-unlock
  con(locker, &score::ObjectLocker::lock, this, [&](QByteArray arr) {
    qDebug() << "client send lock";
    if(!this->sendCommands())
      return;
    m_session->master().sendMessage(m_session->makeMessage(mapi.lock, arr));
  });
  con(locker, &score::ObjectLocker::unlock, this, [&](QByteArray arr) {
    qDebug() << "client send unlock";
    if(!this->sendCommands())
      return;
    m_session->master().sendMessage(m_session->makeMessage(mapi.unlock, arr));
  });

  /////////////////////////////////////////////////////////////////////////////
  /// From the master
  /////////////////////////////////////////////////////////////////////////////
  // - command comes from the master
  //   -> apply it to the computer only
  s->mapper().addHandler(mapi.command_new, [&](const NetworkMessage& m) {
    score::CommandData cmd;
    DataStreamWriter writer{m.data};
    writer.writeTo(cmd);

    m_ctx.document.commandStack().redoAndPushQuiet(
        m_ctx.app.instantiateUndoCommand(cmd));
  });

  s->mapper().addHandler(mapi.command_undo, [&](const NetworkMessage&) {
    m_ctx.document.commandStack().undoQuiet();
  });

  s->mapper().addHandler(mapi.command_redo, [&](const NetworkMessage&) {
    m_ctx.document.commandStack().redoQuiet();
  });

  s->mapper().addHandler(mapi.command_index, [&](const NetworkMessage& m) {
    QDataStream stream{m.data};
    int32_t idx;
    stream >> idx;
    m_ctx.document.commandStack().setIndexQuiet(idx);
  });

  s->mapper().addHandler(mapi.lock, [&](const NetworkMessage& m) {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.document.locker().on_lock(data);
  });

  s->mapper().addHandler(mapi.unlock, [&](const NetworkMessage& m) {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.document.locker().on_unlock(data);
  });

  s->mapper().addHandler(mapi.play, [&](const NetworkMessage& m) { play(); });
  s->mapper().addHandler(mapi.stop, [&](const NetworkMessage& m) { stop(); });

  s->mapper().addHandler(mapi.ping, [&](const NetworkMessage& m) {
    qint64 t = std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::high_resolution_clock::now().time_since_epoch())
                   .count();
    m_session->sendMessage(m.clientId, m_session->makeMessage(mapi.pong, t));
  });

  s->mapper().addHandler(mapi.pong, [&](const NetworkMessage& m) { m_keep.on_pong(m); });

  s->mapper().addHandler(mapi.session_portinfo, [&](const NetworkMessage& m) {
    QString name, ip;
    int port;
    QDataStream stream{m.data};
    stream >> name >> ip >> port;

    auto clt = m_session->findClient(m.clientId);
    qDebug() << "(client) REMOTE CLIENT IP" << name << ip << port << (bool)clt
             << m.clientId.val();
    if(clt)
    {
      clt->setName(name);
      clt->m_clientServerAddress = ip;
      clt->m_clientServerPort = port;
    }
    else
    {
      connectToOtherClient(ip, port);
      auto sock = new NetworkSocket{ip, port, nullptr};
      auto clt = new RemoteClient{sock, m.clientId};
      clt->setName(name);

      m_session->addClient(clt);
    }
  });
}

void ClientEditionPolicy::connectToOtherClient(QString ip, int port)
{
  SCORE_TODO;
}

GUIClientEditionPolicy::GUIClientEditionPolicy(
    ClientSession* s, const score::DocumentContext& c)
    : ClientEditionPolicy{s, c}
{
  auto& mapi = MessagesAPI::instance();
  auto& play_act = c.app.actions.action<Actions::NetworkPlay>();
  connect(play_act.action(), &QAction::triggered, this, [&] {
    m_session->master().sendMessage(m_session->makeMessage(mapi.play));
  });
}

void GUIClientEditionPolicy::play()
{
  auto sm = score::IDocument::try_get<Scenario::ScenarioDocumentModel>(m_ctx.document);
  if(sm)
  {
    Netpit::setCurrentDocument(m_ctx);
    auto& plug
        = score::GUIAppContext().guiApplicationPlugin<Engine::ApplicationPlugin>();
    plug.execution().request_play_interval(
        sm->baseInterval(), BasicPruner{m_ctx.plugin<NetworkDocumentPlugin>()},
        TimeVal{});
  }
}

void GUIClientEditionPolicy::stop()
{
  auto act = m_ctx.app.actions.action<Actions::Stop>().action();
  act->trigger();
}

PlayerClientEditionPolicy::PlayerClientEditionPolicy(
    ClientSession* s, const score::DocumentContext& c)
    : ClientEditionPolicy{s, c}
{
}

void PlayerClientEditionPolicy::play()
{
  Netpit::setCurrentDocument(m_ctx);
  if(onPlay)
    onPlay();
}

void PlayerClientEditionPolicy::stop()
{
  if(onStop)
    onStop();
}
}
