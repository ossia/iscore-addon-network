#include <Network/Communication/MessageMapper.hpp>
#include <iscore/tools/std/Optional.hpp>
#include <iscore/document/DocumentContext.hpp>
#include <QByteArray>
#include <QDataStream>
#include <algorithm>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupMetadata.hpp>

#include <Engine/ApplicationPlugin.hpp>
#include "MasterPolicy.hpp"
#include <Network/Group/GroupManager.hpp>
#include <Network/Communication/NetworkMessage.hpp>
#include <Network/Document/Execution/BasicPruner.hpp>

#include <iscore/application/ApplicationContext.hpp>
#include <core/command/CommandStack.hpp>
#include <iscore/command/Command.hpp>
#include <iscore/locking/ObjectLocker.hpp>
#include <iscore/plugins/customfactory/StringFactoryKey.hpp>
#include <iscore/serialization/DataStreamVisitor.hpp>
#include <iscore/model/Identifier.hpp>
#include <iscore/tools/Todo.hpp>
#include <iscore/actions/ActionManager.hpp>
#include <iscore/document/DocumentInterface.hpp>
#include <core/document/Document.hpp>
#include <Network/Session/MasterSession.hpp>

namespace Network
{

class Client;

MasterNetworkPolicy::MasterNetworkPolicy(
    MasterSession* s,
    const iscore::DocumentContext& c):
  m_session{s},
  m_ctx{c},
  m_keep{*s}
{
  auto& stack = c.document.commandStack();

  /////////////////////////////////////////////////////////////////////////////
  /// From the master to the clients
  /////////////////////////////////////////////////////////////////////////////
  con(stack, &iscore::CommandStack::localCommand,
      this, [=] (iscore::Command* cmd)
  {
    m_session->broadcastToAllClients(
          m_session->makeMessage("/command/new",iscore::CommandData{*cmd}));
  });

  // Undo-redo
  con(stack, &iscore::CommandStack::localUndo,
      this, [&] ()
  { m_session->broadcastToAllClients(m_session->makeMessage("/command/undo")); });
  con(stack, &iscore::CommandStack::localRedo,
      this, [&] ()
  { m_session->broadcastToAllClients(m_session->makeMessage("/command/redo")); });
  con(stack, &iscore::CommandStack::localIndexChanged,
      this, [&] (int32_t idx)
  {
    m_session->broadcastToAllClients(m_session->makeMessage("/command/index", idx));
  });

  // Lock - unlock
  con(c.objectLocker, &iscore::ObjectLocker::lock,
      this, [&] (QByteArray arr)
  { m_session->broadcastToAllClients(m_session->makeMessage("/lock", arr)); });
  con(c.objectLocker, &iscore::ObjectLocker::unlock,
      this, [&] (QByteArray arr)
  { m_session->broadcastToAllClients(m_session->makeMessage("/unlock", arr)); });

  // Play
  auto& play_act = c.app.actions.action<Actions::NetworkPlay>();
  connect(play_act.action(), &QAction::triggered,
          this, [&] {
    m_session->broadcastToAllClients(m_session->makeMessage("/play"));
    play();
  });


  /////////////////////////////////////////////////////////////////////////////
  /// From a client to the master and the other clients
  /////////////////////////////////////////////////////////////////////////////
  s->mapper().addHandler("/command/new", [&] (NetworkMessage m)
  {
    iscore::CommandData cmd;
    DataStreamWriter writer{m.data};
    writer.writeTo(cmd);

    stack.redoAndPushQuiet(
          m_ctx.app.instantiateUndoCommand(cmd));


    m_session->broadcastToOthers(m.clientId, m);
  });

  // Undo-redo
  s->mapper().addHandler("/command/undo", [&] (NetworkMessage m)
  {
    stack.undoQuiet();
    m_session->broadcastToOthers(m.clientId, m);
  });
  s->mapper().addHandler("/command/redo", [&] (NetworkMessage m)
  {
    stack.redoQuiet();
    m_session->broadcastToOthers(m.clientId, m);
  });

  s->mapper().addHandler("/command/index", [&] (NetworkMessage m)
  {
    QDataStream stream{m.data};
    int32_t idx;
    stream >> idx;
    stack.setIndexQuiet(idx);
    m_session->broadcastToOthers(m.clientId, m);
  });


  // Lock-unlock
  s->mapper().addHandler("/lock", [&] (NetworkMessage m)
  {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.objectLocker.on_lock(data);
    m_session->broadcastToOthers(m.clientId, m);
  });

  s->mapper().addHandler("/unlock", [&] (NetworkMessage m)
  {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.objectLocker.on_unlock(data);
    m_session->broadcastToOthers(m.clientId, m);
  });

  s->mapper().addHandler("/play", [&] (NetworkMessage m)
  {
    m_session->broadcastToAllClients(m_session->makeMessage("/play"));
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
    QString s;
    int p;
    QDataStream stream{&m.data, QIODevice::ReadOnly};
    stream >> s >> p;

    auto clt = m_session->findClient(m.clientId);
    if(clt)
    {
      clt->m_clientServerAddress = s;
      clt->m_clientServerPort = p;
    }
    qDebug() << "REMOTE CLIENT IP" <<  s << p;
    m_session->broadcastToOthers(m.clientId, m);
  });
}


void MasterNetworkPolicy::play()
{
  auto sm = iscore::IDocument::try_get<Scenario::ScenarioDocumentModel>(m_ctx.document);
  if(sm)
  {
    auto& plug = m_ctx.app.applicationPlugin<Engine::ApplicationPlugin>();
    plug.on_play(
          sm->baseConstraint(),
          true,
          BasicPruner{m_ctx.plugin<NetworkDocumentPlugin>()},
          TimeValue{});
  }
}

}

