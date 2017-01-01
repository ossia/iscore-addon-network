#include <Serialization/MessageMapper.hpp>
#include <iscore/tools/std/Optional.hpp>
#include <iscore/document/DocumentContext.hpp>
#include <QByteArray>
#include <QDataStream>
#include <algorithm>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <DistributedScenario/Group.hpp>
#include <DistributedScenario/GroupMetadata.hpp>
#include <Engine/Executor/ConstraintComponent.hpp>
#include <Scenario/Process/ScenarioInterface.hpp>

#include "NetworkMasterDocumentPlugin.hpp"
#include <DistributedScenario/GroupManager.hpp>
#include "Serialization/NetworkMessage.hpp"
#include <Engine/ApplicationPlugin.hpp>
#include <Engine/Executor/BaseScenarioComponent.hpp>

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
#include "session/MasterSession.hpp"

namespace Network
{

class Client;

MasterNetworkPolicy::MasterNetworkPolicy(MasterSession* s,
                                         const iscore::DocumentContext& c):
  m_session{s},
  m_ctx{c}
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


    m_session->broadcastToOthers(Id<Client>(m.clientId), m);
  });

  // Undo-redo
  s->mapper().addHandler("/command/undo", [&] (NetworkMessage m)
  {
    stack.undoQuiet();
    m_session->broadcastToOthers(Id<Client>(m.clientId), m);
  });
  s->mapper().addHandler("/command/redo", [&] (NetworkMessage m)
  {
    stack.redoQuiet();
    m_session->broadcastToOthers(Id<Client>(m.clientId), m);
  });

  s->mapper().addHandler("/command/index", [&] (NetworkMessage m)
  {
    QDataStream stream{m.data};
    int32_t idx;
    stream >> idx;
    stack.setIndexQuiet(idx);
    m_session->broadcastToOthers(Id<Client>(m.clientId), m);
  });


  // Lock-unlock
  s->mapper().addHandler("/lock", [&] (NetworkMessage m)
  {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.objectLocker.on_lock(data);
    m_session->broadcastToOthers(Id<Client>(m.clientId), m);
  });

  s->mapper().addHandler("/unlock", [&] (NetworkMessage m)
  {
    QDataStream stream{m.data};
    QByteArray data;
    stream >> data;
    m_ctx.objectLocker.on_unlock(data);
    m_session->broadcastToOthers(Id<Client>(m.clientId), m);
  });

  s->mapper().addHandler("/play", [&] (NetworkMessage m)
  {
    m_session->broadcastToAllClients(m_session->makeMessage("/play"));
    play();
  });
}

struct NetworkBasicPruner
{
  NetworkDocumentPlugin& doc;

  const Id<Client>& self = doc.policy()->session()->localClient().id();


  void recurse(Engine::Execution::ConstraintComponent& cst, const Group& cur)
  {
    const auto& gm = doc.groupManager();
    // First look if there is a group

    //auto comp = iscore::findComponent<GroupMetadata>(cst.iscoreConstraint().components());
    //if(comp)
    {

    }
    //else
    {
      // We assume that we keep the parent group.

    }


    // If no group found through components, maybe through metadata :
    auto& m = cst.iscoreConstraint().metadata().getExtendedMetadata();

    // Default case :
    const Group* cur_group = &cur;

    auto it = m.find("group");
    if(it != m.end())
    {
      auto str = it->toString();
      if(str == "all")
      {
        cur_group = gm.group(gm.defaultGroup());
      }
      else if(str == "parent" || str.isEmpty())
      {
        // Default
      }
      else
      {
        // look for a group of this name
        auto group = gm.findGroup(str);
        if(group)
        {
          cur_group = group; // Else we default to the "parent" case.
        }
      }
    }

    ISCORE_ASSERT(cur_group);

    // Mute the processes that are not meant to execute there.
    cst.iscoreConstraint().setExecutionState(Scenario::ConstraintExecutionState::Muted);

    for(const auto& process : cst.processes())
    {
      auto& proc = process->OSSIAProcess();
      proc.mute(!cur_group->hasClient(self));
    }

    // Recursion
    for(const auto& process : cst.processes())
    {
      auto ip = dynamic_cast<Scenario::ScenarioInterface*>(&process->process());
      if(ip)
      {
        for(Scenario::ConstraintModel& cst : ip->getConstraints())
        {
          auto comp = iscore::findComponent<Engine::Execution::ConstraintComponent>(cst.components());
          if(comp)
            recurse(*comp, *cur_group);
        }
      }
    }

  }


  void operator()(const Engine::Execution::Context& exec_ctx)
  {
    // We mute all the processes that are not in a group
    // of this client.
    auto& root = exec_ctx.sys.baseScenario()->baseConstraint();

    // Let's assume for now that we start in the "all" group...
    const auto& gm = doc.groupManager();
    recurse(root, *gm.group(gm.defaultGroup()));
  }

};

void MasterNetworkPolicy::play()
{
  auto sm = iscore::IDocument::try_get<Scenario::ScenarioDocumentModel>(m_ctx.document);
  if(sm)
  {
    auto& plug = m_ctx.app.applicationPlugin<Engine::ApplicationPlugin>();
    plug.on_play(
          sm->baseConstraint(),
          true, NetworkBasicPruner{m_ctx.plugin<NetworkDocumentPlugin>()}, TimeValue{});
  }
}
}

