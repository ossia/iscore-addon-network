#include <Network/Document/Execution/MasterPolicy.hpp>
#include <Network/Session/Session.hpp>
#include <Network/Communication/MessageMapper.hpp>
#include <iscore/model/path/PathSerialization.hpp>

namespace Network
{

MasterExecutionPolicy::MasterExecutionPolicy(
    Session& s,
    NetworkDocumentPlugin& doc,
    const iscore::DocumentContext& c)
{
  qDebug("MasterExecutionPolicy");
  auto& mapi = MessagesAPI::instance();
  s.mapper().addHandler_(mapi.trigger_entered,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeNodeModel> p)
  {
    qDebug() << "<< trigger_entered";
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
    qDebug() << "<< trigger_left";
    // TODO there should be a consensus on this point.
    qDebug() << m.address << p;
  });

  s.mapper().addHandler_(mapi.trigger_finished,
                         [&] (const NetworkMessage& m, Path<Scenario::TimeNodeModel> p, bool val)
  {
    qDebug() << "<< trigger_finished";
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
    qDebug() << "<< trigger_expression_true";
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
    qDebug() << "<< trigger_previous_completed";
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
    qDebug() << "<< trigger_triggered";
    auto it = doc.trigger_triggered.find(p);
    if(it != doc.trigger_triggered.end())
    {
      if(it.value())
        it.value()(m.clientId);
    }

    s.broadcastToOthers(m.clientId, m);
  });

}


}
