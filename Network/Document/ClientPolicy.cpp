#include <Network/Communication/MessageMapper.hpp>
#include <iscore/document/DocumentContext.hpp>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <algorithm>
#include <iscore/command/CommandData.hpp>
#include "ClientPolicy.hpp"
#include <Network/Communication/NetworkMessage.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Engine/ApplicationPlugin.hpp>

#include <iscore/application/ApplicationContext.hpp>
#include <core/command/CommandStack.hpp>
#include <core/document/Document.hpp>
#include <iscore/command/Command.hpp>
#include <iscore/locking/ObjectLocker.hpp>
#include <iscore/plugins/customfactory/StringFactoryKey.hpp>
#include <iscore/serialization/DataStreamVisitor.hpp>
#include <iscore/tools/Todo.hpp>
#include <iscore/actions/ActionManager.hpp>
#include <Network/Client/RemoteClient.hpp>
#include <Network/Session/ClientSession.hpp>
#include <Network/Document/Execution/BasicPruner.hpp>

namespace Network
{

ClientNetworkPolicy::ClientNetworkPolicy(
    ClientSession* s,
    const iscore::DocumentContext& c):
  m_session{s},
  m_ctx{c},
  m_keep{*s}
{
  auto& stack = c.document.commandStack();
  auto& locker = c.document.locker();
  /////////////////////////////////////////////////////////////////////////////
  /// To the master
  /////////////////////////////////////////////////////////////////////////////
  con(stack, &iscore::CommandStack::localCommand,
      this, [=] (iscore::Command* cmd)
  {
    m_session->master()->sendMessage(
          m_session->makeMessage("/command/new", iscore::CommandData{*cmd}));
  });

  // Undo-redo
  con(stack, &iscore::CommandStack::localUndo,
      this, [&] ()
  { m_session->master()->sendMessage(m_session->makeMessage("/command/undo")); });
  con(stack, &iscore::CommandStack::localRedo,
      this, [&] ()
  { m_session->master()->sendMessage(m_session->makeMessage("/command/redo")); });
  con(stack, &iscore::CommandStack::localIndexChanged,
      this, [&] (int32_t idx)
  { m_session->master()->sendMessage(m_session->makeMessage("/command/index", idx)); });

  // TODO : messages : peut-être utiliser des tuples en tant que structures ?
  // Cela permettrait de spécifier les types proprement ?
  // Lock-unlock
  con(locker, &iscore::ObjectLocker::lock,
      this, [&] (QByteArray arr)
  { qDebug() << "client send lock"; m_session->master()->sendMessage(m_session->makeMessage("/lock", arr)); });
  con(locker, &iscore::ObjectLocker::unlock,
      this, [&] (QByteArray arr)
  { qDebug() << "client send unlock"; m_session->master()->sendMessage(m_session->makeMessage("/unlock", arr)); });

  auto& play_act = c.app.actions.action<Actions::NetworkPlay>();
  connect(play_act.action(), &QAction::triggered,
          this, [&] {
    m_session->master()->sendMessage(m_session->makeMessage("/play"));
  });


  /////////////////////////////////////////////////////////////////////////////
  /// From the master
  /////////////////////////////////////////////////////////////////////////////
  // - command comes from the master
  //   -> apply it to the computer only
  s->mapper().addHandler("/command/new",
                         [&] (NetworkMessage m)
  {
    iscore::CommandData cmd;
    DataStreamWriter writer{m.data};
    writer.writeTo(cmd);

    m_ctx.document.commandStack().redoAndPushQuiet(
          m_ctx.app.instantiateUndoCommand(cmd));
  });

  s->mapper().addHandler("/command/undo", [&] (NetworkMessage)
  { m_ctx.document.commandStack().undoQuiet(); });

  s->mapper().addHandler("/command/redo", [&] (NetworkMessage)
  { m_ctx.document.commandStack().redoQuiet(); });

  s->mapper().addHandler("/command/index", [&] (NetworkMessage m)
  {
    QDataStream stream{m.data};
    int32_t idx;
    stream >> idx;
    m_ctx.document.commandStack().setIndexQuiet(idx);
  });

  s->mapper().addHandler("/lock", [&] (NetworkMessage m)
  {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.document.locker().on_lock(data);
  });

  s->mapper().addHandler("/unlock", [&] (NetworkMessage m)
  {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.document.locker().on_unlock(data);
  });

  s->mapper().addHandler("/play", [&] (NetworkMessage m)
  {
    play();
  });

  s->mapper().addHandler("/ping", [&] (NetworkMessage m)
  {
    qint64 t = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    m_session->sendMessage(m.clientId, m_session->makeMessage("/pong", t));
  });

  s->mapper().addHandler("/pong", [&] (NetworkMessage m)
  {
    m_keep.on_pong(m);
  });


  s->mapper().addHandler("/session/portinfo", [&] (NetworkMessage m)
  {
    QString ip;
    int port;
    QDataStream stream{&m.data, QIODevice::ReadOnly};
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
    qDebug() << "REMOTE CLIENT IP" <<  ip << port;
  });
}

void ClientNetworkPolicy::play()
{

  auto sm = iscore::IDocument::try_get<Scenario::ScenarioDocumentModel>(m_ctx.document);
  if(sm)
  {
    auto& plug = iscore::AppContext().applicationPlugin<Engine::ApplicationPlugin>();
    plug.on_play(
          sm->baseConstraint(),
          true,
          BasicPruner{m_ctx.plugin<NetworkDocumentPlugin>()},
    TimeValue{});
  }
}

void ClientNetworkPolicy::connectToOtherClient(QString ip, int port)
{

}
}
