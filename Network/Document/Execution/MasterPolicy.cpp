#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>

#include <score/model/path/PathDebug.hpp>
#include <score/model/path/PathSerialization.hpp>

#include <Network/Communication/MessageMapper.hpp>
#include <Network/Document/Execution/MasterPolicy.hpp>
#include <Network/Document/MasterPolicy.hpp>
#include <Network/Group/Group.hpp>
#include <Network/Group/GroupManager.hpp>
#include <Network/Session/Session.hpp>
namespace Network
{

MasterExecutionPolicy::MasterExecutionPolicy(
    Session& s, NetworkDocumentPlugin& doc, const score::DocumentContext& c)
    : m_session{s}
    , m_keep{dynamic_cast<MasterEditionPolicy&>(doc.policy()).timekeeper()}
{
  qDebug("MasterExecutionPolicy");
  auto& mapi = MessagesAPI::instance();
  s.mapper().addHandler_(
      mapi.trigger_entered,
      [&](const NetworkMessage& m, Path<Scenario::TimeSyncModel> p) {
    qDebug("master << trigger_entered");
    s.broadcastToOthers(m.clientId, m);

    // if(m.clientId != s.master().id())
    {
      // TODO there should be a consensus on this point.
      auto it = doc.noncompensated.trigger_evaluation_entered.find(p);
      if(it != doc.noncompensated.trigger_evaluation_entered.end())
      {
        // TODO also start evaluating expressions.
        if(it.value())
          it.value()(m.clientId);
      }
    }
      });

  s.mapper().addHandler_(
      mapi.trigger_left, [&](const NetworkMessage& m, Path<Scenario::TimeSyncModel> p) {
        qDebug("master << trigger_left");
        // TODO there should be a consensus on this point.
        qDebug() << m.address << p;
      });

  s.mapper().addHandler_(
      mapi.trigger_finished,
      [&](const NetworkMessage& m, Path<Scenario::TimeSyncModel> p, bool val) {
    qDebug("master << trigger_finished");

    if(m.clientId != s.master().id())
    {
      // TODO there should be a consensus on this point.
      auto it = doc.noncompensated.trigger_evaluation_finished.find(p);
      if(it != doc.noncompensated.trigger_evaluation_finished.end())
      {
        if(it.value())
          it.value()(m.clientId, val);
      }
    }

    s.broadcastToOthers(m.clientId, m);
      });

  s.mapper().addHandler_(
      mapi.trigger_expression_true,
      [&](const NetworkMessage& m, Path<Scenario::TimeSyncModel> p) {
    qDebug("master << trigger_expr_true");
    auto it = doc.noncompensated.network_expressions.find(p);
    if(it != doc.noncompensated.network_expressions.end())
    {
      NetworkExpressionData& e = it.value();
      Group* grp = doc.groupManager().group(e.thisGroup);
      if(!grp)
        return;

      if(!grp->hasClient(m.clientId))
        return;

      std::optional<bool>& opt = e.values[m.clientId];
      if(bool(opt)) // Checks if the std::optional is initialized
        return;

      opt = true; // Initialize and set it to true

      const auto count_ready = ossia::count_if(
          e.values, [](const auto& p) { return bool(p.second) && *p.second; });

      if(e.ready(count_ready, grp->clients().size()))
      {
        // Trigger the others :

        // Note : there is no problem for the ordered mode if we have
        // A--|--A because the i-score algorithm keeps this order. The
        // 'NonCompensated' will still be ordered in this case (but
        // instantaneous). However we don't have a "global" order, only a
        // "local" order. We want a global order... this means splitting
        // the time_node execution.

        switch(e.sync)
        {
          case SyncMode::NonCompensatedSync: {
            // Trigger all the clients before the time node.
            const auto& clients = doc.groupManager().clients(e.prevGroups);
            s.broadcastToClients(
                clients, s.makeMessage(mapi.trigger_triggered, p, true));

            break;
          }
          case SyncMode::NonCompensatedAsync: {
            // Everyone should trigger instantaneously.
            s.broadcastToAll(s.makeMessage(mapi.trigger_triggered, p, true));

            break;
          }
          case SyncMode::CompensatedSync: {
            break;
          }
          case SyncMode::CompensatedAsync: {
            // Compute delay for each client. Self delay == 0;
            // The execution has to take place at the time where we can
            // guarantee that all clients will have received the message.
            // Hence we look for the client with the bigger delay.

            std::chrono::nanoseconds max_del{};
            for(auto& ts : m_keep.timestamps())
            {
              auto del = ts.second.roundtrip_latency / 2;
              if(del > max_del)
                max_del = del;
            }
            // For testing
            max_del += std::chrono::nanoseconds(3000000000);

            for(const auto& client : s.remoteClients())
            {
              // Send to each clients how long it has to wait
              const auto& m = s.makeMessage(
                  mapi.trigger_triggered_compensated, p, true,
                  (qint64)(max_del - (m_keep.timestamp(client->id()).roundtrip_latency / 2))
                      .count());

              client->sendMessage(m);
            }
            s.mapper().map(s.makeMessage(
                mapi.trigger_triggered_compensated, p, true, (qint64)max_del.count()));
            break;
          }
        }
      }

      // TODO reset the trigger for when we are looping
    }
      });

  s.mapper().addHandler_(
      mapi.trigger_previous_completed,
      [&](const NetworkMessage& m, Path<Scenario::TimeSyncModel> p) {
    qDebug("master << trigger_prev_completed");
    auto it = doc.noncompensated.network_expressions.find(p);
    if(it != doc.noncompensated.network_expressions.end())
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
          // s.broadcastToClients(doc.groupManager().clients(e.nextGroups),
          // s.makeMessage(mapi.trigger_triggered, p, true));
        }
      }
    }
      });

  s.mapper().addHandler_(
      mapi.trigger_triggered,
      [&](const NetworkMessage& m, Path<Scenario::TimeSyncModel> p, bool val) {
    qDebug("master << noncompensated.trigger_triggered");
    if(m.clientId != s.master().id())
    {
      auto it = doc.noncompensated.trigger_triggered.find(p);
      if(it != doc.noncompensated.trigger_triggered.end())
      {
        if(it.value())
          it.value()(m.clientId);
      }
    }

    s.broadcastToOthers(m.clientId, m);
      });
  s.mapper().addHandler_(
      mapi.trigger_triggered_compensated,
      [&](const NetworkMessage& m, Path<Scenario::TimeSyncModel> p, qint64 ns,
          bool val) {
    qDebug("master << compensated.trigger_triggered");
    if(m.clientId != s.master().id())
    {
      auto it = doc.compensated.trigger_triggered.find(p);
      if(it != doc.compensated.trigger_triggered.end())
      {
        if(it.value())
          it.value()(m.clientId, ns);
      }
    }

    s.broadcastToOthers(m.clientId, m);
      });

  s.mapper().addHandler_(
      mapi.interval_speed,
      [&](const NetworkMessage& m, Path<Scenario::IntervalModel> p, double val) {
    qDebug("master << constraint_speed");
    if(m.clientId != s.master().id())
    {
      auto it = doc.noncompensated.interval_speed_changed.find(p);
      if(it != doc.noncompensated.interval_speed_changed.end())
      {
        if(it.value())
          it.value()(m.clientId, val);
      }
    }

    s.broadcastToOthers(m.clientId, m);
      });

  s.mapper().addHandler_(
      mapi.netpit_in_message, [&](const NetworkMessage& m, uint64_t id, ossia::value v) {
        // Got a message that updates a process value from a remote client
        auto& messages = m_messages[id];
        messages[m.clientId] = std::move(v);

        m_session.broadcastToAll(
            m_session.makeMessage(mapi.netpit_out_message, id, messages.container));
      });

  s.mapper().addHandler_(
      mapi.netpit_in_audio,
      [&](const NetworkMessage& m, uint64_t id, std::vector<std::vector<float>> v) {
    // Got a message that updates a process value from a remote client
    auto& messages = m_audios[id];
    messages[m.clientId] = std::move(v);

    m_session.broadcastToAll(
        m_session.makeMessage(mapi.netpit_out_audio, id, messages.container));
    messages.clear();
      });

  s.mapper().addHandler_(
      mapi.netpit_out_message,
      [&](const NetworkMessage& m, uint64_t process,
          std::vector<std::pair<Id<Client>, ossia::value>> vec) {
    // Apply to the local process
    this->on_message(process, std::move(vec));
      });

  s.mapper().addHandler_(
      mapi.netpit_out_audio,
      [&](const NetworkMessage& m, uint64_t process,
          std::vector<std::pair<Id<Client>, std::vector<std::vector<float>>>> vec) {
    // Apply to the local process
    this->on_audio(process, std::move(vec));
      });
}

void MasterExecutionPolicy::writeMessage(Netpit::OutboundMessage m)
{
  // Local execution writes a message: it is applied to m_messages ; everyone is notified.
  auto& messages = m_messages[m.instance];
  messages[m_session.master().id()] = std::move(m.val);

  auto& mapi = MessagesAPI::instance();
  m_session.broadcastToAll(
      m_session.makeMessage(mapi.netpit_out_message, m.instance, messages.container));
}

void MasterExecutionPolicy::writeAudio(Netpit::OutboundAudio&& m)
{
  // Local execution writes a message: it is applied to m_messages ; everyone is notified.
  auto& messages = m_audios[m.instance];
  messages[m_session.master().id()] = std::move(m.channels);

  auto& mapi = MessagesAPI::instance();
  m_session.broadcastToAll(
      m_session.makeMessage(mapi.netpit_out_audio, m.instance, messages.container));
  messages.clear();
}
}
