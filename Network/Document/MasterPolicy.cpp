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
#include <iscore/model/path/PathSerialization.hpp>

#include <Scenario/Application/ScenarioActions.hpp>
#include <Network/Group/NetworkActions.hpp>
namespace Network
{

class Client;

MasterEditionPolicy::MasterEditionPolicy(
    MasterSession* s,
    const iscore::DocumentContext& c):
  m_session{s},
  m_ctx{c},
  m_keep{*s}
{
  auto& stack = c.document.commandStack();
  auto& mapi = MessagesAPI::instance();

  /////////////////////////////////////////////////////////////////////////////
  /// From the master to the clients
  /////////////////////////////////////////////////////////////////////////////
  con(stack, &iscore::CommandStack::localCommand,
      this, [&] (iscore::Command* cmd)
  {
    m_session->broadcastToAllClients(
          m_session->makeMessage(mapi.command_new,iscore::CommandData{*cmd}));
  });

  // Undo-redo
  con(stack, &iscore::CommandStack::localUndo,
      this, [&] ()
  { m_session->broadcastToAllClients(m_session->makeMessage(mapi.command_undo)); });
  con(stack, &iscore::CommandStack::localRedo,
      this, [&] ()
  { m_session->broadcastToAllClients(m_session->makeMessage(mapi.command_redo)); });
  con(stack, &iscore::CommandStack::localIndexChanged,
      this, [&] (int32_t idx)
  {
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.command_index, idx));
  });

  // Lock - unlock
  con(c.objectLocker, &iscore::ObjectLocker::lock,
      this, [&] (QByteArray arr)
  { m_session->broadcastToAllClients(m_session->makeMessage(mapi.lock, arr)); });
  con(c.objectLocker, &iscore::ObjectLocker::unlock,
      this, [&] (QByteArray arr)
  { m_session->broadcastToAllClients(m_session->makeMessage(mapi.unlock, arr)); });

  // Play
  auto& play_act = c.app.actions.action<Actions::NetworkPlay>();
  connect(play_act.action(), &QAction::triggered,
          this, [&] {
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.play));
    play();
  });
  auto& stop_act = c.app.actions.action<Actions::NetworkStop>();
  connect(stop_act.action(), &QAction::triggered,
          this, [&] {
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.stop));
    stop();
  });


  /////////////////////////////////////////////////////////////////////////////
  /// From a client to the master and the other clients
  /////////////////////////////////////////////////////////////////////////////
  s->mapper().addHandler(mapi.command_new, [&] (const NetworkMessage& m)
  {
    iscore::CommandData cmd;
    DataStreamWriter writer{m.data};
    writer.writeTo(cmd);

    stack.redoAndPushQuiet(
          m_ctx.app.instantiateUndoCommand(cmd));


    m_session->broadcastToOthers(m.clientId, m);
  });

  // Undo-redo
  s->mapper().addHandler(mapi.command_undo, [&] (const NetworkMessage& m)
  {
    stack.undoQuiet();
    m_session->broadcastToOthers(m.clientId, m);
  });
  s->mapper().addHandler(mapi.command_redo, [&] (const NetworkMessage& m)
  {
    stack.redoQuiet();
    m_session->broadcastToOthers(m.clientId, m);
  });

  s->mapper().addHandler(mapi.command_index, [&] (const NetworkMessage& m)
  {
    QDataStream stream{m.data};
    int32_t idx;
    stream >> idx;
    stack.setIndexQuiet(idx);
    m_session->broadcastToOthers(m.clientId, m);
  });


  // Lock-unlock
  s->mapper().addHandler(mapi.lock, [&] (const NetworkMessage& m)
  {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.objectLocker.on_lock(data);
    m_session->broadcastToOthers(m.clientId, m);
  });

  s->mapper().addHandler(mapi.unlock, [&] (const NetworkMessage& m)
  {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.objectLocker.on_unlock(data);
    m_session->broadcastToOthers(m.clientId, m);
  });

  s->mapper().addHandler(mapi.play, [&] (const NetworkMessage& m)
  {
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.play));
    play();
  });
  s->mapper().addHandler(mapi.stop, [&] (const NetworkMessage& m)
  {
    m_session->broadcastToAllClients(m_session->makeMessage(mapi.stop));
    stop();
  });

  s->mapper().addHandler(mapi.ping, [&] (const NetworkMessage& m)
  {
    qint64 t = std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    m_session->sendMessage(m.clientId, m_session->makeMessage(mapi.pong, t));
  });

  s->mapper().addHandler(mapi.pong, [&] (const NetworkMessage& m)
  {
    m_keep.on_pong(m);
  });

  s->mapper().addHandler(mapi.session_portinfo, [&] (const NetworkMessage& m)
  {
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
    qDebug() << "REMOTE CLIENT IP" <<  s << p;
    m_session->broadcastToOthers(m.clientId, m);
  });

}


void MasterEditionPolicy::play()
{
  auto sm = iscore::IDocument::try_get<Scenario::ScenarioDocumentModel>(m_ctx.document);
  if(sm)
  {
    auto& plug = m_ctx.app.guiApplicationPlugin<Engine::ApplicationPlugin>();
    plug.on_play(
          sm->baseConstraint(),
          true,
          BasicPruner{m_ctx.plugin<NetworkDocumentPlugin>()},
          TimeVal{});
  }
}

void MasterEditionPolicy::stop()
{
  auto sm = iscore::IDocument::try_get<Scenario::ScenarioDocumentModel>(m_ctx.document);
  if(sm)
  {
    auto stop_action = m_ctx.app.actions.action<Actions::Stop>().action();
    stop_action->trigger();
  }
}

