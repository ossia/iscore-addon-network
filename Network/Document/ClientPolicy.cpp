#include <Scenario/Application/ScenarioActions.hpp>
#include <Scenario/Application/ScenarioApplicationPlugin.hpp>
#include <Scenario/Application/Menus/TransportActions.hpp>
#include <Scenario/Document/BaseScenario/BaseScenario.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>

#include <Engine/ApplicationPlugin.hpp>

#include <score/actions/ActionManager.hpp>
#include <score/tools/Bind.hpp>

#include <core/application/ApplicationSettings.hpp>
#include <core/command/CommandStack.hpp>
#include <core/document/Document.hpp>

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Document/ClientPolicy.hpp>
#include <Network/Document/RemoteCommand.hpp>
#include <Network/Document/ObjectQueries.hpp>
#include <Network/Document/Transport.hpp>
#include <Network/Document/DeviceStatus.hpp>
#include <Network/Document/DeviceValues.hpp>
#include <Network/Document/Execution/BasicPruner.hpp>
#include <Network/Group/NetworkActions.hpp>
namespace Network
{
ClientEditionPolicy::ClientEditionPolicy(
    ClientSession* s, const score::DocumentContext& c)
    : m_session{s}
    , m_ctx{c}
    , m_keep{*s}
{
  auto& stack = c.document.commandStack();
  auto& locker = c.document.locker();
  auto& mapi = MessagesAPI::instance();
  /////////////////////////////////////////////////////////////////////////////
  /// To the master
  /////////////////////////////////////////////////////////////////////////////
  con(stack, &score::CommandStack::localCommand, this, [&](score::Command* cmd) {
    using namespace std::literals;
    if(!this->sendCommands())
      return;
    if(this->sendControls()
       || (!this->sendControls() && cmd->key().toString() != "SetControlValue"sv))
      m_session->master().sendMessage(
          m_session->makeMessage(mapi.command_new, score::CommandData{*cmd}));

    // Our own edits need the same treatment as the ones we receive: adding a
    // shader here builds it from a path on the other machine and gets an empty
    // process. Asked after sending, so the master has applied the command by
    // the time the question reaches it -- same socket, in order.
    if(auto* plug = m_ctx.findPlugin<NetworkDocumentPlugin>())
      if(auto* rpc = plug->rpc())
        fillStandIns(*rpc, m_ctx, m_session->master().id());
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
  s->mapper().addHandler(this, mapi.command_new, [&](const NetworkMessage& m) {
    applyRemoteCommand(m_ctx, m.data);

    // A command can name a process this build cannot make; what it stands for
    // is only known where it was made, so ask.
    if(auto* plug = m_ctx.findPlugin<NetworkDocumentPlugin>())
      if(auto* rpc = plug->rpc())
        fillStandIns(*rpc, m_ctx, m_session->master().id());
  });

  // The master could not apply a command we sent, so it did not relay it: we
  // applied it locally and nobody else did.
  s->mapper().addHandler(this, mapi.command_rejected, [&](const NetworkMessage&) {
    if(auto* plug = m_ctx.findPlugin<NetworkDocumentPlugin>())
      plug->setDiverged(
          QStringLiteral("the master could not apply one of our commands"));
  });

  s->mapper().addHandler(this, mapi.command_undo, [&](const NetworkMessage&) {
    auto& stack = m_ctx.document.commandStack();
    if(canApplyRemoteEdit(m_ctx) && stack.canUndo())
      stack.undoQuiet();
  });

  s->mapper().addHandler(this, mapi.command_redo, [&](const NetworkMessage&) {
    auto& stack = m_ctx.document.commandStack();
    if(canApplyRemoteEdit(m_ctx) && stack.canRedo())
      stack.redoQuiet();
  });

  s->mapper().addHandler(this, mapi.command_index, [&](const NetworkMessage& m) {
    if(!canApplyRemoteEdit(m_ctx))
      return;
    QDataStream stream{m.data};
    int32_t idx{};
    stream >> idx;
    auto& stack = m_ctx.document.commandStack();
    if(idx >= 0 && idx <= stack.size())
      stack.setIndexQuiet(idx);
  });

  s->mapper().addHandler(this, mapi.lock, [&](const NetworkMessage& m) {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.document.locker().on_lock(data);
  });

  s->mapper().addHandler(this, mapi.unlock, [&](const NetworkMessage& m) {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.document.locker().on_unlock(data);
  });

  s->mapper().addHandler(this, mapi.play, [&](const NetworkMessage& m) { play(); });
  s->mapper().addHandler(this, mapi.stop, [&](const NetworkMessage& m) { stop(); });

  s->mapper().addHandler(this, mapi.ping, [&](const NetworkMessage& m) {
    qint64 t = std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::high_resolution_clock::now().time_since_epoch())
                   .count();
    m_session->sendMessage(m.clientId, m_session->makeMessage(mapi.pong, t));
  });

  s->mapper().addHandler(this, mapi.pong, [&](const NetworkMessage& m) { m_keep.on_pong(m); });

  s->mapper().addHandler(this, mapi.session_portinfo, [&](const NetworkMessage& m) {
    QString ip;
    int port;
    QDataStream stream{m.data};
    stream >> ip >> port;

    auto clt = m_session->findClient(m.clientId);
    if(clt)
    {
      clt->m_clientServerAddress = ip;
      clt->m_clientServerPort = port;
    }
    else
    {
      connectToOtherClient(ip, port);
      auto sock = new NetworkSocket{ip, port, nullptr};
      auto clt = new RemoteClient{sock, m.clientId};

      m_session->addClient(clt);
    }
    qDebug() << "REMOTE CLIENT IP" << ip << port;
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
  // not the Stop action: it is registered by the GUI only
  score::GUIAppContext()
      .guiApplicationPlugin<Engine::ApplicationPlugin>()
      .execution()
      .request_stop();
}

TerminalEditionPolicy::TerminalEditionPolicy(
    ClientSession* s, const score::DocumentContext& c)
    : ClientEditionPolicy{s, c}
{
  // Nothing here moves the playhead or holds the devices, so the host says
  // where it is and which of them are connected.
  bindTransportMirror(*this, *m_session, m_ctx);
  bindDeviceStatusMirror(*this, *m_session, m_ctx);

  // No devices here, so editing the tree has to be performed where they are.
  bindValueForwarding(*m_session, m_ctx);

  if(!c.app.applicationSettings.gui)
    return;

  // The ordinary transport actions, not just the network ones: on a terminal
  // there is nothing else Play could mean, and a person who presses it expects
  // the score to start -- on the machine that has it.
  auto& acts = c.app.actions;
  connect(
      acts.action<Actions::Play>().action(), &QAction::triggered, this,
      &TerminalEditionPolicy::requestPlay);
  connect(
      acts.action<Actions::PlayGlobal>().action(), &QAction::triggered, this,
      &TerminalEditionPolicy::requestPlay);
  connect(
      acts.action<Actions::NetworkPlay>().action(), &QAction::triggered, this,
      &TerminalEditionPolicy::requestPlay);

  connect(
      acts.action<Actions::Stop>().action(), &QAction::triggered, this,
      &TerminalEditionPolicy::requestStop);
  connect(
      acts.action<Actions::NetworkStop>().action(), &QAction::triggered, this,
      &TerminalEditionPolicy::requestStop);
}

void TerminalEditionPolicy::requestPlay()
{
  m_session->master().sendMessage(
      m_session->makeMessage(MessagesAPI::instance().play));
}

void TerminalEditionPolicy::requestStop()
{
  m_session->master().sendMessage(
      m_session->makeMessage(MessagesAPI::instance().stop));
}

void TerminalEditionPolicy::play()
{
  // Nothing starts here, but the buttons have to say what the score is doing:
  // ExecutionController declines a terminal outright, so without this they
  // would sit in whatever state the person left them in.
  if(!m_ctx.app.applicationSettings.gui)
    return;

  auto& plug = m_ctx.app.guiApplicationPlugin<Scenario::ScenarioApplicationPlugin>();
  plug.transportActions().onPlayGlobal();
}

void TerminalEditionPolicy::stop()
{
  if(!m_ctx.app.applicationSettings.gui)
    return;

  auto& plug = m_ctx.app.guiApplicationPlugin<Scenario::ScenarioApplicationPlugin>();
  plug.transportActions().onStop();

  // Back to the start, as a stopped score is.
  if(auto* sm = score::IDocument::try_get<Scenario::ScenarioDocumentModel>(
         m_ctx.document))
    sm->baseInterval().duration.setPlayPercentage(0.);
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