MasterExecutionPolicy::MasterExecutionPolicy(
    Session& s,
    NetworkDocumentPlugin& doc,
    const iscore::DocumentContext& c)
{
  auto& mapi = MessagesAPI::instance();
  s.mapper().addHandler_(mapi.trigger_entered,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeNodeModel> p)
  {
    s.broadcastToOthers(m.clientId, m);

    // TODO there should be a consensus on this point.
    auto it = doc.trigger_evaluation_entered.find(p);
    if(it != doc.trigger_evaluation_entered.end())
    {
      // TODO also start evaluating expressions.
      if(it.value())
        it.value()(m.clientId);
    }
  });

  s.mapper().addHandler_(mapi.trigger_left,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeNodeModel> p)
  {
    // TODO there should be a consensus on this point.
    qDebug() << m.address << p;
  });

  s.mapper().addHandler_(mapi.trigger_finished,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeNodeModel> p, bool val)
  {
    // TODO there should be a consensus on this point.
    auto it = doc.trigger_evaluation_finished.find(p);
    if(it != doc.trigger_evaluation_finished.end())
    {
      if(it.value())
        it.value()(m.clientId, val);
    }

    s.broadcastToOthers(m.clientId, m);
  });

  s.mapper().addHandler_(mapi.trigger_expression_true,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeNodeModel> p)
  {
    auto it = doc.network_expressions.find(p);
    if(it != doc.network_expressions.end())
    {
      NetworkExpressionData& e = it.value();
      Group* grp = doc.groupManager().group(e.thisGroup);
      if(!grp)
        return;

      if(!grp->hasClient(m.clientId))
        return;

      optional<bool>& opt = e.values[m.clientId];
      if(bool(opt)) // Checks if the optional is initialized
        return;

      opt = true; // Initialize and set it to true

      const auto count_ready = ossia::count_if(
            e.values,
            [] (const auto& p) { return bool(p.second) && *p.second; });

      if(e.ready(count_ready, grp->clients().size()))
      {
        // Trigger the others :

        // Note : there is no problem for the ordered mode if we have A--|--A
        // because the i-score algorithm keeps this order. The 'Unordered' will still be ordered in
        // this case (but instantaneous). However we don't have a "global" order, only a "local" order.
        // We want a global order... this means splitting the time_node execution.

        switch(e.sync)
        {
          case SyncMode::AsyncOrdered:
          {
            // Trigger all the clients before the time node.
            const auto& clients = doc.groupManager().clients(e.prevGroups);
            s.broadcastToClients(
                  clients,
                  s.makeMessage(mapi.trigger_triggered, p, true));

            break;
          }
          case SyncMode::AsyncUnordered:
          {
            // Everyone should trigger instantaneously.
            s.broadcastToAll(s.makeMessage(mapi.trigger_triggered, p, true));

            break;
          }
          case SyncMode::SyncOrdered:
          {
            break;
          }
          case SyncMode::SyncUnordered:
            break;
        }

      }

      // TODO reset the trigger for when we are looping

    }
  });

  s.mapper().addHandler_(mapi.trigger_previous_completed,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeNodeModel> p)
  {
    auto it = doc.network_expressions.find(p);
    if(it != doc.network_expressions.end())
    {
      NetworkExpressionData& e = it.value();
      Group* grp = doc.groupManager().group(e.thisGroup);
      if(!grp)
        return;

      if(!grp->hasClient(m.clientId))
        return;

      // Add the client to the list if meaningful
      auto it = e.previousCompleted.find(m.clientId);
      if(it == e.previousCompleted.end())
      {
        e.previousCompleted.insert(m.clientId);

        if(e.previousCompleted.size() >= doc.groupManager().clientsCount(e.prevGroups))
        {
          // If we're in a synchronized scenario :
          s.broadcastToAll(s.makeMessage(mapi.trigger_triggered, p, true));
          // Mixed :
          // s.broadcastToClients(doc.groupManager().clients(e.nextGroups), s.makeMessage(mapi.trigger_triggered, p, true));
        }
      }
    }

  });

  s.mapper().addHandler_(mapi.trigger_triggered,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeNodeModel> p, bool val)
  {
    auto it = doc.trigger_triggered.find(p);
    if(it != doc.trigger_triggered.end())
    {
      if(it.value())
        it.value()(m.clientId);
    }

    s.broadcastToOthers(m.clientId, m);
  });

}


SlaveExecutionPolicy::SlaveExecutionPolicy(
    Session& s,
    NetworkDocumentPlugin& doc,
    const iscore::DocumentContext& c)
{
  auto& mapi = MessagesAPI::instance();
  s.mapper().addHandler_(mapi.trigger_entered,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeNodeModel> p)
  {
    auto it = doc.trigger_evaluation_entered.find(p);
    if(it != doc.trigger_evaluation_entered.end())
    {
      if(it.value())
        it.value()(m.clientId);
    }
  });

  s.mapper().addHandler_(mapi.trigger_left,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeNodeModel> p)
  {
  });

  s.mapper().addHandler_(mapi.trigger_finished,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeNodeModel> p, bool val)
  {
    auto it = doc.trigger_evaluation_finished.find(p);
    if(it != doc.trigger_evaluation_finished.end())
    {
      if(it.value())
        it.value()(m.clientId, val);
    }
  });

  s.mapper().addHandler_(mapi.trigger_triggered,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeNodeModel> p, bool val)
  {
    auto it = doc.trigger_triggered.find(p);
    if(it != doc.trigger_triggered.end())
    {
      if(it.value())
        it.value()(m.clientId);
    }
  });

}

}

